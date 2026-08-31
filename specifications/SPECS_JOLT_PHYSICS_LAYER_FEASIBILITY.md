# Jolt Physics Layer Feasibility For Harfang

Date: 2026-08-31

Method: static local source review plus official Jolt repository and
documentation review only. I did not integrate Jolt into Harfang, and I did
not run comparative CPU benchmarks against Bullet or Tau.

Assumption: in this document, "Physics" is interpreted as Bullet Physics,
because Bullet is Harfang's current scene-physics backend.

## Executive Summary

Building a Jolt-based physics layer as an alternative to Bullet in Harfang is
feasible.

If the goal is to choose a serious high-performance option between Bullet, Tau,
and Jolt, Jolt is the strongest candidate.

However, if Harfang must remain on C++14, Jolt is a no-go for the current
roadmap.

The core reasons are:

- Jolt is modern, active, multithread-oriented, and explicitly designed for
  game rigid-body workloads.
- Jolt covers most of the rigid-body, query, and constraint surface that
  Harfang already exposes or is likely to want.
- Jolt's recommended mesh workflow is close to Harfang's existing offline
  Bullet mesh-cooking pipeline: cook shapes offline, serialize them, and
  restore runtime-ready data.
- Tau remains interesting as a small legacy lightweight engine, but it is a
  much weaker strategic option because of feature gaps, age, dependency drift,
  and known correctness/safety problems.

The main blocker is not Jolt itself. The blocker is Harfang's current physics
architecture:

- `SceneBullet3Physics` is a public engine type.
- `SceneUpdateSystems`, `SceneSyncToSystems`, bindings, tutorials, and
  documentation are typed directly on the Bullet backend.
- a Bullet native type leaks into the public API:
  `btGeneric6DofConstraint *`.

So the realistic conclusion is:

- Feasible as a new backend.
- Not feasible as a zero-refactor drop-in replacement.
- Worth doing only if Harfang first accepts a backend-neutral physics layer, or
  at least a partially neutral runtime contract.

Bottom line:

- Best short-term low-risk option: keep Bullet and optimize the current
  integration.
- Best strategic high-performance option in abstract: introduce a neutral
  physics layer and implement Jolt first.
- Best practical decision under the current Harfang C++14 constraint: do not
  pursue Jolt.
- Tau should not be the lead candidate if the target is a durable
  high-performance backend.

## Feasibility Verdict

Feasible with medium-to-high integration cost.

Not feasible as a "just swap the library" change.

If C++17 is off the table, this study should be treated as a no-go decision for
Jolt, not as an active integration candidate.

Feasible in two practical forms:

1. Add a separate `SceneJoltPhysics` backend next to `SceneBullet3Physics`.
2. Introduce a new engine-owned physics layer, then plug Bullet and Jolt behind
   it.

Option 1 is faster but keeps Harfang fragmented and Bullet-specific in public
API design.

Option 2 is more work, but it is the only clean route if the real objective is
to choose between Bullet, Tau, and Jolt over time.

## Current Harfang Situation

Harfang is not currently structured around a backend-neutral physics contract.

The important local facts are:

- the project is still configured for `CMAKE_CXX_STANDARD 14`
  (`harfang3d/CMakeLists.txt`);
- scene helper functions are typed directly on `SceneBullet3Physics`
  (`harfang/engine/scene_systems.h`, `scene_systems.cpp`);
- bindings expose `hg::SceneBullet3Physics` directly
  (`binding/bind_harfang.py`);
- the public binding surface also exposes `btGeneric6DofConstraint *`;
- tutorials and manuals teach `SceneBullet3Physics` directly;
- the current backend is effectively single-threaded in the active code path
  (`scene_bullet3_physics.cpp`);
- mesh collision assets are compiled through `bulletc` and deserialized at
  runtime with `btBulletWorldImporter`
  (`tools/assetc/bulletc/main.cpp`,
  `harfang/engine/scene_bullet3_physics.cpp`).

That means a Jolt study is really about two different problems:

1. Is Jolt a good physics engine candidate?
2. Is Harfang ready for a second serious backend?

The first answer is mostly yes.

The second answer is only yes if Harfang is willing to pay an API and tooling
refactor cost.

## Why Jolt Is A Serious Candidate

From the official Jolt repository and docs, Jolt has several properties that
make it a credible high-performance backend:

- It is explicitly described as a "multi core friendly rigid body physics and
  collision detection library."
- It is built around concurrency, including background body preparation,
  collision queries that can run in parallel with simulation work, and
  multi-threaded world update.
- It supports deterministic simulation with documented limits.
- It supports the mainstream rigid-body shapes Harfang needs:
  sphere, box, capsule, cylinder, convex hull, mesh, compound, terrain.
