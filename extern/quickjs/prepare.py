"""Stage the pinned upstream core; never modify the user's source checkout."""
import pathlib
import sys

source, output = map(pathlib.Path, sys.argv[1:3])
if (source / 'VERSION').read_text().strip() != '2026-06-04':
    raise SystemExit('HGJS requires official QuickJS 2026-06-04')
output.mkdir(parents=True, exist_ok=True)
files = {item.name: item.read_text() for item in source.iterdir()
         if item.suffix in ('.c', '.h') or item.name == 'LICENSE'}

def replace(name, old, new):
    text = files[name]
    if text.count(old) != 1:
        raise SystemExit(f'Unreviewed QuickJS source: {name}: {old}')
    files[name] = text.replace(old, new)

# Single-threaded embedded VM: no blocking Atomics.wait or worker runtime.
replace('quickjs.c', '#if !defined(__EMSCRIPTEN__)\n#define CONFIG_ATOMICS',
        '#if !defined(__EMSCRIPTEN__) && !defined(HG_QUICKJS_SINGLE_THREAD)\n#define CONFIG_ATOMICS')
# MSVC ABI makes enum bitfields signed: module closure kinds 4..7 otherwise
# become negative and abort in js_closure2. This is internal runtime storage.
replace('quickjs.c', 'JSClosureTypeEnum closure_type : 3;', 'unsigned int closure_type : 3;')
# Select one BigInt ABI for the Clang C core AND MSVC C++ consumers.
replace('quickjs.h', '#if defined(__SIZEOF_INT128__) && (INTPTR_MAX >= INT64_MAX)',
        '#if !defined(_MSC_VER) && defined(__SIZEOF_INT128__) && (INTPTR_MAX >= INT64_MAX)')
# MSVC C++ does not accept C compound literals. Preserve the exact value layout.
replace('quickjs.h', '#define JS_MKVAL(tag, val) (JSValue){ (JSValueUnion){ .uint64 = (uint32_t)(val) }, tag }',
        'static inline JSValue hg_js_mkval(int64_t tag, uint32_t val) { JSValue v; v.u.uint64 = val; v.tag = tag; return v; }\n#define JS_MKVAL(tag, val) hg_js_mkval(tag, val)')
replace('quickjs.h', '#define JS_MKPTR(tag, p) (JSValue){ (JSValueUnion){ .ptr = p }, tag }',
        'static inline JSValue hg_js_mkptr(int64_t tag, void *p) { JSValue v; v.u.ptr = p; v.tag = tag; return v; }\n#define JS_MKPTR(tag, p) hg_js_mkptr(tag, p)')
replace('quickjs.h', '#define JS_NAN (JSValue){ .u.float64 = JS_FLOAT64_NAN, JS_TAG_FLOAT64 }',
        '#define JS_NAN __JS_NewFloat64(NULL, JS_FLOAT64_NAN)')
replace('quickjs.h', 'JSCFunctionType ft = { .generic_magic = func };',
        'JSCFunctionType ft; ft.generic_magic = func;')
# Only C++ consumers use helper functions: C initializers require literals.
s = files['quickjs.h']
start = s.index('static inline JSValue hg_js_mkval')
end = s.index('\n\n#define JS_TAG_IS_FLOAT64', start)
original = (source / 'quickjs.h').read_text()
a = original.index('#define JS_MKVAL(tag, val) (JSValue){')
b = original.index('\n\n#define JS_TAG_IS_FLOAT64', a)
s = s[:start] + '#ifdef __cplusplus\n' + s[start:end] + '\n#else\n' + original[a:b] + '\n#endif' + s[end:]
files['quickjs.h'] = s
files['include/quickjs.h'] = s
(output / 'include').mkdir(exist_ok=True)
for name, content in files.items():
    path = output / name
    if not path.exists() or path.read_text() != content:
        path.write_text(content)
