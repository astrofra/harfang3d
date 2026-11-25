# glTF skinning implementation and validation

Date: 2026-11-25. Implemented on `main`, on top of the uncommitted [TRS track correction](GLTF_EXPORTER_TRS_TRACK_VALIDATION.md).

## Result

Colossus now exports one skin with **40 distinct joints**, 40 inverse bind matrices, and `JOINTS_0`/`WEIGHTS_0` attributes for all **3,821 render vertices**. The `Layer` node references this skin. All four clips retain their 153 transform channels each, and the surface deforms when those channels are evaluated.

Both `.gltf` and `.glb` pass Khronos validation with **zero errors**. An independent CPU glTF evaluator was compared against the actual native HARFANG animation evaluator at nine times in each clip. This verifies deformation numerically; no viewer screenshot or rendered normal-mapping comparison was performed.

## Implementation

Source: [gltf_exporter.cpp](../tools/gltf_converter/gltf_exporter.cpp).

Geometry conversion retains the native vertex ID for each compact output record. Seam-split records therefore inherit the same native vertex weights, while disconnected native IDs remain distinct. The geometry cache retains native skin records, bind matrices, and the source-ID mapping.

Skin resolution runs after all scene nodes are exported. This permits joints to be siblings or later roots, rather than requiring them to have been visited when the mesh was created. The object supplies its bone-node palette, so objects sharing geometry resolve their skins independently.

For each object:

1. Check palette size, bind matrices, exported joint references, source vertex references, and influence indices.
2. Canonicalize repeated references to the same joint node only when their bind matrices are exactly equal. Reject conflicting matrices for one node rather than silently choosing one.
3. Remap the native palette to the distinct glTF joint list and combine influences that now target the same joint.
4. Normalize each vertex's nonzero byte-weight total, then emit four unsigned-short joint indices and four float weights. Unused components have zero weight and joint index zero.
5. Convert each native affine matrix using `C * B * C`, where `C = diag(1, 1, -1, 1)`, expand to 4 × 4, and serialize in column-major order.
6. Attach the attributes to every primitive of that mesh instance, write the inverse-bind accessor, and assign the skin to its node.

The native renderer uses `bone_world * bind_pose`. The glTF world-space deformation used in validation is the corresponding sum of `joint_world * inverse_bind * position`; the mesh-local inverse and mesh world transform cancel in that expression. No extra mesh transform is baked into the bind matrices.

Missing/filtered joints, conflicting bind matrices, zero-total weights, out-of-range slots, and invalid weight-array lengths are rejected. A failed skin causes scene export to return failure before writing a new output file, rather than deliberately exporting that object as a static replacement. Existing output files are not deleted on failure. Existing unrelated missing-model handling is not redesigned by this patch.

Global export caches are reset at the start of a scene export, preventing stale node/accessor references across calls.

## Source normalization policy

Colossus has 80 native palette slots representing two identical 40-entry node/bind-matrix lists. All duplicate pairs were verified before they were collapsed. This rule is data-driven, not a Colossus-specific index shortcut.

The source has 484 vertices whose byte weights do not sum to 255. Their sums range from 246 to 256. The exporter divides each combined influence by the actual vertex total, not unconditionally by 255, and logs how many output records required normalization. Zero-total vertices are rejected.

This intentionally corrects the source's weight sums. The native reference used below applies the same normalization policy; it does **not** claim byte-for-byte equivalence to unnormalized native GPU weight behavior. It isolates palette, matrix, coordinate, and animation fidelity from the normalization correction.

## Automated tests

The explicit `gltf_exporter_tests` target exercises the production helpers. Skin checks include:

- Preservation of native IDs across a normal seam and rejection of incorrectly sized native skin arrays.
- Duplicate-node palette canonicalization and accumulation of repeated influences.
- Weight normalization and propagation to repeated render records.
- Unsigned-short indices, inverse-bind matrix shape, column-major translation placement, and Z reflection.
- Two object instances resolving distinct bone palettes without overwriting the first skin.
- Rejection of conflicting bind matrices, zero weights, invalid slots, and missing joint nodes.
- No partial skin buffers committed by those failure cases.

Existing geometry and TRS tests also pass. The pre-existing nested-comment warning in the disabled tangent code remains.

## Native-evaluator deformation comparison

The test binary's `--skin-reference` mode loads the original JSON scene without graphics resources and calls HARFANG's `BindAnim` / `EvaluateBoundAnim` / `ComputeNodeWorldMatrix`. At each time, it computes normalized-weight deformed source positions and records native joint world matrices.

