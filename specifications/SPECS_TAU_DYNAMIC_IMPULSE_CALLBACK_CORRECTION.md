# Tau Dynamic Impulse Callback: Amplitude And Correction Proposal

## Scope

This note compares Bullet and Tau on
`physics-qa/rb_dynamic_impulse_callback.lua` after 600 deterministic fixed
steps at 60 Hz. The two cubes in each capture are equivalent within the QA's
`1e-4` position/velocity tolerance, so body 1 is representative.

The capture changes its vertical target from 2 m to 4.358526746242 m at 5 s.
Both backends use the same seeded target and the same position sampled before
`SceneUpdateSystems`; this is important because a pre-tick callback runs while
the scene world-matrix cache is invalid.

## Measured Motion

| Metric | Bullet | Tau | Tau - Bullet |
|---|---:|---:|---:|
| Full vertical range | 2.34376490 m | 2.34235012 m | -0.00141478 m |
| Minimum Y | 1.67482603 m | 1.67500055 m | +0.00017452 m |
| Maximum Y | 4.01859093 m | 4.01735067 m | -0.00124026 m |
| Mean absolute position error | - | 0.00328005 m | - |
| RMS position error | - | 0.00646979 m | - |
| Maximum position error | - | 0.03582168 m | - |

Tau's peak-to-peak amplitude is only 1.415 mm lower, a relative difference of
about 0.060%. Scaling impulses, gravity, mass, damping, or contact response to
fix this would be disproportionate and would risk regressions in unrelated
physics scenarios.

The visible error is primarily temporal. At a fixed 60 Hz step, Bullet's
`SyncTransformsToScene` publishes an interpolated previous/current transform,
whereas Tau publishes the current solved transform directly. Tau is therefore
ahead during the sharp target transition even though the response amplitude
and velocity envelope closely match.

## Corrective Trajectory

The conservative proposal is a display-only half-step interpolation:

```text
display_position = lerp(current_position, previous_position, 0.5)
display_rotation = slerp(current_orientation, previous_orientation, 0.5)
```

Applied to the recorded Tau states, this proposal gives:

| Metric | Raw Tau | Proposed display trajectory | Improvement |
|---|---:|---:|---:|
| Mean absolute position error | 3.280 mm | 3.144 mm | 4.2% |
| RMS position error | 6.470 mm | 4.619 mm | 28.6% |
| Maximum position error | 35.822 mm | 17.619 mm | 50.8% |
| Peak-to-peak amplitude | 2.342350 m | 2.342211 m | remains within 1.56 mm of Bullet |

This trajectory deliberately uses a half-step instead of a coefficient fitted
to this one capture. A fitted history weight near 0.685 minimizes this
scenario's maximum error, but hard-coding that value would be an unjustified
QA-specific calibration.

## Implementation Path Without Solver Regression

1. Keep Tau integration, contacts, impulses, velocities, and collision events
   unchanged.
2. Add a small internal helper that builds a display transform from
   `TauNode::previous_position/orientation` and the current solved state.
3. Apply it only in `SyncTransformsToScene`; physics queries continue to read
   the current solver state.
4. Initially make the interpolation coefficient selectable internally and
   sweep `0`, `0.5`, and the accumulator-derived value across the full QA
   matrix. Do not adopt `0.5` globally unless aggregate results improve.
5. If another scenario regresses, preserve current Tau synchronization as the
   default and expose the Bullet-compatible display interpolation as an
   explicit compatibility mode.

## Non-Regression Gates

Before changing Tau synchronization, require all of the following:

- `engine.scene_tau_physics` and `engine.scene_tau_physics_contact` unit tests
  pass.
- The callback QA keeps exactly one callback per fixed step and retains its
  intra-backend `1e-4` cube-parity check.
- Callback amplitude difference stays below 5 mm, RMS position error below
  5 mm, and maximum position error below 20 mm.
- `compare_physics_dumps.py` remains green for variable friction,
  restitution, and rolling-friction captures.
- `validate_tau_rings_chain.py` remains green, including a byte-identical Tau
  repeat capture.
- No contact, velocity, or constraint solver code changes are bundled with the
  display interpolation patch.

The generated diagnostic is
`physics-qa/qa_dumps/rb_dynamic_impulse_callback_amplitude.png`; regenerate it
with `physics-qa/analyze_impulse_callback_amplitude.py`.
