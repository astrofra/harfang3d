# glTF importer animation corrections

Date: 2026-11-25. Baseline: `51bd9da`, following the [skinning/accessor corrections](GLTF_IMPORTER_SKIN_ACCESSOR_VALIDATION.md).

## Result

Animation channels are now imported independently of sampler order and sharing. Translation, scale and rotation retain the correct target and coordinate conversion. Multiple clips remain separate, while clips with no supported targets in the current scene are omitted.

LINEAR and STEP use explicit native transform interpolation modes, retaining the original key counts. CUBICSPLINE values and tangents are decoded and adaptively sampled into linear/spherical tracks. Quaternion conversion uses direct basis reflection instead of an Euler roundtrip, both on import and export. STEP survives re-export to glTF.

Colossus retains **4 clips, 612 channels and 67,206 keys**. `car_engine` retains **1 clip, 87 channels and 2,887 keys**. At the 36 Colossus sample times, the maximum joint-world matrix component error relative to the input glTF falls from approximately **1.29e-4 to 6.06e-7**. Original native Colossus animation playback is unchanged.

## Native runtime and compatibility

Two serialized flags apply specifically to transform tracks in an `Anim`:

| Flag | Bit | Vec3 transform evaluation | Quaternion evaluation |
| --- | --- | --- | --- |
| `TransformLinearInterpolation` | `0x02` | Linear | Spherical, shortest path |
| `TransformStepInterpolation` | `0x04` | Hold preceding value | Hold preceding rotation |
| Neither flag | Existing default | Native Hermite | Existing component interpolation, normalized by the scene evaluator |

STEP takes precedence if both flags are supplied manually. The importer emits one mode per native animation, with one independent animation per glTF channel, so mixed interpolation between channels remains possible.

JSON serialization retains these flags. Binary serialization already stores bits `0x00–0x0f`, so its record layout does not change. Runtime dispatch is confined to node transforms; material, light and other native track behavior is unchanged.

**New imports must be compiled and played using the updated HARFANG runtime and asset compiler.** Older JSON readers ignore the new flag names, and older runtimes do not interpret the bits. Existing unflagged native assets retain their previous interpolation. The local engine, `assetc`, glTF tools and test executables were rebuilt; externally installed bindings or packaged applications are not updated by these source changes.

Sources: [animation.h](../harfang/engine/animation.h), [anim_load_json.cpp](../harfang/engine/anim_load_json.cpp), [scene.cpp](../harfang/engine/scene.cpp).

## Import and export behavior

The [importer](../tools/gltf_converter/gltf_importer.cpp) now visits channels, resolves each channel's sampler and target, validates the input/output accessors, then creates its native track. A sampler shared between translation and scale no longer shares a mutable native target. Unused samplers do not create spurious tracks or affect target lookup.

Validation rejects invalid sampler/node references, duplicate node/property targets within a clip, unsupported interpolation names, inconsistent accessor types/counts, non-finite data, undefined zero quaternions, negative/out-of-range times and timestamps that collapse at native nanosecond resolution. Errors identify the clip and channel where conversion failed. Unsupported targets such as morph weights and animation-pointer properties are warned about and skipped; support for those properties is not added here.

Translation reflects Z; scale is unchanged. Quaternion basis conversion is `(-x,-y,z,w)`. Cubic rotation tangents receive the same basis change, without independently changing their signs before curve evaluation. Emitted quaternion keys are normalized and hemisphere-conformed after sampling.

The [exporter](../tools/gltf_converter/gltf_exporter.cpp) emits STEP for native transform-step animations and LINEAR otherwise. Its quaternion conversion now normalizes and reflects directly. The former Euler conversion could introduce a tiny W sign change at 180 degrees, choosing the opposite interpolation arc on re-export. This case is covered by the integration tests. Existing unflagged native Hermite export remains a key-sample approximation; this batch does not implement general native-Hermite-to-glTF curve conversion.

