# Tau Physics Pool Performance Analysis And Optimization Roadmap

Date: 2026-09-01

Status: Proposed implementation roadmap based on a local source audit and the
provided Bullet/Tau screenshots. No instrumented CPU profile has been collected
yet; Phase 0 exists to replace the preliminary FPS observation with controlled
measurements.

Scope: Harfang's Tau rigid-body backend under
`harfang/engine/scene_tau_physics.cpp`, using
`tutorials/physics_pool_of_objects.lua` as the primary stress workload.

## Executive Summary

The supplied screenshots show approximately 12 FPS for Bullet and 3 FPS for
Tau with 1,505 displayed dynamic objects. This is an indicative fourfold
end-to-end gap in Tau's disfavor, but it is not yet a precise physics-only
benchmark: the tutorial enables VSync and MSAA, renders about 1,500 objects,
spawns objects per rendered frame, and allows up to four physics substeps per
frame.

The source audit nevertheless identifies several certain scalability problems
that are sufficient to explain a large gap:

1. Tau has no effective sleeping or island deactivation. The public flag is
   stored, but every dynamic body is integrated and solved on every substep,
   and `NodeWake` is empty.
2. Tau's broad phase is a nested loop over every body pair. The displayed
   1,505 dynamic bodies plus the five static container bodies produce
   1,139,295 body-pair loop iterations per substep before narrow phase.
3. Cuboid manifold lookup is a linear scan through a vector capped at 4,096
   entries. Dense contact sets can therefore add another near-quadratic cost
   and can churn when the cap is reached.
4. Sphere-related contacts are transient and receive no persistent warm start.
   This increases solver work and makes reliable sleeping harder.
5. Tau rebuilds all body/shape proxies on every substep. In this tutorial,
   every one-shape body creates its own temporary shape vector allocation.
   Previous-frame world shapes are also computed and stored although they are
   not consumed by the current contact path.
6. The solver repeatedly rebuilds world OBBs, normalizes OBB axes, and
   recomputes world inverse inertia inside contact iteration loops.
7. Tau's fixed-step behavior differs from Bullet when a frame is shorter than
   one fixed step or exceeds the substep cap. At 3 FPS with a cap of four, Tau
   can use substeps near 83 ms instead of four fixed 16.67 ms steps. This is a
   simulation-stability issue and makes the screenshots represent different
   simulation histories.

The recommended path is therefore algorithmic first:

- establish a deterministic physics-only benchmark and phase timings;
- align fixed-step behavior with Bullet;
- eliminate per-substep allocation and dead proxy work;
- replace all-pairs broad phase with a reusable dynamic AABB tree;
- make contact persistence O(1) and cover every supported primitive pair;
- implement wake/sleep state and contact islands;
- then optimize solver data access, scene synchronization, and parallelism.

Reducing solver iterations before those stages is not recommended. It may
improve the FPS counter while degrading stacks, friction, the cuboid chain,
and collision compatibility.

Single-threaded parity with Bullet is a reasonable target after the broad
phase, cache, and sleeping work. Beating Bullet on this particular mixed
primitive pool is plausible because Tau can use compact specialized paths,
but it must be demonstrated over active and settled windows rather than a
single favorable frame.

## Workload Characterization

`physics_pool_of_objects.lua` creates:

- one static floor and four static walls;
- dynamic bodies added in batches of seven;
- an approximately even random mix of one-shape cubes and spheres;
- one renderable model per body;
- a 60 Hz requested physics step with a maximum of four substeps;
- a 1280x720 forward-rendered window with VSync and 4x MSAA.

At the captured count, the Tau physics world contains approximately 1,510
bodies. The current nested broad-phase loops therefore execute:

```text
1,510 * 1,509 / 2 = 1,139,295 body-pair iterations per substep
```

At a sustained 60 physics steps per second, that would be about 68.4 million
body-pair iterations per second. When a slow rendered frame requests the
maximum of four substeps, the loop executes about 4.56 million iterations in
that frame before shape-pair and narrow-phase work.

