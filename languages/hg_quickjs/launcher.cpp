#include "quickjs_vm.h"
#include "engine/scene.h"
#include "engine/physics.h"
#include "foundation/file_rw_interface.h"
#include "foundation/build_info.h"
#include "foundation/path_tools.h"
#include "platform/window_system.h"
#include "bind_QuickJS.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace {
struct Host {
    hg::QuickJSVM &vm;
    struct Wait { hg::Window *window; JSValue resolve; };
    std::vector<Wait> waits;
    std::chrono::steady_clock::time_point previous = std::chrono::steady_clock::now();
    ~Host() { for (auto &w : waits) JS_FreeValue(vm.GetContext(), w.resolve); }
    static JSValue NextFrame(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
        auto &host = *static_cast<Host *>(JS_GetContextOpaque(ctx));
        hg::Window *window = nullptr;
        if (argc > 0 && !JS_IsUndefined(argv[0])) {
            if (!hg_quickjs_check_Window(ctx, argv[0])) return JS_ThrowTypeError(ctx, "nextFrame expects a Window");
            hg_quickjs_to_c_Window(ctx, argv[0], &window);
        }
        JSValue callbacks[2];
        JSValue promise = JS_NewPromiseCapability(ctx, callbacks);
        if (JS_IsException(promise)) return promise;
        host.waits.push_back({window, callbacks[0]});
        JS_FreeValue(ctx, callbacks[1]);
        return promise;
    }
    void ServiceFrames() {
        if (waits.empty()) return;
        auto ctx = vm.GetContext();
        auto ready = std::move(waits);
        waits.clear();
        auto now = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(now - previous).count();
        previous = now;
        try { for (auto &w : ready) {
            hg::QuickJSValue resolve(ctx, w.resolve);
            w.resolve = JS_UNDEFINED;
            hg::QuickJSValue frame(ctx, JS_NewObject(ctx));
            if (w.window && hg::IsWindowOpen(w.window)) hg::UpdateWindow(w.window);
            bool closed = w.window && !hg::IsWindowOpen(w.window);
            JS_SetPropertyStr(ctx, frame.value, "closed", JS_NewBool(ctx, closed));
            JS_SetPropertyStr(ctx, frame.value, "dtNs", JS_NewBigInt64(ctx, dt));
            hg::QuickJSValue result(ctx, JS_Call(ctx, resolve.value, JS_UNDEFINED, 1, &frame.value));
            vm.Check(result.value);
        } } catch (...) {
            for (auto &w : ready) JS_FreeValue(ctx, w.resolve);
            throw;
        }
    }
};
int InitHost(JSContext *ctx, JSModuleDef *module) {
    return JS_SetModuleExport(ctx, module, "nextFrame", JS_NewCFunction(ctx, Host::NextFrame, "nextFrame", 1));
}
bool ReadFileModule(const std::string &name, std::string &source) {
    hg::ScopedReadHandle file(hg::g_file_read_provider, name.c_str());
    if (!hg::g_file_reader.is_valid(file)) return false;
    source = hg::LoadString(hg::g_file_reader, file);
    return true;
}
std::string ResolveFileModule(const std::string &base, const std::string &name) {
    if (name == "harfang" || name == "harfang-host") return name;
    // Module identities are absolute paths. Relative imports follow the importing
    // file, including parent directories; they do not depend on the process cwd.
    auto path = hg::IsPathAbsolute(name) ? name : hg::PathJoin(hg::GetFilePath(base), name);
    path = hg::GetAbsolutePath(path);
    if (path.empty()) throw std::runtime_error("Cannot resolve JavaScript module: " + name);
    path = hg::CleanPath(path);
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}
void Usage(std::ostream &out) {
    out << "Usage: hgjs <script.js> [args...]\n"
        << "  -h, --help     Show usage\n"
        << "  -v, --version  Show version\n";
}
}
int main(int argc, char **argv) {
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) { Usage(std::cout); return 0; }
    if (argc == 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")) {
        std::cout << "HARFANG " << hg::get_version_string() << " (QuickJS 2026-06-04)\n";
        return 0;
    }
    if (argc < 2 || argv[1][0] == '-') { Usage(std::cerr); return 2; }
    try {
        const auto entry = ResolveFileModule("", argv[1]);
        hg::QuickJSVM vm;
        auto ctx = vm.GetContext();
        vm.SetBindingCleanup(hg_quickjs_release_harfang);
        if (!hg_quickjs_js_init_module_harfang(ctx, "harfang")) throw std::runtime_error(vm.TakeException());
        vm.SetModuleReader(ReadFileModule, ResolveFileModule);
        hg::QuickJSValue global(ctx, JS_GetGlobalObject(ctx));
        hg::QuickJSValue script_args(ctx, JS_NewArray(ctx));
        vm.Check(script_args.value);
        if (JS_SetPropertyUint32(ctx, script_args.value, 0, JS_NewString(ctx, entry.c_str())) < 0)
            throw std::runtime_error(vm.TakeException());
        for (int i = 2; i < argc; ++i)
            if (JS_SetPropertyUint32(ctx, script_args.value, i - 1, JS_NewString(ctx, argv[i])) < 0)
                throw std::runtime_error(vm.TakeException());
        if (JS_SetPropertyStr(ctx, global.value, "scriptArgs", JS_DupValue(ctx, script_args.value)) < 0)
            throw std::runtime_error(vm.TakeException());
        Host host{vm};
        JS_SetContextOpaque(ctx, &host);
        auto host_module = JS_NewCModule(ctx, "harfang-host", InitHost);
        if (!host_module || JS_AddModuleExport(ctx, host_module, "nextFrame") < 0) throw std::runtime_error(vm.TakeException());
        hg::QuickJSValue completion(ctx, vm.Evaluate("const app = await import(scriptArgs[0]); if (typeof app.main === 'function') await app.main(); else await app.completion;", "<hgjs>"));
        while (JS_PromiseState(ctx, completion.value) == JS_PROMISE_PENDING || JS_IsJobPending(vm.GetRuntime()) || !host.waits.empty()) {
            vm.CheckPromise(completion.value, false);
            int jobs = vm.PumpJobs(64);
            if (!host.waits.empty()) host.ServiceFrames();
            else if (!jobs && JS_PromiseState(ctx, completion.value) == JS_PROMISE_PENDING)
                throw std::runtime_error("Entry completion stalled without host work");
            if (!JS_IsJobPending(vm.GetRuntime())) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        vm.CheckPromise(completion.value, true);
        vm.CheckUnhandledRejections();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "HGJS: " << e.what() << '\n';
        return 1;
    }
}
