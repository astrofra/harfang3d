# Tau Physics Pool Complete Performance Matrix

Date: 2026-09-02

Status: complete for the deterministic physics-only matrix and the controlled
scene/render acceptance passes described below. The first post-matrix solver
optimization is recorded in the follow-up section.

Related roadmap:
`specifications/SPECS_TAU_POOL_OF_OBJECTS_PERFORMANCE_ROADMAP.md`.

## Outcome

Tau now meets the roadmap's representative 1,500-body mixed-workload gates
without reducing the solver's three position or eight velocity iterations:

- active physics p95: 15.179 ms for Tau versus 14.261 ms for Bullet, or
  **1.064x**;
- settled physics p95: 3.612 ms for Tau versus 15.249 ms for Bullet, or
  **0.237x**;
- rendered active p95: 26.541 ms for Tau versus 26.420 ms for Bullet, or
  **1.005x**;
- rendered settled p95: 16.699 ms for Tau versus 27.019 ms for Bullet, or
  **0.618x**.

The complete matrix exposed active cube-only physics as the next optimization
target: it was 1.18x to 1.84x slower than Bullet by median. Prepared active
velocity constraints now reduce that hotspot, as documented below, but the
global stretch goal still requires a fresh complete-matrix rerun after the
remaining cuboid contact work.

## Protocol

Both Lua packages were rebuilt and installed from source revision
`63769827234e3b15e81995ac3f24ce278e15b28d` before the run. The benchmark ran
on `ZOTAC-MAGNUS`, an Intel Family 6 Model 165 processor with 16 logical
processors, using the Release target configuration.

The permanent harness is `tutorials/physics_pool_benchmark.lua`. For the
physics matrix it used:

- 250, 500, 1,000, 1,500, and 2,000 dynamic bodies plus five static container
  bodies;
- mixed, cube-only, and sphere-only populations;
- an active window and a settled window after 600 fixed steps;
- one exact 1/60 s physics substep per sample;
- 10 untimed warm-up steps and 120 timed samples per window;
- five repetitions using seeds 5,521,749 through 5,521,753;
- alternating Bullet and Tau runs for every body-count/shape group;
- the tutorial's deterministic seven-bodies-per-frame creation history,
  excluded from timed samples.

Each table entry is the median of the five per-repetition medians. The p95
entry is the median of the five per-repetition p95 values. Lower is better.
Diagnostics and `HG_TAU_PROFILE` were disabled for backend comparisons.

## Physics-only Matrix

### Active window

| Bodies | Shapes | Bullet median | Tau median | Tau/Bullet | Bullet p95 | Tau p95 | Tau/Bullet p95 |
|---:|---|---:|---:|---:|---:|---:|---:|
| 250 | mixed | 0.373 ms | 0.362 ms | 0.972x | 0.946 ms | 1.525 ms | 1.612x |
| 250 | cube | 0.381 ms | 0.447 ms | 1.175x | 0.910 ms | 2.168 ms | 2.384x |
| 250 | sphere | 0.323 ms | 0.292 ms | 0.904x | 0.789 ms | 0.734 ms | 0.930x |
| 500 | mixed | 1.268 ms | 1.685 ms | 1.329x | 2.531 ms | 3.654 ms | 1.444x |
| 500 | cube | 1.313 ms | 2.314 ms | 1.762x | 2.413 ms | 5.543 ms | 2.297x |
| 500 | sphere | 1.074 ms | 1.023 ms | 0.953x | 2.109 ms | 1.929 ms | 0.915x |
| 1,000 | mixed | 5.257 ms | 6.208 ms | 1.181x | 7.612 ms | 8.766 ms | 1.152x |
| 1,000 | cube | 4.975 ms | 9.166 ms | 1.842x | 6.898 ms | 13.828 ms | 2.005x |
| 1,000 | sphere | 4.287 ms | 3.637 ms | 0.848x | 6.520 ms | 4.934 ms | 0.757x |
| 1,500 | mixed | 11.779 ms | 12.170 ms | 1.033x | 14.261 ms | 15.179 ms | 1.064x |
| 1,500 | cube | 11.011 ms | 18.248 ms | 1.657x | 12.636 ms | 23.432 ms | 1.854x |
| 1,500 | sphere | 9.884 ms | 7.055 ms | 0.714x | 12.992 ms | 8.667 ms | 0.667x |
| 2,000 | mixed | 19.510 ms | 19.061 ms | 0.977x | 22.362 ms | 22.500 ms | 1.006x |
| 2,000 | cube | 17.705 ms | 28.877 ms | 1.631x | 19.791 ms | 33.467 ms | 1.691x |
| 2,000 | sphere | 16.962 ms | 11.438 ms | 0.674x | 20.082 ms | 13.356 ms | 0.665x |

### Settled window

