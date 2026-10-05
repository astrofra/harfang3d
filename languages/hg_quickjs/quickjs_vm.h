#pragma once
#include <quickjs.h>
#include <functional>
#include <string>
#include <stdexcept>
#include <map>

namespace hg {
// Values must be released before their VM. No JS value may cross VM boundaries.
struct QuickJSValue {
    JSContext *ctx;
    JSValue value;
    QuickJSValue(JSContext *ctx, JSValue value) : ctx(ctx), value(value) {}
    ~QuickJSValue() { JS_FreeValue(ctx, value); }
    QuickJSValue(const QuickJSValue &) = delete;
    QuickJSValue &operator=(const QuickJSValue &) = delete;
    QuickJSValue(QuickJSValue &&other) : ctx(other.ctx), value(other.value) { other.value = JS_UNDEFINED; }
};

class QuickJSVM {
public:
    using ModuleReader = std::function<bool(const std::string &, std::string &)>;
    using ModuleResolver = std::function<std::string(const std::string &, const std::string &)>;
    QuickJSVM();
    ~QuickJSVM();
    QuickJSVM(const QuickJSVM &) = delete;
    QuickJSVM &operator=(const QuickJSVM &) = delete;
    JSContext *GetContext() const { return ctx; }
    JSRuntime *GetRuntime() const { return rt; }
    void SetModuleReader(ModuleReader read, ModuleResolver resolve = {});
    void SetBindingCleanup(std::function<void(JSContext *)> fn) { cleanup = std::move(fn); }
    // Caller owns the returned evaluation promise.
    JSValue Evaluate(const std::string &source, const std::string &name);
    int PumpJobs(int budget = 64);
    void Check(JSValueConst value);
    void CheckPromise(JSValueConst value, bool require_settled);
    void CheckUnhandledRejections();
    std::string TakeException();
private:
    JSRuntime *rt = nullptr;
    JSContext *ctx = nullptr;
    ModuleReader reader;
    ModuleResolver resolver;
    std::function<void(JSContext *)> cleanup;
    std::map<void *, JSValue> unhandled;
    static JSModuleDef *LoadModule(JSContext *, const char *, void *);
    static char *ResolveModule(JSContext *, const char *, const char *, void *);
    static void PromiseRejected(JSContext *, JSValueConst, JSValueConst, JS_BOOL, void *);
};
}
