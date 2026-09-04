# Tau Physics Pool Complete Performance Matrix

Date: 2026-09-04

Status: complete for the deterministic physics-only acceptance rerun after
ordered dense ownership and hot/cold body-data separation. The 2026-09-02 and
2026-09-03 matrices are retained below as historical baselines; the current
physics results and next-stage decision are in the final section. Controlled
scene/render results were not rerun because this slice changes only Tau's
internal physics storage.

Related roadmap:
`specifications/SPECS_TAU_POOL_OF_OBJECTS_PERFORMANCE_ROADMAP.md`.

## Outcome

Tau now beats Bullet on the roadmap's representative 1,500-body
mixed-workload gates
without reducing the solver's three position or eight velocity iterations:

- active physics p95: 12.945 ms for Tau versus 15.264 ms for Bullet, or
  **0.848x**;
- settled physics p95: 3.280 ms for Tau versus 15.684 ms for Bullet, or
  **0.209x**;
- rendered active p95: 24.616 ms for Tau versus 26.840 ms for Bullet, or
  **0.917x** in the retained 2026-09-03 control;
- rendered settled p95: 16.829 ms for Tau versus 27.661 ms for Bullet, or
  **0.608x** in the retained 2026-09-03 control.

Across all 30 physics cells, Tau's sum-time median ratio is **0.555x** and its
equal-cell median ratio is **0.621x**. Active sum-time parity is retained at
0.973x by median and 0.980x by p95. The remaining exception is active
cube-only physics, which is 1.12x to 1.56x slower by median. The stretch goal
therefore remains open because the same six median cells and seven p95 cells
exceed 1.10x.

## 2026-09-02 Baseline Protocol

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

## 2026-09-02 Baseline Physics-only Matrix

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

## 2026-09-02 Baseline Aggregate Views

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

## 2026-09-02 Baseline Scene And Render Controls

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

## 2026-09-02 Baseline Acceptance Gates

| Roadmap gate | Result | Evidence |
|---|---|---|
| comparable deterministic inputs and fixed steps | PASS | Identical seeded creation protocol and one exact 1/60 s substep per physics sample |
| algorithmic scaling | PASS | Dynamic broad phase and O(1) persistent manifold lookup were established in earlier milestones |
| 1,500 mixed settled p95 no more than 1.10x | PASS | 0.237x |
| 1,500 mixed active p95 no more than 1.10x | PASS | 1.064x |
| rendered end-to-end no more than 1.10x | PASS | 1.005x active and 0.618x settled by p95 |
| stretch: at least 15% faster with no cell over 1.10x | FAIL | Equal-cell median is 14.4% faster, but cube-active cells and the 250-cube settled cell exceed 1.10x |

## 2026-09-02 Baseline Hotspot Attribution

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

The corrective sequence remains behavior-preserving and keeps solver iteration
counts unchanged:

1. **Completed:** exploit active cuboid manifold coherence. Refresh cached local anchors and
   validate the previous separating/contact features before falling back to a
   complete SAT and clipping pass. Preserve deterministic contact ordering and
   use the current generator whenever the cached feature is invalid.
2. **Completed:** add measured coefficient fast paths for zero restitution,
   non-positive friction, and world-wide zero rolling friction, without
   changing the general nonzero material paths.
3. **Completed:** add an all-sleeping fast path for small worlds when no proxy moved, no wake
   request is pending, and no tracked-contact publication requires traversal.
   This targets the 0.287 ms fixed floor revealed by the 250-cube cell.
4. **Completed:** re-run the complete matrix and the existing physics QA after every stage.
   Accept no active cube regression and no contact, wake, callback, chain, or
   raycast regression.
5. **Next:** replace hot `std::map` body traversal and scattered node access
   with stable dense body slots plus a `NodeRef` lookup. Re-profile the active
   cube cell before considering independent-island parallelism. Scene-sync
   work is lower priority because the controlled scene pass already follows
   physics-only performance.

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
changed settled friction/sleep behavior.

