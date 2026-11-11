# Tau Legacy Cleanup And Native Runtime Consolidation

Date: 2026-09-05

Status: stages 1 and 2 complete

## Objective

Remove the unused legacy Tau import and its Harfang compatibility scaffold,
while preserving the current Tau backend's functionality, reliability,
deterministic behavior, public bindings, and measured performance.

The resulting backend must read as a Harfang-native physics implementation. It
must not expose or retain an apparent dependency on the historical nEngine
runtime when no such dependency exists in the executable.

## Audited State

The runtime Tau backend is implemented by Harfang under:

- `harfang/engine/scene_tau_physics.cpp`;
- `harfang/engine/scene_tau_physics.h`;
- `harfang/engine/scene_tau_physics_contact.h`;
- the reusable Harfang `DynamicAABBTree` and collision-geometry facilities.

The imported `extern/tau/source/physic` tree contains 42 files, approximately
7,590 lines and 195 KiB of source. CMake marks every one of those files
`HEADER_FILE_ONLY`; none is compiled. The five `*_nml.cpp` files account for
approximately 690 lines and have no caller in the active backend.

The imported sources contain 43 references to the historical `nLinkedList`
family across 18 files. Its supporting header is not present in the Harfang
repository. These containers therefore have no runtime, binary-size, or
performance impact: they belong exclusively to an incomplete source archive.

The `tau_legacy` static library compiles only:

- `compat/tau_harfang_runtime.cpp`;
- an empty `compat/tau_placeholder.cpp`.

The only active compatibility type is `tau_compat::NodeMotionAdapter`. It
stores current and previous matrices, decomposed pose components, an inverse
matrix cache, and cache state. The active backend reads only its current world
matrix. Its other representations and getters have no caller.

At Harfang's current float type layout, the adapter occupies approximately 220
bytes per physics body. It also composes and then decomposes every awake body
pose once per substep. Existing 1,500-cuboid profiles attribute approximately
0.17-0.18 ms per substep to `Tau.MotionUpdate`, which bounds the direct active
runtime opportunity near one percent. Removing the adapter also removes about
330 KiB from 1,500 bodies' cold storage.

## Required Compatibility Boundaries

The following are not part of the legacy Tau scaffold and must remain:

- `ScenePhysics`, the backend-neutral public API;
- `SceneTauPhysics`, the Harfang scene/backend boundary;
- the binding-only historical `SceneBullet3Physics` constructor behavior in a
  Tau build;
- the C++ compatibility alias currently provided by `scene_physics.h`;
- `GetScenePhysicsBackendName()`;
- Lua, Squirrel, and Python/Fabgen API compatibility;
- physics callbacks, collision events, scene synchronization, raycasts,
  constraints, debug rendering, sleeping, and activation behavior.

The scene/backend boundary is necessary. The obsolete layer is the emulation
of historical nEngine concepts between that boundary and the current Tau
state.

## Cleanup Stage 1: Remove The Dead Import

Delete:

- `extern/tau/source/physic` in full, including NML, debug, old constraints,
  old collision code, and linked-list users;
- `extern/tau/compat`;
- `tau_placeholder.cpp`;
- the `tau_legacy` CMake target and its engine link dependency.

Historical provenance remains available in Git history and in this document.
The removed snapshot identified its source as the legacy nEngine `gl3` branch
and attributed the original engine to Emmanuel Julien / XBarr. If independently
required by licensing review, that attribution should be copied into the
project's durable third-party notices rather than retaining unused source in
the build tree.

Expected runtime performance change: none. Expected maintenance benefit: high.
The removal makes accidental future compilation of incomplete legacy code
impossible and eliminates misleading IDE/build-project entries.

## Cleanup Stage 2: Remove The Transform Compatibility Adapter

Use the already authoritative Tau body state:

- `position`;
- `orientation`;
- cached `world_rotation`;
- `scale`.

Create the Harfang world matrix with `ComposeTauWorld()` only when the scene
requires publication. Preserve the existing previous-position and
previous-orientation updates used by broad-phase displacement, sleeping, and
external-movement detection.

The cleanup must not change:

- integration order;
- contact or manifold order;
- position or velocity constraint order;
- floating-point equations;
- solver iteration counts;
- sleeping thresholds or wake propagation;
- fixed-step and pre-tick callback semantics.

The obsolete `motion_updates` diagnostic is removed with its compatibility
object. Existing awake/sleeping body diagnostics already provide the relevant
workload counts without retaining a misleading name or a redundant body scan.

Expected result:

- no legacy Tau include from `SceneTauPhysics`;
- no transform decomposition after composing solved body state;
- no per-body compatibility object;
- unchanged scene transforms and physics trajectories;
- a small active-step improvement or, at minimum, no measurable regression.

## Container Decisions

| Container or structure | Decision | Reason |
|---|---|---|
| Legacy `nLinkedList`, `nLinkedListId`, `ntPairList` | Delete with the dead import | Uncompiled, incomplete, and pointer-chasing by design |
| `tau_internal::TauNodeStore` | Keep | Recent dense vector storage with hashed lookup, deterministic traversal, focused invariant tests, and measured locality benefit |
| `DynamicAABBTree` | Keep | Harfang-owned, generic, vector/index based, generation checked, tested, and already performance-positive |
| `TauAlignedAllocator` | Keep while Harfang is C++14 | Required to guarantee the 64-byte and 16-byte solver-stream alignments without relying on C++17 aligned allocation |
| Cold resource/event `std::map` instances | Keep | Low-frequency paths where clarity and deterministic ordering dominate |
| Persistent manifold hash index | Keep until profiling rejects it | It removed linear lookup and is protected by deterministic contact ordering |
| Broad-phase `unordered_map<proxy, unordered_set<proxy>>` pair cache | Measure separately | It duplicates edges and uses node allocations; a flat canonical edge set may improve locality, but replacement carries ordering and invalidation risk |

No active container is replaced as part of the legacy cleanup. Container work
must have its own same-binary or preserved-binary A/B oracle and must not be
mixed with source deletion or transform-state cleanup.

## Readability Follow-Up

After this cleanup, the remaining structural debt is the approximately
4,700-line `scene_tau_physics.cpp` implementation, not the historical Tau
archive. It should later be divided along explicit internal boundaries:

1. body storage and scene synchronization;
2. broad phase and world-shape preparation;
3. primitive and mesh narrow phase;
4. persistent manifolds and contact construction;
5. position, velocity, and rolling-friction solving;
6. activation, islands, and sleeping;
7. diagnostics and debug rendering.

Moving hot functions across translation units can change inlining and code
generation. Initial readability-only extraction may therefore use private
internal headers or unity compilation, followed by measured translation-unit
splits. Public bindings must continue to see only the stable scene-physics API.

## Validation Gates

### Build and API

- Configure and build the Tau backend.
- Configure and build the Bullet backend.
- Run the complete C++ test executable.
- Regenerate or compile Fabgen bindings where affected.
- Preserve `ScenePhysics`, `SceneTauPhysics`, and legacy bound
  `SceneBullet3Physics` construction.

### Behavior

- Preserve the accepted `rb_rings_chain.lua` Tau trajectory hash or explain
  any difference before acceptance.
- Preserve the accepted `rb_dynamic_impulse_callback.lua` trajectory hash.
- Preserve variable restitution, friction, rolling-friction, and chair hashes.
- Pass sphere/cuboid/capsule contacts.
- Pass primitive and mesh raycasts, including rotating mesh terrain.
- Pass sleep, wake, support movement, transform persistence, and collision
  callback tests.

### Performance

- Run a focused 1,500 active-cuboid profile and confirm that
  `Tau.MotionUpdate` is gone while comparing full `Tau.StepSimulation` time.
- Run active and settled mixed/cube/sphere controls.
- Reject an unexplained regression greater than three percent in median or
  p95 for this cleanup, using guarded interleaved measurements when possible.
- Do not change solver iteration counts to satisfy the performance gate.

## Commit Structure

Keep the work reviewable and bisectable:

1. cleanup specification;
2. dead-import/build-graph and transform-adapter removal as one atomic change,
   because the adapter is the final consumer of the imported target;
3. validation result update.

