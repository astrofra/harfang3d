# Squirrel Language Integration Feasibility

Date: 2026-08-30

Re-evaluation of the 2026-04-15 draft after FABGen gained a working Squirrel backend and after revisiting the public-interpreter packaging target.

## Executive Summary

Adding Squirrel to Harfang remains feasible, and the feasibility assessment is materially better than it was on April 15, 2026.

The main change is that FABGen Squirrel support is no longer hypothetical. The repository now contains:

- `FABGen/lang/squirrel.py`
- `FABGen/lib/squirrel/__init__.py`
- `FABGen/lib/squirrel/std.py`
- `FABGen/lib/squirrel/stl.py`
- `FABGen/bind.py --squirrel`
- a dedicated Squirrel test host in `FABGen/tests.py`

The local FABGen progress notes reported a validated Squirrel suite at `30 run, 0 failed, 0 skipped` on Sunday, August 30, 2026, and that same full suite was re-run successfully during this re-evaluation with the same result. The backend already covers the core binding surface needed for Harfang to start integration work: functions, classes, enums, inheritance, operators, sequences, `std::vector`, `std::map<std::string, int>`, `std::function<>` callbacks, extern types, `std::future`, type-info export, and embedded/non-embedded module initialization.

The second important change is about packaging. Stock Squirrel does not provide a Lua-style native-module loader for `.dll` / `.so`, but if Harfang vendors Squirrel, adding one is practical and aligns well with the existing FABGen export model. In other words:

- unmodified stock `sq.exe` is not enough,
- a Harfang-provided vendored `hg_squirrel` interpreter with native-module loading is feasible,
- that interpreter can load a Harfang Squirrel module DLL through a stable exported entry point.

As of Sunday, August 30, 2026, that vendoring step is no longer theoretical in the local branch. The Squirrel source tree is now present at `harfang3d/extern/squirrel`, populated from upstream commit `f9267f2` dated February 28, 2026.

This changes the project classification:

- Before: greenfield language-backend effort plus engine integration.
- Now: Harfang integration, API adaptation, packaging, and test-hardening effort built on an existing backend.

The remaining work is still substantial, but the critical path has moved. The primary risk is no longer "can FABGen support Squirrel classes at all?"; it is "how much Harfang-specific integration and API shaping is required to expose Squirrel cleanly and maintainably?"

The recommended first public gate is no longer hypothetical. It was reached locally on Sunday, August 30, 2026:

1. `harfang3d/binding/bind_harfang.py` now generates under `--squirrel`,
2. the non-embedded Harfang Squirrel module DLL compiles,
3. a vendored `hg_squirrel` interpreter with native-module loading builds from `harfang3d/extern/squirrel`,
4. an installed package containing `hg_squirrel.exe`, `harfang.dll`, `squirrel.dll`, `sqstdlib.dll`, `glfw3.dll`, and `lua54.dll` runs a smoke script through `require("harfang")` and `include()`,
5. the packaged `assetc.exe` copies a `.nut` file unchanged into compiled assets,
6. `rebuild_hg_squirrel.bat` rebuilds and installs the full package using the vendored default path, with no external `SQUIRREL_DIR`.

That proves the public red line is technically viable. `SceneSquirrelVM` should be treated as phase 2.

## What Changed Since April 15, 2026

The previous draft correctly identified the required architecture, but it overstated backend uncertainty because the backend did not exist yet.

What is now present in FABGen:

- Native Squirrel class/instance binding based on `sq_setclassudsize()` and `sq_settypetag()`.
- Generated release hooks for wrapped C++ instance ownership.
- Non-owning instance caching so repeated returns of the same C++ object can reuse the same Squirrel proxy.
- Reverse callback support using captured Squirrel closures stored through `SquirrelValueRef`.
- Public generated type info APIs:
  - `gen_get_bound_type_info(uint32_t type_tag)`
  - `gen_get_c_type_info(const char *type)`
- Embedded and non-embedded module entry points:
  - `gen_create_<module>(v)`
  - `gen_bind_<module>(v, symbol)`
  - `gen_release_<module>(v)`
  - `sqmodule_<module>(v)`
