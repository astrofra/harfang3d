# Tau Physics Pool Performance Analysis And Optimization Roadmap

Date: 2026-09-03

Status: the deterministic benchmark and fixed-step correction are in place;
the hashed manifold lookup, Phase 2 dynamic broad phase, Phase 4 sleeping and
contact-island activation, sleeping proxy/cuboid-manifold reuse, and persistent
per-world step scratch, shape-neutral contact persistence, solver
pose/inertia/cuboid-axis caches, and prepared active velocity constraints are
implemented and measured. Active face-feature cuboid manifold coherence is
also implemented with conservative validation and periodic full refresh.
Zero-restitution, non-positive-friction, and world-wide zero-rolling-friction
solver fast paths are implemented and measured. A guarded all-sleeping fast
path for worlds of at most 512 bodies is also implemented and measured. The
complete physics-only body-count/shape matrix and controlled scene/render
passes have been rerun against all accumulated optimizations. Tau beats Bullet
on the representative mixed active, settled, and end-to-end gates. A first,
behavior-preserving dense body-storage slice and a solver-hot/body-cold data
split are implemented and measured. The split improves both the focused
active-cube median and p95 while preserving byte-identical QA trajectories.
The complete matrix is the next acceptance step. Solver iteration tuning
remains deferred.

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

The baseline source audit nevertheless identified several certain scalability
problems that were sufficient to explain a large gap:

1. Tau has no effective sleeping or island deactivation. The public flag is
   stored, but every dynamic body is integrated and solved on every substep,
   and `NodeWake` is empty.
2. At the audited baseline, Tau's broad phase was a nested loop over every body
   pair. The displayed 1,505 dynamic bodies plus the five static container bodies produce
   1,139,295 body-pair loop iterations per substep before narrow phase.
3. At the audited baseline, cuboid manifold lookup was a linear scan through a
   vector capped at 4,096 entries. Dense contact sets can therefore add another near-quadratic cost
   and can churn when the cap is reached.
4. Sphere-related contacts are transient and receive no persistent warm start.
   This increases solver work and makes reliable sleeping harder.
5. Tau rebuilds all body/shape proxies on every substep. In this tutorial,
   every one-shape body creates its own temporary shape vector allocation.
   Previous-frame world shapes are also computed and stored although they are
   not consumed by the current contact path.
6. The solver repeatedly rebuilds world OBBs, normalizes OBB axes, and
   recomputes world inverse inertia inside contact iteration loops.
7. At the audited baseline, Tau's fixed-step behavior differed from Bullet when
   a frame was shorter than one fixed step or exceeded the substep cap. At 3 FPS with a cap of four, Tau
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

## Phase 0 Implementation And Initial Baseline

The headless benchmark is `tutorials/physics_pool_benchmark.lua`. It creates
the same deterministic body sequence for both backends, excludes body creation
from samples, advances one exact 1/60 s substep per sample, and emits JSONL.
Its environment variables select the backend, body count, shape mix, measured
phase, warm-up and settling lengths, repetitions, execution mode, and output
file. `HG_TAU_PROFILE=1` enables aggregate phase attribution; profiled samples
are marked and must not be compared directly with unprofiled backend samples.

Both backend packages were rebuilt from the same source tree before the final
comparison. The first controlled run used a Release build, 1,500 dynamic
bodies plus five static container bodies, the mixed cube/sphere sequence, 60 measured steps,
10 warm-up steps, 300 pre-settling steps for the second window, three
repetitions, and seed 5,521,749. Values below are medians across the three
per-repetition medians; p95 is treated the same way.

| Backend | Window | Median step | p95 step | Tau / Bullet median |
|---|---:|---:|---:|---:|
| Bullet | active | 9.971 ms | 12.175 ms | 1.00x |
| Tau | active | 18.762 ms | 23.211 ms | 1.88x |
| Bullet | after 300 settling steps | 14.463 ms | 15.564 ms | 1.00x |
| Tau | after 300 settling steps | 39.229 ms | 39.811 ms | 2.71x |

The second window is deliberately described as "after 300 settling steps",
not fully settled. Tau does not yet sleep bodies and contact density was still
increasing; the label prevents this preliminary window from overstating an
equilibrium result.

An attribution-only Tau run measured these average costs per substep:

| Tau phase | Active | After 300 settling steps |
|---|---:|---:|
| `Tau.StepSimulation` | 19.8 ms | 38.5 ms |
| `Tau.VelocitySolve` | 9.37 ms | 21.3 ms |
| `Tau.BuildContacts` | 7.46 ms | 11.3 ms |
| `Tau.BroadPhase` | 4.25 ms | 5.39 ms |
| `Tau.NarrowPhase` | 2.60 ms | 5.52 ms |
| `Tau.PositionSolve` | 2.46 ms | 5.36 ms |
| `Tau.ProxyUpdate` | 0.408 ms | 0.317 ms |

Diagnostics at 1,500 dynamic bodies confirm the scaling mechanism. Every Tau
substep tests 1,131,760 theoretical body pairs. At step 240, 4,009 AABB
candidates produce 1,721 constraints and 2,502 contact points. At step 540,
7,147 candidates produce 4,298 constraints and 6,309 points; all 1,500 dynamic
bodies are still integrated and published. The cuboid manifold lookup performs
about 302,000 linear comparisons at step 240 and 1,229,512 at step 540.

These results refine the first implementation order:

1. make manifold lookup O(1) and retain/reuse all scratch capacity;
2. replace the all-pairs scan with the reusable dynamic AABB tree;
3. add wake/sleep state and islands before tuning solver iteration counts;
4. then reduce repeated solver transforms and inertia/axis work.

The velocity solver is currently the largest measured phase, but its work is
driven by a contact set that keeps growing because every body stays active.
Lowering solver iterations now would trade correctness for a temporary FPS
gain and is therefore still outside the corrective path.

### First optimization result: hashed cuboid manifold index

The cuboid manifold vector now has a persistent hashed index. Lookup is
expected O(1), stale and node-related removals use indexed swap removal, and
the vector remains the authoritative compact storage so solver constraint
indices stay inexpensive. A dedicated lifecycle test covers pruning, key
reuse, node removal, and node recreation.

The same unprofiled three-repetition run produced:

| Window | Tau before | Tau with hashed cache | Improvement | New Tau / Bullet |
|---|---:|---:|---:|---:|
| active | 18.762 ms | 18.450 ms | 1.7% | 1.85x |
| after 300 settling steps | 39.229 ms | 37.145 ms | 5.3% | 2.57x |

At diagnostic steps 240 and 540, contact counts, manifold counts, accumulated
impulses, friction clamps, and pre/post-solve penetration values are identical
before and after the change. Cache key comparisons fell from 302,014 to 553
at step 240 and from 1,229,512 to 1,461 at step 540. This removes the confirmed
cache pathology without changing the simulated trajectory in the controlled
run.

### Second optimization result: shared foundation BVH broad phase

Tau now builds Harfang's reusable `foundation/bvh` over body AABBs each
substep and queries it for overlaps. This is runtime engine infrastructure, not
an `assetc`-specific acceleration structure. Candidate pairs are sorted back
into the previous `(body_a, body_b)` order before narrow phase so the sequential
solver sees the same constraint sequence. The former all-pairs scan remains a
correctness fallback if BVH construction rejects invalid bounds.

The same unprofiled three-repetition run produced:

| Window | Tau with hashed cache | Tau with cache + BVH | Additional improvement | Total improvement | New Tau / Bullet |
|---|---:|---:|---:|---:|---:|
| active | 18.450 ms | 16.288 ms | 11.7% | 13.2% | 1.63x |
| after 300 settling steps | 37.145 ms | 34.157 ms | 8.0% | 12.9% | 2.36x |

