# Tau Cuboid Contact Manifold Implementation Plan

Date: 2026-09-01

Status: Proposed implementation plan.

Scope: Harfang's compile-time Tau physics backend, initially restricted to
dynamic/static cuboid (`CT_Cube`) contacts.

## Purpose

The current Tau integration reduces each cuboid-versus-cuboid SAT overlap to a
single, transient contact point. That is adequate for basic falling cuboids,
but not for persistent contacts, stacked bodies, or interlocked compound
shapes. `physics-qa/rb_rings_chain.lua` exposes the limitation: its chain has
no explicit joints and must remain connected through multiple contacts between
the cuboids composing adjacent rings.

This plan introduces a persistent OBB contact manifold with one to four
contact points, accumulated impulses, and warm-starting. It is a solver
foundation, not a cosmetic tuning pass.

## Goals

- Keep `CT_Cube` versus `CT_Cube` contacts stable across fixed simulation
  steps.
- Generate one to four geometrically valid points for each cuboid pair.
- Preserve contact identity between frames using feature IDs and local-space
  anchors.
- Accumulate normal and friction impulses during each solver step.
- Warm-start matching contacts at the following step.
- Improve `rb_rings_chain.lua` without regressing existing cuboid, sphere,
  friction, restitution, and rolling-friction QA scenarios.
- Keep the public Harfang Lua and C++ physics APIs unchanged.

## Non-Goals

- Mesh, capsule, cone, cylinder, or convex-hull manifolds.
- Continuous collision detection.
- Joints, motors, soft constraints, or an ISO-compatible Bullet solver.
- Bit-exact Bullet replay.
- Porting scene graph, collision, or solver code from another engine.

## Current Limitation

`scene_tau_physics.cpp` currently does the following for a cube pair:

1. SAT selects the separating axis with the smallest overlap.
2. The overlap is reduced to one normal, one penetration depth, and one
   approximate contact point.
3. Contacts are rebuilt every step with no persistent identity.
4. The velocity solver applies impulses without a per-contact accumulated
   impulse cache.

This loses the information required to keep a broad face contact or an
interlocked compound contact stable. Deriving several points after SAT without
stable feature provenance is insufficient: point order can change from one
frame to the next and makes warm-start impulses unsafe.

## Target Contact Model

Add an internal `TauContactManifold` for each colliding shape pair:

```cpp
struct TauContactFeature {
    enum Type { FaceA, FaceB, EdgeEdge } type;
    uint8_t axis_a, axis_b;
    int8_t sign_a, sign_b;
};

struct TauManifoldPoint {
    Vec3 local_point_a, local_point_b;
    float penetration;
    float accumulated_normal_impulse;
    Vec3 accumulated_tangent_impulse;
    uint32_t feature_id;
};

struct TauContactManifold {
    NodeRef ref_a, ref_b;
    uint32_t shape_a, shape_b;
    Vec3 normal;
    TauContactFeature feature;
    std::array<TauManifoldPoint, 4> points;
    uint8_t point_count;
    uint32_t last_seen_step;
};
```

The final storage can use reusable vectors rather than fixed arrays if that
fits the backend better. The required limits are four points per cuboid pair
and a bounded lifetime for unused cached manifolds.

### Required Invariants

- The normal has one documented orientation: from shape A toward shape B.
- An impulse applied to A is the opposite of the impulse applied to B.
- Local anchors reconstruct current world contact positions from current body
  transforms.
- A point matches only when its feature ID and local anchors remain compatible
  within a small tolerance.
- Destroying nodes, clearing physics, or garbage-collecting nodes removes all
  dependent cached manifolds.
- The cache belongs to `SceneTauPhysics`; it is never global shared state.

## Implementation Stages

### Stage 1: Preserve SAT Feature Provenance

Refactor OBB SAT so it returns the minimum-overlap feature, not only a normal
and penetration depth.

- Record whether the axis is a face axis of A, a face axis of B, or a
  cross-product edge axis.
- Record source axis indices and signs.
- Derive normal orientation from the selected feature and center delta.
- Keep the current single-point path only as a temporary degenerate fallback.

Acceptance criteria:

- Unit-test face A, face B, and edge-edge selections with axis-aligned,
  rotated, and nearly parallel cuboids.
- Verify normal orientation and penetration sign independently of velocity.

### Stage 2: Generate Geometric Manifold Points

Implement two narrowphase paths using the SAT feature.

Face-face path:

1. Select the SAT reference face.
2. Select the most anti-parallel incident face on the other OBB.
3. Clip the incident quadrilateral against the four reference-face side
   planes.
4. Discard points outside the penetration tolerance.
5. Reduce deterministically to at most four points.

Edge-edge path:

1. Construct the two support edges from the recorded axes and signs.
2. Find closest points between those segments.
3. Store their midpoint as the single manifold point.