The workload has two materially different regimes:

1. **Active fill:** newly spawned bodies fall, collide, and rearrange most of
   the pile. Broad phase, narrow phase, and the solver all matter.
2. **Settled pool:** only the upper layer should remain active. Sleeping and
   incremental broad-phase updates should dominate performance.

Both regimes must be measured. Optimizing only the settled pool can conceal a
regression during spawning; optimizing only active motion leaves the largest
Bullet advantage unused.

## Why The Screenshot FPS Is Preliminary Evidence

The screenshots are useful as a user-visible symptom and establish that the
current gap is large. They should not be used as the final acceptance metric
for these reasons:

- VSync quantizes visible FPS into coarse divisors of the display refresh
  rate. The values 12 and 3 do not expose the underlying frame-time
  distribution.
- Rendering, scene traversal, transform synchronization, Lua, and physics are
  all included in the counter.
- Object creation is tied to rendered frames, not to a deterministic physics
  timeline.
- The two backends do not currently handle the substep cap in the same way.
- The screenshots show different pile configurations and therefore potentially
  different numbers and types of active contacts.

The user-visible tutorial remains an important end-to-end gate, but the main
optimization metric must be milliseconds per fixed physics substep.

## Current Bullet And Tau Pipelines

### Bullet

The current Harfang Bullet configuration is single-threaded despite accepting
a thread-count constructor argument. It uses:

- `btDbvtBroadphase`, a dynamic bounding-volume tree;
- Bullet's persistent collision algorithms and manifold cache;
- Bullet activation and sleeping;
- a sequential impulse solver;
- Bullet's fixed-step accumulator and capped fixed substeps.

The initial comparison is therefore not "mature multithreaded Bullet versus
single-threaded Tau." Tau is primarily losing on algorithms, contact lifetime,
and data handling.

### Tau

Every Tau substep currently performs the following sequence:

1. Traverse every node and integrate every dynamic body.
2. Rebuild all body and world-shape proxies.
3. Test all body pairs, then overlapping shape pairs.
4. Generate contacts and update the cuboid-only persistent manifold vector.
5. Run three position iterations.
6. Warm start persistent cuboid contacts and run eight velocity iterations.
7. Apply rolling friction and copy solved impulses back to cuboid manifolds.
8. Traverse contacts for collision tracking even when no tracking mode is
   active.
9. Traverse every dynamic body again to update motion state.
10. Later traverse the node map again to publish every dynamic transform to
    the scene.

There is currently no active-body filtering anywhere in this sequence.

## Source-Audit Findings

| Priority | Finding | Evidence in the current implementation | Likely effect |
|---|---|---|---|
| P0 | No sleeping or islands | `TauNode::deactivation_enabled` is not used by stepping; `SceneTauPhysics::NodeWake` is empty | Settled bodies continue integration, collision generation, solving, and scene sync forever |
| P0 | Quadratic broad phase | `BuildTauContacts` uses nested `i/j` loops over every `TauBodyProxy` | More than 1.13 million body-pair iterations per captured substep |
| P0 | Non-equivalent stepping | Tau uses `ceil(dt / step)`, clamps the count, then sets `substep_dt = dt / count` | Large and variable substeps under load, divergent simulation history, difficult comparison |
| P1 | Linear manifold lookup | `UpdateTauManifoldCache` scans the complete vector; replacement scans it again | Contact-cache cost rises with both active cuboid contacts and retained stale entries |
| P1 | Fixed 4,096-manifold ceiling | New manifolds are refused or replace inactive entries at the hard cap | Cache churn, lost warm starts, unstable performance cliff |
| P1 | Cuboid-only persistence | All non-cuboid/cuboid contacts take the transient path | Cube-sphere and sphere-sphere contacts restart with zero impulse every substep |
| P1 | Per-body temporary allocations | Each rebuilt `TauBodyProxy` owns a newly reserved `std::vector<TauWorldShape>` | Approximately one small heap allocation per physics body per substep in this sample |
| P1 | Dead previous-shape work | `previous_position`, `previous_obb`, and `previous_capsule` are built in proxies but not read by collision generation | Extra transforms, OBBs, bounds-sized data, and cache traffic |
| P2 | Expensive solver refresh | Persistent anchors rebuild world OBBs; world inverse inertia is recomputed repeatedly inside solver iterations | Multiplies transform/matrix cost by contacts and iteration count |
| P2 | Repeated OBB normalization | `GetTauObbAxis` normalizes matrix axes at many SAT, clipping, and anchor call sites | Redundant square roots and vector work in cuboid-heavy contact sets |
| P2 | Cache-unfriendly ownership | Bodies live in `std::map<NodeRef, TauNode>` and hot contact data stores multiple pointers and refs | Pointer chasing and poor spatial locality in all hot loops |
| P2 | Unconditional contact-tracking scan | `CollectTauTrackedContacts` examines all constraints and performs map lookups even when tracking is empty | Avoidable work in the tutorial; smaller than the broad-phase and solver costs |
| P3 | Thread count ignored | `SceneTauPhysics(int thread_count)` discards the value | No path to parallel island or narrow-phase work after single-thread optimization |