## Post-Matrix Follow-Up: Coherent Cuboid Manifold Refresh

The next cube-specific stage reuses a previous FaceA/FaceB feature and its
ordered body-local anchors when relative motion, normal alignment, incident
face selection, cached-axis overlap, and per-point anchor separation all remain
compatible. It falls back to the complete 15-axis SAT and clipping generator
on any failed check, for every edge/edge feature, and unconditionally after
four consecutive coherent refreshes. Warm-start feature IDs and constraint
order are retained; solver iteration counts remain three and eight.

A five-seed, same-binary interleaved A/B on the 1,500-cube active cell produced:

| Path | Median | p95 | Improvement |
|---|---:|---:|---:|
| prepared solver, full cuboid generation | 16.515 ms | 21.120 ms | baseline |
| prepared solver, coherent cuboid refresh | 16.076 ms | 19.955 ms | 2.7% / 5.5% |

Three attribution runs reduce narrow phase from 3.78-3.87 ms to 3.20-3.23 ms
and complete contact construction from 6.19-6.33 ms to 5.55-5.66 ms. A
diagnostic active step reused 1,163 cuboid manifolds and 2,887 points without
full SAT/clipping, approximately 38% of the emitted cuboid manifolds. The full
C++ suite and the deterministic chain envelope pass; the impulse callback
amplitude delta remains -0.00141478 m.

## Post-Matrix Follow-Up: Coefficient Fast Paths

The prepared constraint stream now classifies restitution and friction while
it is built. Zero-restitution constraints skip their warm-start point-velocity
evaluation. Non-positive-friction constraints clear any cached tangent impulse
and skip tangent solving in all eight velocity iterations. Contact construction
also detects a world with no nonzero rolling-friction body and omits the full
rolling contact pass. Any nonzero rolling-friction body conservatively retains
the original general pass, and all nonzero material arithmetic remains in its
original order.

A five-seed same-binary interleaved A/B on the 1,500-cube active cell produced:

| Path | Median | p95 | Improvement |
|---|---:|---:|---:|
| prepared/coherent solver, coefficient work retained | 16.183 ms | 19.721 ms | baseline |
| coefficient fast paths | 15.991 ms | 19.477 ms | 1.2% / 1.2% |

At active step 300, all 6,122 solver-active constraints had zero restitution,
all had positive friction, and the rolling pass was skipped. The workload thus
validates that the measured gain comes from the restitution and rolling guards;
the friction branch is covered by focused tests and the variable-friction QA.
Legacy/fast captures for variable restitution, variable friction, variable
rolling friction, and the impulse callback are byte-identical. The complete
C++ suite, capsule pairs, deterministic chain envelope, and callback amplitude
gate pass with no solver iteration change.

The next matrix candidate at that milestone was the guarded small-world all-sleeping shortcut.
Iteration tuning remains deferred.

## Post-Matrix Follow-Up: All-Sleeping Small Worlds

Worlds containing at most 512 physics bodies now skip a complete Tau substep
when they contain at least one dynamic body, all dynamics are sleeping, every
proxy is valid and unmoved, no structural or support change requires service,
all force/torque accumulators are zero, and collision-event tracking is empty.
The pre-tick callback still executes before the decision, so callback-driven
wakes are processed immediately. The contact epoch is frozen while skipping,
keeping the last persistent manifolds eligible for warm start after wake.

Creation, destruction, garbage collection, reset, teleport, and synchronized
support motion force a full substep. Static and kinematic support removal or
replacement also explicitly wakes dependent sleeping bodies. Tracking keeps
the existing contact-publication path, and worlds above the 512-body limit
reject before scanning their nodes.

A five-seed, same-binary interleaved A/B with 120-sample settled windows
produced:

| Cube-only settled workload | Legacy median / p95 | Fast median / p95 | Improvement |
|---|---:|---:|---:|
| 250 dynamic + 5 static bodies | 262.7 / 270.5 us | 3.0 / 3.1 us | 98.9% / 98.9% |
| 500 dynamic + 5 static bodies | 794.6 / 814.7 us | 7.6 / 7.8 us | 99.0% / 99.0% |

