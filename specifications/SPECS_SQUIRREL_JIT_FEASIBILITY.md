# Squirrel JIT Feasibility For Harfang

Date: 2026-08-30

## Executive Summary

Moving Squirrel to a JIT-based runtime inside Harfang is not a small extension of the current work. It is a separate runtime strategy with high technical risk.

Based on the current Harfang branch and the current upstream/runtime landscape:

- A direct JIT upgrade of official Squirrel is not a practical near-term goal.
- The official Squirrel runtime used by Harfang is an interpreter plus bytecode compiler/serializer, not a JIT-capable VM.
- The closest performance-oriented relative in the ecosystem is Quirrel, but Quirrel is explicitly not compatible with original Squirrel and should be treated as a language/runtime migration, not as "Squirrel with JIT."
- For Harfang specifically, the likely first performance bottlenecks are script/native boundary cost, callback dispatch frequency, and startup compilation, not necessarily raw VM dispatch alone.

Recommendation:

- `No-go` for an in-house official-Squirrel JIT on the current roadmap.
- `Go` for profiling and low-risk performance work on the existing official Squirrel integration.
- `Conditional go` for a short Quirrel spike only if profiling later proves that pure script execution, not C++ boundary traffic, is a dominant cost.

## Scope

This note evaluates three different meanings of "move Squirrel to JIT" inside Harfang:

1. Add a real JIT backend to official Squirrel while keeping Harfang's current Squirrel language target.
2. Replace official Squirrel with a faster Squirrel-derived runtime.
3. Reach most of the practical performance benefit without a JIT by optimizing the current Harfang architecture.

This is a feasibility study for Harfang itself, not a general survey of scripting languages.

## Current Baseline In Harfang

As of Sunday, August 30, 2026, Harfang already has a concrete official-Squirrel integration path in progress:

- `harfang3d/extern/squirrel` vendors upstream Squirrel, recorded in `harfang3d/extern/versions.txt` at commit `f9267f2` dated February 28, 2026.
- `harfang3d/languages/hg_squirrel/CMakeLists.txt` builds:
  - the generated `hg_squirrel` module,
  - a public `hg_squirrel` interpreter,
  - launcher binaries.
- `harfang3d/languages/hg_squirrel/launcher.cpp` and `launcher_app.cpp` load scripts and native modules through the official Squirrel C API.
- `harfang3d/tools/assetc/assetc.cpp` currently treats `.nut` as source assets rather than as a separate native-code target.

The current public Squirrel runtime model in Harfang is therefore:

1. resolve a script or native module,
2. compile Squirrel source to bytecode/closure form with the stock compiler,
3. execute it in the stock VM,
4. cross the binding boundary into Harfang C++ through FABGen-generated glue.

That is important because a JIT would only replace or augment step 2/3. It would not remove binding overhead, object marshaling, or scene-system design constraints.

## What The Official Squirrel Runtime Provides Today

The official Squirrel runtime and documentation exposed in the current vendored tree provide:

- source compilation through `sq_compile()` and `sq_compilebuffer()`,
- bytecode serialization through `sq_writeclosure()` and `sq_readclosure()`,
- a stock `sq` interpreter with `-c` bytecode compilation mode,
- a build mode without the compiler, intended to run precompiled bytecode.

What is not present in the current upstream-facing surface:

- a JIT backend,
- an IR layer intended for machine-code generation,
- tiered execution infrastructure,
- deoptimization support,
- register allocator or native code cache,
- any Harfang-usable official "turn on JIT" path.

This means Harfang is not one build flag away from JIT. A JIT would require either:

- a major new subsystem inside the vendored Squirrel runtime, or
- a move to another runtime with materially different language and API behavior.

## Where A JIT Would Help In Harfang

A JIT can improve:

- hot pure-script arithmetic,
- table/array iteration done entirely inside script,
- branch-heavy script logic that stays mostly within the VM,
- repeated callback bodies with little C++ interaction.

A JIT does not directly solve:

- cost of script-to-C++ calls,
- cost of C++-to-script callback dispatch,
- wrapped object lookup and ownership handling,
- asset I/O,
- renderer, physics, audio, or platform work done in native code,
- startup cost caused by module discovery, DLL loading, and other non-VM work.

For Harfang, this distinction matters. The scripting model is binding-heavy by design:

- public scripts call Harfang APIs frequently,
- scene callbacks are expected to cross the engine boundary often,
- object wrappers and native resource handles remain C++ owned.

Inference:

If representative Harfang gameplay scripts spend most of their time calling engine APIs, a JIT may produce disappointing real-world gains even if it accelerates standalone Squirrel code.