These are confirmed structural findings, not profiler attribution. Their
relative wall-time percentages must be measured before deciding how much
engineering effort to spend inside any one narrow-phase primitive.

## Phase 0: Build A Trustworthy Baseline

### 0.1 Add phase timing

Use Harfang's existing `ProfilerPerfSection` infrastructure with constant-name
sections around:

- `Tau.StepSimulation`;
- integration;
- proxy update;
- broad phase;
- narrow phase and manifold generation;
- manifold lookup/update;
- position solve;
- velocity solve;
- rolling friction;
- contact-event collection;
- motion-state update;
- `SyncTransformsFromScene` and `SyncTransformsToScene`.

Add low-overhead counters, reported outside the measured interval:

- total, dynamic, awake, and sleeping bodies;
- created, moved, and queried broad-phase proxies;
- theoretical pairs, broad-phase candidates, and AABB rejects;
- narrow-phase calls by shape combination;
- manifolds, points, cache hits, misses, evictions, and capacity;
- position and velocity constraint evaluations;
- scratch-vector capacities and reallocations;
- dirty scene transforms written.

`HG_TAU_CONTACT_DIAGNOSTICS` already exposes useful contact values, but it does
not provide phase timings, broad-phase counts, allocation behavior, or active
body counts. Logging must remain disabled during timed samples.

### 0.2 Add a deterministic benchmark mode

Keep `physics_pool_stress.lua` as an end-to-end smoke test, but add a benchmark
mode or a dedicated `physics_pool_benchmark.lua` with these properties:

- choose Bullet or Tau without editing the workload;
- generate the initial bodies from a fixed seed or a pre-generated transform
  list;
- create all bodies before the timed interval;
- disable VSync, MSAA, text, and rendering for the physics-only pass;
- call exactly one fixed 1/60 s substep per measured sample;
- measure active-drop, settling, and settled windows separately;
- run cube-only, sphere-only, and 50/50 mixed variants;
- run at 250, 500, 1,000, 1,500, and 2,000 dynamic bodies;
- emit machine-readable JSON or CSV with median, p95, counters, build type,
  CPU, and backend revision.

Run a second end-to-end mode with normal rendering, VSync disabled, and the
same pre-created physics state. Also run a no-physics control to establish the
render/scene floor.

Use an optimized build on a fixed machine and power profile. Discard warm-up
samples and execute at least five repetitions per backend/configuration.

### 0.3 Align fixed-step semantics

Implement a Tau accumulator equivalent to the behavior expected by
`SceneUpdateSystems` and Bullet:

- accumulate frame time;
- execute zero or more fixed `step` substeps;
- never stretch a fixed substep to consume an overloaded frame;
- cap executed substeps at `max_step` with an explicitly documented backlog
  policy;
- keep the remainder for transform interpolation;
- invoke the pre-tick callback once per executed fixed substep.