One 250-body seed transitioned to the accepted state during measurement; the
table reports cross-seed medians. A fully sleeping same-seed run measured Tau
at 2.8 us and Bullet at 72.2 us. Active-window pairwise mean changes average
+0.1% at 250 bodies and +0.5% at 500 bodies, with mixed signs and no consistent
regression. Focused callback, warm-start, tracking, support-removal, support-
motion, and body-limit tests pass. The full C++ suite, byte-identical chain and
callback captures, chain envelope, callback amplitude, and capsule checks also
pass without an iteration change.

The fresh complete acceptance matrix below selects dense body storage before
independent-island parallelism. Iteration tuning remains deferred.

## 2026-09-03 Complete Acceptance Rerun

Both Release packages were rebuilt and installed from revision
`b492e916ec6a69d25ce8ed61f8287aa71cc0fe2c` from a clean source tree. The
rerun used the original protocol unchanged: five seeds from 5,521,749, 120
samples, 10 warm-up steps, a 600-step settling transition, and alternating
backend order per body-count/shape cell. Profiling and contact diagnostics were
disabled for all comparative samples.

### Current Physics-Only Matrix

#### Active window

| Bodies | Shapes | Bullet median | Tau median | Tau/Bullet | Bullet p95 | Tau p95 | Tau/Bullet p95 |
|---:|---|---:|---:|---:|---:|---:|---:|
| 250 | mixed | 0.373 ms | 0.366 ms | 0.980x | 0.931 ms | 1.226 ms | 1.317x |
| 250 | cube | 0.383 ms | 0.435 ms | 1.134x | 0.903 ms | 1.748 ms | 1.936x |
| 250 | sphere | 0.326 ms | 0.290 ms | 0.891x | 0.813 ms | 0.634 ms | 0.780x |
| 500 | mixed | 1.268 ms | 1.552 ms | 1.225x | 2.555 ms | 3.027 ms | 1.185x |
| 500 | cube | 1.332 ms | 2.070 ms | 1.554x | 2.452 ms | 4.589 ms | 1.872x |
| 500 | sphere | 1.074 ms | 0.949 ms | 0.884x | 2.173 ms | 1.683 ms | 0.774x |
| 1,000 | mixed | 5.341 ms | 5.571 ms | 1.043x | 7.842 ms | 7.523 ms | 0.959x |
| 1,000 | cube | 5.193 ms | 8.005 ms | 1.542x | 7.246 ms | 11.571 ms | 1.597x |
| 1,000 | sphere | 4.474 ms | 3.285 ms | 0.734x | 7.065 ms | 4.306 ms | 0.609x |
| 1,500 | mixed | 12.465 ms | 10.820 ms | 0.868x | 14.715 ms | 13.091 ms | 0.890x |
| 1,500 | cube | 11.405 ms | 16.170 ms | 1.418x | 13.251 ms | 19.557 ms | 1.476x |
| 1,500 | sphere | 11.140 ms | 6.696 ms | 0.601x | 13.908 ms | 8.115 ms | 0.584x |
| 2,000 | mixed | 20.168 ms | 16.947 ms | 0.840x | 22.612 ms | 19.608 ms | 0.867x |
| 2,000 | cube | 18.796 ms | 24.816 ms | 1.320x | 20.504 ms | 28.241 ms | 1.377x |
| 2,000 | sphere | 17.541 ms | 10.360 ms | 0.591x | 20.103 ms | 11.644 ms | 0.579x |

#### Settled window