The attribution-only broad-phase average fell from 4.35 to 2.29 ms in the
active window and from 5.36 to 2.29 ms after 300 settling steps. Diagnostics
remain identical through step 540, including the 7,147 candidates, 4,298
constraints, 6,309 contact points, accumulated impulses, friction clamps, and
penetration values. The ordered BVH candidate stream therefore preserves the
controlled trajectory while eliminating most theoretical pair visits.

This first BVH integration rebuilt and validated a static hierarchy every
substep. It established a useful, low-risk speedup and a measured upper bound
of about 2.29 ms per step for that implementation.

### Third optimization result: persistent dynamic AABB tree

A generic runtime `DynamicAABBTree` now lives in `foundation`, independently
of Tau, scenes, rendering, cooked assets, and `assetc`. It provides
generation-checked opaque proxies, insertion/removal/incremental update, fat
AABBs, AABB and self-overlap queries, moved-proxy tracking, validation, and
tree statistics. Randomized tests compare both query modes against brute-force
fat-AABB oracles and cover proxy removal, reuse, and stale generations.

Tau assigns a persistent proxy to each physics body. Only proxies that leave
their fat AABB are reinserted. Cached fat-overlap adjacency is invalidated and
rebuilt only for moved proxies, then exact body AABBs filter the cached pairs.
The resulting `(body_a, body_b)` stream is sorted before narrow phase, retaining
the previous deterministic solver order. Creation, destruction, garbage
collection, reset, teleport, scale refresh, and static/kinematic scene sync all
maintain proxy lifecycle. Invalid tree state still selects the all-pairs
correctness fallback.

The final unprofiled three-repetition run produced:

| Window | Tau with rebuilt BVH | Tau with dynamic tree | Additional improvement | Total vs original Tau | Tau / Bullet |
|---|---:|---:|---:|---:|---:|
| active | 16.288 ms | 16.045 ms | 1.5% | 14.5% | 1.61x |
| after 300 settling steps | 34.157 ms | 32.578 ms | 4.6% | 17.0% | 2.25x |

The final p95 values are 19.785 ms active and 32.935 ms after 300 settling
steps. Attribution puts `Tau.BroadPhase` at 1.67 ms active and 0.97 ms after
300 settling steps, respectively about 10% and 3% of Tau step time. The
velocity solver now dominates at 9.21 ms and 21.0 ms.

The contact diagnostics oracle reported zero missing and zero extra exact
pairs through step 540 of the 1,500-body pool. At step 540 only nine proxies
were reinserted; 9,731 cached fat pairs filtered to the same 7,147 exact body
candidates, 4,298 constraints, and 6,309 manifold points as the previous
implementations.

`BENCH_LAYOUT=spread` adds a deterministic non-overlapping layout to the
benchmark. With 2,000 dynamic bodies and the normal staggered spawn history,
the unprofiled median is 13.768 ms for Tau versus 2.916 ms for Bullet. A Tau
profile attributes only 0.247 ms of its 14.1 ms average step to broad phase,
while velocity solving costs 7.69 ms. In a batched 2,000-body oracle run, Tau
examines the tree's eight conservative fat pairs, emits zero exact candidates
instead of scanning 2,009,010 theoretical pairs, and reports `oracle=0/0`.
This closes the quadratic broad-phase issue and makes the remaining spread
gap strong evidence for prioritizing sleep/island deactivation.

The next implementation order after sleeping/islands was:

1. avoid rebuilding sleeping body shapes and regenerating unchanged
   sleeping/sleeping narrow-phase contacts while preserving event semantics;
2. reuse proxy, candidate, contact, and island scratch storage and remove dead
   proxy fields/work;
3. complete shape-neutral contact persistence, then reduce repeated solver
   transforms, inverse-inertia work, and axis normalization.

### Fourth optimization result: sleeping and bounded activation cohorts

Tau now has explicit `Awake`, `SleepCandidate`, and `Sleeping` states. A body
accumulates two simulated seconds below the linear, angular, force, contact
speed, and penetration thresholds. Small intermittent solver noise decays the
timer instead of erasing it; motion or contact four times over the threshold
resets it. This hysteresis was necessary because a strict reset allowed a few
changing contacts at the top of the pool to keep almost every body active
indefinitely.

Each substep builds a deterministic dynamic-body contact/constraint graph.
Sleep-ready bodies form activation cohorts capped at 64 bodies. The cap avoids
a local impact waking all 1,500 bodies in the dense pool. API mutations and a
moved static/kinematic support still traverse the persisted contact graph and
wake every connected sleeping body before the mutation is applied; an actual
dynamic impact wakes the bounded cohort it touches. Mixed awake/sleeping
contacts treat the sleeping endpoint as immovable until that wake condition is
met. Tests cover explicit wake propagation, collision-driven wake, disabled
deactivation, moving supports, and tracked-contact continuity.

Dynamic support dependencies are directional: a contact whose normal opposes
gravity records the lower body as a support for the upper body. Entering sleep
captures each dynamic support's position and orientation. A supported sleeper
wakes after 2 cm of cumulative support translation, 2 degrees of rotation, or
two consecutive fixed steps without a snapshotted support contact. This catches
slow drift that an instantaneous velocity threshold misses while tolerating a
single frame of manifold churn. Higher layers follow on subsequent steps. Side
contacts do not propagate this wake, and bounded inline support storage avoids
per-contact heap allocation. Diagnostics split support-pose and support-loss
wakes so remaining sleep/contact failures can be distinguished in captures.

Sleeping bodies keep broad-phase proxies, manifolds, and contact-event
visibility. They skip integration, sleeping/sleeping solver constraints,
motion-state updates, and unchanged scene-transform publication. Narrow phase
is intentionally still regenerated in this first version, which keeps cache
and event semantics simple and exposes the next measured optimization target.
When a body first enters sleep, Tau persists its final world pose into the
scene `Transform` once. This is required because `SetNodeWorldMatrix` only
overrides Harfang's cache for the current frame; without the one-time write,
the next `ReadyWorldMatrices` call would rebuild the node at its creation pose.
Subsequent sleeping frames still avoid transform publication. Awake and sleep
candidate bodies remain published every rendered frame, including frames where
the fixed-step accumulator does not advance physics.

On the deterministic mixed 1,500-body pool, 1,395 bodies (93.0%) are sleeping
at step 600 and 1,448 (96.5%) at step 660. At step 660 only 54 motion states are
updated and 5,728 contact points are skipped by the solver. The broad-phase
oracle still reports zero missing and zero extra exact pairs.

The authoritative Release comparison uses the same three-repetition setup as
the previous milestones. Values are medians across repetition medians (and
across repetition p95 values):

| Backend / milestone | Window | Median step | p95 step | Relative to Bullet |
|---|---:|---:|---:|---:|
| Bullet | active | 9.669 ms | 12.418 ms | 1.00x |
| Tau with dynamic tree | active | 16.045 ms | 19.785 ms | 1.66x |
| Tau with sleeping/islands | active | 16.686 ms | 20.528 ms | 1.73x |
| Bullet | after 300 settling steps | 14.678 ms | 15.423 ms | 1.00x |
| Tau with dynamic tree | after 300 settling steps | 32.578 ms | 32.935 ms | 2.22x |
| Tau with sleeping/islands | after 300 settling steps | 7.384 ms | 8.103 ms | 0.50x |

Sleeping therefore cuts the Tau settled median by 77.3% and makes Tau about
1.99 times faster than Bullet in the mixed settled window. The active Tau
median regresses by 4.0%, inside the 10% milestone policy but still visible;
island scratch reuse belongs in the next pass. Active mixed motion remains
1.73 times slower than Bullet, so this result is not a general parity claim.