- It supports 6DOF constraints and constraint motors.
- It supports ray casts, shape-vs-world tests, shape casts, sensors, vehicles,
  characters, ragdolls, and more.
- It is cross-platform, MIT licensed, STL-only, and does not require RTTI or
  exceptions.
- It is actively maintained, which is visible from recent 2025-2026 API and
  release notes.

This does not prove it will beat Bullet in Harfang on every scene.

It does prove that Jolt is a technically serious modern candidate, while Tau is
still in the "interesting legacy lightweight engine" category.

## Why Jolt Fits Harfang Better Than Tau

This is the most important strategic comparison point.

Tau would require Harfang to absorb an old engine-style runtime with custom
containers, missing dependencies, and a narrower collision/constraint contract.

Jolt is harder than Bullet to integrate, but it is architecturally much closer
to what a modern optional backend should look like:

- active upstream;
- explicit thread model;
- documented serialization model;
- broad rigid-body shape coverage;
- broad query support;
- modern build story;
- production use in current games and engines.

Jolt also aligns much better with Harfang's current offline mesh collision
pipeline philosophy than Tau does.

Jolt's own architecture documentation explicitly recommends:

- cooking mesh shapes offline,
- serializing them with `Shape::SaveWithChildren`,
- discarding the editable mesh data,
- restoring runtime-ready shapes with `Shape::sRestoreWithChildren`.

That is conceptually very close to what Harfang already does with `bulletc` and
`btBulletWorldImporter`.

This is a major point in Jolt's favor.

## What A Real Physics Layer Would Require

If the goal is "an alternative to Bullet," Harfang has to decide whether it
wants:

- another backend-specific class, or
- an actual engine-owned physics layer.

For a real layer, the minimum abstraction should move the engine away from
Bullet-native naming and types.

At minimum, the layer should own:

- body creation and destruction;
- shape creation/caching;
- scene sync from nodes to physics;
- scene sync from physics to nodes;
- fixed-step update;
- contact event collection;
- overlap / collide-world query;
- first-hit and all-hit raycasts;
- force, impulse, torque, and velocity operations;
- sleeping / activation;
- backend-neutral constraint handles;
- debug rendering hooks.

Today Harfang does not own that contract. Bullet does.

That is why a Jolt backend is feasible, but a true backend-neutral layer is a
larger architectural change than just importing Jolt sources.

## Jolt Versus Harfang's Current Contract

| Area | Harfang Today | Jolt Fit | Notes |
| --- | --- | --- | --- |
| Body modes | dynamic, kinematic, static | good | direct mapping exists |
| Sphere / box / capsule / cylinder | required | good | direct mapping exists |
| Cone collision shape | required in Harfang API | partial | no obvious first-class cone shape in Jolt's documented core shape list; use convex hull or custom shape |
| Mesh collision | required | good with limits | Jolt supports triangle mesh shapes |
| Mesh convex | helper exists in Harfang | good | Jolt convex hull support is stronger than Harfang's current Bullet integration here |
| Raycast first hit | required | good | direct support |
| Raycast all hits | required | good | collector-based support |
| Overlap / collide world | required | good | `NarrowPhaseQuery::CollideShape` / `TransformedShape` fit this well |
| Contact events | required | good with adaptation | callback model is multi-threaded and read-only |
| 6DOF constraint | exposed today | good | supported by Jolt |
| Axis lock factors | exposed today | partial | Jolt has allowed DOFs and constraints, but not a direct 1:1 Bullet-style factor API |
| Vehicles / characters | not central to current Harfang tests | strong upside | Jolt already has first-class support if Harfang ever wants it |
| Mesh asset cooking | Bullet-specific today | good | Jolt has a natural offline-cook and restore model |

The key pattern is this:

- Jolt covers the important backend capabilities better than Tau.
- Jolt still does not map perfectly to Harfang's current public Bullet-shaped
  API.

## Important Jolt Caveats

Jolt is strong, but it is not magic.

### 1. Harfang is C++14 today, Jolt uses C++17

Jolt's official build requirements state:

- Visual Studio 2022+, Clang 16+, or GCC 12+;
- C++17;
- STL only;
- no RTTI;
- no exceptions.

Harfang currently builds as C++14.

That means one of these must happen:

- the engine target moves to C++17;
- or Jolt-facing backend code is isolated very carefully behind private
  implementation boundaries that keep Jolt headers out of C++14-facing code.

This is a real integration cost, not a footnote.

If Harfang is not allowed to move to C++17, this point alone is sufficient to
reject Jolt for now.

### 2. Jolt wants explicit runtime capacity management

Jolt's standard initialization flow requires explicit world limits such as:

- max bodies,
- max body pairs,
- max contact constraints,
- temp allocator size,
- job system configuration.

That is good for predictable performance, but Harfang would need new backend
configuration points or at least sane defaults.

Bullet integration today is looser in this area.

### 3. Jolt's threading model changes Harfang's contact/event design

Jolt's `ContactListener` callbacks are documented as multi-threaded and
read-only while bodies are locked.

Harfang's current Bullet flow is simpler:

- step the world,
- scan manifolds afterward,
- build contact output on the main thread.

A Jolt backend should therefore not expose raw callback behavior upward.

It should queue backend events internally and publish a stable
post-step `NodePairContacts` result afterward.

### 4. Triangle mesh support has restrictions

Jolt documents `MeshShape` as mostly intended for static geometry.

It also documents that dynamic or kinematic mesh shapes should not collide with
other mesh or heightfield shapes.

In practice, that means:

- static environment meshes are a good fit;
- dynamic concave mesh bodies are not a great general-purpose fit;
- Harfang should prefer convex hulls, compounds, or decomposition for moving
  complex shapes.

That is still a much better position than Tau, but it is a real rule that the
engine layer must encode.

### 5. Active project also means API churn

Jolt's `Docs/APIChanges.md` shows active 2025-2026 changes, including several
entries that explicitly break binary serialization compatibility (`SBS`).

This is not a reason to reject Jolt.

It means Harfang should expect one rule:

- if Harfang serializes cooked Jolt shape data, upgrading Jolt may require a
  physics asset rebuild.

That is manageable, but it should be planned from the start.

## Mesh Pipeline Outlook

Jolt is much more compatible with Harfang's current asset philosophy than Tau,
but it still needs its own path.

Today Harfang does this for Bullet mesh collision:

- read raw geometry,
- build Bullet mesh or convex collision shapes,
- serialize runtime-ready Bullet data,
- restore it later through Bullet's importer.

For Jolt, the equivalent production path should be:

- read raw geometry,
- build `MeshShape` or `ConvexHullShape`,
- serialize with Jolt's save/restore path,
- cache the cooked shape as an engine collision asset,
- restore runtime-ready Jolt shapes on demand.

So Jolt does not remove tooling work.

It reduces conceptual mismatch.

That matters because Tau would need Harfang to invent much more custom physics
asset logic around an older engine architecture.

## What Jolt Could Improve Over The Current Harfang Bullet Backend

Jolt should be viewed as a potential backend improvement in these areas:

- better native multi-thread scaling than Harfang's current active Bullet path;
- better fit for scenes that need high rigid-body throughput and background
  body/query work;
- cleaner modern feature path for convex hulls, characters, vehicles, and
  future higher-end gameplay use cases;
- potentially cleaner backend-specific configuration around sleeping, capacity,
  broadphase behavior, and batch body insertion;
- a credible long-term alternative that is maintained upstream.

There is also one specific Harfang-local upside:

- Harfang already has `CT_MeshConvex` in scene helpers, but the current Bullet
  backend does not implement that path in `NodeCreatePhysics()`.
- Jolt has first-class convex hull support, so a Jolt backend could make that
  shape route more coherent than the current Harfang/Bullet implementation.

## What Jolt Does Not Solve Automatically

Jolt does not automatically solve these Harfang problems:

- Bullet-specific public API naming;
- backend-specific scripting bindings;
- backend-specific tutorials and manual text;
- backend-neutral save/load contracts;
- backend-neutral debug rendering;
- backend-neutral constraint handles;
- Harfang-side scene ownership and handle lifetime rules.

In other words: Jolt can be a good backend, but it is not a substitute for a
proper engine-level abstraction.

## Decision Matrix: Bullet vs Tau vs Jolt

| Option | CPU upside potential | Integration cost | Feature coverage | Maintenance risk | Strategic value |
| --- | --- | --- | --- | --- | --- |
| Keep Bullet and optimize current backend | low to medium | low | high, already integrated | low | best short-term |
| Build Tau backend | unknown to medium in very simple scenes | medium to high | low to medium | high | weak strategic choice |
| Build Jolt backend | medium to high | high to prohibitive under C++14 | medium to high | medium | best long-term performance candidate in abstract, but no-go under current language constraint |

Important nuance:

- "CPU upside" here is an engineering judgment based on architecture and
  feature surface, not a benchmark result from Harfang.
- If Harfang wants a safe answer in the next short cycle, Bullet remains the
  rational default.
- If Harfang wants a second serious backend worth carrying forward, Jolt is the
  better investment than Tau.

## Recommended Engineering Shape

If Harfang chooses Jolt, I recommend this shape:

1. Introduce a backend-neutral internal interface first.
2. Keep the first contract intentionally narrow.
3. Implement Bullet and Jolt behind that contract.
4. Keep backend-native advanced features out of the first public layer.

The first neutral contract should target only what Harfang already uses
heavily:

- rigid bodies,
- primitive colliders,
- convex hull collider,
- static mesh collider,
- fixed-step stepping,
- transform sync,
- contact collection,
- raycasts,
- overlap/collide query,
- force/impulse/torque,
- sleep/wake.

The following should be phase 2 or backend-specific extensions:

- vehicles,
- characters,
- soft bodies,
- backend-native constraint classes,
- advanced collision filtering surface,
- backend-native serialization details.

This reduces risk and makes Bullet vs Jolt comparison meaningful on the same
feature slice.

## Suggested MVP

The minimum serious Jolt spike for Harfang should do the following:

1. Build Jolt as an optional dependency in Harfang.
2. Prove a `SceneJoltPhysics` backend with:
   - sphere,
   - box,
   - capsule,
   - cylinder,
   - convex hull,
   - static mesh,
   - dynamic / kinematic / static bodies.
3. Implement:
   - step,
   - scene sync,
   - contact collection,
   - first-hit raycast,
   - all-hits raycast,
   - collide-world equivalent,
   - force / impulse / torque,
   - wake / sleep.
4. Add a Jolt mesh-cooking path for collision assets.
5. Benchmark the same Harfang scenes against Bullet.

If that spike does not show a meaningful CPU advantage on Harfang's real scene
types, then Harfang should stop there and keep Bullet.

## Effort Estimate

These numbers are directional only.

### Narrow spike

- 1 to 2 weeks

Scope:

- build integration,
- primitive bodies,
- basic stepping,
- basic queries,
- local smoke tests.

### Usable optional backend

- 4 to 8 weeks

Scope:

- shape coverage,
- contact collection,
- asset cooking path,
- scene-system integration,
- bindings or a first internal API surface,
- comparative benchmarks.

### Clean multi-backend architecture

- 8 to 14 weeks

Scope:

- internal physics layer refactor,
- Bullet and Jolt implementations,
- doc/tutorial updates,
- binding reshaping,
- asset pipeline cleanup,
- regression tests,
- performance tuning.

The main uncertainty is not Jolt coding difficulty.

It is how far Harfang wants to go in removing Bullet-native assumptions from the
public engine API.

## Recommendation

If the question is:

"Which option should Harfang back as a high-performance alternative between
Bullet, Tau, and Jolt?"

My recommendation is:

- if C++17 were acceptable, choose Jolt over Tau as the serious alternative
  backend candidate;
- under the actual current C++14 constraint, treat Jolt as a no-go;
- keep Bullet as the baseline shipping backend;
- do not start with Tau if the goal is a durable, performant, general-purpose
  alternative.

If the budget only allows a narrow short-term change, the correct decision is
still to optimize Bullet rather than start a backend migration.

If Harfang later relaxes the language constraint, Jolt is still the option most
worth studying through implementation and benchmarks.

## Sources Checked

Local Harfang sources:

- `harfang3d/CMakeLists.txt`
- `harfang3d/harfang/engine/node.h`
- `harfang3d/harfang/engine/scene.cpp`
- `harfang3d/harfang/engine/scene_systems.h`
- `harfang3d/harfang/engine/scene_systems.cpp`
- `harfang3d/harfang/engine/scene_bullet3_physics.h`
- `harfang3d/harfang/engine/scene_bullet3_physics.cpp`
- `harfang3d/tools/assetc/bulletc/main.cpp`
- `harfang3d/binding/bind_harfang.py`
- `harfang3d/harfang/tests/engine/scene.cpp`

Local Tau material for comparison:

- `S:/works/engine-neogs/vendor/tau/README.md`
- `S:/works/engine-neogs/documentation/tau-physics-audit.md`

Official Jolt sources:

- https://github.com/jrouwe/JoltPhysics
- https://github.com/jrouwe/JoltPhysics/blob/master/Docs/Architecture.md
- https://github.com/jrouwe/JoltPhysics/blob/master/Docs/APIChanges.md
- https://jrouwe.github.io/JoltPhysicsDocs/5.1.0/class_body_creation_settings.html
- https://jrouwe.github.io/JoltPhysicsDocs/5.1.0/class_contact_listener.html
- https://jrouwe.github.io/JoltPhysicsDocs/5.1.0/class_mesh_shape.html
- https://jrouwe.github.io/JoltPhysicsDocs/5.1.0/class_narrow_phase_query.html
- https://github.com/jrouwe/JoltPhysics/blob/master/HelloWorld/HelloWorld.cpp
