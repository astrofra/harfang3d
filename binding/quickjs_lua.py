"""QuickJS adapters for the engine's existing embedded Lua scene VM."""
from lang.quickjs import QuickJSClassTypeConverter, QuickJSTypeConverterCommon


class LuaReferenceConverter(QuickJSClassTypeConverter):
    """LuaObject (including lists) must keep its originating SceneLuaVM alive."""
    def get_type_glue(self, generator, module_name):
        glue = super().get_type_glue(generator, module_name)
        glue = glue.replace('JSValue ' + self.from_c_func + '(', 'JSValue ' + self.from_c_func + '_unrooted(')
        if str(self.ctype) == 'hg::LuaObject':
            # push_back()/set() copy a Lua reference into a native list. Retain
            # the JS reference too, so its originating VM outlives that list.
            glue = glue.replace('void ' + self.to_c_func + '(', 'void ' + self.to_c_func + '_unrooted(')
            glue += f'''
void {self.to_c_func}(JSContext *ctx, JSValueConst value, void *out) {{
    {self.to_c_func}_unrooted(ctx, value, out);
    auto *scope = qjs::CallScope::current();
    auto *info = hg_quickjs_get_c_type_info("std::vector<hg::LuaObject>");
    if (scope && scope->ctx == ctx && info && qjs::is_native(scope->self, info->type_tag))
        qjs::keep_alive(ctx, scope->self, value);
}}
'''
        return glue + f'''
JSValue {self.from_c_func}(JSContext *ctx, void *object, OwnershipPolicy policy) {{
    qjs::Value result(ctx, qjs::checked({self.from_c_func}_unrooted(ctx, object, policy)));
    auto *scope = qjs::CallScope::current();
    if (scope && scope->ctx == ctx) {{
        qjs::keep_alive(ctx, result, scope->self);
        for (int i = 0; i < scope->argc; ++i) {{
            qjs::keep_alive(ctx, result, scope->argv[i]);
            if (qjs::is_array(ctx, scope->argv[i])) {{
                // A list owns copies of references even if the source array changes.
                const auto count = qjs::array_length(ctx, scope->argv[i]);
                for (uint32_t j = 0; j < count; ++j) {{
                    qjs::Value item(ctx, qjs::array_element(ctx, scope->argv[i], j));
                    qjs::keep_alive(ctx, result, item);
                }}
            }}
        }}
    }}
    return result.release();
}}
'''


class LuaValueConverter(QuickJSTypeConverterCommon):
    def get_type_glue(self, gen, module_name):
        return '''
struct HGQuickJSLuaValue {
    enum Kind { Null, Integer, Float, Bool, String, Object } kind{Null};
    int64_t integer_value{0};
    double float_value{0};
    bool bool_value{false};
    std::string string_value;
    void *object_value{nullptr};
    uint32_t type_tag{0};
};
''' + f'''
bool {self.check_func}(JSContext *ctx, JSValueConst value) {{
    return JS_IsNull(value) || JS_IsUndefined(value) || JS_IsBool(value) || JS_IsNumber(value)
        || JS_IsBigInt(ctx, value) || JS_IsString(value) || qjs::native(value);
}}
void {self.to_c_func}(JSContext *ctx, JSValueConst value, void *object) {{
    auto &out = *static_cast<HGQuickJSLuaValue *>(object);
    if (JS_IsNull(value) || JS_IsUndefined(value)) return;
    if (JS_IsBool(value)) {{
        out.kind = HGQuickJSLuaValue::Bool;
        out.bool_value = JS_ToBool(ctx, value) != 0;
    }} else if (JS_IsBigInt(ctx, value)) {{
        out.kind = HGQuickJSLuaValue::Integer;
        if (!qjs::integer(ctx, value, &out.integer_value)) qjs::type_error(ctx, "Lua integer must fit int64");
    }} else if (JS_IsNumber(value)) {{
        if (qjs::integer(ctx, value, &out.integer_value)) out.kind = HGQuickJSLuaValue::Integer;
        else {{
            out.kind = HGQuickJSLuaValue::Float;
            qjs::check(JS_ToFloat64(ctx, &out.float_value, value));
        }}
    }} else if (JS_IsString(value)) {{
        qjs::CString text(ctx, value);
        out.kind = HGQuickJSLuaValue::String;
        out.string_value.assign(text.data, text.size);
    }} else if (auto *native = qjs::native(value)) {{
        out.kind = HGQuickJSLuaValue::Object;
        out.object_value = native->object;
        out.type_tag = native->tag;
    }} else qjs::type_error(ctx, "expected a primitive or HARFANG object");
}}
JSValue {self.from_c_func}(JSContext *ctx, void *object, OwnershipPolicy) {{
    auto &value = *static_cast<HGQuickJSLuaValue *>(object);
    switch (value.kind) {{
    case HGQuickJSLuaValue::Null: return JS_NULL;
    case HGQuickJSLuaValue::Integer: return JS_NewBigInt64(ctx, value.integer_value);
    case HGQuickJSLuaValue::Float: return JS_NewFloat64(ctx, value.float_value);
    case HGQuickJSLuaValue::Bool: return JS_NewBool(ctx, value.bool_value);
    case HGQuickJSLuaValue::String: return JS_NewStringLen(ctx, value.string_value.data(), value.string_value.size());
    case HGQuickJSLuaValue::Object:
        if (auto *info = hg_quickjs_get_bound_type_info(value.type_tag))
            return info->from_c(ctx, value.object_value, Copy);
    }}
    return JS_NULL;
}}
'''


