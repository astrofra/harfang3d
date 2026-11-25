# glTF transform-track export correction

Date: 2026-11-25. Implemented on `main`, following the [Colossus pipeline audit](AUDIT_GLTF_COLOSSUS_PIPELINE.md).

This report records the TRS-only validation stage. Skinning was subsequently implemented and tested in [the skinning validation report](GLTF_EXPORTER_SKINNING_VALIDATION.md).

## Change

The exporter now emits independent translation, scale, and rotation channels for each native node animation. It selects tracks by their semantic target, following the native binder's last-matching-track rule. Quaternion presence no longer suppresses translation and scale.

- Position keys retain the existing Z reflection.
- Scale keys retain all three signed components without a Z reflection.
- Rotation follows the native `AF_UseQuaternionForRotation` flag. Selected Euler keys are converted to VEC4 quaternions, using the same coordinate conversion as static node rotation. Existing quaternion-key conversion is retained.
- Constant and single-key tracks are retained, including clip-specific poses.
- Each clip has its own channels and samplers, retaining its name.
- Shared float-accessor serialization computes bounds from the actual converted values, correcting the quaternion W-bound defect.
- Invalid animation references and missing exported-node mappings are skipped with warnings, without silently assigning node zero.
- Empty selected tracks produce no accessors. Non-finite samples, negative times, and non-increasing float timestamps cause the channel to be skipped with a warning before any output buffers are committed. Duplicate node/property channels are also skipped with a warning.

The exporter remains a key-sample exporter using `LINEAR`. It does not yet bake native Hermite curves, reproduce native interpolation between rotation keys, trim/rebase clip intervals, export skins, or export animated material properties. A valid file and complete track coverage do not establish full native playback fidelity.

## Tests

The existing `gltf_exporter_tests` target now exercises production animation export as well as geometry. New checks cover simultaneous TRS tracks in shuffled order, ignored unknown targets, signed scales, translation reflection, quaternion bounds, Euler conversion, the native rotation-mode flag, distinct named clips, missing-node mappings, empty tracks, and float-time collisions. The geometry tests continue to pass.

Build and tests passed:

```sh
cmake --build build/studio-macos --target gltf_exporter gltf_exporter_tests -j 6
build/studio-macos/harfang/tools/gltf_converter/gltf_exporter_tests
```

The pre-existing nested-comment warning in the disabled tangent block remains.

## Colossus: four clips and 612 channels

Fresh exports used the previously compiled isolated asset snapshot from `build/gltf-colossus-audit/`. No source scene or geometry was changed.

| Clip | Before: channels | After: translation / rotation / scale |
| --- | ---: | ---: |
| `colossus_idle` | 51 | 51 / 51 / 51 |
| `colossus_walking` | 51 | 51 / 51 / 51 |
| `colossus_heavy_walking` | 51 | 51 / 51 / 51 |
| `colossus_crouched_walking` | 51 | 51 / 51 / 51 |
| Total | 204 | 204 / 204 / 204 |

All **408 previously omitted tracks** are now represented, including hips translation in all four clips and the 85 Euler rotation tracks omitted by the old selection logic.

The comparison checks prove:

- Position and scale output samples match every source key exactly after position-coordinate conversion.
- Their key counts and timestamps match the source within float-time precision.
- Existing quaternion output samples and times remain exactly equal to the earlier export.
- Every source node binding has a unique translation, rotation, and scale target; rotation outputs are VEC4.
- Mesh surface attributes, material assignments, nodes, and scene structure are unchanged from the earlier export.
- glTF and GLB animation structures and decoded samples are equal.

**The mesh remains unskinned.** Bone transforms and hips motion now have their missing tracks, but the exported surface is still not connected to those bones. Restoring tracks does not implement deformation.

## car_engine regression

The `Take 001` clip now contains 87 channels: **29 translations, 29 rotations, and 29 scales**, versus 29 rotation-only channels before. All six varying translations are restored. The same source-sample comparisons and geometry/material/scene regression checks passed.

## Khronos validation

Validator: `gltf-validator`, reported version `2.0.0-dev.3.10`.

| File | Errors before | Errors after | Warnings after |
| --- | ---: | ---: | ---: |
| Colossus glTF | 223 | 0 | 1 |
| Colossus GLB | 223 | 0 | 1,232 |
| car_engine glTF | 61 | 3 | 62 |

The three remaining car_engine errors are existing out-of-range light-color values. No animation errors remain on these fixtures. Colossus GLB warnings consist of the existing generated-tangent warning plus 1,231 URI warnings: adding channels creates more separately embedded buffers under the current GLB packaging strategy. Consolidating GLB buffers remains separate work.

Validation does not assess absent skinning, shader fidelity, or differences between native and glTF interpolation. No rendered comparison was performed.

## Reproduction and artifacts

Generated exports and logs are under `build/gltf-trs-validation/`, including:

- `colossus/colossus.gltf`, `colossus/colossus.glb`, and their validator reports.
- `car_engine/engine.gltf` and its validator report.
- `tests.log`, `comparison.json`, and `validation-summary.txt`.

The checked-in [comparison script](../tools/gltf_converter/tests/compare_animation_exports.py) accepts a native JSON scene, a baseline export, and a corrected export. It is intended for complete TRS fixtures such as these two scenes, not arbitrary clips with deliberately absent properties.

```sh
python3 harfang3d/tools/gltf_converter/tests/compare_animation_exports.py \
  build/gltf-colossus-audit/source/generated/anims/colossus.scn \
  build/gltf-colossus-audit/out/colossus.gltf \
  build/gltf-trs-validation/colossus/colossus.gltf

python3 harfang3d/tools/gltf_converter/tests/compare_animation_exports.py \
  build/gltf-vertex-audit/source/car_engine/engine.scn \
  build/gltf-vertex-audit/after/engine.gltf \
  build/gltf-trs-validation/car_engine/engine.gltf
```
