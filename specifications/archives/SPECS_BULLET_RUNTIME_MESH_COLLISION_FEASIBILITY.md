# Bullet Runtime Mesh Collision Feasibility For Harfang

Date: 2026-08-31

Method: static source review only. I did not implement runtime Bullet cooking,
and I did not benchmark startup time or per-frame cost on a modified build.

## Executive Summary

Moving Harfang's Bullet mesh collision cooking from the asset pipeline to
runtime is feasible, but the main benefit is workflow and flexibility, not
physics performance.

Today, Harfang does the expensive mesh-collider work offline:

- `bulletc` loads raw geometry,
- builds Bullet collision shapes,
- serializes them,
- runtime only deserializes the result.

That design is good for startup cost because it preserves Bullet's prebuilt BVH
for triangle meshes. A runtime-only path would remove a Bullet-specific asset
compiler step and would make procedural or late-loaded collision meshes much
easier to support, but it would also shift real CPU work to load time and would
require Harfang to ship triangle-source data suitable for Bullet cooking.

Bottom line:

- If the goal is simpler workflows, runtime cooking has real value.
- If the goal is lower simulation CPU cost, runtime cooking does not solve that.
- The best target is a hybrid design:
  - keep prebuilt Bullet mesh blobs as the fast path,
  - add runtime Bullet cooking as a fallback and for procedural content.

## Feasibility Verdict

Feasible with medium integration risk.

Recommended only as:

- a fallback for missing cooked Bullet collision resources,
- or an explicit runtime path for procedural/generated geometry,
- or a hybrid replacement that preserves offline cooking as an optimization.

Not recommended as a blind replacement of the existing offline path for all
content.

## Current Harfang Design

The current split is clear in the code:

- `SceneBullet3Physics::NodeCreatePhysics()` creates primitive Bullet shapes at
  runtime for sphere, cube, cone, capsule, and cylinder.
- For `CT_Mesh`, it calls `LoadCollisionTree(...)` and deserializes a Bullet
  collision shape from a resource path.
- `LoadCollisionTree(...)` uses `btBulletWorldImporter` and caches the resulting
  `btCollisionShape *` by resource name.
- `assetc` invokes `bulletc` for physics resources.
- `bulletc` loads raw geometry with `LoadGeometryFromFile(...)`, builds either a
  `btBvhTriangleMeshShape` or a `btConvexHullShape`, then serializes the shape
  through `btDefaultSerializer`.

In other words, Harfang already constructs Bullet primitives in-process. Meshes
are the only part still delegated to an offline Bullet-specific cooking step.

## What The Offline Path Buys Today

The current design is not just historical convenience. It preserves real work.

`btBvhTriangleMeshShape` normally builds an optimized BVH when constructed.
Bullet's importer can also reconstruct a mesh shape from an already serialized
`btOptimizedBvh` without rebuilding that BVH.

That means the current offline pipeline buys Harfang:

- lower scene startup cost for mesh colliders,
- less runtime CPU spent on Bullet mesh preparation,
- no runtime need for raw triangle-source assets,
- smaller runtime implementation surface,
- deterministic cooked collision resources.

So the current pipeline is Bullet-specific, but it is not pointless.

## What A Runtime Shift Would Actually Mean

A runtime shift would not mean changing physics backends. It would still use
Bullet.

It would mean that when a mesh collision resource is requested, Harfang would:

- read a collision descriptor or triangle-source asset at runtime,
- load the required mesh data,
- build the Bullet collision shape in memory,
- cache the resulting Bullet objects for reuse,
- skip the serializer/importer round-trip for that resource.

For triangle meshes, this would typically mean building:

- `btTriangleMesh`
- `btBvhTriangleMeshShape`

For convex mesh colliders, this would typically mean building:

- `btConvexHullShape`

Conceptually, this is localized work. Harfang already does similar runtime
construction for primitive Bullet shapes in `SceneBullet3Physics`.

## The Real Benefits Of Runtime Bullet Cooking

### 1. Simpler Authoring And Iteration

This is the strongest benefit.

Today a mesh collision workflow depends on:

- a Bullet-specific resource preparation step,
- `bulletc` being available in the toolchain,
- an extra cooked artifact existing beside the source resource.

Moving cooking to runtime reduces that coupling. It becomes easier to:

- load scenes directly from the filesystem,
- work with partially built content,
- bypass assetc during fast iteration,
- avoid "missing cooked collision blob" failures during development.

### 2. Better Support For Procedural And Late-Loaded Content

Offline cooking is a poor fit for:

- procedural meshes,
- user-generated content,
- downloaded assets,
- editor-generated collision meshes,
- runtime importers.

Runtime Bullet cooking is a much better fit for those cases because the collider
can be derived directly from the triangles available at runtime.

This is likely the best product reason to add the feature.

### 3. Less Dependence On Bullet Serialization Format

The current path depends on:

- `btDefaultSerializer`
- `btBulletWorldImporter`
- Bullet's serialized collision-shape format and associated DNA/version logic

Runtime cooking avoids that dependency for the cooking path itself. That has
two practical advantages:

- less reliance on Bullet serialization internals,
- fewer compatibility concerns if Bullet serialization behavior changes.

It would also make the runtime code more direct: build the shape you need
instead of loading a previously serialized shape blob.

### 4. Easier To Unify The Runtime Story

Right now Harfang already builds primitive collision shapes at runtime, but mesh
colliders use a separate offline path.

A runtime mesh-cooking path would make the runtime behavior more uniform:

- all collision shapes are ultimately constructed in-process,
- the mesh case stops being architecturally special,
- the engine can own more of the collision resource logic directly.

### 5. Opportunity To Close Existing Feature Drift

`bulletc` already contains a convex-hull build path, but the current runtime
Bullet setup only handles `CT_Mesh` in `NodeCreatePhysics()` and does not
implement `CT_MeshConvex` there.

If Harfang introduces direct runtime Bullet cooking, it can use that moment to
close the gap and support:

- triangle mesh cooking,
- convex hull cooking,
- with one consistent runtime code path.

This is a secondary benefit, but it is concrete.

### 6. Potentially Cleaner Ownership And Caching

The current mesh-shape cache is a map of resource name to `btCollisionShape *`.
Runtime cooking would still need caching, but it could formalize ownership
better by storing a cache entry that owns:

- the triangle data container,
- the Bullet mesh interface,
- the Bullet collision shape,
- optional metadata such as source kind and build time.

That would be a cleaner design surface than a bare shared shape pointer cache.

## What Runtime Bullet Cooking Does Not Improve

This part matters because it is easy to over-credit the idea.

Runtime Bullet cooking does not improve steady-state simulation cost in the
general case.

Once the mesh shape exists, Bullet is still solving against Bullet shapes.
Using a `btBvhTriangleMeshShape` built at runtime versus deserialized from a
cooked blob should produce broadly similar simulation characteristics after
startup.

So runtime cooking does not directly buy:

- lower per-frame physics CPU,
- better broadphase scaling,
- better narrow-phase quality,
- better contact stability,
- new Bullet collision semantics.

If the target is lower physics-step CPU, this change is mostly orthogonal.

## The Main Costs And Risks

### 1. Startup And Hitch Cost

This is the biggest technical tradeoff.

Offline cooking prebuilds the Bullet BVH. Runtime cooking has to build it on
the client machine, during scene load or first use.

That means:

- slower first load for mesh colliders,
- possible hitches if collider creation happens mid-frame,
- more CPU spikes in editor or gameplay flows that instantiate physics lazily.

If Harfang moves to runtime cooking, it should assume some form of:

- explicit preload,
- background cooking,
- or load-screen-time preparation.

### 2. Runtime Asset Requirements Change

This is the second biggest cost, and it is easy to miss.

`bulletc` currently consumes raw `Geometry` data through
`LoadGeometryFromFile(...)`.

But Harfang's standard geometry asset pipeline does not preserve that exact raw
format for normal runtime mesh loading. The asset compiler turns geometry into a
runtime model file through `SaveGeometryModelToFile(...)`, while the raw
geometry loader expects a different layout based on `vtx`, `pol`, and
`binding`.

So a runtime Bullet cook cannot simply reuse today's offline cook inputs
without one of these decisions:

- ship raw geometry-style assets for physics cooking,
- define a collision descriptor that references a runtime triangle-source asset
  Harfang can read on CPU,
- or add a new path that reconstructs collision triangles from a different
  runtime asset format.

This is why runtime cooking is not only an engine change. It is also an asset
contract change.

### 3. More Runtime Complexity

The current runtime path for mesh colliders is simple:

- read blob,
- import shape,
- cache pointer.

A runtime-cooking path must additionally handle:

- parsing collision descriptors,
- loading triangle-source assets,
- building triangle or convex shapes,
- preserving mesh-interface lifetime,
- caching build results,
- surfacing build errors well,
- preventing duplicate builds.

That is all reasonable work, but it is real work.

### 4. More Need For Correct Lifetime Management

For deserialized shapes, Bullet's importer owns the internal allocations it
creates during import.

For runtime-built mesh shapes, Harfang would need to keep the underlying mesh
data alive for as long as the Bullet shape needs it. In practice, the cache
entry must own more than a `btCollisionShape *`.

This argues for a dedicated cache struct rather than extending the current
`std::map<std::string, btCollisionShape *>`.

### 5. Possible Asset Size Or Packaging Tradeoff

The current runtime only needs the cooked Bullet collision resource.

A runtime-cooking path may require shipping more triangle-source data than the
current path does, depending on the chosen asset contract.

So the likely packaging benefit is:

- fewer special-purpose cooked collision artifacts,

