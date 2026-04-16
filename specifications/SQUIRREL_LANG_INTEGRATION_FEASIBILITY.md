# Squirrel Language Integration Feasibility

Date: 2026-04-15

## Executive Summary

Adding Squirrel to Harfang with the same practical status as Lua is feasible, but it is a medium-to-large integration project. The engine-side scripting model is already close to language-neutral: `Scene::Script_` stores a path and typed parameters, and `SceneLuaVM` owns the language-specific execution state. The largest cost is not the scene system itself; it is creating a production-quality Squirrel backend in Fabgen and then using that backend twice, once for the public Squirrel Harfang API and once for the embedded scene VM binding.

The recommended target is:

- A new public Squirrel package/launcher built on the official Squirrel 3.2 interpreter and runtime.
- A new `SceneSquirrelVM` with API parity to `SceneLuaVM`.
- Generated embedded Squirrel bindings for the full Harfang API, using the same Fabgen binding script where possible.
- The same scene callback surface as Lua: `OnAttachToScene`, `OnDetachFromScene`, `OnAttachToNode`, `OnDetachFromNode`, `OnUpdate`, `OnCollision`, `OnDestroy`, and any pipeline callbacks generated through reverse bindings.

The main qualification is that Squirrel does not provide the same de facto binary module loading convention as Lua's `require("harfang")`. A Harfang Squirrel distribution can still be based on the official interpreter, but it should probably ship as a Harfang-provided `hg_sq`/launcher or as an official `sq` build with Harfang linked and initialized. If the requirement is an unmodified stock `sq` executable dynamically discovering `harfang`, that part is not equivalent to Lua without adding a Squirrel-side loading convention.

## Current Lua Integration In Harfang

The current Lua implementation is split into three layers:

1. Low-level VM utilities in `harfang/script/lua_vm.h` and `harfang/script/lua_vm.cpp`.
   These wrap `lua_State`, references stored in the Lua registry, stack guards, compilation, execution, calls, custom error handling, watchdog hooks, and basic value creation.

2. Cross-VM and cross-language object transfer in `harfang/engine/lua_object.h` and `harfang/engine/lua_object.cpp`.
   `LuaObject` is an opaque handle. `PushForeign` can transfer primitives and Fabgen-wrapped C++ objects between Lua VMs by using the generated `hg_lua_type_info` API from `bind_Lua.h`.

3. Scene script integration in `harfang/engine/scene_lua_vm.h`, `harfang/engine/scene_lua_vm.cpp`, and `harfang/engine/scene_systems.cpp`.
   `SceneLuaVM` keeps one Lua environment per `ComponentRef`, exposes a shared `G` table, injects the generated `hg` table, applies `ScriptParam` values, and dispatches scene/node callbacks through Fabgen reverse-binding functions such as `hg_lua_OnUpdate_NodeCtx`.

Build-wise, the engine always has an embedded Lua binding object library:

- `binding/CMakeLists.txt` generates `bind_Lua.cpp` with `--lua --embedded --prefix hg_lua`.
- `harfang/engine/CMakeLists.txt` links `engine` against `bind_hg_lua` and `script`.
- `languages/hg_lua/CMakeLists.txt` generates the non-embedded Lua extension, builds the `harfang` Lua module, and installs the Lua launcher.
- `tools/assetc` has special handling for `.lua`, runs `luac`, and installs `luac` in the asset compiler toolchain.

This is a good model to clone conceptually, but not mechanically: many pieces are strongly tied to Lua's stack, registry, `_ENV`, module loading, and metatable semantics.

## Squirrel Fit

The official Squirrel runtime has the primitives needed to implement the same architecture:

- VM lifecycle: `sq_open`, `sq_close`, `sq_newthread`.
- Compilation and execution: `sq_compilebuffer`, `sq_call`, `sq_resume`, `sq_getlasterror`.
- Object lifetime: `HSQOBJECT`, `sq_getstackobj`, `sq_addref`, `sq_release`, `sq_pushobject`.
- Tables, arrays, classes, userdata, typetags, release hooks, and root/registry tables.
- Native closures through `sq_newclosure`.
- Error and debug integration through `sqstd_seterrorhandlers`, `sq_stackinfos`, `sq_setdebughook`, and `sq_setnativedebughook`.
- Standard libraries through `sqstd_register_bloblib`, `sqstd_register_iolib`, `sqstd_register_systemlib`, `sqstd_register_mathlib`, and `sqstd_register_stringlib`.

The official build is compatible with Harfang's C++14 baseline: Squirrel's own CMake uses C++11 for the runtime, provides static and shared targets, and supports 64-bit builds through `_SQ64`.

