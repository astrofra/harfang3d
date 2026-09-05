# Tau Dynamic Compound Interpenetration And Deferred CCD Work

Date: 2026-09-05

Status: documented and deliberately deferred

## Decision

Keep the current Tau continuous-collision implementation unchanged for now.

The implemented CCD slice remains opt-in and limited to moving cuboids, including
multi-cuboid compounds, against immovable cuboids and static triangle meshes.
Dynamic compound-against-compound CCD is not part of the current contract.

Do not increase the Tau solver iteration counts or introduce a global TOI event
loop as part of the current work. Resume this subject only as a separately scoped
correctness task with dedicated measurements and QA acceptance criteria.

## Observed Problem

The physics QA sample `physics-qa/rb_dynamic_chair_multi_colbox.lua` drops 200
chairs. Each chair is a dynamic rigid body made from six cuboid collision shapes.

Enabling the current Tau CCD option prevents chairs from tunneling through the
static floor. However, some chairs can still become visibly embedded in other
chairs. Once a deeply overlapping pair has been accepted by the discrete solver,
the geometry of the six-part compounds can leave the bodies mechanically
interlocked.

This is not a regression in the static CCD path. It exposes a separate limitation
in dynamic compound-against-compound collision handling.

## Current Tau Behavior

Tau currently:

- gathers only immovable cuboids and static triangle meshes as CCD targets;
- computes a linear swept-SAT time of impact for every moving cuboid in an
  opted-in compound;
- resolves the earliest supported impact;
- continues through the remaining substep time, with a bounded impact count;
- delegates dynamic-against-dynamic contacts to the regular discrete manifold
  and constraint solvers.

Consequently, two dynamic chairs are not swept against each other. At the QA
fixed step of 1/120 second, a body moving at approximately 98 m/s travels about
0.82 m in one step. This distance is large relative to individual chair collision
boxes. A newly arrested chair and the chair falling above it can therefore enter
the discrete phase with substantial overlap.

## Why Bullet Does Not Show The Same Artifact

The observed Bullet result does not prove that Bullet performs complete dynamic
CCD for these chairs.

Harfang currently creates a `btCompoundShape` root for every Bullet rigid body,
including bodies with only one collision component. The per-body CCD option sets
Bullet's motion threshold and swept-sphere radius on that root.

Bullet's predictive-contact and motion-clamping paths execute only when the root
collision shape reports itself as convex. A `btCompoundShape` is not a convex
shape, so the Harfang chair compounds do not use that path. Bullet's
compound-compound TOI implementation is also not a production event queue.

Bullet avoids the visible chair embedding primarily through its mature discrete
pipeline:

- per-child compound broadphase and collision algorithms;
- persistent contact manifolds;
- warm-started sequential impulses;
- ten default solver iterations;
- split-impulse penetration correction.

Tau currently uses three position iterations and eight velocity iterations. It
has persistent manifolds and warm starting, but it does not have an equivalent
of Bullet's complete compound collision pipeline and split-impulse recovery.

## Bullet API Caveat

`RigidBody::SetContinuousCollisionDetection(true)` is mapped to Bullet's native
CCD parameters, but that mapping is ineffective for the compound roots currently
created by Harfang.

Until this is revisited, the generic option must not be documented as guaranteeing
continuous collision behavior for Bullet compounds. It is a best-effort backend
request whose effective shape coverage is backend-dependent.

This caveat is accepted for the current freeze because the discrete Bullet result
is already satisfactory in the chair QA. It should nevertheless be addressed if
the public CCD option becomes part of a documented cross-backend guarantee.

## Deferred Corrective Path

When work resumes, investigate the discrete path before implementing a full
dynamic TOI event system.

### Stage 1: Instrumentation

Add measurements for dynamic compound pairs:

- swept broadphase candidate count;
- relative motion per fixed step;
- initial and post-solve penetration;
- manifold count and points per child-shape pair;
- contacts rejected or replaced during manifold persistence;
- solver residuals for deeply penetrating pairs;
- number and lifetime of visibly interlocked pairs.

The instrumentation must distinguish static-floor tunneling from dynamic chair
interpenetration.

### Stage 2: Predictive Dynamic Contacts

Prefer a limited predictive-contact implementation before a global TOI loop:

1. build candidate dynamic pairs from swept AABBs;
2. reuse the existing cuboid swept SAT with relative linear displacement;
3. generate symmetric predictive contacts before geometric penetration;
4. feed those contacts into the existing velocity constraint solver;
5. retain the ordinary discrete manifolds for resting stacks and final position
   correction;
6. bound work per substep and record every budget fallback.

This approach should preserve the current solver architecture and avoid
order-dependent per-body motion clamping.

### Stage 3: Deep-Penetration Recovery

If predictive contacts are insufficient, add a split positional correction path
for deep dynamic penetrations. It must not inject artificial kinetic energy and
must remain independent from the velocity restitution response.

### Stage 4: Full Dynamic TOI, Only If Required

Implement a synchronized TOI event loop only if the earlier stages cannot satisfy
the QA. Such a loop would need:

- global or island-local time ordering;
- symmetric pair response;
- invalidation and recomputation of stale candidate events;
- a strict event budget for dense piles;
- a deterministic fallback when the budget is exhausted;
- protection against zero-time event loops.

Angular sweeps and non-cuboid shape combinations remain separate extensions.

## Acceptance Criteria For Resuming Work

Future changes are acceptable only if they satisfy all of the following:

- no persistent chair-against-chair embedding in the 200-chair QA;
- no regression in static floor and triangle-mesh CCD;
- deterministic results for identical fixed-step inputs;
- no change to solver iteration counts unless separately justified;
- bounded predictive or TOI work in dense piles;
- no material regression in the pool-of-objects performance matrix;
- all Tau scene, contact, serialization, and binding tests remain green;
- the effective Bullet compound CCD behavior is either implemented or explicitly
  exposed as unsupported.

## Current Non-Goals

The frozen implementation does not promise:

- dynamic-against-dynamic CCD;
- exact angular CCD;
- CCD for moving spheres or capsules;
- compound CCD parity between Tau and Bullet;
- recovery from arbitrary initial overlap;
- backend-independent CCD guarantees beyond the opt-in API surface.