For every retained point, inverse-transform the world position into both
cuboid spaces. Generated-point order must not be the sole contact identity.

Acceptance criteria:

- Face-on-face overlap produces up to four stable anchors.
- Edge-on-edge overlap produces one stable anchor.
- Slight rotations do not exchange reference and incident bodies on
  alternating steps.
- Tau debug rendering can show every manifold point and normal.

### Stage 3: Per-Step Sequential Impulse Accumulation

Refactor the velocity solver from stateless impulses to incremental projected
Gauss-Seidel impulses. For each manifold point:

1. Compute normal effective mass including angular terms.
2. Add the normal impulse delta to the accumulated normal impulse.
3. Clamp total normal impulse to zero or above and apply only the delta.
4. Accumulate tangent impulse deltas.
5. Clamp tangent magnitude to `friction * accumulated_normal_impulse`.
6. Apply only the clamped tangent delta.

Restitution applies only to contacts entering with sufficient negative normal
velocity; it must not be reintroduced by every solver iteration.

Acceptance criteria:

- A resting cuboid neither gains vertical energy nor sinks progressively.
- A four-point floor contact is not four times stronger than a one-point
  contact.
- Variable friction and restitution QA remain finite and physically ordered.

### Stage 4: Persistent Cache and Warm-Start

Store active manifolds in `SceneTauPhysics` between substeps and frames.

- Index by ordered body refs, shape indices, and SAT feature.
- Match points by feature ID and local-anchor distance.
- Apply cached normal and tangent impulses before velocity iterations.
- Decay or discard unmatched entries after a small fixed number of steps.
- Clamp reused impulses if the normal changes materially.

Start with deterministic bounded linear searches rather than a complex hash
table. The cuboid-only Tau scope is small, and observability is more valuable
than premature optimization.

Acceptance criteria:

- Tau diagnostics report warm-start hits and misses.
- Interlocked cuboids retain related contacts across frames.
- Clearing or destroying bodies leaves no stale cache entries.

### Stage 5: Position Solver and Contact Refresh

After manifold identity is stable:

- Solve positional correction per manifold point with a share factor that
  avoids correcting the same penetration four times.
- Add angular positional correction only with validated local anchors and
  effective mass.
- Re-evaluate geometry between position iterations only if QA and profiling
  justify it; do not blindly rebuild all contacts in the inner loop.

Acceptance criteria:

- Stacked cuboids do not show increased penetration.
- Compound bodies do not receive a multi-contact energy burst.
- Existing chair and variable-friction Tau scenarios do not regress.

## QA Plan

Use the existing dump and trajectory workflow in `physics-qa`.

Primary scenario:

```text
rb_rings_chain.lua
```

It captures all ring rigid bodies for Bullet and Tau. It is high-signal because
the chain uses interlocked compound cuboids without masking failure through
explicit joints.

Required checks:

- At 600 fixed samples, Tau keeps all dynamic ring centers within a defined
  vertical envelope relative to Bullet; no ring enters unbounded freefall.
- The top static ring remains stationary.
- The five-second force reversal does not cause a cache crash or impulse spike.
- Repeated runs are deterministic within documented floating-point tolerance.

Regression scenarios:

- `rb_dynamic_chair_multi_colbox.lua`
- `rb_dynamic_variable_friction.lua`
- `rb_dynamic_variable_restitution.lua`
- `rb_dynamic_variable_rolling_friction.lua`

For each scenario, retain Bullet and Tau JSONL dumps, a trajectory plot where
useful, and a concise comparison of final position, orientation, linear
velocity, and angular velocity.

## Instrumentation

Add Tau-only diagnostics behind a compile-time or environment-controlled
switch. It must report without changing solver behavior:

- active shape pairs;
- manifolds and point counts by feature type;
- warm-start hits and misses;
- normal/tangent impulse totals and clamps;
- discarded stale cache entries;
- maximum penetration before and after solving.

Extend Tau debug draw to show manifold points and normals in a color distinct
from collision-shape wireframes.

## Performance and Safety Limits

- Hard-cap each cuboid pair at four manifold points.
- Hard-cap cache lifetime and total cache entries.
- Use stable ordering for bodies, shape pairs, and points.
- Avoid per-step heap churn where reusable bounded vectors are sufficient.
- Benchmark cuboid QA scenarios before and after every stage.
- Do not enable this path for sphere contacts until cube-cube QA is stable.

## Decision Gates

Proceed from Stage 2 to Stage 3 only after feature and point-identity tests
pass. Proceed from Stage 4 to Stage 5 only after warm-start diagnostics show
stable reuse with no cache leaks.

Stop and reassess Tau as a viable backend if, after Stages 1 through 4,
`rb_rings_chain.lua` still loses the chain or the solver cost exceeds the CPU
budget targeted for Tau. A lightweight backend cannot justify a contact system
that is both less reliable and more expensive than Bullet for its intended
cuboid workload.