| Bodies | Shapes | Bullet median | Tau median | Tau/Bullet | Bullet p95 | Tau p95 | Tau/Bullet p95 |
|---:|---|---:|---:|---:|---:|---:|---:|
| 250 | mixed | 0.785 ms | 0.253 ms | 0.322x | 0.809 ms | 0.262 ms | 0.324x |
| 250 | cube | 0.073 ms | 0.003 ms | 0.041x | 0.075 ms | 0.003 ms | 0.041x |
| 250 | sphere | 0.639 ms | 0.257 ms | 0.401x | 0.666 ms | 0.277 ms | 0.416x |
| 500 | mixed | 2.260 ms | 0.643 ms | 0.285x | 2.485 ms | 0.676 ms | 0.272x |
| 500 | cube | 2.529 ms | 0.008 ms | 0.003x | 2.826 ms | 0.008 ms | 0.003x |
| 500 | sphere | 1.521 ms | 0.489 ms | 0.321x | 1.669 ms | 0.536 ms | 0.321x |
| 1,000 | mixed | 7.502 ms | 1.689 ms | 0.225x | 8.288 ms | 1.791 ms | 0.216x |
| 1,000 | cube | 7.759 ms | 2.097 ms | 0.270x | 8.459 ms | 2.224 ms | 0.263x |
| 1,000 | sphere | 5.386 ms | 1.146 ms | 0.213x | 6.288 ms | 1.203 ms | 0.191x |
| 1,500 | mixed | 14.880 ms | 3.048 ms | 0.205x | 15.671 ms | 3.440 ms | 0.220x |
| 1,500 | cube | 15.017 ms | 3.864 ms | 0.257x | 15.997 ms | 4.375 ms | 0.273x |
| 1,500 | sphere | 12.104 ms | 2.301 ms | 0.190x | 12.975 ms | 3.084 ms | 0.238x |
| 2,000 | mixed | 22.593 ms | 4.805 ms | 0.213x | 23.820 ms | 5.368 ms | 0.225x |
| 2,000 | cube | 22.008 ms | 6.001 ms | 0.273x | 23.382 ms | 6.588 ms | 0.282x |
| 2,000 | sphere | 19.197 ms | 3.321 ms | 0.173x | 20.452 ms | 4.013 ms | 0.196x |

The all-sleeping fast path removes the former 250-cube fixed-cost outlier.
Four of five 250-cube seeds and three of five 500-cube seeds were fully asleep
during the settled window; using the median-of-five rule correctly selects the
3 us and 8 us fast-path populations. Seeds that had not yet fully converged
continued through the complete solver path and were not discarded.

### Current Aggregate Views

| Scope | Cells | Equal-cell median ratio | Equal-cell p95 ratio | Sum-time median ratio | Sum-time p95 ratio |
|---|---:|---:|---:|---:|---:|
| complete suite | 30 | 0.634x | 0.676x | 0.563x | 0.607x |
| active | 15 | 1.042x | 1.120x | 0.974x | 0.996x |
| settled | 15 | 0.226x | 0.232x | 0.223x | 0.235x |
| mixed, both phases | 10 | 0.621x | 0.647x | 0.521x | 0.562x |
| cube, both phases | 10 | 0.781x | 0.912x | 0.751x | 0.830x |
| sphere, both phases | 10 | 0.500x | 0.469x | 0.396x | 0.412x |

Compared with the 2026-09-02 matrix, the active equal-cell median ratio falls
from 1.177x to 1.042x and the active sum-time median ratio falls from 1.150x
to 0.974x. The complete sum-time median ratio improves from 0.658x to 0.563x.
Six median cells and seven p95 cells remain above 1.10x; every median failure
is active, five of six are cube-only, and the remaining one is the 500-body
mixed cell.

### Current Scene And Render Controls

The valid render pass was launched from `tutorials` so that
`resources_compiled` resolved and the default forward shaders loaded. An
initial asset-less launch was discarded before aggregation.

