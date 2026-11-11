# Tau Physics Backend Feasibility For Harfang

Date: 2026-08-30

Method: static source review only. I did not build Tau inside Harfang, and I
did not run comparative CPU benchmarks against Bullet.

## Executive Summary

Supporting Tau as an optional physics alternative inside Harfang is feasible
only if Harfang accepts a deliberately reduced contract and if Tau is presented
as a separate backend, not as a transparent replacement for
`SceneBullet3Physics`.

Tau is attractive because it is small, legacy-simple, fixed-step, and clearly
aimed at lightweight rigid-body scenes. The vendored snapshot is only 42 source
files and about 195 KB. It already contains a rigid-body solver, sleeping,
sphere/box/static-mesh collision, collision contacts, and closest-hit raycasts.

The main blockers are not just missing features. They are:

- the age of the codebase and incomplete implementation areas,
- the dependency drift between the vendored Tau folder and its original host
  runtime,
- Harfang's current Bullet-specific mesh collision asset pipeline,
- several correctness and safety issues already identified in Tau,
- unclear licensing/provenance for redistribution.

Bottom line:

- Feasible as an experimental or niche `SceneTauPhysics` / `SceneTauPhysicsLite`
  backend with a reduced feature matrix.
- Not feasible as a Bullet-compatible backend without significant adaptation,
  feature cuts, and safety hardening.

## Feasibility Verdict

Feasible with medium-to-high integration risk for a reduced-scope backend.

Not feasible as a drop-in replacement for `SceneBullet3Physics`.

## Why Tau Is Interesting

Tau has a few properties that make it worth studying:

- Small implementation footprint: `vendor/tau/source/physic` contains 42 files
  totaling about 195 KB.
- Clear fixed-step runtime model: `Tau` defaults to 75 Hz and keeps its own
  update timer (`tau_system.cpp`, `tau_system.h`).
- Built-in sleeping/deactivation support.
- Compound bodies are possible through multiple `GColShape` entries per item.
- Collision is optimized for a narrow but practical subset:
  sphere-sphere, sphere-box, box-box, sphere-mesh, box-mesh.
- Ray tracing exists already.
- The code is simple enough that a focused backend or adapter is realistic to
  prototype.

CPU benefit is plausible but still unproven. That is an inference from the
smaller feature surface and simpler solver, not a measured result.

## Current Harfang Physics Contract

From the current Harfang sources, the backend surface is broader than Tau:

- Body types: `RBT_Dynamic`, `RBT_Kinematic`, `RBT_Static`
  (`harfang/engine/node.h`).
- Collision types in C++: sphere, cube, cone, capsule, cylinder, mesh, and
  mesh-convex (`harfang/engine/node.h`, `harfang/engine/scene.cpp`).
- The current Bullet implementation instantiates shapes for sphere, cube, cone,
  capsule, cylinder, and mesh (`harfang/engine/scene_bullet3_physics.cpp`).
- Runtime API includes:
  - stepping and scene sync,
  - collision event collection with contacts,
  - `NodeCollideWorld`,
  - `RaycastFirstHit`,
  - `RaycastAllHits`,
  - force, impulse, torque, velocity getters/setters,
  - deactivation control,
  - teleport/reset,
  - linear and angular factor locks,
  - 6DOF constraints,
  - pre-tick callback
  (`harfang/engine/scene_bullet3_physics.h`,
  `binding/bind_harfang.py`).

There are also two useful nuances:

- Harfang's current Bullet backend is effectively single-threaded in the active
  path (`SceneBullet3Physics::SceneBullet3Physics`, `scene_bullet3_physics.cpp`).
  Tau being single-thread oriented is therefore not a unique regression against
  the current Harfang backend.
- The declared Harfang contract is already not perfectly uniform today:
  `CT_MeshConvex` exists in C++ scene helpers, but the current Bullet creation
  path does not implement it, and the current bindings only expose collision
  types through `CT_Mesh` (`node.h`, `scene.cpp`, `bind_harfang.py`).

That lowers the practical bar slightly for a reduced backend, but only if the
reduced scope is explicit.

## What Harfang Tests Actually Exercise Today

The current Harfang tests exercise a smaller subset than the public Bullet API:

- dynamic freefall,
- kinematic body not falling,
- collision callback collection,
- `NodeCollideWorld`,
- `RaycastFirstHit`,
- `RaycastAllHits`
  (`harfang/tests/engine/scene.cpp`).