- Standard converters for:
  - booleans
  - integers
  - floats/doubles
  - `const char *`
  - `std::string`
  - `std::vector<T>` through Squirrel arrays
  - `std::map<K, V>` through Squirrel tables
  - `std::function<>` through Squirrel closures

This no longer merely removes the original "backend from scratch" assumption. During this re-evaluation, a first full `bind_harfang.py --squirrel` generation was executed and compiled successfully after two concrete fixes:

- Squirrel array-to-`std::vector<T>` adaptation was added to `harfang3d/binding/bind_harfang.py`,
- primitive/string `from_c()` return types in `FABGen/lib/squirrel/std.py` and `FABGen/lib/squirrel/stl.py` were corrected from `int` to `SQInteger` for MSVC-compatible generated code.

The `sqmodule_<module>(v)` export is especially important for public packaging because it gives Harfang a clean ABI for a custom Squirrel native-module loader.

## Current Lua Integration In Harfang

The current Lua implementation is still the right structural template.

It is split into three layers:

1. Low-level VM utilities in `harfang/script/lua_vm.h` and `harfang/script/lua_vm.cpp`.
   These wrap `lua_State`, references stored in the Lua registry, compilation, execution, calls, custom error handling, watchdog hooks, and basic value creation.

2. Cross-VM and cross-language object transfer in `harfang/engine/lua_object.h` and `harfang/engine/lua_object.cpp`.
   `LuaObject` is an opaque handle. `PushForeign()` can transfer primitives and Fabgen-wrapped C++ objects between Lua states by using the generated `hg_lua_type_info` API from `bind_Lua.h`.

3. Scene script integration in `harfang/engine/scene_lua_vm.h`, `harfang/engine/scene_lua_vm.cpp`, and `harfang/engine/scene_systems.cpp`.
   `SceneLuaVM` keeps one Lua environment per `ComponentRef`, exposes a shared `G` table, injects the generated `hg` table, applies `ScriptParam` values, and dispatches scene/node callbacks through generated reverse-binding functions.

Build-wise, the engine already has the pattern that Squirrel should follow:

- `binding/CMakeLists.txt` generates the embedded Lua binding object library.
- `harfang/engine/CMakeLists.txt` links the engine against the generated embedded binding.
- `languages/hg_lua/CMakeLists.txt` generates the public Lua package and launcher.
- `tools/assetc/assetc.cpp` has Lua-specific source/bytecode handling.

That architecture is still sound. What changed is that the Squirrel side now has a realistic generator target to plug into it.

## Revised Squirrel Fit

The official Squirrel runtime still has the primitives required for Harfang's scene-script architecture:

- VM lifecycle: `sq_open`, `sq_close`
- Compilation and execution: `sq_compilebuffer`, `sq_call`
- Object lifetime: `HSQOBJECT`, `sq_getstackobj`, `sq_addref`, `sq_release`, `sq_pushobject`
- Tables, arrays, classes, typetags, release hooks, closures
- Standard libraries and error helpers through `sqstd*`

The new FABGen backend materially strengthens that fit:

- `lang/squirrel.py` already emits class wrappers that look like Harfang-usable runtime objects.
- It already emits callback holders that survive beyond the immediate call.
- It already exports enough type metadata to support a Harfang-side script object abstraction.
- It already provides a release helper that the host must call before `sq_close(v)`.

The embedded-host contract is now clearer than it was in April. `FABGen/tests.py` already uses this model:

1. open a Squirrel VM,
2. register standard libraries,
3. bind the generated module into the root table,
4. run the script,
5. call `gen_release_<module>(v)`,
6. then `sq_close(v)`.

That is a credible basis for both a Harfang launcher and a `SceneSquirrelVM`.

## Vendored Native Module Loading

If the goal is to let the Squirrel interpreter load native Harfang bindings the way Lua users expect, then vendorizing Squirrel is the right move.

### Stock Squirrel Limitation

The stock `sq.exe` source currently does a very small amount of setup:

1. open a VM,
2. register the built-in blob, IO, system, math, and string libraries,
3. load and execute a script.

There is no built-in `require()` equivalent that searches for native modules and loads a `.dll` / `.so`.

### Why A Harfang Loader Is Practical

This limitation is not architectural. It is simply absent from the stock runtime surface.

Harfang can vendor Squirrel and add either:

- a `require(name)` implementation, or
- a lower-level `loadmodule(path)` plus a small script-side `require()`.

That custom loader can rely on the ABI already emitted by FABGen in non-embedded mode:

- `extern "C" _DLL_EXPORT_ SQRESULT sqmodule_<module>(HSQUIRRELVM v)`

The generated function creates the module object and leaves it on the Squirrel stack, which is exactly what a native loader needs.

### Recommended Loader Model

Recommended public behavior:

1. `require("harfang")` resolves the logical module name against a Harfang-defined script path and native path.
2. For script modules:
   - try `.nut`
   - optionally try compiled `.cnut`
3. For native modules:
   - try `.dll` on Windows
   - try `.so` on Linux
   - try `.dylib` on macOS
4. Load the dynamic library with the host OS API.
5. Resolve `sqmodule_<module_name>`.
6. Call it with the current `HSQUIRRELVM`.
7. Cache the returned module object in a hidden loaded-module table.
8. Return the cached module on subsequent `require()` calls.

That gives Harfang the Lua-like public entry point it needs without requiring embedded scene-VM integration first.

### Harfang Packaging Consequence

With that model, the public target becomes:

- a vendored `hg_squirrel` interpreter executable,
- a generated non-embedded Harfang Squirrel module DLL,
- a Harfang-defined module search path,
- optional helpers such as `include(path)` for script composition.

This is feasible and, for the first milestone, simpler than integrating Squirrel into the engine scene runtime.

## Current Harfang Delta

The missing pieces are now mostly in `harfang3d`, not in FABGen.

### Engine and Runtime Work Still Missing

Expected new Harfang files still do not exist:

- `harfang/script/squirrel_vm.h`
- `harfang/script/squirrel_vm.cpp`
- `harfang/engine/squirrel_object.h`
- `harfang/engine/squirrel_object.cpp`
- `harfang/engine/scene_squirrel_vm.h`
- `harfang/engine/scene_squirrel_vm.cpp`

`scene_systems.cpp` and `scene_systems.h` still only expose `SceneLuaVM` overloads. Squirrel needs either:

- parallel overloads for `SceneSquirrelVM`, or
- an internal shared helper layer with thin Lua/Squirrel wrappers on top.

### Binding Script Work Still Missing

The public binding milestone is now partially in place:

- `bind_std_vector()` now emits Squirrel array-to-`std::vector<T>` adapters,
- `expand_std_vector_proto()` now expands Squirrel-friendly overload names,
- `bind_harfang.py --squirrel` now generates and compiles as a non-embedded module,
- `languages/hg_squirrel` now exists with a launcher, install rules, `bin.nut`, a Windows rebuild helper, and first tutorial ports,
- `extern/squirrel` is now vendored in-tree and recorded in `extern/versions.txt`.

What is still missing in the binding layer is the embedded and cross-VM side:

- `bind_LuaObject()` is still Lua-specific and there is no Squirrel equivalent yet,
- `bind_lua_scene_vm()` still only binds `hg::SceneLuaVM`,
- `bind_scene_systems()` still only exposes the `SceneLuaVM` overload families.

### Asset Compiler and Packaging State

The asset toolchain now has a minimal phase-1 Squirrel policy:

- `tools/assetc/assetc.cpp` special-cases `.nut` as `AssetType::Squirrel` and copies it unchanged.
- `tools/assetc/CMakeLists.txt` still only installs `luac` as a script compiler, which is acceptable because phase 1 intentionally keeps Squirrel as source.
- The packaged `assetc.exe` under `hg_squirrel/harfang/assetc` was smoke-tested locally on Sunday, August 30, 2026 with a `.nut` input/output match.

For the public Squirrel target, the packaging work now has two distinct parts:

- keep the vendored Squirrel interpreter build reproducible in-tree,
- package Harfang as a loadable Squirrel native module.

For script assets, the phase-1 policy should remain:

1. accept `.nut` source files,
2. copy them unchanged into compiled assets,
3. compile them at runtime,
4. defer bytecode until compatibility is proven.

## FABGen Impact Reclassified

FABGen is no longer the project's primary feasibility blocker.

It is still a source of integration risk, but the nature of the risk changed:

- not "build the language backend from scratch",
- now "run the very large Harfang binding script through the existing backend and fix the gaps it exposes".

That distinction matters.

The Harfang binding surface is much larger than the current FABGen Squirrel test suite, so the first full `bind_harfang.py --squirrel` generation remained a necessary checkpoint. That checkpoint has now been exercised successfully, but it still does not prove every future Harfang API addition will pass untouched.

However, the baseline capabilities Harfang needs are already present:

- class binding
- inheritance
- routed methods
- enum exposure
- callbacks/reverse calls
- vectors and tables
- argument-out patterns
- non-copyable and move-oriented cases
- module-level variables

The remaining FABGen-specific concerns are narrower:

- The primitive and STL Squirrel converters needed one real backend fix during this work:
  generated `from_c()` functions must return `SQInteger`, not `int`, otherwise MSVC rejects the generated Harfang binding.

- The class `from_c` path currently expects the generated module to be bound into the Squirrel root table.
  This is acceptable for Harfang, and a vendored `require("harfang")` implementation can satisfy it explicitly by binding the loaded module into the root table before returning it.

- The public Squirrel backend does not currently export a Lua-style helper equivalent to `hg_lua_get_wrapped_object_type_tag(L, idx)`.
  If Harfang wants `SquirrelObject` to mirror `LuaObject::PushForeign()` across multiple VMs, FABGen will likely need one small additional public helper for wrapped-object type-tag extraction.
  If Harfang accepts same-VM semantics first, this can be deferred.

- The generated release helper `gen_release_<module>(v)` is mandatory for safe shutdown when C++ holds captured Squirrel callbacks.
  Harfang must build this into VM teardown from day one for embedded use, and should also honor it in the vendored interpreter when the public module exposes callback-capturing APIs.

## Public API Differences From Lua

This is now a real product decision, not a theoretical concern.

The Squirrel backend does not match Lua semantics in several places, and Harfang should not pretend that it does.

### Multi-Result Returns

Stock Squirrel native closures return zero or one VM value.

FABGen therefore exposes multiple script-visible results as a packed Squirrel array. This directly affects Harfang because `harfang3d/binding/bind_harfang.py` currently contains about 100 `arg_out` / `arg_in_out` declarations.

Implication:

- the Squirrel API cannot be source-compatible with Lua on those signatures,
- unless Harfang adds manual Squirrel-specific wrappers for selected high-value functions.

The right policy is to document this difference and only wrap special cases that matter for ergonomics.

### Static Mutable Members

The Squirrel backend exposes mutable static data through explicit accessors:

- `MyType.get_value()`
- `MyType.set_value(v)`

That is acceptable, but it is not property-style parity with all other bindings.

### Equality And Deep Comparison

Squirrel `==` and `!=` on distinct bound class instances remain identity-based at the VM level.
FABGen supports deep comparison through `_cmp`, which maps to `<=>`-style comparisons in Squirrel.

This is acceptable for Harfang, but it must be documented anywhere value-style comparison is expected.

### Sequence Behavior

Squirrel sequence support is already useful:

- integer `_get`
- integer `_set`
- explicit `len()`
- `foreach` through `_nexti`

That is enough for Harfang to move forward, but it is not a promise of native-container parity for every wrapped container type.

## Risks And Open Questions

The remaining risks are now:

- Ongoing generator edge-case risk.
  The first full generation/compile pass is now proven, but future Harfang APIs may still expose uncovered ownership or overload patterns outside the current FABGen Squirrel suite.

- `SceneSquirrelVM` environment semantics.
  The per-script root table model still needs a Harfang-side prototype for top-level declarations, `G`, injected `hg`, and callback lookup.