No container replacement or runtime modularization belongs in these commits.

## Stage 1 And 2 Implementation Result

Completed on 2026-09-05:

- deleted the entire unused `extern/tau` snapshot, including all NML and
  linked-list-based sources;
- removed the `tau_legacy` target and the engine's private link dependency;
- removed `NodeMotionAdapter` from each Tau body;
- made Tau's canonical position, orientation, rotation cache, and scale the
  sole runtime transform representation;
- compose a Harfang world matrix only when an awake or dirty dynamic body is
  published to the scene;
- retained previous-pose history, transform-dirty publication, and wake
  behavior while removing the obsolete `motion_updates` counter and scan;
- changed the CMake feature report from `Tau Physics (scaffold)` to
  `Tau Physics`.

No solver, contact, manifold, iteration, activation, or ordering policy was
changed.

## Validation Result

### Build And Tests

- Tau Release configuration: engine, generated Lua binding, install target,
  and complete C++ test executable pass.
- Bullet Release configuration: engine, install target, and complete C++ test
  executable pass.
- Generated Squirrel and Python binding modules compile with Tau after the
  cleanup, and the Squirrel folder-launcher regression passes. The public
  neutral API and the binding-only historical `SceneBullet3Physics`
  compatibility constructor are unchanged.

### Deterministic Behavior

The following Tau QA dumps are byte-identical to their pre-cleanup baselines:

| QA | SHA-256 |
|---|---|
| `rb_rings_chain.lua` | `619CC48B2BE46D91E7B4961DDAE9705DEE0CA151F25104CE9DEF7BA3735A557E` |
| `rb_dynamic_impulse_callback.lua` | `928E478D3CABE1DE0135CFE16D0AAABF8D67288BE76BB22671A7DB51223296E9` |
| variable restitution | `7ACCE6ADE2B2C7D42B845E996A541CC0149794EB2C41E6902166066AC327C69F` |
| variable friction | `AB3F3F8A17619DD176D3113014985B65F6FCF2C083E6B8BD55A411938F14C310` |
| variable rolling friction | `3F64B896281FD4DB2A2F6B854C8DB641276D6CCD882026FBE3DA81EE4D1B5561` |
| multi-collision-box chair | `BD95207B09C317063398231C57BA63AE9D03B7D93FCA7E31F9842388B53A1F86` |

Primitive raycasts and mesh raycasts also pass, including the rotating mesh
terrain case. The mesh QA observations remain 361 hits / 80 misses for the
subdivided collider, 1,399 hits / 2,352 misses for the static terrain, and 403
hits / 3,348 misses for the terrain rotated by 90 degrees.

### Performance

A clean interleaved A/B used the exact pre-cleanup `HEAD` runtime and the
post-cleanup runtime, five identical seeds, 1,500 active cuboids, 120 measured
steps per seed, Release builds, and no Tau diagnostics. `ollama ps` reported no
loaded model before measurement.

| Runtime | Median of per-seed medians | Median of per-seed p95 values |
|---|---:|---:|
| Pre-cleanup adapter | 16.212 ms | 19.923 ms |
| Harfang-native state | 15.911 ms | 19.579 ms |
| Change | **-1.86%** | **-1.73%** |

The profiled control attributes 0.186 ms per substep to `Tau.MotionUpdate` in
the pre-cleanup runtime. That profiler section is absent after cleanup, as
intended. The full profiled `Tau.StepSimulation` control moved from 16.2 ms to
16.1 ms; the unprofiled interleaved result above is the acceptance metric.

A final five-seed control after removing the now-unused diagnostic counter and
its O(n) scan measured 15.946 ms median and 20.328 ms p95. Compared with the
same pre-cleanup records this is -1.64% median and +2.03% p95, still within the
three-percent guard despite the non-interleaved p95 noise.

Benchmark and QA artifacts are stored outside the source tree under
`build/tau-legacy-cleanup-20260905`.

## Follow-Up Boundary

This cleanup is complete. Further changes to the broad-phase pair cache,
active containers, translation-unit layout, solver, or parallel scheduling
must be separate measured work. They are not required to remove the legacy Tau
layer and must not be folded into this change set.