Those tests use spheres and cubes only.

This matters because it suggests a first Tau prototype could target the tested
subset first, but it does not change the public API gap.

## What Tau Actually Provides

Tau and GCollide currently provide:

- fixed-frequency rigid-body stepping,
- sleeping and wake-up,
- forces, impulses, torque, point velocity,
- sphere, cuboid, and polygon-mesh collision shapes,
- multiple shapes per item,
- sweep-and-prune broad phase,
- sphere-sphere, sphere-cuboid, cuboid-cuboid collisions,
- sphere-mesh and cuboid-mesh collisions,
- closest-hit ray tracing,
- collision monitor/contact storage,
- distance, position, hinge, and velocity constraints
  (`tau_system.h`, `tau_item.h`, `gcol_shape.h`, `gcol_system.h`,
  `gcol_raytrace.cpp`, `tau_constraint.h`).

That is enough for a light rigid-body backend, but not enough for current
Harfang parity.

## Major Gaps Between Harfang And Tau

| Area | Harfang Expectation | Tau Reality | Impact |
| --- | --- | --- | --- |
| Collision shapes | sphere, box, cone, capsule, cylinder, mesh | sphere, cuboid, mesh only | direct shape gap |
| Mesh convex | C++ scene helper exists | no equivalent | would need omission or translation strategy |
| Kinematic bodies | explicit sync-from-scene path | only partial fit, with fragile moving-static behavior | risky for movers/platforms |
| Raycasts | first hit and all hits | closest hit only | `RaycastAllHits` missing |
| Constraints | 6DOF exposed by Bullet backend | no 6DOF; angular constraint is a stub | major API gap |
| Axis locking | linear/angular factor getters/setters | no direct equivalent | backend-specific omission or emulation |
| Mesh-mesh | not always required, but expected from Bullet for some use cases | absent | hard feature limit |
| CCD robustness | Bullet offers broader CCD options | Tau CCD is partial | tunneling risk |
| Animated scale / pivots | expected to work reasonably | explicitly fragile | unsafe scenes |
| Asset pipeline | Bullet mesh trees compiled by `bulletc` | Tau expects its own mesh tree/runtime | toolchain work required |

## Age And Maintenance Risk

Tau is not just old in style. Its headers still identify the core code as
2004-2005 era code (`tau_system.h`, `tau_constraint.h`, `gcol_system.h`).

That has a few practical consequences:

- custom containers and intrusive list patterns instead of STL-era ergonomics,
- runtime assumptions from a different engine architecture,
- unfinished or half-disabled features left in public headers,
- more hidden behavioral debt than a modern narrow library would normally have.

This does not make Tau unusable. It does mean that every integration shortcut
has a maintenance cost later.

## Dependency And Porting Cost

This is one of the biggest real costs.

The vendored Tau snapshot only contains `vendor/tau/source/physic`. Its direct
includes reference external legacy infrastructure such as:

- `framework/framework.h`
- `framework/math/*`
- `framework/data_structure/*`
- `framework/geometry/boundingbox.h`
- `framework/tools/benchmark.h`
- `../core/core.h`
- `../manager/manager.h`

Some framework math/data-structure code still exists elsewhere in NeoGS, but
the vendored Tau folder is not self-contained, and some paths referenced by Tau
do not exist in the current tree at those exact locations.

For Harfang, that means a direct "copy vendor/tau and compile" approach is not
realistic. There are only two credible integration strategies:

1. Import a compatible slice of the old supporting framework.
2. Port Tau onto Harfang/Foundation math, geometry, and container abstractions.

Option 1 is faster for a spike but drags legacy architecture into Harfang.

Option 2 is cleaner long-term, but it is no longer a thin vendor integration.
It becomes a partial port.

My recommendation is to avoid importing a large legacy support layer into
Harfang. If Tau is pursued, keep the imported legacy surface as small as
possible and isolate it behind a dedicated backend.

## Harfang Mesh Collision Pipeline Is Bullet-Specific

This is the other major cost center.

Today, Harfang mesh collision resources are compiled through `bulletc` in the
asset compiler pipeline (`tools/assetc/assetc.cpp`). At runtime,
`SceneBullet3Physics::LoadCollisionTree()` loads Bullet collision data through
`btBulletWorldImporter` (`scene_bullet3_physics.cpp`).