The post-sleep profile attributes 4.74 ms of the 7.75 ms settled average to
`BuildContacts`, including 3.58 ms of narrow phase. Velocity solving falls to
1.39 ms and position solving to 0.841 ms. Island construction costs 0.265 ms
and activation update 0.307 ms (0.154 ms and 0.159 ms in the active window).
Caching unchanged sleeping contacts is consequently a higher-value next step
than reducing iteration counts.

The spread 2,000-body check improves from 13.768 ms to 9.462 ms active and
reaches 3.677 ms settled, but Bullet needs only 2.944 ms and 0.605 ms. This
sixfold settled spread gap is direct evidence that Tau still rebuilds proxies
and contacts for sleeping bodies even when every body only rests on the floor.

One-repetition shape spot checks produced:

| Shape mix | Window | Tau | Bullet | Tau / Bullet |
|---|---:|---:|---:|---:|
| cubes | active | 28.780 ms | 9.506 ms | 3.03x |
| cubes | after 300 steps | 14.868 ms | 13.775 ms | 1.08x |
| spheres | active | 8.525 ms | 9.058 ms | 0.94x |
| spheres | after 300 steps | 3.292 ms | 11.411 ms | 0.29x |

All 54 C++ unit-test groups pass. Fresh 600-sample QA captures also pass the
bounded cuboid-chain validator (4.45449 m maximum vertical error, 16.8203 m/s
maximum Tau speed, zero static-ring drift). The impulse-callback capture stays
within 0.0359 m maximum position error and differs from Bullet by only
-0.00142 m peak-to-peak amplitude. A scene-loop regression now drops a cube
from `y=3`, waits for sleep at `y=0.5`, invalidates and rebuilds scene world
matrices, and verifies that the rendered pose remains at the settled height. A
second regression verifies active-pose continuity on a frame too short to
execute a fixed substep.

### Fifth optimization result: sleeping proxy and cuboid-manifold reuse

Each Tau node now owns persistent world-shape and body-bounds storage. Awake
dynamic nodes refresh it every substep, while sleeping dynamics and unchanged
static/kinematic nodes retain it. The dynamic AABB tree is updated only when
that cached proxy is refreshed. Entering sleep explicitly invalidates the
cache once because the position solver can move a body after contact building;
the following substep captures the final sleep pose and steady-state reuse then
begins.

For an unchanged pair with at least one sleeping dynamic body, a cuboid/cuboid
manifold seen on the previous substep is now re-injected from its body-local
anchors without rerunning SAT and contact clipping. It remains present in the
contact graph and tracked-event path. Any awake dynamic endpoint or moved
static/kinematic endpoint forces normal proxy and narrow-phase regeneration.
Tests cover steady sleeping reuse, explicit/API wake, whole-stack wake, moved
kinematic support invalidation, and collision-event continuity.

At diagnostic step 780 of the mixed 1,500-body pool, Tau reuses 1,436 of the
1,500 dynamic proxies and refreshes only 64. It re-injects 1,397 cuboid
manifolds containing 3,330 contact points, reducing narrow-phase calls from a
potential 7,072 overlapping shape pairs to 5,675. The remaining 854 eligible
cuboid cache misses are conservative AABB overlaps without a previous-step
contact; they correctly fall back to narrow phase. Sphere and capsule contacts
remain transient and are deliberately left for shape-neutral persistence.

An attribution run over 120 settled steps measures `Tau.BuildContacts` at
2.40 ms and `Tau.NarrowPhase` at 1.59 ms, down from 4.74 ms and 3.58 ms after
the sleeping/island milestone. `Tau.ProxyUpdate` is 0.048 ms. Position and
velocity solver iteration counts remain unchanged at three and eight.

A same-seed, same-source A/B build with reuse compiled off and on isolates the
feature-level effect:

| Mixed 1,500-body pool | Reuse off | Reuse on | Change |
|---|---:|---:|---:|
| active median | 20.638 ms | 20.798 ms | -0.8% (timer noise band) |
| settled median | 6.634 ms | 4.923 ms | 25.8% faster |

The normal three-seed Release run with reuse enabled has a 5.055 ms median of
per-run medians and a 5.378 ms median p95. Seed-dependent settling still causes
visible dispersion, so the paired A/B above is the regression gate for this
milestone. The 2,000-body mixed spread check reaches 2.460 ms settled versus
3.677 ms before reuse, a 33.1% improvement, but remains well above Bullet's
previously measured 0.605 ms because transient primitive contacts and full
node/contact scratch reconstruction are still paid.

The next implementation order after this milestone was:

1. retain proxy lists, broad-phase candidates, contact constraints, and island
   scratch across substeps, and remove remaining dead proxy fields/work;
2. make contact persistence shape-neutral, starting with sphere/cuboid and
   sphere/sphere floor-heavy paths, then capsule combinations;
3. reduce repeated solver transforms, inverse-inertia work, and cuboid-axis
   normalization, without changing solver iteration counts until QA proves an
   equivalent envelope.

A 65-body cohort-boundary regression now verifies dynamic-support wake: the
lower support occupies the last slot of one 64-body cohort, the upper cuboid
the first slot of the next, and moving only the lower cohort in 3 mm increments
wakes the upper once cumulative displacement invalidates its support snapshot.
The same regression re-settles the pair and verifies a 3-degree support
rotation. A same-machine A/B of the initial directional support tracking on the
1,500-body mixed settled pool measured 6.646 ms median / 7.398 ms p95 without
support tracking and 6.806 ms / 7.333 ms with it (one 120-sample validation
repetition), a 2.4% median cost within the 10% milestone policy. The final
persistent-pose implementation reports 6.632 ms / 7.021 ms under the same
one-repetition setup, so the stronger invalidation adds no measured regression.
Active timings were unchanged within run-to-run noise.
The packaged Release 1,500-body mixed settled scene benchmark reports a
6.632 ms median and 7.021 ms p95 over 120 samples (one validation repetition).

### Sixth optimization result: persistent step scratch and dead proxy work removal

`SceneTauPhysics` now owns a lazily allocated `TauStepScratch` high-water-mark
cache. Body proxies and bounds, exact broad-phase candidates, moved-proxy
lists, contact constraints, both union-find graphs, wake/stability flags,
activation-cohort sets, and assigned island IDs are cleared and reused rather
than constructed and destroyed on every substep. `ClearNodes` releases the
cache together with the world. The implementation also removes unused previous
world sphere/capsule/OBB fields and their transform work. Ready-island
constraint lookup now uses the node map instead of scanning every island body,
with an explicit dynamic-body guard for static constraint endpoints.

`HG_TAU_CONTACT_DIAGNOSTICS=1` now reports scratch growth and the principal
capacities. During the staggered spawn, capacity grows only when the body or
contact high-water mark increases. From step 240 through step 780 of the mixed
1,500-body pool, `scratch(growths=0)` is stable. At step 780 the retained
capacities are 1,505 body proxies, 12,040 candidate pairs, 8,092 contact points,
and 1,505 island bodies; the existing per-push candidate and contact
reallocation counters are also zero. A unit regression warms a two-body stack,
verifies a zero-growth complete substep, and verifies that `ClearNodes` resets
the cache.

The three-seed Release comparison against the immediately preceding sleeping
proxy/manifold milestone is:

| Workload | Previous median / p95 | Persistent scratch median / p95 | Median change |
|---|---:|---:|---:|
| mixed 1,500 active | 20.798 / 26.956 ms | 20.254 / 26.452 ms | 2.6% faster |
| mixed 1,500 settled | 5.055 / 5.378 ms | 4.106 / 4.546 ms | 18.8% faster |
| mixed 2,000 spread settled | 2.460 / 2.739 ms | 2.125 / 2.269 ms | 13.6% faster |

The settled attribution run falls from 4.70 to 4.23 ms average Tau step time.
`Tau.BuildContacts` falls from 2.40 to 2.05 ms, narrow phase including contact
emission from 1.59 to 1.29 ms, island construction from 0.480 to 0.456 ms, and
sleep update from 0.311 to 0.277 ms. The diagnostic contact/manifold/island
counts remain identical at the recorded checkpoints, and the solver remains at
three position and eight velocity iterations.