| Mode | Window | Bullet median | Tau median | Ratio | Bullet p95 | Tau p95 | p95 ratio |
|---|---|---:|---:|---:|---:|---:|---:|
| scene + physics | active | 12.876 ms | 10.959 ms | 0.851x | 15.306 ms | 13.240 ms | 0.865x |
| scene + physics | settled | 15.181 ms | 3.174 ms | 0.209x | 15.949 ms | 3.605 ms | 0.226x |
| bare scene, no physics | active | 0.064 ms | 0.064 ms | 1.002x | 0.068 ms | 0.069 ms | 1.016x |
| bare scene, no physics | settled | 0.064 ms | 0.064 ms | 1.002x | 0.069 ms | 0.069 ms | 1.013x |
| rendered + physics | active | 24.225 ms | 22.380 ms | 0.924x | 26.840 ms | 24.616 ms | 0.917x |
| rendered + physics | settled | 26.735 ms | 16.698 ms | 0.625x | 27.661 ms | 16.829 ms | 0.608x |
| rendered, no physics | active | 16.697 ms | 16.700 ms | 1.000x | 16.941 ms | 16.984 ms | 1.003x |
| rendered, no physics | settled | 16.697 ms | 16.698 ms | 1.000x | 16.926 ms | 16.944 ms | 1.001x |

### Current Acceptance And Attribution

The representative mixed-workload physics, scene, and rendered parity gates
all pass. The stretch gate remains open because active cube cells exceed
1.10x even though the complete weighted suite is substantially faster.

Three attribution-only runs of the 1,500-cube active cell report:

| Tau scope | Average per step |
|---|---:|
| velocity solve | 6.31 ms |
| contact construction | 5.63 ms |
| narrow phase, included in contact construction | 3.22 ms |
| position solve | 3.04 ms |
| broad phase | 1.78 ms |
| proxy update | 0.524 ms |
| island construction | 0.232 ms |
| complete Tau step | 15.87 ms |

A diagnostic sample at contact step 300 contained 1,303 awake bodies, 196
sleep candidates, one sleeper, 146 contact islands, 3,085 manifolds, and 5,897
contact points. The dense pile still drives the solver and contact cost, while
island construction itself is only 0.232 ms. The next stage is therefore
stable dense body slots and hot-loop data locality before island parallelism.

All 54 C++ test groups pass. Two fresh 600-sample Tau chain captures are
byte-identical and pass the established envelope at 4.64718 m maximum vertical
error, 16.4712 m/s maximum Tau speed, and zero static drift. The callback gate
remains at 0.03582168 m maximum position error and -0.00141478 m amplitude
delta. Capsule/sphere, capsule/cuboid, capsule/capsule, all analytic raycast
primitives, the 361/80 mesh raycast, and the rotating-terrain 403/3,348
raycast checks pass. Chair, friction, restitution, and rolling-friction
captures also completed for both backends; their generic strict trajectory
comparison continues to report backend-specific differences and is retained
as an inspection report rather than a parity gate.

## 2026-09-04 Hot/Cold Storage Acceptance Rerun

Both Release packages were run from the same source tree based on revision
`e248efdfd1cb18dc8f91e716dbf4f051ffa56388`, with the staged dense-storage and
hot/cold Tau changes subsequently recorded as commit
`8d1aee6f7d87da7c7dcac2755ce11c76ee2ac9e1`. The measured physics sources are
byte-identical to that commit. The original protocol was preserved: five seeds
from 5,521,749, 120 samples, 10 warm-up steps, a 600-step settling transition,
and alternating backend order per body-count/shape cell. Profiling and contact
diagnostics were disabled for all comparative samples.

An initial attempt was rejected before aggregation when an external `ollama`
process occupied one complete CPU core and inflated both backends by 40-80%.
Those partial files are retained under an explicit `invalid-system-load`
suffix. A quiet 1,500-cube smoke then reproduced the established timing range,
and a process-load guard stayed green throughout the complete rerun. The
accepted `build/tau-pool-matrix-20260904-hot-cold` artifact directory contains
30 physics JSONL files and 300 records.

### Hot/Cold Physics-Only Matrix

#### Active window