This change must have dedicated tests for `dt < step`, exact multiples,
remainders, and `dt` above the cap. Capture performance before and after it so
the semantic correction is not mistaken for an algorithmic speedup.

### Phase 0 gate

- Bullet and Tau consume byte-identical initial transforms.
- Both execute the same number and duration of fixed substeps in the measured
  interval.
- Physics-only timings, end-to-end timings, and counters are reproducible
  within a documented tolerance.
- The original screenshots remain attached to the benchmark report as
  preliminary evidence, not as the numeric baseline.

## Phase 1: Remove Allocation And Redundant Per-Step Work

This is a low-risk preparation phase that also makes later algorithms easier
to profile.

1. Store body proxies persistently in a dense slot array. Keep a `NodeRef` to
   slot lookup for public API calls instead of traversing a `std::map` in hot
   loops.
2. Store world shapes in one flat persistent array with body span offsets.
   A one-shape body must not allocate a private vector every substep.
3. Reuse broad-phase candidate, contact, and solver scratch vectors. Reserve
   from the previous measured high-water mark.
4. Update world shapes and AABBs only for newly created, teleported,
   kinematic, or awake moving bodies.
5. Remove previous-world proxy fields until a continuous-collision path
   actually consumes them. Body-level previous state used for display
   interpolation remains separate.
6. Cache normalized world axes and half-extents once per updated cuboid proxy.
7. Cache world inverse inertia once after integration/position correction, not
   at every constraint-mass or impulse call.
8. Return immediately from collision-event collection when no node is being
   tracked.

### Phase 1 gate

- A steady substep with no body creation performs no body/shape/contact vector
  reallocation after warm-up.
- Proxy AABBs and world shapes match the current builder in randomized tests.
- Removing previous-shape data does not change any contact output.
- All Tau unit tests and physics QA state envelopes remain unchanged.

## Phase 2: Replace All-Pairs Broad Phase

### Recommended shared component

Add a generic dynamic AABB tree beside the existing static
`foundation/bvh` implementation, for example:

```text
foundation/dynamic_aabb_tree.h
foundation/dynamic_aabb_tree.cpp
```

The type must have no dependency on Tau, scene nodes, rendering, or assetc. It
should store opaque integer handles and support:

- insert, remove, and update;
- fat AABBs with configurable displacement margin;
- AABB query and self-overlap pair generation;
- moved-proxy tracking;
- validation and statistics;
- deterministic tie-breaking where tree costs are equal.

This keeps the acceleration structure reusable for scene queries, editor
selection, visibility experiments, and other Harfang systems. The existing
serialized static `BVH` remains the right type for cooked mesh triangles; a
dynamic physics broad phase should not rebuild or mutate that format every
substep.

### Tau integration

1. Give each physics body one persistent broad-phase proxy initially. Preserve
   the existing shape-level AABB test after a body pair is emitted.
2. Partition static and dynamic handling so static/static pairs are never
   generated.
3. Update only moved proxies; retain sleeping proxies without tree updates.
4. Query moved dynamic proxies and cache ordered overlap pairs.
5. Sort emitted pairs by stable body/shape IDs before narrow phase and solving.
   Hash/tree iteration order must not make the solver nondeterministic.
6. In debug/test builds, retain a brute-force oracle that compares the tree's
   candidate set against all-pairs results on randomized scenes.

An incremental sweep-and-prune implementation is a valid A/B alternative if
profiling shows the dynamic tree itself dominates this highly coherent pool.
It should use the same opaque broad-phase interface so Tau is not coupled to a
one-off implementation.

### Phase 2 gate

- No overlapping pair found by the brute-force oracle is missed.
- Candidate ordering is deterministic across repeated runs.
- The spread-out 2,000-body benchmark no longer shows quadratic candidate
  growth.
- Body creation, destruction, teleportation, scale change, and kinematic
  movement correctly insert/update/remove proxies.
- Broad phase is below 20% of Tau physics time in the 1,500-body mixed pool;
  otherwise profile and compare the sweep-and-prune alternative.