The next implementation order is now:

1. make primitive contact persistence shape-neutral, starting with
   sphere/cuboid and sphere/sphere, then capsule combinations;
2. cache solver world transforms, inverse inertia, and normalized cuboid axes;
3. complete the rendered and body-count acceptance matrix before considering
   any iteration-count change.

### Seventh optimization result: shape-neutral persistent contacts

All solver contact manifolds now store their two surface anchors in the local
orthonormal frames of the rigid bodies rather than in cuboid-only OBB frames.
The cuboid SAT generator keeps its focused OBB-local contract and its output is
converted at the cache boundary. Sphere/sphere, sphere/cuboid,
capsule/sphere, capsule/cuboid, and capsule/capsule contacts now enter the same
hashed manifold cache, retain normal and tangent impulses, refresh their world
anchors during the solve, and use the sleeping-manifold fast path.

Parallel capsule segments previously selected an arbitrary endpoint from an
infinite set of equally close pairs. The persistent anchor exposed the
resulting artificial torque and cache churn. Parallel segment pairs and
capsule/cuboid side contacts now prefer a central closest point. This is both
more stable and makes the primitive feature identity reusable. The former
4,096-manifold hard ceiling was also changed into an initial reservation: the
cache is allowed to grow with the live scene and remains bounded in time by
the existing three-step stale-manifold pruning. The mixed pool reached 4,150
live cache entries while settling, which demonstrates that retaining the old
ceiling would have silently disabled persistence in the target workload.

The three-repetition Release comparison against the persistent-scratch
milestone is:

| Workload | Persistent scratch median / p95 | Shape-neutral median / p95 | Median change |
|---|---:|---:|---:|
| mixed 1,500 active | 20.254 / 26.452 ms | 17.116 / 22.674 ms | 15.5% faster |
| mixed 1,500 settled | 4.106 / 4.546 ms | 3.497 / 3.631 ms | 14.8% faster |
| mixed 2,000 spread settled | 2.125 / 2.269 ms | 1.549 / 1.661 ms | 27.1% faster |

Shape-specific absolute checks, also using three repetitions, report:

| Workload | Active median / p95 | Settled median / p95 |
|---|---:|---:|
| cube-only pool, 1,500 | 26.888 / 35.307 ms | 4.340 / 4.652 ms |
| sphere-only pool, 1,500 | 10.203 / 13.517 ms | 2.439 / 2.660 ms |

At step 780 of the diagnostic mixed run, 1,475 of 1,500 dynamic proxies are
reused. Tau emits 2,630 primitive manifolds, reuses 3,985 sleeping manifolds
and 5,972 sleeping points, performs 2,874 narrow-phase calls, and records
6,073 warm-start hits versus eight misses. Scratch growth, cache eviction, and
cache overflow are all zero. The profiled 120-step settled run averages
3.68 ms in `Tau.StepSimulation`: 2.07 ms in contact building, 1.34 ms in its
narrow-phase scope, 0.337 ms in position solving, and 0.400 ms in velocity
solving.

Focused regressions cover sphere/cuboid in both dispatch orders,
sphere/sphere, every capsule combination, active warm-start reuse, and sleeping
sphere/capsule manifold reuse. All 54 unit-test groups pass. The 600-sample
rings-chain validator passes with 4.39042 m maximum vertical error, 16.8385 m/s
maximum finite Tau speed, and zero static-ring drift. The impulse-callback QA
remains within 0.03582168 m maximum position error and its peak-to-peak
amplitude delta remains -0.00141478 m. The capsule-pair QA reports one contact
for each of capsule/sphere, capsule/cuboid, and capsule/capsule. Chair,
friction, restitution, and rolling-friction captures each complete all 600
samples; the collision-event check retains four published contact points after
landing. The solver remains at three position and eight velocity iterations.

The implementation order after this milestone was:

1. cache solver world transforms, inverse inertia, and normalized cuboid axes;
2. complete the rendered and body-count acceptance matrix;
3. optimize hot contact/island data layout before considering any solver
   iteration change.

### Eighth optimization result: solver pose, inertia, and cuboid-axis caches

Each Tau body now retains its quaternion-derived world rotation and its world
inverse-inertia tensor. Both are refreshed at the complete set of orientation
mutation points: world reset/teleport/scene synchronization, free integration,
position-correction rotation, and the focused support-transform test hook. A
mass-property refresh also rebuilds the world tensor after changing the body
tensor. World composition, shape proxy construction, body-local persistent
anchors, constraint effective mass, impulse application, rolling friction,
and torque integration now consume these cached values instead of repeatedly
converting the same quaternion and evaluating `R * I^-1 * transpose(R)`.

Updated cuboid proxies normalize their three world axes once. SAT, clipping,
support-edge construction, local-anchor conversion, sphere/cuboid, and
capsule/cuboid queries then read those unit columns directly. The focused
internal OBB helper boundary still normalizes caller-provided OBB rotations,
so this runtime contract does not make tests or future non-Tau callers depend
on already-clean input.

The same three-seed Release benchmark used by the preceding milestone reports:

| Workload | Shape-neutral median / p95 | Pose/inertia/axis cache median / p95 | Median change |
|---|---:|---:|---:|
| mixed 1,500 active | 17.116 / 22.674 ms | 12.018 / 15.302 ms | 29.8% faster |
| mixed 1,500 settled | 3.497 / 3.631 ms | 3.175 / 3.547 ms | 9.2% faster |
| mixed 2,000 spread settled | 1.549 / 1.661 ms | 1.383 / 1.495 ms | 10.7% faster |
| cube-only 1,500 active | 26.888 / 35.307 ms | 18.860 / 23.840 ms | 29.9% faster |
| cube-only 1,500 settled | 4.340 / 4.652 ms | 4.026 / 4.533 ms | 7.2% faster |
| sphere-only 1,500 active | 10.203 / 13.517 ms | 7.109 / 8.842 ms | 30.3% faster |
| sphere-only 1,500 settled | 2.439 / 2.660 ms | 2.378 / 2.643 ms | 2.5% faster |

The mixed active attribution run averages 12.1 ms in
`Tau.StepSimulation`, down from 26.3 ms in the preceding capture.
`Tau.VelocitySolve` falls from 14.3 to 5.37 ms, `Tau.BuildContacts` from
7.50 to 4.23 ms, `Tau.NarrowPhase` from 3.88 to 1.98 ms, and
`Tau.PositionSolve` from 3.15 to 1.77 ms. The settled attribution run averages
3.33 ms, with 1.95 ms in contact building, 1.20 ms in narrow phase, 0.222 ms
in position solving, and 0.267 ms in velocity solving. This confirms that the
active gain is not a sleeping shortcut and that cached tensor construction was
a first-order solver cost.

A focused asymmetric-cuboid regression verifies the analytic world angular
response before and after a 90-degree API reset and after a 90-degree
integration-only rotation. Another regression feeds residual scale through the
internal OBB helper boundary and verifies a manifold identical to normalized
input. All 54 C++ test groups pass. The 600-sample rings-chain validator stays
inside its bounded envelope: 4.56054 m maximum vertical error, 16.5758 m/s
maximum finite Tau speed, and zero static-ring drift. The impulse-callback
capture remains exactly at 0.03582168 m maximum position error and
-0.00141478 m peak-to-peak amplitude delta. Capsule-pair, primitive raycast,
and rotated 651k-triangle terrain raycast checks pass; the latter retains the
expected 403 hits and 3,348 misses. Position and velocity solver iterations
remain unchanged at three and eight.

The next implementation order at that milestone was:

1. complete the rendered end-to-end and 250/500/1,000/1,500/2,000 body-count
   acceptance matrix against freshly packaged Bullet and Tau builds;
2. use that matrix and fresh profiles to choose between compact hot constraint
   storage/precomputed effective masses and dense body-type/dirty scene-sync
   lists;
3. add zero-coefficient solver fast paths where the workload data justifies
   them;
4. consider iteration tuning only after those behavior-preserving changes and
   only behind the existing QA gates.

### Ninth optimization result: complete acceptance matrix

The complete dated report is
`specifications/SPECS_TAU_POOL_OF_OBJECTS_PERFORMANCE_MATRIX.md`. Fresh Release
Bullet and Tau packages from the same source revision were compared over 250,
500, 1,000, 1,500, and 2,000 bodies; mixed, cube-only, and sphere-only shapes;
active and 600-step-settled windows; 120 samples; and five deterministic
seeds. Controlled scene, rendered, and no-physics passes cover the 1,500-body
mixed workload.

The primary 1,500-body mixed gates are now met:

| Gate | Bullet | Tau | Tau / Bullet | Result |
|---|---:|---:|---:|---|
| active physics p95 | 14.261 ms | 15.179 ms | 1.064x | pass |
| settled physics p95 | 15.249 ms | 3.612 ms | 0.237x | pass |
| active rendered p95 | 26.420 ms | 26.541 ms | 1.005x | pass |
| settled rendered p95 | 27.019 ms | 16.699 ms | 0.618x | pass |

Across all 30 physics cells, the equal-cell mean Tau/Bullet ratio is 0.856x
by median and 0.945x by p95. The ratio of summed cell times is 0.658x and
0.722x, respectively. Tau is faster in every measured sphere-only active and
settled cell, and in every settled cell at 500 bodies or more. The stretch
gate is nevertheless not met: active cube-only cells are 1.18x to 1.84x
slower by median, and a 250-cube fully sleeping case exposes a fixed Tau step
floor of 0.287 ms versus 0.073 ms for Bullet.

A fresh three-repetition 1,500-cube active profile attributes approximately
8.20 ms per step to velocity solving, 6.01 ms to contact construction
(including 3.74 ms of narrow phase), 2.98 ms to position solving, and only
1.65 ms to broad phase. Scene synchronization is not the leading delta: the
controlled active scene pass is 1.005x by median and 1.043x by p95. The next
implementation order is therefore:

1. build compact active velocity constraints after position solving and
   precompute iteration-invariant arms, Jacobians, normal effective masses,
   bias/restitution inputs, and friction limits;
2. exploit active cuboid manifold feature coherence before falling back to a
   full SAT/clipping regeneration;
3. add profile-justified zero-coefficient fast paths;
4. add a guarded all-sleeping small-world fast path for the residual fixed
   step overhead;
5. defer dense scene-sync work, parallelism, and any iteration tuning until
   these behavior-preserving stages have passed the complete matrix and QA.

### Tenth optimization result: prepared active velocity constraints

Tau now compacts solver-active contacts into persistent per-world scratch once
after the position solve. This prepared stream consumes the freshly refreshed
post-position anchors, retains the two contact arms, and precomputes the normal
effective mass and penetration bias that remain invariant across all eight velocity
iterations. Point velocities consume those retained arms, and the repeated
loop no longer scans sleeping contacts, refreshes anchors, or recomputes normal
angular mass.

The source contact remains authoritative for accumulated impulses, diagnostics,
and manifold publication. Restitution is deliberately evaluated in sequential
warm-start order because prior warm-start impulses change the velocity seen by
later contacts. Tangent direction, tangent effective mass, and the Coulomb
limit are also still evaluated in the iteration loop because they depend on
the current velocity and accumulated normal impulse. Position and velocity
iteration counts remain three and eight.

An interleaved same-binary A/B oracle alternated the legacy and prepared paths
for the same five seeds. It avoids the thermal and scheduler drift observed
when two complete Windows benchmark series were run far apart:

| Workload | Legacy median / p95 | Prepared median / p95 | Change |
|---|---:|---:|---:|
| cube-only 1,500 active | 18.747 / 23.517 ms | 16.646 / 20.856 ms | 11.2% / 11.3% faster |
| sphere-only 1,500 settled | 2.363 / 2.657 ms | 2.224 / 2.516 ms | 5.9% / 5.3% faster |

The final three-run cube-active attribution averages 16.4-16.6 ms per Tau
step. `Tau.VelocitySolve` falls from the matrix baseline's 8.20 ms to
6.28-6.31 ms, a reduction of about 23%. Position solving remains 2.96-2.99 ms
and contact construction remains the other leading cost at 6.19-6.33 ms,
including 3.78-3.87 ms of narrow phase. A separate five-seed run also improves the
1,500-cube active median/p95 from 18.248/23.432 ms to 17.590/21.736 ms despite
whole-run machine drift.

A more aggressive experiment precomputed a quadratic tangent angular-mass
form. Although it removed additional work, its changed floating-point
evaluation order altered friction trajectories and delayed sleep in settled
cube scenes. That experiment was rejected and removed. The accepted path
preserves the solver's arithmetic and sequential impulse ordering.

All 54 C++ unit-test groups pass. The focused scratch regression verifies that
prepared capacity reaches a high-water mark, is reused without growth, and is
released by `ClearNodes`. Two fresh 600-sample chain captures are
byte-identical and pass the bounded envelope at 4.64699 m maximum vertical
error, 16.7043 m/s maximum Tau speed, and zero static-ring drift. The impulse
callback remains at 0.03582168 m maximum position error and -0.00141478 m
peak-to-peak amplitude delta. Capsule/sphere, capsule/cuboid, and
capsule/capsule contact checks also pass.

The next implementation order is now:

1. add profile-justified zero-restitution and zero-rolling-friction paths;
2. add the guarded all-sleeping small-world shortcut;
3. rerun the complete matrix before considering dense scene storage,
   independent-island parallelism, or any iteration change.

### Eleventh optimization result: coherent active cuboid manifolds

Active cube/cube pairs now look up the manifold seen during the immediately
preceding substep before invoking the complete cuboid generator. FaceA and
FaceB manifolds can retain their feature and ordered body-local anchors when
all conservative coherence checks pass: relative translation is below 2 cm,
each endpoint rotated by less than 2 degrees, the reference normal remains
aligned, the same incident face is selected, the cached contact axis still
overlaps, and every retained anchor remains within the normal and tangential
contact tolerances. Warm-start impulses remain attached to the same feature
IDs; normal impulses are alignment-scaled and tangent impulses are reprojected
onto the refreshed contact plane.

The fast path is deliberately bounded to four consecutive coherent refreshes.
The fifth persistent substep regenerates the manifold with all 15 SAT axes and
face clipping. Any failed check immediately uses that same complete fallback.
Edge/edge features, fallback face-center points, externally moved endpoints,
and large transform changes also remain on the complete generator. This keeps
the optimization local to stable contact patches and prevents a cached feature
from becoming authoritative indefinitely.

A temporary same-binary switch was used only for measurement and then removed.
Five seeds alternated legacy/coherent execution order for the 1,500-cube active
workload, with 120 samples per seed:

| Path | Median | p95 | Change from legacy |
|---|---:|---:|---:|
| complete SAT/clipping every substep | 16.515 ms | 21.120 ms | baseline |
| coherent feature reuse | 16.076 ms | 19.955 ms | 2.7% / 5.5% faster |

Three attribution runs place `Tau.NarrowPhase` at 3.20-3.23 ms instead of
3.78-3.87 ms and `Tau.BuildContacts` at 5.55-5.66 ms instead of 6.19-6.33 ms.
At active step 300, diagnostics reported 1,163 coherent manifold refreshes and
2,887 retained points, roughly 38% of emitted cuboid manifolds, while 1,854
previous features conservatively fell back. No position or velocity iteration
count changed.

