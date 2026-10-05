#include "quickjs_vm.h"
#include <cstdio>

namespace hg {
static JSValue Print(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int error) {
    FILE *stream = error ? stderr : stdout;
    for (int i = 0; i < argc; ++i) {
        const char *s = JS_ToCString(ctx, argv[i]);
        if (!s) return JS_EXCEPTION;
        std::fprintf(stream, "%s%s", i ? " " : "", s);
        JS_FreeCString(ctx, s);
    }
    std::fputc('\n', stream);
    return JS_UNDEFINED;
}
QuickJSVM::QuickJSVM() {
    rt = JS_NewRuntime();
    if (!rt) throw std::runtime_error("Cannot allocate QuickJS runtime");
    // Leave room for the native host on Windows' default 1 MiB thread stack.
    JS_SetMaxStackSize(rt, 256 * 1024);
    ctx = JS_NewContext(rt);
    if (!ctx) { JS_FreeRuntime(rt); throw std::runtime_error("Cannot allocate QuickJS context"); }
    JS_SetModuleLoaderFunc(rt, nullptr, LoadModule, this);
    JS_SetHostPromiseRejectionTracker(rt, PromiseRejected, this);
    QuickJSValue global(ctx, JS_GetGlobalObject(ctx));
    JSValue console = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, console, "log", JS_NewCFunctionMagic(ctx, Print, "log", 1, JS_CFUNC_generic_magic, 0));
    JS_SetPropertyStr(ctx, console, "error", JS_NewCFunctionMagic(ctx, Print, "error", 1, JS_CFUNC_generic_magic, 1));
    JS_SetPropertyStr(ctx, global.value, "console", console);
    JS_SetPropertyStr(ctx, global.value, "print", JS_NewCFunctionMagic(ctx, Print, "print", 1, JS_CFUNC_generic_magic, 0));
}
QuickJSVM::~QuickJSVM() {
    JS_SetHostPromiseRejectionTracker(rt, nullptr, nullptr);
    for (const auto &value : unhandled) JS_FreeValue(ctx, value.second);
    if (cleanup) cleanup(ctx);
    JS_RunGC(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}
void QuickJSVM::PromiseRejected(JSContext *ctx, JSValueConst promise, JSValueConst, JS_BOOL handled, void *opaque) {
    auto &self = *static_cast<QuickJSVM *>(opaque);
    auto key = JS_VALUE_GET_PTR(promise);
    auto found = self.unhandled.find(key);
    if (found != self.unhandled.end()) { JS_FreeValue(ctx, found->second); self.unhandled.erase(found); }
    if (!handled) self.unhandled.emplace(key, JS_DupValue(ctx, promise));
}
void QuickJSVM::CheckUnhandledRejections() {
    if (!unhandled.empty()) CheckPromise(unhandled.begin()->second, true);
}
void QuickJSVM::SetModuleReader(ModuleReader read, ModuleResolver resolve) {
    reader = std::move(read);
    resolver = std::move(resolve);
    JS_SetModuleLoaderFunc(rt, resolver ? ResolveModule : nullptr, LoadModule, this);
}
char *QuickJSVM::ResolveModule(JSContext *ctx, const char *base, const char *name, void *opaque) {
    auto &self = *static_cast<QuickJSVM *>(opaque);
    try {
        const auto resolved = self.resolver(base, name);
        return js_strdup(ctx, resolved.c_str());
    } catch (const std::exception &e) { JS_ThrowReferenceError(ctx, "%s", e.what()); return nullptr; }
}
JSModuleDef *QuickJSVM::LoadModule(JSContext *ctx, const char *name, void *opaque) {
    auto &self = *static_cast<QuickJSVM *>(opaque);
    try {
        std::string source;
        if (!self.reader || !self.reader(name, source)) {
            JS_ThrowReferenceError(ctx, "Cannot load JavaScript module: %s", name);
            return nullptr;
        }
        JSValue compiled = JS_Eval(ctx, source.c_str(), source.size(), name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(compiled)) return nullptr;
        auto module = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(compiled));
        JS_FreeValue(ctx, compiled);
        return module;
    } catch (const std::exception &e) { JS_ThrowInternalError(ctx, "%s", e.what()); return nullptr; }
}
std::string QuickJSVM::TakeException() {
    QuickJSValue error(ctx, JS_GetException(ctx));
    const char *text = JS_ToCString(ctx, error.value);
    std::string message = text ? text : "QuickJS exception";
    JS_FreeCString(ctx, text);
    QuickJSValue stack(ctx, JS_GetPropertyStr(ctx, error.value, "stack"));
    if (!JS_IsUndefined(stack.value)) {
        text = JS_ToCString(ctx, stack.value);
        if (text) { message += "\n"; message += text; }
        JS_FreeCString(ctx, text);
    }
    return message;
}
void QuickJSVM::Check(JSValueConst value) { if (JS_IsException(value)) throw std::runtime_error(TakeException()); }
void QuickJSVM::CheckPromise(JSValueConst value, bool require_settled) {
    Check(value);
    auto state = JS_PromiseState(ctx, value);
    if (state == JS_PROMISE_REJECTED) { JS_Throw(ctx, JS_PromiseResult(ctx, value)); throw std::runtime_error(TakeException()); }
    if (require_settled && state == JS_PROMISE_PENDING) throw std::runtime_error("Promise has not settled");
}
JSValue QuickJSVM::Evaluate(const std::string &source, const std::string &name) {
    JSValue result = JS_Eval(ctx, source.c_str(), source.size(), name.c_str(), JS_EVAL_TYPE_MODULE);
    Check(result);
    return result;
}
int QuickJSVM::PumpJobs(int budget) {
    int count = 0;
    JSContext *job_ctx = nullptr;
    for (; count < budget; ++count) {
        int result = JS_ExecutePendingJob(rt, &job_ctx);
        if (result < 0) throw std::runtime_error(TakeException());
        if (result == 0) break;
    }
    return count;
}
}