- Script-object transport policy.
  Harfang must decide whether `SquirrelObject` needs cross-VM object transfer semantics comparable to `LuaObject`, or whether same-VM semantics are enough for phase 1.

- API divergence due to single-return semantics.
  The packed-array policy for multiple outs is a real user-facing difference and will affect docs, examples, and migration guidance.

- Native loader policy.
  The main question is no longer whether Squirrel can load DLLs at all; it can, if Harfang vendors the interpreter. The real decisions are module naming, search-path behavior, cache semantics, and whether Harfang ever unloads native modules before process exit.

- Auxiliary native module parity.
  Tutorial dependencies that currently exist only as Lua native modules need Squirrel-specific equivalents. In practice `harfang3d/tutorials/hg_lua/say.dll` should become a Harfang Squirrel native module DLL, for example `say.dll` exporting `sqmodule_say(HSQUIRRELVM)`, so `audio_play_tts_say.nut` can use the same packaged `require("say")` flow as `require("harfang")`.

- Public interpreter versus embedded VM sequencing.
  The vendored `hg_squirrel` interpreter is now the fastest public red line. `SceneSquirrelVM` remains useful, but it should not block the first milestone.

- Asset policy.
  Runtime `.nut` loading is low risk. Bytecode should remain out of scope for phase 1.

- Security posture.
  Registering the full Squirrel IO/system libraries already exposes filesystem and process access. Adding native `.dll` / `.so` loading increases the trust boundary further. Harfang should treat the public interpreter as trusted-code tooling, not as a sandbox.

## Effort Estimate

Revised engineering estimate:

| Work item | Estimate |
| --- | ---: |
| Vendor/build Squirrel in `extern` and add a native-module loader to the interpreter | achieved locally on Sunday, August 30, 2026 |
| Make `bind_harfang.py --squirrel` generate and compile as a non-embedded native module, fix exposed Harfang/FABGen gaps | 1-2 weeks |
| Add `languages/hg_squirrel`, package search paths, bootstrap scripts, docs, and example ports | 4-7 days |
| Optional phase 2: implement `SquirrelObject`, `squirrel_vm`, `SceneSquirrelVM`, and scene-system overloads | 2-4 weeks |

Revised total as of Sunday, August 30, 2026:

- Public `hg_squirrel` red line with in-tree vendored Squirrel, DLL loading, installable packaging, non-interactive smoke validation, and packaged `assetc` `.nut` passthrough: achieved locally.
- Remaining public productization work (broader tutorial/runtime validation, docs, packaging polish): about 1-2 engineer-weeks.
- Full embedded scene-scripting integration afterward: an additional 3-5 engineer-weeks.

That is materially lower-risk than the original 10-18 engineer-week estimate because the language backend is no longer a greenfield item, and because the public interpreter milestone can now be split cleanly from embedded scene scripting.

## Recommended Implementation Plan

1. Keep Squirrel vendored in-tree and preserve the current public `hg_squirrel` launcher model:
   - `require()` native-module loading
   - module search paths
   - loaded-module cache
   - `include()` for script composition
2. Keep the current public package stable:
   - install `hg_squirrel.exe`, `harfang.dll`, `squirrel.dll`, `sqstdlib.dll`, `glfw3.dll`, and `lua54.dll`
   - keep `rebuild_hg_squirrel.bat` aligned with `rebuild_hg_lua.bat`
   - keep the launcher shutdown order safe by unloading native modules only after `sq_close()`
3. Keep `.nut` passthrough support in `assetc`; defer Squirrel bytecode.
4. Run the first public tutorial ports end-to-end in the packaged runtime:
   - `basic_loop.nut`
   - `draw_and_create_model_no_pipeline.nut`
   - extend that validation to audio tutorials, noting that `audio_play_tts_say` additionally requires a Squirrel-native `say.dll`
5. Decide whether the public launcher should call an exported module release helper before `sq_close()`.
   The generated `gen_release_<module>(v)` function exists, but it is not currently exported with a C ABI for dynamic lookup.
