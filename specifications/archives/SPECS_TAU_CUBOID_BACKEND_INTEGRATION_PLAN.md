# Tau Cuboid-First Backend Integration Plan For Harfang

Date: 2026-08-31

> Historical note (2026-09-05): this document records the original backend
> integration plan. Its legacy-import and compatibility-adapter strategy has
> since been superseded by the Harfang-native Tau runtime. See
> `SPECS_TAU_LEGACY_CLEANUP.md` for the audited cleanup and retained
> compatibility boundaries.

Method: static source review of the current Harfang repository, the local Tau
vendor snapshot at `S:\works\engine-neogs\vendor\tau`, and the related
supporting legacy sources under `S:\works\engine-neogs\source`.

Scope of this document: define a practical implementation plan for a
compile-time Tau alternative to Bullet, starting with cuboid rigid bodies only,
and validate reliability through the QA strategy described in
`SPECS_PHYSICS_QA_AUTOMATION_FEASIBILITY.md`.

## Executive Summary

The right way to test Tau inside Harfang is not a general Bullet replacement.

It is a cuboid-first, compile-time-selectable backend spike with a narrow
contract:

- dynamic and static cuboid rigid bodies first,
- Bullet kept as the reference backend,
- reliability measured through shared deterministic tests and Bullet baseline
  traces,
- no import of NeoGS scenegraph or higher-level runtime systems.

This plan is feasible.

The main integration difficulty is not cuboid collision itself. Tau already has
that.

The main difficulty is structural:

- Harfang exposes `SceneBullet3Physics` directly in engine helpers and
  bindings,
- Tau depends on legacy support types,
- and Tau still expects a small `nMItem` / `nItem` transform interface during
  simulation.

So the MVP should be treated as a private backend spike whose goal is to answer
three questions:

1. Can Tau be compiled in Harfang without dragging in a legacy scenegraph?
2. Can dynamic/static cuboid scenarios behave closely enough to Bullet for the
   supported subset?
3. Is the adapter cost small enough to justify phase 2 work?

## Feasibility Verdict

Feasible as a cuboid-first backend spike with medium-to-high integration cost.

Recommended only if the project accepts these constraints:

- one active scene physics backend per build,
- no Bullet parity promise,
- no mesh support in phase 1,
- no kinematic guarantee in phase 1,
- explicit skipping of unsupported QA scenarios.

Not recommended as:

- a public backend replacement on the first pass,
- or a direct reuse of current `SceneBullet3Physics` naming and full API shape.

## Planning Assumptions

This plan follows the current local findings:

- Harfang is still built as C++14.
- Scene systems are typed directly on `SceneBullet3Physics`.
- The current automated physics tests are still Bullet-only and primitive-only.
- `harfang3d/physics-qa` exists, but most of it is still interactive and
  Bullet-shaped.
- Tau already supports cuboid shapes, cuboid-cuboid collisions, rigid-body
  stepping, sleeping, forces, impulses, and closest-hit ray tracing.
- Tau does not natively provide full Bullet parity:
  - no `RaycastAllHits`,
  - no mesh-mesh,
  - no capsule/cylinder/cone/convex hull parity,
  - fragile kinematic and moving-static behavior,
  - and known safety issues that must be fixed before any reliability claim.

## Target Product Shape

The target is a build-time backend switch, not a runtime plugin system.

Recommended build setting:

- `HG_SCENE_PHYSICS_BACKEND=bullet`
- `HG_SCENE_PHYSICS_BACKEND=tau`

Recommended generated compile definitions:

- `HG_PHYSICS_BACKEND_BULLET`
- `HG_PHYSICS_BACKEND_TAU`

And a validation rule:

- exactly one scene physics backend must be active in a given build.

This is better than keeping multiple loosely related booleans because it makes
CI, bindings, tests, and tooling unambiguous.

## Phase 1 Feature Contract

Phase 1 should be intentionally narrow.

Supported:

- rigid body types:
  - dynamic,
  - static;
- collision shape:
  - `CT_Cube` only;
- simulation:
  - fixed-step update,
  - scene-to-physics sync,
  - physics-to-scene sync,
  - gravity,
  - force, impulse, torque,
  - wake / sleep,
  - collision event collection,
  - closest-hit raycast on cuboids,
  - optional cuboid overlap query if the adapter cost is small.

Explicitly unsupported in phase 1:

- `CT_Sphere`, `CT_Cone`, `CT_Capsule`, `CT_Cylinder`, `CT_Mesh`,
  `CT_MeshConvex`;
- `RBT_Kinematic`;
- `RaycastAllHits`;
- 6DOF constraints;
- Bullet linear and angular factor API;
- mesh pipeline integration;
- physics debug rendering parity;
- script-facing parity with the full current Bullet binding.

## Non-Goals

To keep the spike honest, the following should be excluded from phase 1:

- importing NeoGS `nScene`, `nMItem`, `scene3d`, or editor code;
- importing Tau serialization and legacy meta-file systems;
- importing NeoGS renderer or debug drawing subsystems;
- building a Tau asset compiler;
- adapting the whole existing Lua QA suite up front;
- emulating missing Tau features only to preserve a Bullet-shaped API.

## Main Technical Constraint: Tau Still Expects `nMItem` And `nItem`

This is the most important architectural fact.

Tau is not only coupled to legacy math and containers. During update it still
expects:

- `TauItem::GetMItem()->GetBaseItem()`
- `nItem::SetPosition(...)`
- `nItem::SetRotation(...)`
- `nItem::GetOrientationMatrix()`
- `nItem::GetMatrix()`
- `nItem::GetPreviousMatrix()`
- `nItem::GetInverseMatrix()`
- `nItem::SnapshotTransformation(...)`
- access to `offset_matrix`-style pivot data

That means the integration must choose between:

1. importing part of the old scenegraph,
2. rewriting Tau deeply,
3. or creating Harfang-owned minimal adapter stubs with the legacy method
   surface Tau actually calls.

Recommendation: option 3.

Do not import the old scenegraph.

Instead, create private Tau-facing adapter types inside Harfang that expose only
the transform and collision hooks Tau needs.

## Recommended Adapter Strategy

### 1. Keep Tau In A Private Third-Party Island

Recommended location:

- `harfang3d/extern/tau_legacy/`

This folder should contain:

- the frozen Tau source subset needed by the spike,
- a very small `tau_compat/` layer,
- and Harfang-local patches clearly separated from upstream files.

### 2. Use Harfang-Owned Minimal Legacy Stubs

Create tiny private adapter classes for the pieces Tau expects:

- `TauCompatItem` as a minimal `nItem` equivalent,
- `TauCompatManagedItem` as a minimal `nMItem` equivalent,
- minimal logging and utility macros expected by Tau,
- minimal benchmark and linked-list support if not rewritten locally.

These adapters should not implement a second scenegraph.

They should only:

- hold current and previous transforms,
- expose world / inverse / rotation matrices,
- expose an identity pivot first,
- and bridge to a Harfang `NodeRef`.

### 3. Do Not Port Tau To Harfang Math In Phase 1

For the cuboid-first spike, a full type-level port from `nVector` /
`nMatrix3` / `nMatrix4` to Harfang math is unnecessary risk.

The faster path is:

- keep Tau's expected math types private,
- vendor or recreate only the minimum low-level support they require,
- convert at the Harfang adapter boundary.

This duplicates some low-level math internally, but that duplication stays
sealed inside the Tau island and avoids importing the old scenegraph.

### 4. Exclude Legacy Systems That Are Not Needed

Do not compile in phase 1:

- `*_nml.cpp` serialization files,
- debug rendering files,
- mesh collision files,
- unused constraint families,
- scene-loading helpers,
- editor/runtime glue not needed for cuboid rigid bodies.

## Minimum Tau Patch Set Before Reliability Testing

The Tau feasibility study already identified issues that should be fixed before
the backend is judged on reliability.

Mandatory patch set:

- add hard bounds checks for collision node and contact buffers;
- make collision mask testing symmetric;
- disable or hide obviously dead / unfinished public features from the Harfang
  adapter;
- force deterministic fixed-step control from Harfang instead of relying on
  Tau's default `fq = 75`;
- keep pivot handling identity-only in phase 1 unless a dedicated test proves a
  broader path safe.

Strong recommendation:

- keep a patch log in the Tau vendor folder so it is always clear what Harfang
  changed versus the frozen source snapshot.

## Recommended Build Integration

### Root CMake

Add one backend selection variable in `harfang3d/CMakeLists.txt`:

- `HG_SCENE_PHYSICS_BACKEND`

Valid values:

- `bullet`
- `tau`

Behavior:

- `bullet` defines `HG_PHYSICS_BACKEND_BULLET` and preserves the current Bullet
  build path.
- `tau` defines `HG_PHYSICS_BACKEND_TAU`, excludes Bullet-only engine files and
  tools from the active build, and includes the Tau backend sources.

### Engine CMake

Update `harfang/engine/CMakeLists.txt` so the engine links one active backend:

- Bullet build:
  - current `scene_bullet3_physics.cpp/.h`
  - `BulletDynamics`, `BulletWorldImporter`
- Tau build:
  - new `scene_tau_physics.cpp/.h`
  - new Tau private library or object sources

### Tooling CMake

In `tools/assetc/CMakeLists.txt`:

- keep `bulletc` only for Bullet builds,
- do not introduce Tau asset tooling in phase 1,
- keep phase 1 collision resources primitive-only.

## Recommended Harfang Code Shape

### Phase 1 Internal Physics Facade

Do not try to make the whole public API backend-neutral at once.

Instead, introduce a narrow internal facade used by:

- scene system helpers,
- new cuboid QA scenarios,
- and backend-comparison tests.

Recommended new internal type:

- `ScenePhysics`

It should expose only the supported shared subset:

- scene creation from current scene components,
- node creation / destruction,
- stepping,
- collision events,
- wake / sleep,
- force / impulse / torque,
- linear and angular velocity,
- closest-hit raycast,
- optional overlap query,
- sync from scene,
- sync to scene,
- clear / garbage collect.

### Existing Bullet Type

Keep `SceneBullet3Physics` for Bullet builds in phase 1.

Do not alias Tau to `SceneBullet3Physics`.

That would hide feature differences and make failures harder to interpret.

### New Tau Type

Add:

- `harfang/engine/scene_tau_physics.h`
- `harfang/engine/scene_tau_physics.cpp`

This class should implement only the phase 1 subset.

### Scene System Helpers

Update `scene_systems.h/.cpp` so new code can target the narrow shared subset.

Recommended approach:

- add overloads for `ScenePhysics`,
- keep existing Bullet-specific overloads during migration if needed,
- move future QA automation and new tests to the shared subset first.

This avoids a repo-wide hard cut while still creating a real testable seam.

## Tau Source Slice For The Cuboid MVP

The cuboid-first build should include only the Tau pieces needed for:

- rigid-body stepping,
- cuboid collision,
- broad phase,
- closest-hit ray tests,
- inertia tensor computation,
- collision event extraction.

Likely include:

- `gcol_item.*`
- `gcol_shape.*`
- `gcol_prune.*`
- `gcol_system.*`
- `gcol_obbobb.cpp`
- `gcol_raytrace.cpp`
- `tau_physicstate.h`
- `tau_item.*`
- `tau_system.*`
- `tau_inertiatensor.cpp`

Likely exclude in phase 1:

- mesh collision files,
- NML serialization files,
- debug rendering files,
- most or all constraint files,
- sphere-specific files unless needed later.

The exact file list should be finalized only after the first compile pass,
because some transitive includes may force a few extra legacy support files.

## Step-By-Step Implementation Plan

### Phase 0: Backend Selection Skeleton

Goal:

- make Bullet and Tau mutually exclusive at configure time.

Work:

- add `HG_SCENE_PHYSICS_BACKEND` to root CMake;
- derive backend compile definitions;
- branch engine source selection on the backend;
- branch `assetc` Bullet tool build on the backend;
- keep Bullet build behavior unchanged.

Exit criteria:

- Bullet build still compiles unchanged;
- Tau build configures cleanly even before Tau implementation is finished.

### Phase 1: Private Tau Compile Spike

Goal:

- compile a sealed Tau subset inside Harfang without importing NeoGS
  scenegraph/runtime code.

Work:

- vendor Tau under `extern/tau_legacy/`;
- create `tau_compat/` with only the minimum low-level dependencies;
- add minimal Harfang-owned `nItem` / `nMItem`-like adapters;
- disable serialization, renderer, and mesh-related code paths;
- apply the mandatory safety patch set.

Exit criteria:

- Tau library or object set builds in Harfang;
- no imported NeoGS scenegraph or renderer code;
- adapter remains narrow and private.

### Phase 2: Cuboid Body Mapping

Goal:

- create a minimal `SceneTauPhysics` that can own Harfang cuboid rigid bodies.

Work:

- map `NodeRef` to Tau adapter objects;
- map `RBT_Dynamic` and `RBT_Static`;
- map `CT_Cube` to `GColShape::AsCuboid(...)`;
- derive Tau mass and inertia from Harfang collision size;
- keep pivot identity-only;
- convert Harfang transforms to Tau state and back.

Exit criteria:

- one dynamic cube falls under gravity;
- one static cube acts as ground;
- transform sync is stable over multiple steps.

### Phase 3: Shared Physics Subset

Goal:

- expose the minimum backend-neutral contract needed for QA and tests.

Work:

- add `ScenePhysics` internal subset or equivalent adapter seam;
- route scene update helpers through that subset;
- implement in Tau:
  - `SceneCreatePhysics...`
  - `NodeCreatePhysics...`
  - `NodeDestroyPhysics`
  - `StepSimulation`
  - `SyncTransformsFromScene`
  - `SyncTransformsToScene`
  - `CollectCollisionEvents`
  - `NodeWake`
  - `NodeAddForce`
  - `NodeAddImpulse`
  - `NodeAddTorque`
  - `NodeGet/SetLinearVelocity`
  - `NodeGet/SetAngularVelocity`
  - `RaycastFirstHit`
  - optional `NodeCollideWorld`

Important rule:

- set Tau frequency from Harfang's `physics_step`, do not keep the default
  75 Hz.

Exit criteria:

- shared subset compiles under both backends;
- Tau produces stable step behavior at the same fixed-step cadence as Bullet.

### Phase 4: Test And QA Harness MVP

Goal:

- compare Bullet and Tau on supported cuboid scenarios.

Work:

- extend current C++ engine tests to run through the shared subset where
  possible;
- create a small deterministic QA runner for cuboid scenarios;
- add new non-interactive scenarios in `harfang3d/physics-qa` rather than
  rewriting the whole legacy suite immediately;
- emit structured JSON traces;
- record Bullet baseline traces with a Bullet build;
- compare Tau traces against baseline using tolerances.

Recommended MVP scenario set:

- dynamic cuboid freefall;
- dynamic cuboid versus static ground collision;
- cuboid impulse response;
- cuboid stack settling;
- closest-hit raycast through multiple cuboids;
- optional cuboid overlap query.

Exit criteria:

- supported invariant tests pass under Tau;
- Bullet-versus-Tau baseline differences are explainable and within tolerance;
- unsupported features are reported as skipped, not silently passed.

### Phase 5: Kinematic Decision Gate

Goal:

- decide whether `RBT_Kinematic` is realistic without major extra debt.

Work:

- prototype a Tau kinematic strategy using explicit sync and forced wake-up;
- validate on narrow deterministic cuboid-only tests;
- measure whether moving-static limitations break the expected behavior.

Decision:

- if kinematic behavior is fragile, leave it unsupported;
- do not expand the backend contract only to mimic Bullet terminology.

### Phase 6: Public Exposure Decision

Goal:

- decide whether the backend stays private / experimental or becomes a real
  build target for wider use.

Promote only if:

- phase 4 passes cleanly on the supported subset;
- the adapter does not require importing larger legacy systems;
- performance is at least competitive on the target cuboid scenes;
- maintenance cost remains acceptable.

## QA Strategy For This Plan

The QA strategy should follow the Bullet-baseline study exactly in spirit:

- Bullet is the compatibility reference backend;
- Tau is compared only on features it actually supports;
- comparisons happen at the Harfang API level, not through Tau internal state;
- deterministic traces are more important than screenshots;
- image regression is optional and not needed for the cuboid MVP.

Recommended test layers for this plan:

### 1. Shared Invariant Tests

- dynamic cuboid falls;
- static cuboid does not move;
- impulses change motion;
- collision event appears;
- closest-hit raycast returns the expected node.

### 2. Bullet Baseline Trace Tests

Capture:

- frame index,
- cuboid transforms,
- linear and angular velocities,
- collision-event counts,
- ray hit node and hit point,
- settle frame or bounce apex where relevant.

### 3. Stress Reliability Tests

Add a small cuboid stress set before calling the backend "reliable":

- 32 to 128 stacked cuboids;
- repeated drop tests;
- repeated contact creation and destruction;
- broad-phase heavy grid scenes.

This is where Tau buffer bounds and ordering issues are most likely to surface.

## Binding And Scripting Strategy

Do not attempt full Lua / Python / Squirrel parity in phase 1.