## Option A: Add A JIT To Official Squirrel

### Technical Feasibility

Possible in theory, poor in practice.

Reasons:

- Official Squirrel currently exposes a direct compilation-to-bytecode model, not a multi-stage compiler intended for alternate code generation.
- Harfang's current Squirrel work already relies on the stock runtime behavior, module loader behavior, and object model.
- A compatible JIT would need to preserve:
  - closures,
  - classes and typetags,
  - coroutines/generators,
  - exceptions,
  - debug hooks,
  - reference-counted object lifetime rules,
  - the C API expected by FABGen-generated bindings.

That is not just "make bytecode faster." It is compiler and VM work.

### Harfang-Specific Risk

The cost is amplified by Harfang's current state:

- `bind_harfang.py` still contains Lua-centric scene/runtime bindings.
- Harfang does not yet have a `SceneSquirrelVM` equivalent to `SceneLuaVM`.
- A JIT project would start before the normal official-Squirrel scene-runtime baseline is complete.

This creates the wrong sequencing:

- Harfang would be stabilizing Squirrel integration and reinventing the runtime at the same time.

### Compatibility Risk

A useful JIT would need to be bug-compatible enough with official Squirrel for:

- existing `.nut` tutorials,
- FABGen-generated wrappers,
- native module loading,
- any future embedded scene scripting work.

That compatibility burden is likely larger than the value of doing the work inside Harfang.

### Estimate

No credible short estimate exists for a production-quality result.

Reasonable planning assumption:

- prototype research effort: one or two senior engineers for one to two quarters,
- production hardening and maintenance: materially longer.

That estimate is still optimistic because it assumes the first approach chosen is viable.

### Verdict

`No-go`.

This path is technically possible, but not strategically sound for Harfang.

## Option B: Replace Official Squirrel With Quirrel

### Why Quirrel Matters

Quirrel is the most relevant external reference point because it is:

- based on Squirrel,
- performance-oriented,
- actively positioned as safer and faster,
- documented as having an AST-based compiler and VM-level performance optimizations.

However, Quirrel's own documentation also makes two critical points:

- it is not compatible with original Squirrel,
- it does not present itself as "official Squirrel with a drop-in JIT."

### Why This Is Not A Drop-In JIT Upgrade

For Harfang, a Quirrel move would change more than execution speed:

- language semantics,
- scoping behavior,
- module behavior,
- root/global behavior,
- parts of the C/C++ binding API,
- script compatibility expectations.

This matters because Harfang's current Squirrel work already targets official Squirrel:

- vendored upstream Squirrel in `extern/squirrel`,
- launcher behavior built around official API calls,
- FABGen `lang/squirrel.py` backend targeting official Squirrel conventions.

Quirrel should therefore be evaluated as:

- a separate runtime target,

not as:

- a transparent optimization layer under the current Squirrel target.

### Feasibility

Moderate as a separate project. Poor as an in-place replacement.

What would need to happen:

1. prove that the current public `hg_squirrel` launcher can be mirrored for Quirrel,
2. validate whether the existing FABGen Squirrel backend can be adapted cheaply or whether a distinct `lang/quirrel.py` backend is needed,
3. measure how many current `.nut` scripts need edits,
4. decide whether Harfang wants two script languages or a migration path.

### Likely Outcome

If Harfang wants performance plus better tooling, Quirrel may be worth investigating later.

If Harfang wants source stability for the official-Squirrel work already underway, Quirrel cuts against that goal.

### Estimate

For a bounded spike:

- launcher and binding proof of concept: 2-4 weeks.

For a credible product track:

- runtime, generator adaptation, docs, sample migration, and regression work: likely 2-3 months or more.

### Verdict

`Conditional go`, but only as a distinct migration study after official-Squirrel baseline profiling.

## Option C: Stay On Official Squirrel And Optimize The Current Path

This is the highest-leverage path for Harfang.

### Low-Risk Gains

1. Profile before redesign.
   Measure:
   - script compile time,
   - time spent in Squirrel callback bodies,
   - time spent crossing script/native boundaries,
   - call counts per frame for hot APIs.

2. Add bytecode caching where build configuration matches.
   This improves startup and reload time, not steady-state VM speed, but it is aligned with the stock runtime.

3. Reduce boundary crossings.
   Examples:
   - batch getters/setters,
   - avoid very chatty per-entity script APIs,
   - move tight update loops or math-heavy helpers into C++.

4. Finish the normal official-Squirrel baseline first.
   In practice:
   - stabilize launcher/runtime behavior,
   - decide whether `SceneSquirrelVM` is needed,
   - only then benchmark representative scene scripting workloads.