| Bodies | Shapes | Bullet median | Tau median | Tau/Bullet | Bullet p95 | Tau p95 | Tau/Bullet p95 |
|---:|---|---:|---:|---:|---:|---:|---:|
| 250 | mixed | 0.372 ms | 0.350 ms | 0.940x | 0.938 ms | 1.199 ms | 1.278x |
| 250 | cube | 0.382 ms | 0.428 ms | 1.121x | 0.921 ms | 1.716 ms | 1.863x |
| 250 | sphere | 0.325 ms | 0.281 ms | 0.864x | 0.797 ms | 0.607 ms | 0.762x |
| 500 | mixed | 1.279 ms | 1.550 ms | 1.212x | 2.582 ms | 2.996 ms | 1.161x |
| 500 | cube | 1.297 ms | 2.003 ms | 1.544x | 2.510 ms | 4.553 ms | 1.814x |
| 500 | sphere | 1.062 ms | 0.892 ms | 0.840x | 2.174 ms | 1.608 ms | 0.739x |
| 1,000 | mixed | 5.448 ms | 5.607 ms | 1.029x | 7.937 ms | 7.506 ms | 0.946x |
| 1,000 | cube | 5.175 ms | 8.078 ms | 1.561x | 7.174 ms | 11.501 ms | 1.603x |
| 1,000 | sphere | 4.504 ms | 3.199 ms | 0.710x | 7.227 ms | 4.160 ms | 0.576x |
| 1,500 | mixed | 12.096 ms | 10.740 ms | 0.888x | 15.264 ms | 12.945 ms | 0.848x |
| 1,500 | cube | 11.312 ms | 15.778 ms | 1.395x | 13.193 ms | 19.084 ms | 1.446x |
| 1,500 | sphere | 10.635 ms | 6.296 ms | 0.592x | 13.129 ms | 7.532 ms | 0.574x |
| 2,000 | mixed | 19.773 ms | 16.909 ms | 0.855x | 22.283 ms | 19.246 ms | 0.864x |
| 2,000 | cube | 18.432 ms | 24.657 ms | 1.338x | 20.209 ms | 27.870 ms | 1.379x |
| 2,000 | sphere | 17.730 ms | 10.056 ms | 0.567x | 20.323 ms | 11.368 ms | 0.559x |

#### Settled window

| Bodies | Shapes | Bullet median | Tau median | Tau/Bullet | Bullet p95 | Tau p95 | Tau/Bullet p95 |
|---:|---|---:|---:|---:|---:|---:|---:|
| 250 | mixed | 0.785 ms | 0.236 ms | 0.301x | 0.826 ms | 0.259 ms | 0.314x |
| 250 | cube | 0.073 ms | 0.002 ms | 0.028x | 0.079 ms | 0.002 ms | 0.025x |
| 250 | sphere | 0.635 ms | 0.242 ms | 0.381x | 0.668 ms | 0.297 ms | 0.445x |
| 500 | mixed | 2.239 ms | 0.601 ms | 0.269x | 2.543 ms | 0.649 ms | 0.255x |
| 500 | cube | 2.508 ms | 0.006 ms | 0.002x | 2.877 ms | 0.006 ms | 0.002x |
| 500 | sphere | 1.503 ms | 0.439 ms | 0.292x | 1.659 ms | 0.503 ms | 0.303x |
| 1,000 | mixed | 7.318 ms | 1.568 ms | 0.214x | 8.180 ms | 1.725 ms | 0.211x |
| 1,000 | cube | 7.752 ms | 1.983 ms | 0.256x | 8.408 ms | 2.193 ms | 0.261x |
| 1,000 | sphere | 5.362 ms | 1.045 ms | 0.195x | 6.277 ms | 1.172 ms | 0.187x |
| 1,500 | mixed | 14.678 ms | 2.842 ms | 0.194x | 15.684 ms | 3.280 ms | 0.209x |
| 1,500 | cube | 14.817 ms | 3.544 ms | 0.239x | 15.473 ms | 4.156 ms | 0.269x |
| 1,500 | sphere | 11.652 ms | 2.031 ms | 0.174x | 12.452 ms | 2.254 ms | 0.181x |
| 2,000 | mixed | 22.489 ms | 4.458 ms | 0.198x | 23.471 ms | 5.239 ms | 0.223x |
| 2,000 | cube | 21.788 ms | 5.733 ms | 0.263x | 23.492 ms | 6.253 ms | 0.266x |
| 2,000 | sphere | 18.891 ms | 3.024 ms | 0.160x | 20.247 ms | 3.598 ms | 0.178x |