Two focused tests cover four coherent generations followed by the mandatory
full refresh, and rejection after excessive relative motion. All 54 C++ test
groups pass. Repeated 600-sample chain captures remain byte-identical and pass
the established envelope with 4.64718 m maximum vertical error, 16.4712 m/s
maximum Tau speed, and zero static-ring drift. The callback amplitude remains
unchanged at a -0.00141478 m Tau/Bullet delta; fresh chair, friction,
restitution, and rolling-friction captures also complete successfully.

### Twelfth optimization result: coefficient fast paths

The prepared active-constraint pass now classifies restitution and friction
once per constraint. A zero-restitution constraint no longer evaluates either
point velocity during warm start; when every active constraint has zero
restitution the whole restitution velocity workload is absent. A constraint
with non-positive friction clears its cached tangent impulse once and skips
all tangent velocity, tangent effective-mass, Coulomb-limit, and tangent
impulse work in each of the eight velocity iterations. The general nonzero
material paths and their arithmetic order remain unchanged.

Contact construction also records whether any shaped body in the world has
nonzero rolling friction. When none does, Tau omits the complete rolling
friction contact pass. This world-level guard is conservative: a single
nonzero body keeps the original per-contact combination and solve path for the
whole substep. Position and velocity iteration counts remain three and eight.

A temporary same-binary legacy switch was used only for measurement and then
removed. Five seeds alternated fast/legacy execution order for the 1,500-cube
active workload, with 120 samples per seed:

| Path | Median | p95 | Change from legacy |
|---|---:|---:|---:|
| prepared/coherent solver, coefficient work retained | 16.183 ms | 19.721 ms | baseline |
| coefficient fast paths | 15.991 ms | 19.477 ms | 1.2% / 1.2% faster |

The median of per-run means also improves from 15.929 ms to 15.784 ms (0.9%).
At active step 300, diagnostics classified all 6,122 active constraints as
zero restitution, all 6,122 as positive friction, and skipped the rolling pass;
therefore this pool cell exercises the restitution and rolling shortcuts but
not the friction shortcut. Three attribution runs place the remaining
velocity solve at 6.21-6.44 ms per step. The skipped rolling scope measures
0.00034-0.00046 ms, effectively only profiling and branch overhead.

Focused tests cover both all-zero and nonzero material paths. Same-binary
legacy/fast captures for restitution, friction, rolling friction, and the
impulse callback are byte-identical. All 54 C++ test groups and the three
capsule pair checks pass. Repeated 600-sample chain captures remain
byte-identical and preserve the established 4.64718 m vertical envelope,
16.4712 m/s maximum speed, and zero static-ring drift. The callback amplitude
delta remains -0.00141478 m.

The next implementation target at that milestone was the guarded all-sleeping small-world
shortcut. A complete matrix rerun remains required after that stage; solver
iteration tuning stays deferred.

### Thirteenth optimization result: all-sleeping small worlds

Tau now bypasses the complete substep for worlds of at most 512 physics bodies
when at least one dynamic body exists and every dynamic body is sleeping. The
guard also requires every body proxy to be valid, the dynamic tree to have no
moved proxy, no static/kinematic endpoint to be marked as externally moved,
no accumulated force or torque, no pending support-loss grace step, no
structural mutation awaiting a full substep, and no collision-event tracking.
The limit includes static and kinematic bodies, so the 250- and 500-object pool
cells contain 255 and 505 Tau bodies and both qualify.

The pre-tick callback still runs before the guard. Any callback or public API
mutation that wakes a body therefore rejects the shortcut in the same
substep. Full processing is also forced after body creation, destruction,
garbage collection, reset, teleport, or synchronized support motion. Removing,
replacing, resetting, or teleporting a static/kinematic support now explicitly
wakes its dependent sleeping bodies, closing an older API-side support-removal
gap exposed while testing this guard.

Skipped time does not advance the contact-cache epoch. Persistent manifolds
therefore remain the immediately previous generation when a later callback,
impulse, support change, or new body wakes the world, preserving warm-start
feature compatibility. With no event tracking and forces already verified as
zero, the skipped call only performs the bounded eligibility scan; it also
avoids the final force-clear traversal.

A temporary same-binary switch, removed after measurement, alternated the
legacy and fast paths across five seeds and 120-sample settled windows:

| Cube-only settled workload | Legacy median / p95 | Fast median / p95 | Improvement |
|---|---:|---:|---:|
| 250 dynamic + 5 static bodies | 262.7 / 270.5 us | 3.0 / 3.1 us | 98.9% / 98.9% |
| 500 dynamic + 5 static bodies | 794.6 / 814.7 us | 7.6 / 7.8 us | 99.0% / 99.0% |

One 250-body seed completed its sleep transition during the measured window,
so that run contains both full and skipped samples; the table reports the
cross-seed median. On a fully sleeping same-seed comparison Tau measured
2.8 us against Bullet's 72.2 us. A fresh active-window A/B found no consistent
directional regression: mean pairwise changes average +0.1% at 250 bodies and
+0.5% at 500 bodies, within the run-to-run noise of those rapidly changing
active windows. Worlds above 512 bodies reject the guard before scanning nodes.

Focused tests cover the accepted path, callback execution and wake, retained
warm-start features, tracked-contact rejection, support removal, moving
supports, and the 512-body boundary. All 54 C++ test groups pass. Legacy/fast
chain and callback captures are byte-identical. Final repeated chain captures
remain byte-identical at 4.64718 m maximum vertical error, 16.4712 m/s maximum
Tau speed, and zero static-ring drift; the callback amplitude delta remains
-0.00141478 m, and all three capsule pair checks pass. Solver iteration counts
remain three and eight.

The next step at that milestone was to rerun the complete body-count, shape,
active, settled, scene, and rendered matrix. Its updated active-cube
attribution would choose between dense body storage and independent-island
parallelism; iteration tuning remained deferred.

### Fourteenth optimization result: complete acceptance rerun

The complete five-seed matrix was rerun from Release Bullet and Tau packages
using revision `b492e916ec6a69d25ce8ed61f8287aa71cc0fe2c`. The original protocol was
preserved: 250, 500, 1,000, 1,500, and 2,000 dynamic bodies; mixed, cube-only,
and sphere-only populations; 120 samples after 10 warm-up steps; a 600-step
settling transition; and alternating backend order by cell.

Tau now reaches active sum-time parity across all 15 active cells:

| Aggregate scope | Median Tau/Bullet | p95 Tau/Bullet |
|---|---:|---:|
| complete 30-cell suite, equal-cell | 0.634x | 0.676x |
| complete 30-cell suite, sum-time | 0.563x | 0.607x |
| active 15-cell suite, equal-cell | 1.042x | 1.120x |
| active 15-cell suite, sum-time | 0.974x | 0.996x |
| settled 15-cell suite, sum-time | 0.223x | 0.235x |

This improves the active equal-cell median ratio from 1.177x to 1.042x, the
active sum-time median ratio from 1.150x to 0.974x, and the complete sum-time
median ratio from 0.658x to 0.563x. The small-world floor is gone: the settled
median is 3 us versus 73 us for 250 cubes and 8 us versus 2.529 ms for 500
cubes. Seeds that remained partially active after 600 steps correctly used the
full path and were retained in the five-seed populations.

The representative 1,500-body mixed cell now measures 10.820 ms versus
12.465 ms active by median and 13.091 ms versus 14.715 ms by p95. Settled Tau
is 3.048/3.440 ms median/p95 versus Bullet's 14.880/15.671 ms. The same result
survives scene and rendering overhead: rendered active is 22.380/24.616 ms for
Tau versus 24.225/26.840 ms for Bullet, and rendered settled is
16.698/16.829 ms versus 26.735/27.661 ms. Bare scene and rendered no-physics
controls remain backend-neutral.