Recommended order:

1. engine C++ tests first;
2. deterministic QA runner second;
3. language bindings for the shared subset only if the backend survives the
   spike.

If a script-facing path is needed early for `physics-qa`, create a very small
backend-neutral entrypoint for the shared subset instead of exposing the whole
Tau surface.

## Go / No-Go Criteria

Stop the effort if one of these becomes true:

- the Tau adapter requires importing a meaningful slice of NeoGS scenegraph;
- the private compatibility layer grows into a second mini-engine;
- cuboid-only invariant tests are unstable under fixed-step replay;
- mandatory Tau safety patches uncover deeper structural corruption risks;
- the Tau build remains slower than Bullet on the intended cuboid scenes with
  no clear product upside.

Continue to phase 2 only if all of these are true:

- Bullet build remains stable;
- Tau compiles in isolation;
- dynamic/static cuboid tests pass;
- collision events and closest-hit raycasts are reliable enough to automate;
- the adapter boundary remains understandable and contained.

## Effort Estimate

These estimates assume one engineer already comfortable with Harfang internals
and willing to patch Tau locally.

Backend-selection skeleton:

- 2 to 4 engineer-days

Private Tau compile spike plus safety patch set:

- 1.5 to 3 engineer-weeks

Cuboid body mapping and shared subset:

- 2 to 3 engineer-weeks

Deterministic QA runner and Bullet-versus-Tau comparison subset:

- 1.5 to 3 engineer-weeks

Practical cuboid MVP total:

- 5 to 9 engineer-weeks

That estimate intentionally excludes:

- mesh support,
- public binding parity,
- `RaycastAllHits`,
- and a wider backend-neutral public API cleanup.

## Recommended First Deliverable

The best first milestone is not "Tau integrated into Harfang."

It is:

- one Tau build configuration,
- one `SceneTauPhysics` cuboid-only backend,
- one shared test subset,
- one Bullet baseline capture,
- and one report showing:
  - what passed,
  - what was skipped,
  - what required patching,
  - and what the real adapter cost was.

That milestone is enough to decide whether Tau deserves phase 2 investment.

## Recommendation

Proceed with a cuboid-first backend spike only.

Do not start with mesh support, public scripting parity, or a large backend
abstraction effort.

The plan should optimize for one honest answer:

"Can Harfang host Tau as a sealed alternative backend for a narrow rigid-body
subset without importing a second engine architecture?"

If the answer is yes, phase 2 can expand carefully.

If the answer is no, the project will still have produced a useful QA baseline
and a precise cost assessment instead of an open-ended migration.

## Sources Checked

Harfang:

- `harfang3d/CMakeLists.txt`
- `harfang3d/harfang/engine/CMakeLists.txt`
- `harfang3d/tools/assetc/CMakeLists.txt`
- `harfang3d/harfang/engine/scene_bullet3_physics.h`
- `harfang3d/harfang/engine/scene_systems.h`
- `harfang3d/harfang/engine/physics.h`
- `harfang3d/harfang/tests/engine/scene.cpp`
- `harfang3d/binding/bind_harfang.py`
- `harfang3d/specifications/SPECS_TAU_PHYSICS_BACKEND_FEASIBILITY.md`
- `harfang3d/specifications/SPECS_PHYSICS_QA_AUTOMATION_FEASIBILITY.md`
- `harfang3d/physics-qa/README.md`

Tau / legacy support:

- `S:\works\engine-neogs\vendor\tau\README.md`
- `S:\works\engine-neogs\vendor\tau\source\physic\physic.h`
- `S:\works\engine-neogs\vendor\tau\source\physic\gcol_item.h`
- `S:\works\engine-neogs\vendor\tau\source\physic\gcol_shape.h`
- `S:\works\engine-neogs\vendor\tau\source\physic\tau_item.h`
- `S:\works\engine-neogs\vendor\tau\source\physic\tau_system.h`
- `S:\works\engine-neogs\vendor\tau\source\physic\tau_system.cpp`
- `S:\works\engine-neogs\source\framework\data_structure\linked_list.h`
- `S:\works\engine-neogs\source\framework\timing\benchmark.h`
- `S:\works\engine-neogs\source\engine\core\item.h`
- `S:\works\engine-neogs\source\engine\scene3d\mitem.h`
- `S:\works\engine-neogs\documentation\tau-physics-audit.md`