| Bodies | Shapes | Bullet median | Tau median | Tau/Bullet | Bullet p95 | Tau p95 | Tau/Bullet p95 |
|---:|---|---:|---:|---:|---:|---:|---:|
| 250 | mixed | 0.789 ms | 0.286 ms | 0.363x | 0.820 ms | 0.311 ms | 0.379x |
| 250 | cube | 0.073 ms | 0.287 ms | 3.914x | 0.079 ms | 0.297 ms | 3.773x |
| 250 | sphere | 0.635 ms | 0.310 ms | 0.488x | 0.656 ms | 0.337 ms | 0.513x |
| 500 | mixed | 2.249 ms | 0.701 ms | 0.312x | 2.370 ms | 0.717 ms | 0.302x |
| 500 | cube | 2.509 ms | 0.840 ms | 0.335x | 2.757 ms | 0.869 ms | 0.315x |
| 500 | sphere | 1.523 ms | 0.553 ms | 0.363x | 1.584 ms | 0.606 ms | 0.382x |
| 1,000 | mixed | 7.049 ms | 1.854 ms | 0.263x | 7.871 ms | 1.928 ms | 0.245x |
| 1,000 | cube | 7.300 ms | 2.374 ms | 0.325x | 8.148 ms | 2.475 ms | 0.304x |
| 1,000 | sphere | 5.080 ms | 1.237 ms | 0.243x | 5.940 ms | 1.274 ms | 0.214x |
| 1,500 | mixed | 14.292 ms | 3.275 ms | 0.229x | 15.249 ms | 3.612 ms | 0.237x |
| 1,500 | cube | 14.354 ms | 4.003 ms | 0.279x | 15.125 ms | 4.293 ms | 0.284x |
| 1,500 | sphere | 11.371 ms | 2.356 ms | 0.207x | 12.181 ms | 2.510 ms | 0.206x |
| 2,000 | mixed | 22.055 ms | 5.002 ms | 0.227x | 22.967 ms | 5.529 ms | 0.241x |
| 2,000 | cube | 21.495 ms | 6.473 ms | 0.301x | 22.418 ms | 7.176 ms | 0.320x |
| 2,000 | sphere | 18.697 ms | 3.530 ms | 0.189x | 19.780 ms | 3.990 ms | 0.202x |

The 250-cube settled cell is a real small-world outlier rather than a general
sleeping failure. Bullet reached a roughly 0.073 ms fully sleeping path in
four of five seeds, while Tau retained about 0.287 ms of fixed step overhead.
At 500 bodies and above Tau is faster in every settled shape population.

## Aggregate Views

Two aggregate views are reported to avoid hiding either workload fairness or
absolute CPU consumption. The equal-cell view gives every matrix cell equal
weight. The sum-time view divides the sum of Tau cell times by the sum of the
matching Bullet cell times.

| Scope | Cells | Equal-cell median ratio | Equal-cell p95 ratio | Sum-time median ratio | Sum-time p95 ratio |
|---|---:|---:|---:|---:|---:|
| complete suite | 30 | 0.856x | 0.945x | 0.658x | 0.722x |
| active | 15 | 1.177x | 1.363x | 1.150x | 1.202x |
| settled | 15 | 0.536x | 0.528x | 0.255x | 0.260x |
| mixed, both phases | 10 | 0.689x | 0.768x | 0.598x | 0.657x |
| cube, both phases | 10 | 1.322x | 1.523x | 0.900x | 1.026x |
| sphere, both phases | 10 | 0.558x | 0.545x | 0.450x | 0.464x |

Eight of 30 cells exceed 1.10x by median and nine exceed 1.10x by p95. Seven
of the eight median failures are active workloads. The remaining one is the
250-cube settled fixed-overhead outlier.

## Scene And Render Controls

The controlled end-to-end pass uses the same 1,500-body mixed scene, seeds,
fixed-step protocol, repetitions, and settling window. Render modes add a
1280x720 forward pipeline, 4x MSAA, visible cube/sphere models, a camera, and
lights. `RF_VSync` is not requested. The benchmark excludes the tutorial UI
overlay and excludes body creation from samples.

| Mode | Window | Bullet median | Tau median | Ratio | Bullet p95 | Tau p95 | p95 ratio |
|---|---|---:|---:|---:|---:|---:|---:|
| scene + physics | active | 12.112 ms | 12.178 ms | 1.005x | 14.443 ms | 15.060 ms | 1.043x |
| scene + physics | settled | 14.537 ms | 3.339 ms | 0.230x | 15.196 ms | 3.492 ms | 0.230x |
| bare scene, no physics | active | 0.064 ms | 0.064 ms | 1.000x | 0.065 ms | 0.069 ms | 1.071x |
| bare scene, no physics | settled | 0.064 ms | 0.064 ms | 1.000x | 0.065 ms | 0.068 ms | 1.056x |
| rendered + physics | active | 23.740 ms | 23.103 ms | 0.973x | 26.420 ms | 26.541 ms | 1.005x |
| rendered + physics | settled | 26.024 ms | 16.667 ms | 0.640x | 27.019 ms | 16.699 ms | 0.618x |
| rendered, no physics | active | 16.664 ms | 16.667 ms | 1.000x | 16.708 ms | 16.716 ms | 1.000x |
| rendered, no physics | settled | 16.667 ms | 16.665 ms | 1.000x | 16.711 ms | 16.712 ms | 1.000x |

