# glTF importer: urgent skinning and accessor fixes

Date: 2026-11-25. Based on the [importer audit](AUDIT_GLTF_IMPORTER.md) at revision `71d5b4f`.

## Result

The two demonstrated crashes on valid input are fixed: a skin without explicit inverse bind matrices imports with identity matrices, and sparse-only accessors import from a zero-initialized base. Sparse overrides on existing data are now applied too.

Skinned instances and fallback materials retain `EnableSkinning`. Inverse bind matrices are converted directly between coordinate systems, preserving shear, reflection, nonuniform scale and translation. Colossus's imported matrices now match the reflected glTF matrices exactly, rather than within `2.69e-7` per component.

This is the first repair batch, covering the audit's P0 accessor failures and urgent skinning defects. Animation interpolation, sampler/channel associations, non-indexed geometry, strip/fan conversion, tangents and material fidelity remain separate work.

## Implementation

Source: [gltf_importer.cpp](../tools/gltf_converter/gltf_importer.cpp).

- `PrepareAccessors` checks accessor buffer spans, offsets, strides and element counts before conversion. It materializes sparse accessors into owned dense buffers, preserving dense-base data and filling absent bases with zeros. Sparse index types UBYTE/USHORT/UINT, ordered indices, range checks, interleaved bases and matrix column padding are handled. Dense accessors with existing buffers are checked without copying them.
- The preprocessing is shared by geometry, animation and bind-matrix consumers. Typed array reads use `memcpy` rather than dereferencing reinterpreted objects.
- `ImportBindPoses` supplies identity matrices when the accessor is omitted. Explicit matrices must be finite affine FLOAT MAT4 data with enough entries for the joint palette. Conversion writes native affine matrices using `C * B * C`, with `C = diag(1,1,-1,1)`, without inversion or Euler/TRS decomposition.
- Material skinning flags follow the node's skin assignment, including geometry-cache hits and generated dummy materials.
- Skin validation checks joint-node indices, attribute types/counts, influence indices, finite weight ranges and nonzero packed weight totals. Joint binding requires a node actually imported into the current scene.
- The node-reference map is reset per scene. Animation channels and skin objects outside that scene are skipped during binding, preventing stale references from attaching to unrelated nodes. Geometry caches are reset per input file and remain shared across scenes of that file.
- Conversion exceptions are reported by the CLI and return failure instead of terminating the process. This is targeted validation, not exhaustive malformed-input hardening. Conversion is not transactional: resources written before a later error can remain on disk.

The defaults for omitted inverse bind matrices and absent accessor buffer views follow the Khronos [skin schema](https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/schema/skin.schema.json) and [accessor schema](https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/schema/accessor.schema.json).

## Automated regression tests

[test_importer_skin_accessors.py](../tools/gltf_converter/tests/test_importer_skin_accessors.py) runs the actual CLI and inspects native scene/geometry output. It reuses the audit fixture generator and native geometry reader, requiring only Python's standard library.

**17 tests pass, comprising 33 CLI cases:**

- 19 positive fixtures independently pass Khronos validator `2.0.0-dev.3.10` with **zero errors and zero warnings**.
- 14 deliberately invalid fixtures return exit status 1 with an error message and no newly written scene; none terminates by signal.

Coverage includes identity defaults, cached instances, dummy materials, general affine bind matrices, sparse positions/indices/animation/skin attributes/bind matrices, absent dense bases, interleaved data and offsets, all three sparse index widths, normalized byte/ushort weights, separate scenes, truncated buffers, out-of-range/duplicate/descending sparse indices, invalid joint references and malformed skin data.

All 22 original audit probes were also rerun. Their changed results are limited to the seven expected cases:

| Probe | Before | After |
| --- | --- | --- |
| `skin_no_ibm` | SIGSEGV | Identity bind matrices |
| `sparse_only` | SIGSEGV | Zero base plus sparse overrides |
| `sparse_base` | Overrides ignored | Overrides applied |
| `skin_instance` | Second object unskinned | Both materials enable skinning |
| `skin_no_material` | Dummy material unskinned | Dummy material enables skinning |
| `skin_shear_ibm` | Affine matrix altered | Exact basis conversion |
| `multiple_scenes` | Scene B animates an unrelated node | No foreign node binding in scene B |

Unrelated animations may still leave empty clip records in a scene; clip pruning is not part of this change. Other known failing audit behaviors remain reproducible and are not counted as successful regression coverage merely because their CLI exits 0.

## Real-asset regression

Fresh imports used the same corrected exporter outputs as the audit:

- Colossus `.gltf` and `.glb`: 3,821 vertices, 4,672 triangles, 40 joints, four clips, 612 animation bindings.
- `car_engine`: 126 nodes, 91 mesh nodes and 87 animation bindings.

Compared with the audit imports, native scene files and all copied textures/metadata are byte-identical. Every native geometry field is unchanged except Colossus's inverse bind matrices, whose rounding/decomposition errors are removed. `car_engine` geometry is identical. This preserves existing material and animation behavior, including the known limitations documented by the audit.

The existing native CPU reference helper evaluated the newly imported Colossus at nine instants per clip. The independent glTF evaluator compared **137,556 vertex/time samples**:

| Clip | Maximum position error after weight normalization |
| --- | ---: |
| Idle | 1.28834e-4 |
| Walking | 1.73926e-4 |
| Heavy walking | 2.14706e-4 |
| Crouched walking | 2.48309e-4 |

These errors remain comparable to the audit baseline and include existing weight quantization and interpolation differences. The helper normalizes by the actual byte-weight total; the importer still rounds four weights independently, so 30 Colossus vertices retain totals of 254 or 256 instead of 255. Weight-sum correction is not included here. There was no GPU rendering or assetc validation; this is source-scene/geometry and CPU playback validation, not a visual equivalence claim.

Logs, full diagnostic outputs, validator messages and numeric playback results are under `build/gltf-import-fix`. The historical audit measurements remain unchanged.

## Reproduction

From the workspace root:

```sh
cmake --build build/studio-macos --target gltf_importer -j 6
python3 harfang3d/tools/gltf_converter/tests/test_importer_skin_accessors.py \
  --importer build/studio-macos/harfang/tools/gltf_converter/gltf_importer \
  --out build/gltf-import-fix
```

Each run creates a fresh directory and retains its fixture files, logs and outputs for inspection. Use the import and native-reference commands in the original audit with `build/gltf-import-fix` as the destination to repeat the real-asset checks.