5. Keep `.nut` as source for phase 1 and avoid coupling performance goals to a custom compiler effort.

### Why This Is Better Than Starting With JIT

- It fits the runtime Harfang already vendors.
- It keeps compatibility risk low.
- It gives performance data before a major architectural commitment.
- It can improve startup and frame time independently of any future JIT decision.

### Estimate

- instrumentation and benchmark corpus: 1-2 weeks,
- bytecode-cache prototype for matching build configs: 1-2 weeks,
- boundary and callback optimization pass on hot scripts/APIs: 2-6 weeks, depending on findings.

### Verdict

`Go`.

## Decision Matrix

| Option | Compatibility With Current Harfang Squirrel Work | Potential Runtime Gain | Cost | Risk | Recommendation |
| --- | --- | --- | --- | --- | --- |
| Add JIT to official Squirrel | Medium in theory, low in practice | Uncertain to high on pure script, low on boundary-heavy workloads | Very high | Very high | No-go |
| Replace official Squirrel with Quirrel | Low to medium | Medium | High | High | Conditional go as separate migration study |
| Keep official Squirrel and optimize | High | Medium in practice, with clearer ROI | Low to medium | Low to medium | Go |

## Recommended Plan

1. Do not start a Harfang official-Squirrel JIT project.
2. Complete and stabilize the current official-Squirrel integration path first.
3. Add measurement points around:
   - script compile/load time,
   - callback dispatch,
   - script/native crossings,
   - representative frame time spent in Squirrel code.
4. Add low-risk performance work:
   - bytecode caching for matching runtime/build configurations,
   - API batching,
   - hot-path migration to C++ where needed.
5. Re-evaluate only if profiling shows that Squirrel VM execution itself remains a major frame-time consumer after those steps.
6. If that happens, prefer a short Quirrel spike over a custom official-Squirrel JIT project.

## Go/No-Go Criteria For Any Future JIT Revisit

Only revisit a JIT-class project if all of the following are true:

- representative Harfang workloads spend a large share of frame time inside pure Squirrel execution,
- script/native boundary cost is not the main bottleneck,
- low-risk optimizations have already been applied,
- the team accepts a long-lived runtime maintenance burden,
- the team accepts either:
  - compatibility risk for official Squirrel, or
  - explicit migration cost to another runtime such as Quirrel.

If those conditions are not met, the JIT path should remain closed.

## Conclusion

For Harfang on August 30, 2026, "move Squirrel to JIT" is not the right next step.

The official Squirrel runtime currently integrated in-tree is an interpreter plus bytecode system, not a JIT-ready platform. Building a compatible JIT on top of it would be a major VM/compiler effort with unclear payoff for Harfang's binding-heavy scripting model.

The closest ecosystem alternative, Quirrel, is better understood as a separate language/runtime migration with performance advantages, not as a transparent JIT upgrade.

The pragmatic recommendation is:

- keep the current official-Squirrel track,
- finish profiling and integration,
- optimize startup and boundary costs first,
- treat any JIT-class move as a later research decision gated by measurements.

## Sources

### Local Harfang code inspected

- `harfang3d/extern/versions.txt`
- `harfang3d/languages/hg_squirrel/CMakeLists.txt`
- `harfang3d/languages/hg_squirrel/launcher.cpp`
- `harfang3d/languages/hg_squirrel/launcher_app.cpp`
- `harfang3d/tools/assetc/assetc.cpp`
- `harfang3d/binding/bind_harfang.py`
- `harfang3d/specifications/SPECS_SQUIRREL_LANG_INTEGRATION_FEASIBILITY.md`

### Local vendored Squirrel docs inspected

- `harfang3d/extern/squirrel/include/squirrel.h`
- `harfang3d/extern/squirrel/sq/sq.c`
- `harfang3d/extern/squirrel/doc/source/reference/api/compiler.rst`
- `harfang3d/extern/squirrel/doc/source/reference/api/bytecode_serialization.rst`
- `harfang3d/extern/squirrel/doc/source/reference/embedding/build_configuration.rst`
- `harfang3d/extern/squirrel/HISTORY`

### External primary sources

- Official Squirrel repository: https://github.com/albertodemichelis/squirrel
- Official Squirrel README: https://raw.githubusercontent.com/albertodemichelis/squirrel/master/README
- Official Squirrel interpreter source: https://raw.githubusercontent.com/albertodemichelis/squirrel/master/sq/sq.c
- Quirrel introduction: https://quirrel.io/doc/introduction.html
- Quirrel differences from Squirrel: https://quirrel.io/doc/diff_from_squirrel.html
- Quirrel repository: https://github.com/GaijinEntertainment/quirrel