6. Add Squirrel-native companion modules for tutorial dependencies that are currently Lua-only:
   - port `harfang3d/tutorials/hg_lua/say.dll` to a Squirrel Harfang module DLL
   - keep the user-facing entry point as `require("say")` in `hg_squirrel`
7. Only after the public DLL-loading line is stable, start phase 2 embedded runtime work:
   - implement `SquirrelObject` and `squirrel_vm.*`
   - honor `gen_release_harfang(v)` before `sq_close(v)`
   - implement `SceneSquirrelVM`
   - add `SceneSquirrelVM` overloads in `scene_systems.*`
8. Port the core Lua scene tests to Squirrel and prove at least:
   - one scene script,
   - one node script,
   - one callback path,
   - one `GetScriptValue` / `SetScriptValue` path.
9. Document Squirrel-specific API differences instead of trying to hide them everywhere.

## Conclusion

The April 15, 2026 conclusion should be revised.

Squirrel integration in Harfang is still a medium integration project, but it is no longer blocked on inventing a language backend. `FABGen/lang/squirrel.py` and its companion Squirrel support code substantially de-risk the original plan.

With Squirrel now vendored in-tree locally, the public path is no longer just clearer; it is already demonstrated locally:

- build a vendored `hg_squirrel` interpreter,
- add a native-module loader,
- load the Harfang binding DLL through `require("harfang")`,
- use that as the first public milestone.

The dominant remaining work is now in Harfang itself:

- integrate the Squirrel runtime,
- add `SquirrelObject` and `SceneSquirrelVM`,
- adapt the binding script,
- decide how much Lua parity is worth preserving where Squirrel semantics differ,
- package and document the result.

The concrete public checkpoint has already been demonstrated locally on Sunday, August 30, 2026: generate the non-embedded Squirrel binding, load it from the vendored in-tree Squirrel interpreter, run a script through `require("harfang")` plus `include()`, and rebuild the package through `rebuild_hg_squirrel.bat` without an external Squirrel checkout. That means the public packaging strategy should now be considered technically viable. Embedded scene scripting can follow as phase 2.

## Sources

- Harfang local code inspected:
  - `harfang3d/binding/bind_harfang.py`
  - `harfang3d/extern/versions.txt`
  - `harfang3d/extern/squirrel/CMakeLists.txt`
  - `harfang3d/languages/hg_squirrel/CMakeLists.txt`
  - `harfang3d/languages/hg_squirrel/launcher.cpp`
  - `harfang3d/languages/hg_squirrel/bin.nut`
  - `harfang3d/rebuild_hg_squirrel.bat`
  - `harfang3d/tutorials/basic_loop.nut`
  - `harfang3d/tutorials/draw_and_create_model_no_pipeline.nut`
  - `harfang3d/harfang/script/lua_vm.h`
  - `harfang3d/harfang/script/lua_vm.cpp`
  - `harfang3d/harfang/engine/lua_object.h`
  - `harfang3d/harfang/engine/lua_object.cpp`
  - `harfang3d/harfang/engine/scene_lua_vm.h`
  - `harfang3d/harfang/engine/scene_lua_vm.cpp`
  - `harfang3d/harfang/engine/scene_systems.h`
  - `harfang3d/harfang/engine/scene_systems.cpp`
  - `harfang3d/tools/assetc/assetc.cpp`
  - `harfang3d/tools/assetc/CMakeLists.txt`
  - `harfang3d/languages/hg_lua/CMakeLists.txt`
  - `harfang3d/languages/hg_lua/launcher.cpp`

- FABGen local code inspected:
  - `FABGen/bind.py`
  - `FABGen/lang/squirrel.py`
  - `FABGen/lib/squirrel/std.py`
  - `FABGen/lib/squirrel/stl.py`
  - `FABGen/lang/lua.py`
  - `FABGen/tests.py`