Tau does not consume Bullet collision trees. Its mesh collision path expects a
geometry tree and polygon intersection support (`nGeometryBIH`, polygon list
queries, and mesh-tree ray traces).

So `CT_Mesh` support under Tau needs one of these paths:

- A Tau-specific collision asset compiler and serialized mesh format.
- A runtime conversion path from Harfang geometry/collision resources to a Tau
  mesh acceleration structure.

The second option is acceptable for a prototype. The first option is required
for a production-quality backend.

Without this work, the realistic MVP should stay primitive-only:

- sphere
- box

Static mesh support should be treated as phase 2, not phase 1.

## Known Technical Risks Inside Tau

The existing Tau audit already identified several issues that matter directly to
Harfang integration:

- collision output buffers are not safely bounded in dense-contact cases,
- collision masking is one-sided and order-dependent,
- CCD is partial and inconsistent,
- pivot and center-of-mass handling is explicitly marked as suspicious,
- sleep tuning serialization is broken,
- angular constraints are declared but not implemented,
- moving static colliders and runtime scale handling are incomplete,
- parts of the geometry logic appear brittle,
- several public/runtime features are effectively dead or stubbed
  (`documentation/tau-physics-audit.md`).

These are not reasons to reject Tau automatically.

They are reasons to refuse any "just wire it in quickly" plan.

## Collision Queries And Events

Some Harfang features map reasonably well to Tau:

- collision contacts can be derived from Tau/GCollide collision nodes and pair
  monitor data,
- closest-hit raycast already exists,
- overlap-style world collision queries are possible,
- fixed-step callbacks are conceptually compatible with Tau's event model.

But some do not map cleanly:

- `RaycastAllHits` is not present natively in Tau,
- there is no direct equivalent for Bullet's linear/angular factor API,
- 6DOF constraint support is absent,
- kinematic reliability is questionable for moving statics, pivots, and scale.

So the feature split is not "Tau cannot do runtime queries". It is "Tau can do
the basic ones, but not the full Bullet-shaped surface".

## CPU Value Proposition

The CPU argument for Tau is reasonable, but it must be framed carefully.

What can be said with confidence:

- Tau is smaller and less feature-dense than Bullet.
- Tau is shaped around a narrow set of rigid-body cases.
- Harfang's active Bullet path is already single-threaded.

What cannot be said yet:

- that Tau is categorically faster than Bullet in Harfang scenes,
- that Tau scales better under many contacts,
- that Tau is safer under mesh-heavy scenes,
- that Tau will win once adapter and conversion overheads are included.

My inference is:

- Tau may use less CPU in simple scenes dominated by spheres/boxes, sleeping,
  and modest body counts.
- Tau may lose badly, or fail semantically, in fast, dense, mesh-heavy, or
  pivot-heavy scenes.

That is exactly why a benchmark spike is worth doing before any broader design
commitment.

## Recommended Product Shape

Do not try to sell Tau as "Bullet but lighter".

Instead:

- introduce it as `SceneTauPhysics` or `SceneTauPhysicsLite`,
- publish a strict support matrix,
- keep the contract intentionally smaller than `SceneBullet3Physics`,
- document unsupported features as hard limits, not future promises.

Recommended MVP contract:

- supported body types:
  - dynamic,
  - static,
  - kinematic only if explicitly marked experimental
- supported shapes:
  - sphere,
  - box
- runtime features:
  - step simulation,
  - sync transforms to/from scene,
  - force, impulse, torque,
  - wake/sleep,
  - collision events with contacts,
  - closest-hit raycast,
  - overlap query similar to `NodeCollideWorld`
- not supported in MVP:
  - cone,
  - capsule,
  - cylinder,
  - mesh-convex,
  - `RaycastAllHits`,
  - 6DOF constraints,
  - axis locking via linear/angular factor,
  - animated scale guarantees,
  - pivot-heavy setups,
  - mesh-mesh collision

Recommended phase 2:

- static mesh collision,
- Tau-specific mesh collider pipeline,
- stronger kinematic validation,
- benchmark-driven decision on whether the backend deserves long-term support.

## Suggested Technical Approach

### Option A: Dedicated Reduced Backend

Create a separate `SceneTauPhysics` backend with its own explicit limits.

Pros:

- honest API,
- less adapter contortion,
- easier to ship experimentally,
- lower risk of false parity assumptions.