The VM abstraction should therefore be realistic:

```cpp
class SquirrelObject {
public:
    SquirrelObject();
    SquirrelObject(HSQUIRRELVM vm, HSQOBJECT obj);
    SquirrelObject(const SquirrelObject &);
    SquirrelObject(SquirrelObject &&);
    ~SquirrelObject();

    void Push() const;
    void Clear();

private:
    HSQUIRRELVM vm;
    HSQOBJECT obj;
};
```

The equivalent of `SceneLuaVM` can use one Squirrel root/environment table per script. A script can be compiled with that table as the closure root, then called with that table as the `this`/root argument. The shared `G` table and generated `hg` module can be inserted into each script root table. This should preserve the user-facing programming model:

```squirrel
function OnUpdate(node, dt) {
    G.frame_count += 1;
}
```

The exact environment setup needs a prototype because Squirrel uses root tables and closure roots rather than Lua 5.x `_ENV`. The API surface exists, but the behavior must be validated for top-level function declarations, global assignment, and function lookup.

## Engine Impact

### New Engine Modules

Expected new files:

- `harfang/script/squirrel_vm.h`
- `harfang/script/squirrel_vm.cpp`
- `harfang/engine/squirrel_object.h`
- `harfang/engine/squirrel_object.cpp`
- `harfang/engine/scene_squirrel_vm.h`
- `harfang/engine/scene_squirrel_vm.cpp`

The new `SceneSquirrelVM` should intentionally mirror `SceneLuaVM`:

- `CreateScriptFromSource`
- `CreateScriptFromFile`
- `CreateScriptFromAssets`
- `CreateNodeScriptsFromFile`
- `CreateNodeScriptsFromAssets`
- `SceneCreateScriptsFromFile`
- `SceneCreateScriptsFromAssets`
- `GarbageCollect`
- `DestroyScripts`
- `GetScriptEnv`
- `GetScriptValue`
- `SetScriptValue`
- `Call`
- `MakeSquirrelObject`
- `GetScriptInterface`
- `OverrideScriptSource`

The scene component itself can remain `Script` rather than becoming `LuaScript`/`SquirrelScript`. It is already language-neutral in storage and serialization. This keeps save files mostly stable and avoids duplicating node/scene script slots.

### Scene System Dispatch

`scene_systems.cpp` currently dispatches Lua callbacks directly through `SceneLuaVM` and generated `hg_lua_*` functions. Squirrel needs either:

- A parallel set of overloads accepting `SceneSquirrelVM`, or
- A small internal template/helper layer to share traversal and garbage collection logic between Lua and Squirrel.

For public API clarity, explicit overloads are still preferable:

- `SceneSyncToSystemsFromFile(Scene &, SceneSquirrelVM &)`
- `SceneSyncToSystemsFromAssets(Scene &, SceneSquirrelVM &)`
- `SceneUpdateSystems(Scene &, SceneClocks &, time_ns, SceneSquirrelVM &)`
- `SceneGarbageCollectSystems(Scene &, SceneSquirrelVM &)`
- `SceneClearSystems(Scene &, SceneSquirrelVM &)`

Bullet physics overloads would need the same combinations currently provided for Lua.

### Mixed-Language Scenes

The current `Script` component has no language field. That is fine for single-language scenes: the application decides whether to feed script components to `SceneLuaVM` or `SceneSquirrelVM`.

Mixed Lua/Squirrel scenes are the first design decision:

- Option A: Keep `Script` language-neutral and document that a scene is evaluated by one script VM family at a time.
- Option B: Add a `ScriptLanguage` field, for example `Auto`, `Lua`, `Squirrel`, and have each VM ignore non-matching scripts.

Option B is better for long-term tooling, editors, and mixed projects, but it impacts scene binary/JSON serialization, docs, UI, and migration. Option A is lower risk for a first implementation.

### Asset Compiler

`assetc` currently classifies `.lua` files and invokes `luac`, producing Lua bytecode at the same asset path. Squirrel has an official bytecode path through the `sq` interpreter's compile mode, but bytecode compatibility depends on Squirrel configuration such as `_SQ64`, `SQUSEDOUBLE`, and character mode.

Recommended asset strategy:

1. Phase 1: copy `.nut`/`.squirrel` sources unchanged and compile at runtime.
2. Phase 2: add optional Squirrel bytecode generation once host-target compatibility is proven.

This avoids making the first Squirrel integration depend on bytecode portability.

## Fabgen Impact

Fabgen is the critical path.

### Required Fabgen Additions

Expected new files in `FABGen`:

- `lang/squirrel.py`
- `lib/squirrel/__init__.py`
- `lib/squirrel/std.py`
- `lib/squirrel/stl.py`
- Squirrel-specific tests beside the existing Lua/CPython/Go tests.