The renderer-only control plateaus at approximately 16.67 ms on this system
despite not requesting VSync, so FPS alone would obscure most settled-physics
gains. Scene synchronization adds little in the active controlled workload;
the physics delta explains nearly all of the remaining backend delta.

The original approximately 12 FPS Bullet / 3 FPS Tau screenshots are not
reproduced by this freshly packaged, pre-created, exact-substep scene. They
remain valid preliminary evidence for the former implementation and its
interactive spawn/substep history, but are not a numeric baseline for the
current code.

## Acceptance Gates

| Roadmap gate | Result | Evidence |
|---|---|---|
| comparable deterministic inputs and fixed steps | PASS | Identical seeded creation protocol and one exact 1/60 s substep per physics sample |
| algorithmic scaling | PASS | Dynamic broad phase and O(1) persistent manifold lookup were established in earlier milestones |
| 1,500 mixed settled p95 no more than 1.10x | PASS | 0.237x |
| 1,500 mixed active p95 no more than 1.10x | PASS | 1.064x |
| rendered end-to-end no more than 1.10x | PASS | 1.005x active and 0.618x settled by p95 |
| stretch: at least 15% faster with no cell over 1.10x | FAIL | Equal-cell median is 14.4% faster, but cube-active cells and the 250-cube settled cell exceed 1.10x |

## Remaining Hotspot Attribution

A three-repetition attribution-only profile of the 1,500-cube active cell
reports an approximately 18.1 ms Tau step. Representative nested scopes are:

| Tau scope | Average per step |
|---|---:|
| velocity solve | 8.20 ms |
| contact construction | 6.01 ms |
| narrow phase, included in contact construction | 3.74 ms |
| position solve | 2.98 ms |
| broad phase | 1.65 ms |
| proxy update | 0.50 ms |
| island construction | 0.22 ms |

The broad phase is no longer the governing cost. Cuboid contacts amplify both
constraint count and per-constraint solver math, while active cuboid
manifolds still require comparatively expensive SAT/clipping refresh.

## Corrective Trajectory

The next work should remain behavior-preserving and keep solver iteration
counts unchanged:

1. Exploit active cuboid manifold coherence. Refresh cached local anchors and
   validate the previous separating/contact features before falling back to a
   complete SAT and clipping pass. Preserve deterministic contact ordering and
   use the current generator whenever the cached feature is invalid.
2. Add measured coefficient fast paths, especially zero restitution and zero
   rolling friction, without changing the general material path.
3. Add an all-sleeping fast path for small worlds when no proxy moved, no wake
   request is pending, and no tracked-contact publication requires traversal.
   This targets the 0.287 ms fixed floor revealed by the 250-cube cell.
4. Re-run the complete matrix and the existing physics QA after every stage.
   Accept no active cube regression and no contact, wake, callback, chain, or
   raycast regression.
5. Only after the single-threaded cube path is competitive, evaluate dense
   body storage and independent-island parallelism. Scene-sync work is lower
   priority because the controlled scene pass is already near physics-only
   parity.

This trajectory targets the measured 8.2 ms velocity-solver and 6.0 ms
contact-construction costs directly. It does not rely on weaker physical
settings or fewer solver iterations.

## Post-Matrix Follow-Up: Prepared Velocity Constraints

The first corrective item is implemented without changing the three position
or eight velocity iterations. After position solving, Tau builds a compact
solver-active stream in persistent step scratch from the freshly refreshed
anchors and precomputes contact arms, normal effective mass, and penetration
bias. The eight-iteration loop no longer rescans sleeping contacts, refreshes anchors, or repeats normal-mass
work. Restitution and tangent/friction terms remain sequential and dynamic;
precomputing them would change either the algorithm or its floating-point
ordering.

A five-seed interleaved A/B used a temporary legacy oracle inside the same
Release binary, alternating old and new order by seed. The oracle was removed
after measurement:

| Workload | Legacy median / p95 | Prepared median / p95 | Improvement |
|---|---:|---:|---:|
| cube-only 1,500 active | 18.747 / 23.517 ms | 16.646 / 20.856 ms | 11.2% / 11.3% |
| sphere-only 1,500 settled | 2.363 / 2.657 ms | 2.224 / 2.516 ms | 5.9% / 5.3% |

The interleaving matters: independent long Windows runs showed upward p95
drift in every settled shape, while paired same-seed comparisons consistently
favored the prepared path. A final attribution run places the cube-active
velocity solve at 6.28-6.31 ms instead of 8.20 ms, approximately 23% lower,
and the complete Tau step at about 16.4-16.6 ms. Contact construction is now
the other leading target at 6.19-6.33 ms, with 3.78-3.87 ms in narrow phase.

The full C++ suite passes. Fresh QA passes include a byte-identical repeated
600-sample cuboid chain, the impulse callback with an unchanged -0.00141478 m
amplitude delta, and all capsule contact combinations. A tangent-mass matrix
experiment was explicitly rejected because reordered floating-point work
changed settled friction/sleep behavior. The next matrix candidate is thus
coherent cuboid feature reuse in the contact generator, not solver iteration
tuning.