Cons:

- more surface area in Harfang,
- users must choose a backend explicitly.

### Option B: Generic Physics Facade Above Multiple Backends

Introduce a small common physics facade for the truly shared subset only.

Pros:

- cleaner cross-backend story long-term.

Cons:

- easy to over-abstract,
- dangerous if designed around Bullet features instead of the actual common
  subset.

If Harfang goes multi-backend, the facade should be built around the shared
minimum, not around Bullet compatibility.

## Suggested Spike Plan

### Phase 0: Technical Spike

Target: prove that Tau can run a useful Harfang subset without dragging too
much legacy code.

Scope:

- sphere and box only,
- dynamic and static only,
- fixed-step update,
- scene transform sync,
- force/impulse/torque,
- collision contacts,
- closest-hit raycast.

Deliverable:

- a private prototype backend,
- a list of imported or rewritten dependencies,
- first CPU comparison on simple scenes.

### Phase 1: Useful MVP

Add:

- explicit reduced API exposure,
- automated tests for the supported subset,
- clearer sleep/wake and contact reporting behavior,
- initial documentation for unsupported features.

### Phase 2: Mesh And Hardening

Add only if phase 0 and phase 1 are promising:

- static mesh collision support,
- Tau-specific asset or runtime mesh conversion path,
- fixes for buffer bounds and other known safety issues,
- kinematic validation,
- benchmark-based go/no-go decision.

## Benchmark Plan Before Any Commitment

Before deciding whether Tau is worth productizing in Harfang, benchmark at
least:

- 100 to 500 dynamic spheres and boxes dropping onto static boxes,
- sleep-heavy scenes with frequent wake-up,
- raycast-heavy scenes,
- static mesh collision scenes,
- kinematic mover scenes,
- dense contact stacks.

Measure:

- average frame time,
- worst-frame physics time,
- contact count,
- correctness regressions,
- tunneling or unstable contacts.

Without these measurements, "lighter than Bullet" remains only a plausible
hypothesis.

## Effort Estimate

These estimates assume one engineer already familiar with Harfang internals.

Prototype spike:

- 1.5 to 3 engineer-weeks

Primitive-only MVP:

- 3 to 6 engineer-weeks

Static mesh production path and hardening:

- add 3 to 6 engineer-weeks

Trying to reach broad Bullet-style parity:

- not recommended,
- likely to cost multiple months while still ending with a weaker result.

## Non-Technical Blocker: Licensing

This needs explicit review before any real integration.

The vendored Tau tree contains "All rights reserved" headers in multiple source
files, and one header refers to an included `license.txt`, but no such license
file is present in the current vendor snapshot or adjacent files.

That means Tau may be technically portable but still not cleanly redistributable
inside Harfang without clarifying provenance and licensing first.

For a feasibility study, this is not a detail. It is a possible stop condition.

## Recommendation

Proceed only as a staged experiment.

The best technical target is not "replace Bullet in Harfang". It is "validate
whether Tau can provide a useful low-CPU rigid-body backend for a narrow subset
of Harfang scenes".

If you want a realistic and honest path, I recommend:

1. Prototype a dedicated `SceneTauPhysics` backend for sphere/box scenes only.
2. Do not promise Bullet parity.
3. Treat static mesh support as a second phase.
4. Fix the identified Tau safety issues before any public exposure.
5. Resolve licensing before committing to redistribution.

If the benchmark spike shows no clear CPU win on the intended scenes, stop
there. In that case Tau is still worth preserving as a legacy/archival physics
engine, but not worth carrying as an active Harfang backend.

## Sources Checked

- Harfang:
  - `harfang/engine/node.h`
  - `harfang/engine/scene.cpp`
  - `harfang/engine/physics.h`
  - `harfang/engine/scene_bullet3_physics.h`
  - `harfang/engine/scene_bullet3_physics.cpp`
  - `harfang/tests/engine/scene.cpp`
  - `binding/bind_harfang.py`
  - `docs/manual/physics.html`
  - `tools/assetc/assetc.cpp`
  - `tools/assetc/bulletc/main.cpp`

- Tau / NeoGS:
  - `vendor/tau/README.md`
  - `vendor/tau/source/physic/*.h`
  - `vendor/tau/source/physic/*.cpp`
  - `documentation/tau-physics-audit.md`