## Phase 3: Make Contact Persistence Scalable And Shape-Neutral

Replace the linear cuboid manifold vector lookup with stable storage plus an
O(1)-average index.

### Proposed model

- Key the primary lookup by ordered body refs and shape indices.
- Keep the current contact feature in the stored value and invalidate reused
  impulses when the normal or feature becomes incompatible.
- Store manifolds in stable slots with generations or another scheme that
  keeps constraint references valid through a substep.
- Use a free list and epoch/last-seen value for stale removal.
- Reserve from observed demand instead of enforcing a global 4,096-contact
  correctness ceiling. A high safety limit may remain for corrupt/unbounded
  input, but reaching it must be visible in diagnostics.
- Preserve deterministic solver order in a vector; do not solve directly in
  hash-table iteration order.

Extend persistent single-point manifolds and warm starting to:

- sphere-sphere;
- sphere-cuboid;
- capsule-sphere;
- capsule-cuboid;
- capsule-capsule.

The pool directly benefits from the first two combinations. The capsule paths
must share the infrastructure so future mixed scenes do not reintroduce the
same transient-contact problem.

### Phase 3 gate

- Cache lookup time scales with active contacts rather than active contacts
  multiplied by retained manifolds.
- A stable shape pair owns one cache entry even when its selected SAT feature
  changes; incompatible impulses are discarded safely.
- Stable resting contacts show a high warm-start hit rate after the first
  frame, reported separately by shape combination.
- Contact count above 4,096 does not silently disable persistence or invalidate
  constraints.
- Node destruction and garbage collection leave no stale body pointers or
  cache entries.

## Phase 4: Implement Sleeping And Contact Islands

Sleeping is expected to provide the largest settled-pool gain, but it must be
built on reliable contacts and wake propagation.

### Body state

Add an explicit state such as `Awake`, `SleepCandidate`, and `Sleeping`, plus a
sleep timer. A body may become a candidate only when:

- deactivation is enabled;
- linear and angular speeds remain below configured thresholds;
- accumulated forces/torques are negligible;
- contact penetration and relative contact speeds are stable;
- the condition persists for a minimum simulated duration.

Sleeping bodies keep their broad-phase proxy and cached manifolds but skip
integration, constraint generation for sleeping/sleeping pairs, solver work,
motion writes, and unchanged scene-transform publication.

### Islands and wake propagation

Build dynamic-body islands from current persistent contacts and constraints.
Sleep or wake an island as a unit. Wake propagation is required when:

- force, torque, impulse, velocity, reset, or teleport APIs modify a body;
- an awake body contacts a sleeping body;
- a supporting kinematic/static transform changes;
- `NodeWake` is called;
- deactivation is disabled.

Collision-event tracking must keep its existing semantics. If an event-tracked
sleeping pair is no longer regenerated by the solver, emit its cached contact
or keep that pair in the event path.

### Phase 4 gate

- More than 90% of bodies in the deterministic settled 1,500-body pool sleep
  after the agreed settling window, unless instrumentation identifies genuine
  ongoing motion.
- Applying an impulse to any sleeping stack wakes all bodies that need to
  respond during the same fixed step.
- `NodeSetDeactivation(false)` keeps the body active; `NodeWake` is functional.
- Sleeping never suppresses required collision events or leaves scene
  transforms stale.
- The active-fill benchmark remains physically stable and does not repeatedly
  sleep/wake entire piles due threshold chatter.

## Phase 5: Optimize The Solver And Scene Synchronization

Only after Phases 0-4 should iteration count be reconsidered.

### Solver hot data

1. Store persistent anchors in body-local space so refreshing an anchor needs
   the body transform, not a rebuilt shape OBB.
2. Precompute contact arms, normal effective mass, tangent basis/effective
   mass, friction, and restitution inputs at the correct point in the substep.
3. Split hot constraint fields from diagnostic/cold identity fields or use a
   compact structure-of-arrays layout if profiling supports it.