def bind_scene_lua_values(gen, vm):
    gen.bind_type(LuaValueConverter('HGQuickJSLuaValue'))
    gen.insert_binding_code('''
extern "C" {
#include "lua.h"
}
// Backends use different type tags; exchange types by their canonical C++ names.
struct hg_lua_type_info {
    uint32_t type_tag;
    const char *c_type;
    const char *bound_name;
    bool (*check)(lua_State *, int);
    void (*to_c)(lua_State *, int, void *);
    int (*from_c)(lua_State *, void *, OwnershipPolicy);
};
hg_lua_type_info *hg_lua_get_bound_type_info(uint32_t);
hg_lua_type_info *hg_lua_get_c_type_info(const char *);
uint32_t hg_lua_get_wrapped_object_type_tag(lua_State *, int);

static hg::LuaObject __QuickJSObjectToLuaObject(hg::SceneLuaVM *vm, const HGQuickJSLuaValue &o) {
    auto L = vm->GetL();
    hg::LuaStackGuard guard(L);
    switch (o.kind) {
    case HGQuickJSLuaValue::Null: lua_pushnil(L); break;
    case HGQuickJSLuaValue::Integer: lua_pushinteger(L, o.integer_value); break;
    case HGQuickJSLuaValue::Float: lua_pushnumber(L, o.float_value); break;
    case HGQuickJSLuaValue::Bool: lua_pushboolean(L, o.bool_value); break;
    case HGQuickJSLuaValue::String: lua_pushlstring(L, o.string_value.data(), o.string_value.size()); break;
    case HGQuickJSLuaValue::Object: {
        auto *js_info = hg_quickjs_get_bound_type_info(o.type_tag);
        auto *lua_info = js_info ? hg_lua_get_c_type_info(js_info->c_type) : nullptr;
        if (lua_info) lua_info->from_c(L, o.object_value, Copy);
        else lua_pushnil(L);
        break;
    }
    }
    return hg::Pop(L);
}

static HGQuickJSLuaValue __LuaObjectToQuickJSObject(hg::SceneLuaVM *, const hg::LuaObject &o) {
    HGQuickJSLuaValue out;
    auto L = o.L();
    if (!L) return out;
    hg::LuaStackGuard guard(L);
    o.Push();
    if (lua_isinteger(L, -1)) {
        out.kind = HGQuickJSLuaValue::Integer;
        out.integer_value = lua_tointeger(L, -1);
    } else if (lua_type(L, -1) == LUA_TNUMBER) {
        out.kind = HGQuickJSLuaValue::Float;
        out.float_value = lua_tonumber(L, -1);
    } else if (lua_isboolean(L, -1)) {
        out.kind = HGQuickJSLuaValue::Bool;
        out.bool_value = lua_toboolean(L, -1) != 0;
    } else if (lua_type(L, -1) == LUA_TSTRING) {
        size_t length = 0;
        const char *text = lua_tolstring(L, -1, &length);
        out.kind = HGQuickJSLuaValue::String;
        out.string_value.assign(text, length);
    } else if (auto tag = hg_lua_get_wrapped_object_type_tag(L, -1)) {
        if (auto *info = hg_lua_get_bound_type_info(tag)) {
            if (auto *js_info = hg_quickjs_get_c_type_info(info->c_type)) {
                out.kind = HGQuickJSLuaValue::Object;
                out.type_tag = js_info->type_tag;
                info->to_c(L, -1, &out.object_value);
            }
        }
    }
    return out;
}
''')
    gen.bind_method(vm, 'Pack', 'hg::LuaObject', ['const HGQuickJSLuaValue &o'],
                    {'route': lambda args: '__QuickJSObjectToLuaObject(%s);' % ', '.join(args)})
    gen.bind_method(vm, 'Unpack', 'HGQuickJSLuaValue', ['const hg::LuaObject &o'],
                    {'route': lambda args: '__LuaObjectToQuickJSObject(%s);' % ', '.join(args)})