`bind.py` needs a `--squirrel` target. `lib.bind_defaults` and `lib.stl.bind_function_T` need Squirrel branches.

The Squirrel generator must implement the same generator contract as `lang/lua.py`:

- Native proxy generation: `static SQInteger proxy(HSQUIRRELVM v)`.
- Argument count and overload dispatch.
- Conversion checks and `to_c`/`from_c` glue.
- Primitive types, strings, pointers, references, const references, and enums.
- Class wrappers with constructors, methods, static methods, members, static members, comparison/arithmetic operators where meaningful.
- `std::vector<T>` conversion from Squirrel arrays.
- `std::function<>` conversion from Squirrel closures.
- Reverse bindings for script callbacks and pipeline callbacks.
- Public type info API equivalent to `hg_lua_type_info`.
- Embedded and non-embedded module initialization paths.

The Lua backend is the closest reference, but several semantics differ:

- Lua tables are often used as both arrays and dictionaries; Squirrel has arrays and tables as separate types.
- Lua userdata/metatables map naturally to Fabgen wrapped objects. For Squirrel, the default representation should be native classes/instances with typetags, because method/member lookup stays on Squirrel's class/instance path and C++ payload access can use instance user pointers. Userdata plus delegates should remain only a fallback for exceptional cases where a measured benchmark shows it is faster or simpler without hurting hot paths.
- Lua uses the registry for object references; Squirrel uses `HSQOBJECT` plus `sq_addref`/`sq_release`.
- Lua extension loading is standardized around `luaopen_*`; Squirrel has no equally standard binary module discovery in the official interpreter.
- Squirrel native functions receive a `this` object at stack index 1. The generator must consistently account for that in methods, functions, constructors, and reverse calls.

### Harfang Binding Script Changes

`binding/bind_harfang.py` can stay mostly shared, but it needs Squirrel-specific branches in the same places that currently branch for Lua:

- `bind_std_vector`: add a Squirrel array-to-vector converter.
- `expand_std_vector_proto`: add Squirrel naming.
- `bind_LuaObject`: either split into `bind_LuaObject` and `bind_SquirrelObject`, or generalize as `bind_script_object`.
- `bind_lua_scene_vm`: add `bind_squirrel_scene_vm`.
- `insert_non_embedded_setup_free_code`: add Squirrel startup logging.
- Final `bind(gen)`: call the Squirrel object and VM binders when `gen.get_language() == "Squirrel"`.

The callback reverse bindings currently live in the scene/forward-pipeline binding area and are generated for every language. Squirrel support must generate functions equivalent to:

- `hg_squirrel_OnCollision`
- `hg_squirrel_OnUpdate_NodeCtx`
- `hg_squirrel_OnAttachToNode`
- `hg_squirrel_OnDetachFromNode`
- `hg_squirrel_OnUpdate_SceneCtx`
- `hg_squirrel_OnAttachToScene`
- `hg_squirrel_OnDetachFromScene`
- `hg_squirrel_OnDestroy`
- `hg_squirrel_OnSubmitSceneToForwardPipeline`

## Public Squirrel API Shape

There are two viable packaging models.

### Model 1: Harfang Squirrel Launcher

Ship a Harfang-specific executable, for example `hg_sq`, based on the official Squirrel interpreter sources and linked with Harfang. At startup it opens the official Squirrel VM, registers standard libraries, binds Harfang into a root slot such as `hg`, and then runs user scripts.

Example user code:

```squirrel
local window = hg.NewWindow(1280, 720);
```

This is the most realistic way to reach Lua-like usability.

### Model 2: Native Module Initializer

Generate a Squirrel binding library exposing a C/C++ initializer such as:

```cpp
SQRESULT hg_squirrel_bind_harfang(HSQUIRRELVM v, const SQChar *symbol);
```

Then provide a small Squirrel bootstrap or host integration that loads and calls that initializer. This is useful for embedders but less user-friendly unless Harfang also standardizes a dynamic module loader.

The two models are compatible. The embedded scene VM only needs the initializer.

## Risks And Open Questions

The primary risks are:

- Fabgen backend scope. Full Harfang parity requires broad feature support: overloads, out/in-out arguments, ownership policies, non-copyable types, inline storage, vectors, callbacks, and generated type-info exchange.
- Wrapped object representation. Use native classes/instances with typetags as the default for performance. The prototype should still microbenchmark this against userdata plus delegates, but only to catch unexpected regressions or identify narrow exceptions.
- Module loading expectations. Lua has a clear `require` path. Squirrel needs a Harfang-defined launcher or loader story.
- Script environment semantics. The root table/closure-root model can support per-script environments, but the exact behavior must be validated against top-level declarations and callbacks.
- Mixed-language scenes. Keeping `Script` language-neutral is simple, but not enough for mixed Lua/Squirrel scenes without additional filtering.
- Asset bytecode. Runtime `.nut` source loading is straightforward; compiled `.cnut` integration should be deferred until build-configuration compatibility is tested.
- Security posture. Squirrel is an embedded scripting language, not a sandbox. Registering the official IO/system libraries exposes filesystem and process capabilities. Harfang should offer a "safe embedded VM" preset if user-authored or downloaded scripts are in scope.
- Documentation and examples. Lua tutorials, `man.Scripting`, `man.Assets`, and generated docs need Squirrel equivalents.

## Effort Estimate

Rough engineering estimate for a solid implementation:

| Work item | Estimate |
| --- | ---: |
| Vendor/build Squirrel in `extern`, CMake options, install rules | 2-4 days |
| Minimal Squirrel VM wrapper and proof-of-concept scene callback | 3-5 days |
| Fabgen Squirrel MVP: primitives, strings, enums, functions, methods, classes | 2-4 weeks |
| Fabgen full parity: vectors, ownership, arg out/in-out, operators, callbacks, type info | 4-8 weeks |
| `SceneSquirrelVM`, scene system overloads, object transfer, tests | 2-4 weeks |
| Public `hg_squirrel` package/launcher, assetc integration, docs, examples | 2-4 weeks |

Total: a credible production implementation is likely 10-18 engineer-weeks. A narrow proof of concept can be done much faster, but it would not demonstrate full Harfang API parity.

## Recommended Implementation Plan

1. Add Squirrel as a third-party dependency and build the official interpreter/runtime with Harfang CMake.
2. Implement `SquirrelObject`, `SquirrelStackGuard`, and basic VM functions: create, destroy, compile, execute, call, get/set table slots, error reporting.
3. Prototype script environments:
   - Create a per-script root table.
   - Insert `G`, `hg`, interpreter metadata, and selected standard symbols.
   - Compile and call a script defining `OnUpdate`.
   - Validate `GetScriptValue`, `SetScriptValue`, and `interface`.
4. Build a minimal Fabgen Squirrel backend supporting enough API to pass one scene callback test with `Node`, `Scene`, and `time_ns`.
5. Port the existing Lua scene tests to Squirrel.
6. Expand Fabgen feature coverage using the existing Fabgen tests as a matrix.
7. Generate the full embedded Harfang binding as `bind_hg_squirrel`.
8. Add public language packaging under `languages/hg_squirrel`.
9. Add documentation and examples.
10. Add optional assetc Squirrel source/bytecode handling.

## Conclusion

The project is feasible and aligns well with Harfang's current separation between scene script declarations and language-specific VMs. The engine work is manageable because `Script` is already neutral and `SceneLuaVM` gives a clear template.

The gating factor is Fabgen. Squirrel support should be treated as a first-class new language backend, not a small variant of Lua. The Lua backend provides the closest roadmap, but Squirrel's object model, reference handling, array/table split, and lack of Lua-style binary module loading require deliberate design.

The safest path is to first prove the embedded VM and scene callbacks with a small generated binding subset, then grow Fabgen to full API parity. Once that is done, exposing the full Harfang API to a Squirrel launcher based on the official interpreter is straightforward.

## Sources

- Harfang local code inspected: `harfang/script/lua_vm.*`, `harfang/engine/lua_object.*`, `harfang/engine/scene_lua_vm.*`, `harfang/engine/scene_systems.*`, `binding/bind_harfang.py`, `binding/CMakeLists.txt`, `languages/hg_lua/CMakeLists.txt`, `tools/assetc/assetc.cpp`.
- Fabgen local code inspected: `FABGen/lang/lua.py`, `FABGen/lib/lua/std.py`, `FABGen/lib/lua/stl.py`, `FABGen/gen.py`, `FABGen/bind.py`.
- Official Squirrel repository: https://github.com/albertodemichelis/squirrel
- Official Squirrel C API header: https://github.com/albertodemichelis/squirrel/blob/master/include/squirrel.h
- Official Squirrel standard auxiliary header: https://github.com/albertodemichelis/squirrel/blob/master/include/sqstdaux.h
- Official Squirrel CMake build: https://github.com/albertodemichelis/squirrel/blob/master/CMakeLists.txt
- Official Squirrel call API documentation: https://squirrel-lang.org/squirreldoc/reference/api/calls.html
- Official Squirrel reference manual PDF: https://squirrel-lang.org/squirreldoc/squirrel3.pdf