The suite-wide stretch gate is not complete because six median cells and seven
p95 cells remain above 1.10x. Five of the six median failures are active
cube-only workloads:

| Active cube bodies | Median Tau/Bullet | p95 Tau/Bullet |
|---:|---:|---:|
| 250 | 1.134x | 1.936x |
| 500 | 1.554x | 1.872x |
| 1,000 | 1.542x | 1.597x |
| 1,500 | 1.418x | 1.476x |
| 2,000 | 1.320x | 1.377x |

The remaining median failure is the 500-body mixed active cell at 1.225x.
Sphere-only active cells range from 0.591x to 0.891x, confirming that the
remaining issue is cuboid-heavy contact and solver locality rather than broad
backend scaling.

Three attribution runs on 1,500 active cubes put the complete Tau step at
15.87 ms, velocity solving at 6.31 ms, contact construction at 5.63 ms,
narrow phase at 3.22 ms inside contact construction, position solving at
3.04 ms, broad phase at 1.78 ms, proxy update at 0.524 ms, and island
construction at 0.232 ms. A diagnostic sample reported 1,303 awake bodies,
196 candidates, one sleeper, 146 contact islands, 3,085 manifolds, and 5,897
contact points.

Dense body storage is therefore selected before independent-island
parallelism. `TauNode` ownership remained in `std::map<NodeRef, TauNode>`, while
the hot proxy, contact, island, and solver vectors repeatedly dereference
scattered node pointers. Ordered dense slots plus a `NodeRef`-to-slot lookup can
improve locality across the dominant single-threaded loops without depending
on the active pile partitioning into balanced islands. Parallelism remains a
subsequent option once the data layout is competitive and deterministic island
work scheduling can be measured.

The correctness gates remain green: all 54 C++ test groups pass; two fresh
600-sample chain captures are byte-identical and preserve the 4.64718 m,
16.4712 m/s, and zero-static-drift envelope; callback maximum position error
is 0.03582168 m with a -0.00141478 m amplitude delta; capsule pairs and all
analytic, mesh, and rotating-terrain raycast checks pass. Solver iteration
counts remain three and eight. Complete tables and render controls are archived
in `SPECS_TAU_POOL_OF_OBJECTS_PERFORMANCE_MATRIX.md`.

### Fifteenth optimization result: ordered dense body storage, limited slice

`TauNode` ownership has moved from one allocation per body in
`std::map<NodeRef, TauNode>` to an ordered `std::vector<TauNodeSlot>`. A hashed
`NodeRef`-to-slot index retains expected O(1) public API lookup. Insertions use
the same strict `NodeRef` order as the former map, and every existing hot loop
continues to traverse bodies in that canonical order. This preserves body,
candidate, contact, island, and sequential-solver ordering rather than hiding a
trajectory change behind a storage optimization.

The implementation deliberately limits this slice to ownership and lookup.
Contact algorithms, manifold feature selection, sleeping decisions, solver
equations, and the three position/eight velocity iterations are unchanged.
Structural mutations happen outside the substep solver; appending newly created
scene nodes is constant amortized time, while an uncommon out-of-order insertion
or middle erase shifts the dense tail and rebuilds the affected lookup suffix.
`ClearNodes` releases the dense allocation, preserving the old clear-world
memory behavior. Scene-wide creation reserves once from the scene node count.

A focused Release run used the complete-matrix protocol for the remaining
hotspot: 1,500 active cube bodies, five deterministic seeds, 120 samples after
10 warm-up steps, with diagnostics and profiling disabled. The archived matrix
row is shown only as the pre-slice reference; the focused row comes from fresh
Bullet and Tau processes on the implementation working tree based on
`b3d271312cd82ba377385436795f6c073dec7cbc`.

| 1,500 active cubes | Bullet median / p95 | Tau median / p95 | Tau/Bullet median / p95 |
|---|---:|---:|---:|
| Complete-matrix reference | 11.405 / 13.251 ms | 16.170 / 19.557 ms | 1.418x / 1.476x |
| Dense-storage focused run | 11.312 / 13.107 ms | 15.947 / 19.756 ms | 1.410x / 1.507x |

Tau's focused median is 1.4% below the archived value and the normalized median
ratio improves by 0.6%. The p95 sample is 1.0% higher and does not establish a
tail-latency win. This is therefore a useful low-risk foundation, not a claim
that dense ownership alone closes the active-cuboid stretch gate. A later
alternating run was affected by substantial system-wide slowdown in both
backends and is excluded from the milestone numbers.

Additional fresh five-seed active spot checks, run after that slowdown and
therefore interpreted only as paired backend ratios, retained substantial Tau
headroom outside the cube-only hotspot: mixed bodies measured 0.824x/0.805x
Bullet by median/p95, and sphere-only bodies measured 0.578x/0.552x. No
cross-backend performance inversion was observed in the two non-cuboid shape
populations. A complete matrix rerun is still required before accepting a
suite-wide p95 claim.

The storage invariant test covers out-of-order insertion, replacement without
duplication, middle erase with lookup reindexing, canonical order, and clear.
All 54 C++ test groups pass. Two new 600-sample chain captures are byte-identical
and also identical to the pre-slice Tau capture; the 4.64718 m vertical envelope,
16.4712 m/s peak speed, and zero static drift are unchanged. The impulse-callback
capture is likewise byte-identical to its pre-slice result, retaining the
0.03582168 m maximum Bullet/Tau position error and -0.00141478 m amplitude
delta.

The next bounded locality slice should split solver-hot body state (pose,
velocities, inverse mass/inertia, activation and factors) from cold ownership,
shape, resource, sleep-history, and debug state. This is implemented in the
following milestone. The complete matrix remains required after that stage;
no solver-iteration change is authorized.

### Sixteenth optimization result: hot/cold body-data separation

The dense store now owns two aligned arrays. `TauNodeSlot` contains the
canonical `NodeRef` and compact solver-hot state: body type, mass terms, pose,
cached world rotation/inertia, linear and angular velocities, motion factors,
and activation state. `TauNodeCold` contains motion-adapter ownership, collision
shapes and world proxies, body inverse inertia, material coefficients, force
accumulators, previous pose, sleep topology/history, broad-phase handle, and
scene-sync flags. Each hot entry keeps a pointer to its parallel cold entry.

Store insert, replacement, erase, and reserve operations rebind each affected
cold pointer, or the complete array when cold storage reallocates. Hot and cold
arrays retain identical strict `NodeRef` order, so all existing body,
candidate, contact, island, and sequential-solver traversal order is
unchanged. Structural mutations still happen outside the substep solver. A
compile-time size guard prevents the hot slot from silently growing larger
than the cold payload.

The focused Release comparison uses the same 1,500 active-cube protocol as the
dense-storage slice: five deterministic seeds, 120 samples after 10 warm-up
steps, separate fresh backend processes, and diagnostics/profiling disabled.

| 1,500 active cubes | Bullet median / p95 | Tau median / p95 | Tau/Bullet median / p95 |
|---|---:|---:|---:|
| Dense-storage focused run | 11.312 / 13.107 ms | 15.947 / 19.756 ms | 1.410x / 1.507x |
| Hot/cold focused run | 11.330 / 13.413 ms | 15.765 / 19.363 ms | 1.391x / 1.444x |

Against the dense-storage focused run, Tau's absolute median improves by 1.1%
and its p95 by 2.0%. Normalizing each run against its fresh Bullet process, the
median ratio improves by 1.3% and the p95 ratio by 4.2%. Relative to the
complete-matrix reference, the normalized active-cube ratios improve from
1.418x/1.476x to 1.391x/1.444x. The cuboid cell still exceeds the 1.10x stretch
limit, but this slice produces a clearer tail-latency gain than dense ownership
alone.