4. Process contacts per island and skip static/sleeping work early.
5. Add zero-friction, zero-restitution, and zero-rolling-friction fast paths.
6. Avoid normalizing already normalized cached cuboid axes.
7. Re-profile three position and eight velocity iterations. Reduce iterations
   only if persistent warm starting gives equal or better QA envelopes.

### Scene synchronization

Maintain dense lists by body type and a dirty/changed bit. Avoid scanning all
nodes in `SyncTransformsFromScene` and avoid publishing an unchanged sleeping
body transform in `SyncTransformsToScene` when scene interpolation semantics
permit it.

If physics-only Tau reaches parity but the rendered tutorial does not, measure
scene traversal, model submission, Lua, and rendering separately. Shared
render/scene costs should be optimized as shared engine costs, not hidden by
weakening Tau physics.

### Phase 5 gate

- Solver output stays inside the existing Bullet/Tau QA envelopes.
- No iteration reduction is accepted solely from the pool FPS result.
- The profiler shows no repeated world-OBB construction or inverse-inertia
  matrix construction inside the velocity iteration loop.
- The number of scene transform writes tracks awake/changed bodies in the
  settled benchmark.

## Phase 6: Parallelism And SIMD Stretch Work

After the single-thread algorithm and data layout are competitive:

- honor `SceneTauPhysics(thread_count)`;
- solve independent islands in parallel;
- batch broad-phase queries and common narrow-phase combinations;
- evaluate SIMD-friendly sphere-sphere and cuboid SAT batches;
- use per-worker scratch buffers and merge candidate/contact vectors in stable
  order;
- retain a deterministic single-thread mode for QA and debugging.

Parallelizing the current all-pairs and linear-cache implementation is not a
substitute for Phases 1-5. It would consume cores while retaining the wrong
scaling behavior.

### Phase 6 gate

- One thread is no slower than the Phase 5 baseline outside normal noise.
- Multi-thread speedup is reported at 1,000, 1,500, and 2,000 active bodies.
- Repeated deterministic-mode captures remain identical within the existing
  floating-point policy.
- Thread count zero/one and body destruction during normal scene updates are
  race-free.

## Performance Acceptance Targets

Use physics milliseconds per fixed substep as the primary metric. FPS remains
an end-to-end secondary metric.

| Target | Required result |
|---|---|
| Baseline recovery | Reproduce and explain the approximate 12 FPS Bullet / 3 FPS Tau symptom with phase timing and a no-physics control |
| Algorithmic gate | Broad-phase candidates and contact-cache lookup cease quadratic growth on spread and dense scaling tests |
| Settled parity | Tau 1,500-body mixed settled p95 physics time is no more than 1.10x Bullet |
| Active parity | Tau 1,500-body mixed active-drop p95 physics time is no more than 1.10x Bullet, or every remaining delta has an attributed phase and accepted follow-up |
| End-to-end parity | With VSync disabled and identical state, Tau tutorial frame time is no more than 1.10x Bullet and produces a comparable stable pile |
| Stretch: beat Bullet | Across the weighted active/settled suite, Tau physics median is at least 15% lower than Bullet, no measured workload is more than 10% slower, and all correctness gates pass |

These are decision gates, not promised gains from the source audit. If the
render-only control already consumes most of Bullet's frame budget, the
end-to-end FPS target must be interpreted through total frame milliseconds,
not a VSync plateau.

## No-Regression Matrix

### Unit tests

- `harfang/tests/engine/scene_tau_physics.cpp`;
- `harfang/tests/engine/scene_tau_physics_contact.cpp`;
- `harfang/tests/foundation/bvh.cpp`;
- new dynamic AABB tree randomized oracle tests;
- new fixed-step accumulator tests;
- new activation, island wake propagation, and cache-lifetime tests.

### Physics QA

At minimum, capture and compare:

- `rb_rings_chain.lua`;
- `rb_dynamic_chair_multi_colbox.lua`;
- `rb_dynamic_variable_friction.lua`;
- `rb_dynamic_variable_restitution.lua`;
- `rb_dynamic_variable_rolling_friction.lua`;
- `rb_dynamic_impulse_callback.lua`;
- `rb_capsule_collision_pairs.lua`;
- `!rb_dynamic_collision_events.lua`.

Raycast and mesh-collider tests should also remain green because the shared
static BVH and dynamic broad phase must not be confused or coupled:

- `rb_raycast_various_collshapes.lua`;
- `rb_mesh_collider_raycast.lua`;
- `rb_mesh_collider_raycast_mesh_terrain_rotating.lua`.

### Behavioral invariants

- no broad-phase false negatives;
- stable normal orientation and finite impulses;
- no increase in maximum penetration outside the accepted envelope;
- deterministic ordered pair/contact generation;
- correct wake behavior for force, torque, impulse, teleport, reset, and
  kinematic motion;
- collision-event continuity for sleeping contacts;
- no stale cache/tree handle after destroy or garbage collection;
- fixed callback count and fixed callback `dt`;
- no change to public Harfang Lua/C++ physics APIs.

### Performance regression policy

Store benchmark results per milestone. A change is not accepted if it improves
the settled 1,500-body pool while regressing any cube-only, sphere-only,
active-drop, or lower-count workload by more than 10% without an understood
and explicitly accepted tradeoff.

## Recommended Change Sequence

Keep each stage separately measurable and revertible:

1. Benchmark harness, counters, profiler scopes, and fixed-step tests.
2. Fixed-step accumulator parity as a semantic change on its own.
3. Persistent dense proxy storage, scratch reuse, and dead-work removal.
4. Generic dynamic AABB tree plus Tau broad-phase integration and oracle.
5. O(1) shape-pair manifold index and persistent primitive contacts.
6. Body activation, islands, wake propagation, and dirty transform sync.
7. Solver hot-data/cache optimization.
8. Profile-guided iteration tuning, only if QA permits.
9. Island parallelism and SIMD as the stretch phase.

Profile and archive the benchmark report after every item. Do not bundle broad
phase, sleeping, manifold changes, and solver tuning into one patch; otherwise
performance gains and behavior regressions will be difficult to attribute.

## Decision Checkpoints

### After dynamic broad phase

If broad phase still exceeds 20% of physics time, compare tree balancing,
fat-AABB margins, and incremental sweep-and-prune. If it is already small,
move immediately to contact cache and sleeping.

### After contact persistence and sleeping

If the settled pool is competitive but active fill is not, the next likely
targets are narrow phase, manifold construction, and the solver hot loop. If
both remain slow, use recorded phase percentages rather than assuming SAT is
the bottleneck.

### After physics-only parity

If the rendered tutorial remains slower, use the no-physics control and scene
sync counters. Optimize dirty transform publication and shared scene/render
work before changing physical behavior.

### Before claiming that Tau beats Bullet

Require the weighted benchmark suite, both median and p95, all supported
primitive mixes, and the no-regression matrix. A win limited to a fully
sleeping pool or obtained through fewer solver iterations is not sufficient.

## Expected Outcome

The current fourfold visible gap is not one narrow-phase bug. It is the
combined result of an O(N^2) candidate scan, permanently awake bodies, a
linearly searched manifold cache, transient sphere contacts, allocation-heavy
proxy reconstruction, repeated solver math, and non-equivalent overload
stepping.

The first credible route back to Bullet-level frame rate is:

```text
fixed benchmark and timestep
    -> allocation-free persistent proxies
    -> dynamic broad phase
    -> O(1) persistent contacts for all primitives
    -> sleeping/contact islands
    -> compact solver and dirty scene sync
```

Tau has a plausible path to outperform Bullet in this workload after that
foundation is in place: it can specialize for Harfang's common primitive
combinations, retain compact contiguous data, skip inactive islands, and later
parallelize them. Until Phase 0 timings exist, however, the responsible claim
is a roadmap to measured parity and a stretch target beyond Bullet, not a
guaranteed FPS number.
