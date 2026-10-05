#include "acutest.h"
#include "quickjs_vm.h"

void test_quickjs_vm() {
    for (int i = 0; i < 8; ++i) {
        hg::QuickJSVM vm;
        auto ctx = vm.GetContext();
        vm.SetModuleReader([](const std::string &name, std::string &source) {
            if (name != "value.js") return false;
            source = "export const value = 9007199254740993n; export function factory() { let n = 0; return () => ++n; }";
            return true;
        });
        hg::QuickJSValue completion(ctx, vm.Evaluate(
            "import {value, factory} from './value.js';"
            "const next = factory(); if (next() !== 1 || next() !== 2) throw Error('closure');"
            "if (await Promise.resolve(value) !== 9007199254740993n) throw Error('BigInt');", "main.js"));
        for (int n = 0; n < 10 && JS_PromiseState(ctx, completion.value) == JS_PROMISE_PENDING; ++n) vm.PumpJobs(32);
        TEST_CHECK(JS_PromiseState(ctx, completion.value) == JS_PROMISE_FULFILLED);
        hg::QuickJSValue rejected(ctx, vm.Evaluate("await Promise.reject(Error('expected'));", "rejected.js"));
        vm.PumpJobs(32);
        bool caught = false;
        try { vm.CheckPromise(rejected.value, true); } catch (const std::exception &e) { caught = std::string(e.what()).find("expected") != std::string::npos; }
        TEST_CHECK(caught);
    }
}

TEST_LIST = {{"quickjs.runtime", test_quickjs_vm}, {nullptr, nullptr}};