Interpolation follows the [Khronos glTF animation specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#animations-interpolation): STEP holds values, rotation LINEAR uses spherical interpolation, and cubic output consists of incoming tangent/value/outgoing tangent triples. Cubic derivatives are multiplied by the source interval duration, and interpolated quaternion components are normalized.

## Cubic approximation policy

Only cubic channels add samples. Each source interval is evaluated using its original values, derivatives and duration. Subdivision compares the source curve with linear Vec3 or spherical quaternion playback at quarter, midpoint and three-quarter times.

- Vec3 subdivision threshold per component: `1e-5 + 1e-6 * max(abs(source), abs(approximation))`, in local track units.
- Quaternion subdivision threshold: squared sign-invariant chord distance `2.5e-11`, approximately `1e-5` radians.
- Nonconstant cubic intervals are subdivided to at most 16 ms, even when the sampled error is small. Constant cubic intervals with zero relevant tangents retain their endpoint keys.
- Source timestamps remain present. The native representation uses integer nanoseconds; conversion truncates fractional nanoseconds as before and rejects collapsed times.
- Conversion fails explicitly if the channel exceeds 100,000 emitted samples or the subdivision limit of 32. Undefined sampled quaternions and non-finite native output values also fail.

These are sampled local tolerances, **not a proven continuous-time or world-space error bound**. Parent scaling and skinning can amplify local error. Cubic curves are approximated, not stored losslessly with their original derivatives; their re-export uses the baked LINEAR keys. Large or sharply curved cubic inputs may generate substantially more keys.

## Verification

The [Python integration tests](../tools/gltf_converter/tests/test_importer_animation.py) generate glTF fixtures and compare playback against an independent Python implementation of the glTF interpolation equations. A [C++ runtime oracle](../tools/gltf_converter/tests/sample_scene_animation.cpp) calls native scene loading, binding and evaluation. Tests sample key times, adjacent nanoseconds, interval interiors and times outside the clip bounds.

Results:

- **11 animation tests pass:** 26 CLI cases comprising 16 positive fixtures and 10 invalid inputs rejected with exit status 1.
- **2,031 pose sample times** across positive fixtures, including multi-clip cases. The largest measured Vec3 component error is `1.36e-5`; the curved-quaternion fixture's maximum sign-invariant chord error is `4.59e-6`.
- Native JSON playback and binary save/reload playback produce identical sampled results.
- All 16 positive fixtures were also compiled by the rebuilt **assetc**; playback of those compiled scenes matches the JSON results exactly.
- Motion re-export uses the production exporter. All source clip/channel counts survive; STEP stays STEP. Re-exported curves are compared too, except the deliberate nanosecond-duration precision stress case, for which channel retention is checked.
- **33 input/re-export glTF/GLB files** pass Khronos validator `2.0.0-dev.3.10` with zero errors and zero warnings.
- All **17 skin/accessor tests** and the existing exporter geometry/animation/skinning tests pass. The native oracle checks legacy Hermite and quaternion behavior explicitly on every invocation.

Coverage includes reordered channels, shared samplers across properties/nodes, unused samplers, unequal key intervals, all three STEP transform types, antipodal and 180-degree quaternions, cubic overshoot and asymmetric derivatives, cubic rotations with signed keys, GLB, single-key channels, multi-clips, constant long cubic curves and invalid data.

Measured details: [GLTF_IMPORTER_ANIMATION_MEASUREMENTS.json](GLTF_IMPORTER_ANIMATION_MEASUREMENTS.json). Full logs and artifacts: `build/gltf-import-animation`; final real-asset results: `build/gltf-import-animation/final`.

## Colossus and car_engine

Fresh imports use the previously validated glTF/GLB exports. Geometry files, materials, textures and all non-animation scene fields match the preceding skin/accessor batch. Key counts do not increase because these assets use LINEAR channels.

| Colossus clip | Joint-world component error before | After | Maximum deformed-position error after |
| --- | ---: | ---: | ---: |
| Idle | 9.48e-7 | 6.06e-7 | 1.29e-4 |
| Walking | 4.37e-5 | 4.34e-7 | 1.83e-4 |
| Heavy walking | 1.29e-4 | 5.21e-7 | 1.94e-4 |
| Crouched walking | 5.86e-7 | 5.90e-7 | 2.48e-4 |

The CPU deformation comparison covers nine times per clip and all 3,821 vertices: **137,556 vertex/time comparisons**. It normalizes native byte weights by their actual total. Position errors still include weight quantization; not every maximum position error must decrease when animation error decreases. Native weight packing is unchanged by this batch.

The original native Colossus scene was also re-evaluated with the rebuilt engine. Its entire saved CPU reference, including joint matrices and deformed positions, matches the earlier reference exactly. This verifies the legacy fallback on the real asset as well as the synthetic tests. No GPU screenshot comparison was performed.

## Reproduction

From the workspace root:

```sh
cmake --build build/studio-macos --target \
  gltf_importer gltf_exporter gltf_exporter_tests gltf_importer_animation_tests assetc -j 6

python3 harfang3d/tools/gltf_converter/tests/test_importer_animation.py \
  --importer build/studio-macos/harfang/tools/gltf_converter/gltf_importer \
  --sampler build/studio-macos/harfang/tools/gltf_converter/gltf_importer_animation_tests \
  --exporter build/studio-macos/harfang/tools/gltf_converter/gltf_exporter_tests \
  --out build/gltf-import-animation

python3 harfang3d/tools/gltf_converter/tests/test_importer_skin_accessors.py \
  --importer build/studio-macos/harfang/tools/gltf_converter/gltf_importer \
  --out build/gltf-import-animation

build/studio-macos/harfang/tools/gltf_converter/gltf_exporter_tests
```

Each Python run creates a fresh artifact directory. The native sampling tool accepts either source JSON or compiled binary scene files; its optional `--binary-roundtrip` argument exercises native serialization. The exporter test tool's `--animation-export scene.scn output.gltf` mode exports only nodes and their animation, without requiring GPU resources.