Fresh five-seed active spot checks retained the faster non-cuboid paths:

| 1,500 active bodies | Bullet median / p95 | Tau median / p95 | Tau/Bullet median / p95 |
|---|---:|---:|---:|
| mixed | 12.221 / 14.742 ms | 10.722 / 12.877 ms | 0.877x / 0.873x |
| sphere-only | 10.553 / 13.105 ms | 6.618 / 7.963 ms | 0.627x / 0.608x |

These spot checks establish that both populations remain comfortably faster
than Bullet; the complete alternating matrix is still required for a formal
suite-wide regression decision. Attribution-only active-cube runs average
15.8 ms for the full Tau step, 6.14 ms for velocity solving, 5.72 ms for
contact construction, 3.07 ms for position solving, 1.79 ms for broad phase,
and 0.510 ms for proxy update. The archived pre-layout attribution measured
6.31 ms in velocity solving, so the observed locality benefit is directionally
consistent with the intended hot loop. Other phase changes are within the
noise of separate profiled runs.

The storage invariant test now additionally covers hot/cold alignment,
payload preservation and pointer rebinding after reserve, replacement, and
middle erase. All 54 C++ test groups pass. Two new 600-sample chain captures
are byte-identical to one another and to the pre-split capture, retaining hash
`619CC48B2BE46D91E7B4961DDAE9705DEE0CA151F25104CE9DEF7BA3735A557E`, the
4.64718 m vertical envelope, 16.4712 m/s peak speed, and zero static drift. The
impulse-callback capture also remains byte-identical with hash
`928E478D3CABE1DE0135CFE16D0AAABF8D67288BE76BB22671A7DB51223296E9`, a
0.03582168 m maximum Bullet/Tau position error, and a -0.00141478 m amplitude
delta. Solver iterations remain three position and eight velocity passes.

The next step is the complete active/settled body-count and shape matrix. If
it confirms this result, the new attribution should choose between a further
structure-of-arrays solver constraint pass and deterministic independent-island
parallelism. Dense scene-sync lists remain independently available for the
rendered settled path.

## Workload Characterization

`physics_pool_of_objects.lua` creates:

- one static floor and four static walls;
- dynamic bodies added in batches of seven;
- an approximately even random mix of one-shape cubes and spheres;
- one renderable model per body;
- a 60 Hz requested physics step with a maximum of four substeps;
- a 1280x720 forward-rendered window with VSync and 4x MSAA.

At the captured count, the Tau physics world contains approximately 1,510
bodies. The audited baseline's nested broad-phase loops therefore executed:

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
then-current gap was large. They should not be used as the final acceptance metric
for these reasons:

- VSync quantizes visible FPS into coarse divisors of the display refresh
  rate. The values 12 and 3 do not expose the underlying frame-time
  distribution.
- Rendering, scene traversal, transform synchronization, Lua, and physics are
  all included in the counter.
- Object creation is tied to rendered frames, not to a deterministic physics
  timeline.
- Before the fixed-step correction, the two backends did not handle the
  substep cap in the same way.
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

### Tau At The Audited Baseline

Every Tau substep at the audited baseline performed the following sequence:

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

## Baseline Source-Audit Findings

The rows below describe the code at the start of this work. Implemented
corrections and their measurements are recorded in the milestone results and
phase status notes; this table is retained as the source-audit baseline.

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

Implementation status (2026-09-02): complete for the current deterministic
acceptance protocol. The full matrix and scene/render controls are archived in
`SPECS_TAU_POOL_OF_OBJECTS_PERFORMANCE_MATRIX.md`. The freshly packaged code
does not reproduce the original 12 FPS Bullet / 3 FPS Tau symptom; the
screenshots therefore remain evidence of the former interactive
spawn/substep implementation rather than a current numeric baseline.

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

Implementation status (2026-09-01): met. Randomized component tests and Tau's
brute-force diagnostics report no missed pair; exact candidates are sorted;
all body lifecycle paths maintain persistent proxies; the 2,000-body spread
case emits zero exact candidates; and broad phase is approximately 10% active
and 3% after 300 settling steps in the 1,500-body pool.

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

Implementation status (2026-09-02): met for the deterministic physics-only
gate. The pool exceeds 90% sleeping by step 600; explicit/API and impact wake
paths, moving supports, disabled deactivation, dirty transform publication,
sleep-pose persistence across scene matrix invalidation, and sleeping contact
events have dedicated tests. Directional dynamic-support wake also crosses a
bounded-cohort boundary without waking unrelated lateral contacts. Bounded
activation cohorts prevent local impacts from waking the complete dense pile.
The controlled rendered gate is now complete and archived in the ninth
result. The interactive tutorial remains a user-visible smoke test rather than
the numeric baseline. Unchanged sleeping cuboid contacts reuse their cached
manifolds while preserving the contact graph and event path. Shape-neutral
persistence, scratch retention, and pose/inertia/axis caching are recorded in
the sixth, seventh, and eighth optimization results above.

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

Implementation status (2026-09-03): the body-local persistent anchors,
world rotation/inverse-inertia/cuboid-axis caches, and compact prepared active
velocity constraints are implemented and satisfy their focused tests and
physics QA envelopes. The profiler no longer rebuilds inverse-inertia matrices,
normalizes cuboid axes, refreshes persistent anchors, or recomputes normal
effective mass inside velocity iterations. Conservative coherent FaceA/FaceB
manifold refresh is also implemented and periodically revalidated by the full
generator. Coefficient fast paths and the guarded all-sleeping small-world
shortcut are also implemented. The first ordered dense body-ownership and
hashed lookup slice is implemented without changing traversal order. The
follow-up hot/cold `TauNode` split improves the focused active-cube median and
p95 while preserving exact QA trajectories. The complete matrix, dense
scene-sync lists, and parallel independent islands remain open; solver
iteration counts remain unchanged.

## Phase 6: Parallelism And SIMD Stretch Work

After the single-thread algorithm and data layout are competitive:

- honor `SceneTauPhysics(thread_count)`;
- solve independent islands in parallel;
- batch broad-phase queries and common narrow-phase combinations;
- evaluate SIMD-friendly sphere-sphere and cuboid SAT batches;
- use per-worker scratch buffers and merge candidate/contact vectors in stable
  order;
- retain a deterministic single-thread mode for QA and debugging.

Parallelizing the audited all-pairs and linear-cache implementation is not a
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

Acceptance status (2026-09-02): settled parity, active parity, and end-to-end
parity pass. The stretch gate remains open because active cube-only workloads
and the 250-cube settled fixed-overhead case exceed the per-cell 1.10x limit.
See `SPECS_TAU_POOL_OF_OBJECTS_PERFORMANCE_MATRIX.md` for the complete values
and aggregation method.

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

Tau now reaches measured parity for the representative mixed workload and
outperforms Bullet strongly once the pool settles. Prepared active constraints
reduce the cube velocity solve by about 23%, while coherent cuboid refresh cuts
the focused active-cube p95 by another 5.5%. Coefficient fast paths then reduce
the focused active-cube median and p95 by another 1.2% without changing any
measured QA trajectory. Fully sleeping small worlds now fall to 3.0 us at 250
dynamic bodies and 7.6 us at 500, eliminating the previous fixed Tau floor.
The updated complete matrix reaches 0.974x/0.996x active sum-time median/p95
and selects dense body storage before parallel independent islands. The first
ordered dense-ownership slice improves the focused 1,500-cube median by 1.4%
but does not improve p95. The follow-up hot/cold split then improves Tau's
focused absolute median/p95 by another 1.1%/2.0%, and the fresh normalized
ratios reach 1.391x/1.444x. Exact chain and callback trajectories are
preserved. The complete matrix and active cube-only per-cell stretch gates must
pass before claiming that Tau beats Bullet without qualification.