The [Python comparison](../tools/gltf_converter/tests/compare_skin_exports.py) independently decodes the exported skin, verifies every bind matrix and influence mapping, reconstructs the hierarchy, and evaluates glTF translation/scale with linear interpolation and quaternion rotation with spherical interpolation. It samples all 3,821 vertices at nine evenly spaced times, including clip start/end: **137,556 vertex/time comparisons across four clips**.

Two errors are measured separately:

- **Skin-only error:** apply exported skin data with the native evaluator's joint world matrices. This isolates the new skin implementation.
- **Full playback error:** also evaluate the exported animation tracks. This includes the remaining native-versus-glTF interpolation differences and float precision.

Distances are Euclidean distances in the asset's exported world-coordinate units, not an asserted physical-unit scale.

| Clip | Skin-only maximum error | Full playback maximum error | Full playback RMS error | Largest displacement from first sampled pose |
| --- | ---: | ---: | ---: | ---: |
| Idle | 3.88e-7 | 1.43e-6 | 6.37e-7 | 0.04988 |
| Walking | 3.90e-7 | 4.57e-5 | 1.95e-5 | 0.63041 |
| Heavy walking | 4.18e-7 | 1.29e-4 | 5.05e-5 | 0.65636 |
| Crouched walking | 3.50e-7 | 4.01e-7 | 1.26e-7 | 0.53486 |

The same checks on GLB produced identical results. The comparison enforces a `2e-6` maximum skin-only error; full playback error is reported separately because native-curve baking is not part of this change. Nine samples per clip are a regression sample set, not a proven bound over continuous time or other assets.

Additional checks confirmed:

- Every joint index is in range; all output weight sums are one within float tolerance.
- The output's joint list contains no duplicate nodes.
- All 40 inverse bind matrices match the native matrices after basis conversion.
- Existing position, normal, UV and index values, animation samples, materials, textures, and scene hierarchy match the preceding TRS export. Nodes differ only by the new skin assignment.
- glTF and GLB structures and every decoded accessor agree.
- A fresh unskinned `car_engine` export passes the geometry/material/scene and complete-TRS regression comparison against the preceding export.

## Validator results and remaining work

Khronos validator version: `2.0.0-dev.3.10`.

| Output | Errors | Warnings |
| --- | ---: | ---: |
| Colossus glTF | 0 | 13 |
| Colossus GLB | 0 | 1,247 |

The glTF warnings are one generated-tangent-space warning and twelve `ANIMATION_CHANNEL_TARGET_NODE_SKIN` warnings. The latter concern the mesh node's three constant TRS channels in each of four clips: a skinned mesh is driven by its joint transforms, not those mesh-node animation channels. They are retained from the TRS patch and do not account for the measured joint-driven deformation. GLB adds 1,234 URI warnings from the existing multi-buffer/data-URI packaging.

Outstanding independent issues remain: native-curve interpolation fidelity, material semantics (including emission and metalness), tangent-frame/normal-map visual parity, animated material properties, and conventional GLB binary-buffer packing. Skin position comparisons do not constitute a rendered appearance test or coverage of arbitrary custom skinning shaders. Joint nodes with different bind matrices under the same node reference are explicitly unsupported rather than silently misrepresented.

## Reproduction

From the workspace root:

```sh
cmake --build build/studio-macos --target gltf_exporter gltf_exporter_tests -j 6
build/studio-macos/harfang/tools/gltf_converter/gltf_exporter_tests

build/studio-macos/harfang/tools/gltf_converter/gltf_exporter \
  build/gltf-colossus-audit/compiled/generated/anims/colossus.scn \
  -compiled-resource-path build/gltf-colossus-audit/compiled \
  -source-resource-path build/gltf-colossus-audit/source \
  -out build/gltf-skin-validation/colossus -name colossus.gltf

build/studio-macos/harfang/tools/gltf_converter/gltf_exporter_tests --skin-reference \
  build/gltf-colossus-audit/source/generated/anims/colossus.scn \
  build/gltf-colossus-audit/source/colossus/colossus_idle/Layer.geo \
  build/gltf-skin-validation/native-reference.json

python3 harfang3d/tools/gltf_converter/tests/compare_skin_exports.py \
  build/gltf-colossus-audit/source/generated/anims/colossus.scn \
  build/gltf-colossus-audit/source/colossus/colossus_idle/Layer.geo \
  build/gltf-skin-validation/native-reference.json \
  build/gltf-skin-validation/colossus/colossus.gltf
```

Use `-name colossus.glb` and the corresponding comparison input for GLB. The native-reference mode and Python probe are fixture tooling for Colossus's single `Layer` mesh; the Python probe requires NumPy. The staged assets originate from the isolated audit snapshot, without modifying the demo project.

Logs, raw reports, native reference samples, `deformation.json`, `deformation-glb.json`, and both output files are in `build/gltf-skin-validation/`. These generated artifacts are not source changes.
