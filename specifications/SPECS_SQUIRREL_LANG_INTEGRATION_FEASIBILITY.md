# Squirrel Language Integration Feasibility

Date: 2026-08-30

Re-evaluation of the 2026-04-15 draft after FABGen gained a working Squirrel backend.

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

This changes the project classification:

- Before: greenfield language-backend effort plus engine integration.
- Now: Harfang integration, API adaptation, packaging, and test-hardening effort built on an existing backend.

The remaining work is still substantial, but the critical path has moved. The primary risk is no longer "can FABGen support Squirrel classes at all?"; it is "how much Harfang-specific integration and API shaping is required to expose Squirrel cleanly and maintainably?"

The recommended first gate is now concrete:

1. make `harfang3d/binding/bind_harfang.py` generate under `--squirrel`,
2. compile the embedded binding,
3. prove one end-to-end `SceneSquirrelVM` callback path.

If that gate passes, the rest of the project is an integration program, not a feasibility gamble.

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

This does not prove full Harfang generation yet, but it removes the original "backend from scratch" assumption.

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

`harfang3d/binding/bind_harfang.py` remains explicitly Lua-oriented in several important places:

- `bind_std_vector()` only creates language-specific sequence adapters for CPython, Lua, and Go.
- `expand_std_vector_proto()` only expands overloads for CPython, Lua, and Go.
- `bind_LuaObject()` is Lua-specific and there is no Squirrel equivalent yet.
- `bind_lua_scene_vm()` only binds `hg::SceneLuaVM`.
- `bind_scene_systems()` only exposes the `SceneLuaVM` overload families.
- `insert_non_embedded_setup_free_code()` only handles Lua and CPython setup.

This is now the first concrete integration task. The backend exists, but the Harfang binding script has not yet been taught how to target it.

### Asset Compiler and Packaging Work Still Missing

The public toolchain is still Lua-only:

- `tools/assetc/assetc.cpp` only special-cases `.lua` and `luac`.
- `tools/assetc/CMakeLists.txt` only installs `luac` into the asset toolchain.
- `languages/hg_lua` exists; there is no `languages/hg_squirrel` yet.

For Squirrel, the phase-1 asset policy should remain:

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

The Harfang binding surface is much larger than the current FABGen Squirrel test suite, so the first full `bind_harfang.py --squirrel` generation is still a necessary checkpoint. It may reveal missing edge cases around unusual overloads, ownership combinations, or container shapes that are not covered by the current 30 tests.

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

- The class `from_c` path currently expects the generated module to be bound into the Squirrel root table.
  This is acceptable for Harfang, but `SceneSquirrelVM` and the public launcher must honor it explicitly.

- The public Squirrel backend does not currently export a Lua-style helper equivalent to `hg_lua_get_wrapped_object_type_tag(L, idx)`.
  If Harfang wants `SquirrelObject` to mirror `LuaObject::PushForeign()` across multiple VMs, FABGen will likely need one small additional public helper for wrapped-object type-tag extraction.
  If Harfang accepts same-VM semantics first, this can be deferred.

- The generated release helper `gen_release_<module>(v)` is mandatory for safe shutdown when C++ holds captured Squirrel callbacks.
  Harfang must build this into VM teardown from day one.

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

- First full generation risk.
  `bind_harfang.py --squirrel` has not been proven yet and may expose generator edge cases not hit by the current FABGen suite.

- `SceneSquirrelVM` environment semantics.
  The per-script root table model still needs a Harfang-side prototype for top-level declarations, `G`, injected `hg`, and callback lookup.

- Script-object transport policy.
  Harfang must decide whether `SquirrelObject` needs cross-VM object transfer semantics comparable to `LuaObject`, or whether same-VM semantics are enough for phase 1.

- API divergence due to single-return semantics.
  The packed-array policy for multiple outs is a real user-facing difference and will affect docs, examples, and migration guidance.