not necessarily:

- smaller shipped content.

### 6. Duplication Risk If `bulletc` And Runtime Logic Diverge

If Harfang adds runtime cooking by re-implementing `bulletc` logic separately,
the two code paths can drift.

That is avoidable.

The better design is to extract the actual Bullet cooking logic into a shared
helper that both:

- `tools/assetc/bulletc`
- `SceneBullet3Physics`

can call.

## The Best Engineering Shape

The best design is not "delete `bulletc` and always cook at runtime."

The best design is a hybrid path.

### Recommended Hybrid Model

Preferred order:

1. Try to load a prebuilt Bullet collision blob.
2. If not available, or if the resource is explicitly marked runtime-cooked,
   build the Bullet shape at runtime.
3. Cache the built result by resource name.

This keeps the existing startup optimization for shipped content while enabling:

- direct filesystem iteration,
- editor-side flexibility,
- procedural/runtime-generated collision meshes,
- fallback behavior when cooked artifacts are missing.

### Recommended Shared Cooking Helper

Refactor the cooking logic out of `bulletc/main.cpp` into a reusable helper,
for example in the engine or a shared tool/runtime module.

That helper would:

- parse the existing JSON collision descriptor,
- load the referenced triangle-source assets through a `Reader` and
  `ReadProvider`,
- build the Bullet collision shape,
- return an owning cache entry rather than a bare pointer.

This avoids maintaining two independent Bullet cooking implementations.

## Practical Implementation Options

### Option A: Runtime Cook From Existing Physics Descriptor

Keep the current collision descriptor format and build from it at runtime.

Pros:

- minimal authoring change,
- preserves current collision authoring semantics,
- easy conceptual migration.

Cons:

- still requires runtime access to triangle-source assets,
- may require shipping raw geometry-like assets or adding a compatible CPU mesh
  source format.

### Option B: Runtime Cook From A Dedicated Collision Mesh Asset

Create a collision-oriented mesh asset format meant for CPU-side triangle
access.

Pros:

- explicit runtime contract,
- no dependence on render-model readback,
- clearer separation of render and physics mesh needs.

Cons:

- new asset type,
- more pipeline work.

### Option C: Hybrid With Offline Preferred, Runtime Fallback

Keep cooked Bullet blobs for production scenes and add runtime cooking for:

- missing blobs,
- procedural geometry,
- editor or debug workflows.

Pros:

- captures most benefits,
- avoids penalizing production startup by default,
- lowest migration risk.

Cons:

- keeps two possible loading modes,
- requires careful diagnostics so users know which path was taken.

Recommendation: Option C.

## Recommendation

Add runtime Bullet mesh cooking, but do not replace the offline path
unconditionally.

The change is most valuable as a hybrid capability because it solves the real
problems runtime cooking is good at:

- workflow friction,
- filesystem iteration,
- missing cooked data,
- procedural content,
- editor/runtime-generated meshes.

At the same time, it preserves the main strength of the current design:

- startup cost stays low for cooked production content.

If Harfang were to switch fully to runtime Bullet mesh cooking, it would gain
architectural simplicity in one area but would pay for it with:

- slower startup,
- more runtime complexity,
- and a new runtime asset-data requirement.

That is usually the wrong default tradeoff for shipped scenes.

## Suggested MVP

The smallest sensible MVP is:

- keep `bulletc` unchanged,
- add a runtime Bullet cooking helper shared with `bulletc`,
- let `LoadCollisionTree(...)` fall back to runtime cooking when deserialization
  fails or when a resource is explicitly marked for runtime cooking,
- add profiling/logging so the user can see which path was used,
- add tests for:
  - triangle mesh collider load,
  - convex mesh collider load,
  - cache reuse,
  - missing cooked blob fallback,
  - first-load timing visibility.

That would deliver the main workflow benefits without forcing an all-or-nothing
migration.

## Effort Estimate

Prototype:

- 3 to 6 engineer-days

Hybrid MVP with shared helper and tests:

- 2 to 4 engineer-weeks

Production-ready runtime cooking with robust asset-contract changes, async
loading strategy, and profiling:

- 4 to 8 engineer-weeks

## Sources Checked

- `harfang/engine/scene_bullet3_physics.cpp`
- `harfang/engine/scene_bullet3_physics.h`
- `harfang/engine/scene.cpp`
- `harfang/engine/meta.cpp`
- `harfang/engine/meta.h`
- `harfang/engine/geometry.cpp`
- `harfang/engine/geometry.h`
- `harfang/engine/render_pipeline.cpp`
- `harfang/engine/render_pipeline.h`
- `tools/assetc/assetc.cpp`
- `tools/assetc/bulletc/main.cpp`
- `extern/bullet3/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp`
- `extern/bullet3/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h`
- `extern/bullet3/Serialize/BulletWorldImporter/btWorldImporter.cpp`