### Hot/Cold Aggregate Views

The equal-cell columns are arithmetic means of the per-cell ratios; the
sum-time columns divide the sum of Tau cell times by the matching Bullet sum.

| Scope | Cells | Equal-cell median ratio | Equal-cell p95 ratio | Sum-time median ratio | Sum-time p95 ratio |
|---|---:|---:|---:|---:|---:|
| complete suite | 30 | 0.621x | 0.658x | 0.555x | 0.593x |
| active | 15 | 1.030x | 1.094x | 0.973x | 0.980x |
| settled | 15 | 0.211x | 0.222x | 0.209x | 0.222x |
| mixed, both phases | 10 | 0.610x | 0.631x | 0.519x | 0.552x |
| cube, both phases | 10 | 0.775x | 0.893x | 0.745x | 0.820x |
| sphere, both phases | 10 | 0.478x | 0.450x | 0.380x | 0.390x |

Every aggregate improves over the 2026-09-03 matrix. Complete equal-cell
median/p95 ratios move from 0.634x/0.676x to 0.621x/0.658x, and complete
sum-time ratios move from 0.563x/0.607x to 0.555x/0.593x. Active equal-cell
ratios improve from 1.042x/1.120x to 1.030x/1.094x; active sum-time stays at
parity and improves primarily in the tail, from 0.974x/0.996x to
0.973x/0.980x. Settled sum-time reaches 0.209x/0.222x.

The six median and seven p95 cells above 1.10x are the same workload families
as before. Five median failures are active cube-only cells; the sixth is the
500-body active mixed cell. The extra p95 failure is the 250-body active mixed
cell. No Tau cell regresses by 10% in absolute median or p95. The only active
Tau median increases are 0.6% for 1,000 mixed and 0.9% for 1,000 cubes, both
inside normal cross-run variation. The 1,500 active-cube ratios improve from
1.418x/1.476x to 1.395x/1.446x.

### Hot/Cold Attribution And Next Decision

Three attribution-only repetitions of the 1,500-cube active cell report:

| Tau scope | Average per step |
|---|---:|
| velocity solve | 6.13 ms |
| contact construction | 5.62 ms |
| narrow phase, included in contact construction | 3.26 ms |
| position solve | 3.06 ms |
| broad phase | 1.76 ms |
| proxy update | 0.502 ms |
| island construction | 0.226 ms |
| complete Tau step | 15.67 ms |

The hot/cold result is accepted: aggregate ratios improve, the cuboid hotspot
moves in the intended direction, no cell crosses the 10% regression limit,
and the previously established byte-identical chain and callback gates remain
applicable because no simulation code changed after those captures.

The next bounded slice should compact the mutable velocity-constraint working
set before implementing island parallelism. Each of the eight velocity passes
currently streams `TauVelocityConstraint` records and follows a pointer into
the much larger `TauContactConstraint` array for normals, coefficients,
accumulated impulses, and body pointers. A compact AoS/AoSoA or split-array
working set can keep those repeatedly accessed fields together, preserve the
existing contact order, and publish accumulated impulses back once after the
iterations. The position pass can be evaluated separately after measuring the
velocity-only slice.

Independent-island parallelism remains a later stretch stage. Island building
is only 0.226 ms, and the scene structure plus existing island/contact counts
indicate that expensive pile contacts are concentrated in one dominant
component while many remaining islands are low-work falling or disconnected
bodies. This is an inference from the current counters and workload topology;
an island-size histogram should be added before committing to a parallel work
scheduler. Solver iteration counts remain three position and eight velocity
passes.