- Local Squirrel runtime code and docs inspected:
  - `harfang3d/extern/squirrel/*`
  - `%TEMP%/fabgen_squirrel_ref2/sq/sq.c`
  - `%TEMP%/fabgen_squirrel_ref2/doc/source/stdlib/introduction.rst`
  - `%TEMP%/fabgen_squirrel_ref2/doc/source/stdlib/stdiolib.rst`
  - `%TEMP%/fabgen_squirrel_ref2/doc/source/stdlib/stdsystemlib.rst`
  - `%TEMP%/fabgen_squirrel_ref2/doc/source/reference/language/builtin_functions.rst`

- FABGen local progress notes inspected:
  - `FABGen/specifications/SQUIRREL_MVP_TESTS_AND_BINDING_2026-08-29.md`
  - `FABGen/specifications/SQUIRREL_CLASS_BINDING_PHASE2_2026-08-29.md`

- Local validation re-run during this re-evaluation:
  - `cd FABGen`
  - `python tests.py --sqbase "$env:TEMP\fabgen_squirrel_ref2"`
  - observed result: `30 run, 0 failed, 0 skipped`
  - `cmake -S harfang3d -B .codex_tmp/build-hg-squirrel-min ... -DHG_BUILD_HG_SQUIRREL=ON -DHG_SQUIRREL_PATH=%TEMP%/fabgen_squirrel_ref2`
  - `cmake --build .codex_tmp/build-hg-squirrel-min --config RelWithDebInfo --target hg_squirrel_launcher`
  - `cmake --install .codex_tmp/build-hg-squirrel-min --config RelWithDebInfo --component squirrel --prefix .codex_tmp/install-hg-squirrel-20260830`
  - `link.exe /dump /dependents .../harfang.dll` identified `lua54.dll` as a required packaged dependency
  - `.../install-hg-squirrel-20260830/hg_squirrel/hg_squirrel.exe .codex_tmp/squirrel_smoke/smoke_require.nut`
  - observed result: `hg_squirrel smoke ok`
  - `.../tools/assetc/RelWithDebInfo/assetc.exe -verbose -toolchain .../assetc/toolchains/host-windows-x64-target-windows-x64 .codex_tmp/assetc_nut_smoke/input .codex_tmp/assetc_nut_smoke/output`
  - observed result: `Squirrel script 'scripts/test.nut'` compiled as a copied source asset
  - `cmd /c "set BUILD_DIR=...build-hg-squirrel-min&& set INSTALL_DIR=...install-rebuild-hg-squirrel-20260830&& set FABGEN_DIR=...\\FABGen&& set SQUIRREL_DIR=%TEMP%\\fabgen_squirrel_ref2&& harfang3d\\rebuild_hg_squirrel.bat RelWithDebInfo"`
  - observed result: `HG Squirrel + AssetC rebuild ok.`
  - `.../install-rebuild-hg-squirrel-20260830/hg_squirrel/harfang/assetc/assetc.exe -verbose .codex_tmp/assetc_nut_smoke/input .codex_tmp/assetc_nut_smoke/output_pkg`
  - observed result: `.codex_tmp/assetc_nut_smoke/output_pkg/scripts/test.nut` matched the input byte-for-byte
  - `cmd /c "set BUILD_DIR=...build-hg-squirrel-vendored&& set INSTALL_DIR=...install-hg-squirrel-vendored-20260830&& set FABGEN_DIR=...\\FABGen&& harfang3d\\rebuild_hg_squirrel.bat RelWithDebInfo"`
  - observed result: `Checking Squirrel path - ok` using the default `harfang3d/extern/squirrel` vendor tree, followed by `HG Squirrel + AssetC rebuild ok.`
  - `.../install-hg-squirrel-vendored-20260830/hg_squirrel/hg_squirrel.exe .codex_tmp/squirrel_smoke/smoke_require.nut`
  - observed result: `hg_squirrel smoke ok`
  - `.../install-hg-squirrel-vendored-20260830/hg_squirrel/harfang/assetc/assetc.exe -verbose .codex_tmp/assetc_nut_smoke/input .codex_tmp/assetc_nut_smoke/output_pkg_vendored`
  - observed result: `.codex_tmp/assetc_nut_smoke/output_pkg_vendored/scripts/test.nut` matched the input byte-for-byte
