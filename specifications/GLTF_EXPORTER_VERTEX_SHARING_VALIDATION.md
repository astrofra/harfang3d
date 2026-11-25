# glTF exporter vertex sharing: implementation and validation

Date: 2026-11-25

This implements the vertex-sharing correction recommended in [the topology audit](AUDIT_GLTF_EXPORTER_VERTEX_TOPOLOGY.md). The audit describes the original implementation; this document records the subsequent change and its validation.

## Implemented behavior

`ExportGeometry` now reuses records with the same native vertex ID and the same emitted corner attributes: normal, UV0, and UV1. Position is determined by the native ID. Separate native IDs remain separate even when their positions coincide. Hard normals and UV seams retain distinct records. Matching vertices can be shared across material primitives.

The remap uses explicit fields, exact float equality, and a hash consistent with that equality, including signed zero. It does not hash the raw `hg::Vertex` struct. First-occurrence numbering is deterministic, and the original material traversal, triangle order, winding reversal, and Z reflection are retained. Index buffers remain 32-bit.

Supporting changes:

- Initialize staging vertices and omit `NORMAL` when the source has none.
- Check native geometry validity, polygon corner counts, and finite exported values before emitting accessors.
- Emit only material slots with triangles. Cache native material slot numbers with the geometry, then assign scene materials on each object's copy. This avoids indexing the primitive vector by a possibly sparse material slot or assuming that all instances have the same material count.
- Handle empty geometry without creating zero-length geometry accessors.

Source: [gltf_exporter.cpp](../tools/gltf_converter/gltf_exporter.cpp).

This is exact attribute deduplication using source identity, not proximity welding. Tangent/color/additional-UV/skinning support, general concave-polygon triangulation, and preservation of original polygon boundaries remain outside this change. Invalid geometry is omitted through the existing object-export failure path; this patch does not redesign scene-wide failure propagation.

## Automated geometry checks

The explicit `gltf_exporter_tests` CMake target exercises the production `ExportGeometry` function directly, without creating a renderer or window. It is excluded from the default build and is not installed as a production tool.

[Test source](../tools/gltf_converter/tests/gltf_exporter.cpp) covers:

- Four records and correct reused indices for a triangulated quad.
- Reuse between adjacent triangles.
- Splits for hard normals, UV0 seams, and UV1 seams.
- No unnecessary split caused by an unexported UV channel.
- Preservation of coincident vertices with different native IDs.
- Signed-zero equality and hashing.
- Sharing across sparse material slots, including slot 255.
- Correct omission of absent normals.
- Rejection of invalid bindings, attribute counts, non-finite UV values, two-corner polygons, and empty geometry.
- Consistent attribute counts and in-range indices.

Build and tests passed in the existing macOS Release build:

```sh
cmake --build build/studio-macos --target gltf_exporter gltf_exporter_tests -j 6
build/studio-macos/harfang/tools/gltf_converter/gltf_exporter_tests
```

The pre-existing nested-comment compiler warning in the disabled tangent block remains.

## Full scene test: car_engine

Input: `harfang3d/tutorials/resources/car_engine/engine.scn`, together with its `core`, `primitives`, and `cyclo` dependencies from the tutorials resource directory. These directories were copied into `build/gltf-vertex-audit/source` and compiled by `assetc` into `build/gltf-vertex-audit/compiled` for Metal (`-api MTL`). The original source assets were not edited.

The existing exporter executable was saved as `build/gltf-vertex-audit/gltf_exporter_before` before rebuilding. Both executables exported the same compiled scene and source geometry to separate `before` and `after` directories, in both glTF and GLB formats. The completed export logs contain no missing-resource errors.

| Measurement | Before | After |
| --- | ---: | ---: |
| Mesh objects | 91 | 91 |
| Triangle corners across mesh primitives | 233,289 | 233,289 |
| Stored vertices across distinct referenced position accessors | 231,561 | 75,351 |
| `.gltf` file bytes, embedded resources included | 56,543,478 | 49,878,318 |
| `.glb` file bytes, embedded resources included | 56,417,784 | 49,752,768 |

The correction removes **156,210 records (67.5%)**. Whole-file size falls by approximately **11.8%**; textures account for much of the remaining file size. Shared geometry accessors are counted once in the vertex totals, while the triangle-corner total counts each mesh's primitives.

[compare_exports.py](../tools/gltf_converter/tests/compare_exports.py) decodes accessors and expands both representations through their indices. The following checks passed:

- Exactly equal position, normal, and UV values for every triangle corner, in the same order.
- Equal primitive modes, material assignments, and mesh names.
- Equal node, scene, and material JSON data.
- In-range indices and equal triangle counts.
- Equal expanded geometry between the corrected `.gltf` and `.glb` files.

```sh
python3 harfang3d/tools/gltf_converter/tests/compare_exports.py \
  build/gltf-vertex-audit/before/engine.gltf \
  build/gltf-vertex-audit/after/engine.gltf
python3 harfang3d/tools/gltf_converter/tests/compare_exports.py \
  build/gltf-vertex-audit/after/engine.gltf \
  build/gltf-vertex-audit/after/engine.glb
```

This is a numerical geometry and serialization comparison, not a rendered screenshot comparison or GPU benchmark.

## Khronos validator results and existing limitations

The npm `gltf-validator` package, version reported as `2.0.0-dev.3.10`, validated all four files with `maxIssues: 10000`. The entire `issues` object is identical before and after for each format, not just the aggregate totals.

| Format | Errors before / after | Warnings before / after |
| --- | ---: | ---: |
| glTF | 61 / 61 | 62 / 62 |
| GLB | 61 / 61 | 473 / 473 |

**The full scene is not validator-clean.** Existing errors consist of three out-of-range light color components and 29 animation accessor maximum mismatches, each also reported as values exceeding the declared bound. The existing implementation assigns `quat.z` to the maximum for `quat.w`. These errors are unrelated to vertex remapping and were not changed here.

Existing warnings include generated tangent space and, for GLB, embedded data URI usage. The exporter still packages its multiple buffers using data URIs rather than consolidating them into a single GLB binary chunk. Other informational diagnostics include missing buffer-view targets and unsupported extension reporting. No new validator issue was introduced by this correction.

Raw logs, validator reports, comparison summaries, compiled test assets, and exported files are in `build/gltf-vertex-audit/` in the workspace. They are generated validation artifacts, not source changes.

## Repeating the export

After staging and compiling the resource directories described above, run from the workspace root:

```sh
build/studio-macos/harfang/tools/gltf_converter/gltf_exporter \
  build/gltf-vertex-audit/compiled/car_engine/engine.scn \
  -compiled-resource-path build/gltf-vertex-audit/compiled \
  -source-resource-path build/gltf-vertex-audit/source \
  -out build/gltf-vertex-audit/after -name engine.gltf
```

Use `-name engine.glb` for the binary container. The converter still needs a graphics-capable session for its existing renderer initialization; the geometry unit tests do not.