- Loader story.
  Lua has a mature `require` story in Harfang today. Squirrel still needs a Harfang-defined launcher or host integration story.

- Asset policy.
  Runtime `.nut` loading is low risk. Bytecode should remain out of scope for phase 1.

- Security posture.
  Registering the full Squirrel IO/system libraries exposes filesystem and process access. Harfang should define whether the embedded scene VM is "full" or "safe by default".

## Effort Estimate

Revised engineering estimate:

| Work item | Estimate |
| --- | ---: |
| Vendor/build Squirrel in `extern`, add CMake options, add VM teardown contract | 2-4 days |
| Make `bind_harfang.py --squirrel` generate and compile, fix exposed Harfang/FABGen gaps | 1-2 weeks |
| Implement `SquirrelObject`, `squirrel_vm`, `SceneSquirrelVM`, and scene-system overloads | 2-4 weeks |
| Add Squirrel branches in the Harfang binding script and adapt script-object APIs | 1-2 weeks |
| Add `assetc` `.nut` support, `languages/hg_squirrel`, launcher, docs, and examples | 1-3 weeks |

Revised total:

- Embedded scene-scripting proof of viability: about 3-5 engineer-weeks.
- Solid public integration with docs/tooling: about 6-10 engineer-weeks.

That is materially lower-risk than the original 10-18 engineer-week estimate because the language backend is no longer a greenfield item.

## Recommended Implementation Plan

1. Add Squirrel as a third-party dependency and make a minimal VM smoke test build in Harfang.
2. Extend `harfang3d/binding/bind_harfang.py` for Squirrel in the most obvious places first:
   - `bind_std_vector()`
   - `expand_std_vector_proto()`
   - non-embedded setup/free code
3. Generate the embedded Harfang binding with `--squirrel --embedded --prefix hg_squirrel`.
4. Fix the concrete generator or binding-script gaps exposed by that first generation/compile pass.
5. Implement `SquirrelObject` and `squirrel_vm.*`, including a mandatory `gen_release_harfang(v)` call before `sq_close(v)`.
6. Implement `SceneSquirrelVM` and add the `SceneSquirrelVM` overload family in `scene_systems.*`.
7. Port the core Lua scene tests to Squirrel and prove at least:
   - one scene script,
   - one node script,
   - one callback path,
   - one `GetScriptValue` / `SetScriptValue` path.
8. Add `.nut` passthrough support to `assetc`; defer Squirrel bytecode.
9. Add `languages/hg_squirrel` and a Harfang launcher/host integration.
10. Document Squirrel-specific API differences instead of trying to hide them everywhere.

## Conclusion

The April 15, 2026 conclusion should be revised.

Squirrel integration in Harfang is still a medium integration project, but it is no longer blocked on inventing a language backend. `FABGen/lang/squirrel.py` and its companion Squirrel support code substantially de-risk the original plan.

The dominant remaining work is now in Harfang itself:

- integrate the Squirrel runtime,
- add `SquirrelObject` and `SceneSquirrelVM`,
- adapt the binding script,
- decide how much Lua parity is worth preserving where Squirrel semantics differ,
- package and document the result.

The correct next milestone is no longer a generic backend prototype. It is a concrete Harfang checkpoint: generate `bind_hg_squirrel`, compile it, and run one end-to-end scene callback. If that succeeds, the project should be considered technically viable with manageable integration risk.

## Sources

- Harfang local code inspected:
  - `harfang3d/binding/bind_harfang.py`
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

- FABGen local progress notes inspected:
  - `FABGen/specifications/SQUIRREL_MVP_TESTS_AND_BINDING_2026-08-29.md`
  - `FABGen/specifications/SQUIRREL_CLASS_BINDING_PHASE2_2026-08-29.md`

- Local validation re-run during this re-evaluation:
  - `cd FABGen`
  - `python tests.py --sqbase "$env:TEMP\fabgen_squirrel_ref2"`
  - observed result: `30 run, 0 failed, 0 skipped`
