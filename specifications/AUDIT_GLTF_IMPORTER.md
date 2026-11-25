# glTF importer audit

Date: 2026-11-25. Source revision: `71d5b4fe37defc208c951c5793e5f84fa3b32895`, branch `main`.

Follow-up: the first batch of [skinning and accessor fixes](GLTF_IMPORTER_SKIN_ACCESSOR_VALIDATION.md) addresses both demonstrated crashes, sparse data, skinning flags, bind-matrix conversion and scene-local node references. The findings and measurements below describe the original audited revision.

The second batch covers [channel mapping, interpolation and animation roundtrips](GLTF_IMPORTER_ANIMATION_VALIDATION.md), including explicit native LINEAR/STEP playback and sampled cubic curves.

## Result

The importer supports indexed triangle meshes, basic metallic/roughness materials, skeletal data, TRS animation keys, and multiple clips. **It is not a reliable general-purpose glTF importer yet.** Valid inputs can crash, silently lose geometry, change animation channels, or disable skinning on instances. Successful conversion and an `[ImportScene: OK]` message do not establish correctness.

The recent Colossus exports take a comparatively successful path: both glTF and GLB reimport with 3,821 vertices, 4,672 triangles, 40 joints and four clips containing 153 channels each. `car_engine` retains its 87 animation channels. Neither asset exercises several serious defects demonstrated by the diagnostic fixtures below.

This audit changes no production importer/exporter code. It adds an executable diagnostic fixture generator and records measurements. No material-animation approximation or project-specific shader conversion is implemented.

## Scope and evidence

Reviewed [gltf_importer.cpp](../tools/gltf_converter/gltf_importer.cpp), native animation evaluation, geometry weight packing, material shader selection, and the default PBR shaders. The importer calls its conversion functions `Export*` because they write native HARFANG data; these are import operations in this report.

The `gltf_importer` target was rebuilt in `build/studio-macos`. Tests used isolated output directories under `build/gltf-import-audit`, with source assets untouched:

| Input | Purpose | Result |
| --- | --- | --- |
| `build/gltf-skin-validation/colossus/colossus.gltf` | Latest corrected exporter output | Import exits 0 |
| `build/gltf-skin-validation/colossus/colossus.glb` | Binary-container equivalent | Import exits 0; native scene bytes identical to glTF import |
| `build/gltf-skin-validation/car_engine/engine.gltf` | Static meshes, materials and animated mechanical parts | Import exits 0 |
| 22 generated minimal glTF files | Isolate importer behavior from exporter behavior | All validate with **0 errors and 0 warnings**; two importer processes terminate with SIGSEGV |

Validator: Khronos `gltf-validator` `2.0.0-dev.3.10`. Validation of the 22 fixtures is separate from the pre-existing exporter validation results: Colossus has zero validation errors, while the `car_engine` export retains the three light-color errors already documented in the exporter audit. Those lights are not used to infer importer correctness.

Checked-in evidence: [measurements](AUDIT_GLTF_IMPORTER_MEASUREMENTS.json) and [fixture generator](../tools/gltf_converter/tests/audit_importer.py). Full native outputs, logs, validator messages and CPU samples remain in `build/gltf-import-audit`.

Specification baseline: glTF allows non-indexed primitives, sparse accessors and omitted inverse bind matrices; omitted matrices mean identity. Animation channels select samplers independently of array ordering. Interpolation modes have distinct meanings, including tangent/value triples for cubic interpolation. These are ordinary supported-format requirements, rather than assumptions about our exporter. See the [Khronos glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html), sections on meshes, accessors, skins and animation.

## Priorities

Here P0 means a process crash on valid input; P1 means lost or corrupted content; P2 means fidelity or operational limitations. This is a functional audit, not a security assessment.

| Priority | Finding | Evidence |
| --- | --- | --- |
| P0 | Optional inverse bind accessor dereferenced as index `-1` | `skin_no_ibm`: SIGSEGV |
| P0 | Sparse-only accessor dereferenced as buffer view `-1` | `sparse_only`: SIGSEGV |
| P1 | Skinning flag missing on cached instances and fallback materials | `skin_instance`, `skin_no_material` |
| P1 | Channel/sampler confusion changes translation and scale signs; shared samplers overwrite track targets | `reordered_channels`, `shared_sampler` |
| P1 | Cubic output decoded incorrectly; interpolation semantics discarded | `cubicspline`, `step`, `linear`; native evaluator review |
| P1 | Non-indexed geometry disappears; strips/fans are truncated | `nonindexed`, `strip`, `fan` |
| P1 | Sparse patches ignored, inverse bind shear altered, tangent basis corrupted | `sparse_base`, `skin_shear_ibm`, `tangents` |
| P1 | Node references leak between scenes | `multiple_scenes`: animation attaches to the wrong node |
| P1/P2 | Material mapping changes rendering semantics | `occlusion_only`, `mask_texture`; default shader review |
| P2 | Weight quantization, unsupported attributes/extensions and misleading CLI options | Measurements and source review |

## Geometry and vertex sharing

### The indexed-triangle path preserves sharing

`ExportGeometry` appends each primitive's POSITION accessor once and constructs native polygon bindings from the index accessor, adding a vertex offset for later primitives (`gltf_importer.cpp:745–837`, `1010–1024`). It does **not** generate one position per triangle corner. Normals and UVs are expanded into native corner arrays, which is the native representation and does not itself duplicate positions.

The `indexed` fixture keeps four positions for two triangles, with bindings `[2,1,0,3,1,2]`. Positions/normals reflect Z and triangle winding reverses consistently. On both real assets, all checked position, normal and UV components are exact after this coordinate conversion, and every triangle binding matches.

There is no reconstruction of a pre-glTF welded topology. Separate glTF vertices at attribute seams remain separate native positions; shared accessor data used by multiple primitives is appended separately. This is distinct from triangle-soup expansion and is not safe to fix with an unconditional position-only weld, especially when skin weights differ.

### Valid topologies and accessors fail

- **No indices:** line 753 logs an error and returns from geometry conversion. The outer importer still writes an empty `.geo` and reports success. The fixture's valid three-vertex triangle becomes zero positions and zero polygons.
- **Strip/fan:** polygon allocation first assumes `index_count / 3`, truncating the original stream before the mode conversion (`822–872`). Four-index strip and fan fixtures each produce one triangle instead of two. The conversion also does not rebuild the polygon array; strip parity and later-primitive offsets are not handled correctly by the current loops.
- **Sparse accessors:** buffer reads never apply sparse patches. A dense-base fixture changing vertex 1 from `(1,0,0)` to `(2,0,0)` keeps the old value. Omitting the dense base causes SIGSEGV. The same unchecked-accessor pattern also appears in animation and bind-matrix reads; those additional crash paths were not separately exercised.
- **Vertex colors:** the `colors` fixture produces no native color entries. Attribute dispatch handles POSITION, NORMAL, UV0/UV1, TANGENT, JOINTS_0 and WEIGHTS_0 only. UV2+, additional joint/weight sets and morph target deltas have no conversion path.
- **Signed normalized data:** BYTE divides by 128 and SHORT by 32768 (`950–966`). The validated quantized-normal fixture imports positive 127 as `0.9921875`, not 1. Unsigned normalization uses the expected 255/65535 divisors.

### Imported tangents are incorrect

At `1099–1103`, the tangent's Z coordinate is not reflected. Bitangent construction then mixes this tangent with a reflected normal. It also indexes the corner-normal array using a vertex index instead of the current corner index.

The fixture supplies normal `(-0.8,0,0.6)` and tangent `(0.6,0,0.8,1)`. Imported normal is `(-0.8,0,-0.6)`, but tangent remains `(0.6,0,0.8)`: their dot product is approximately `-0.96`. The stored bitangent is `(0,0.28,0)`. This is a broken basis, not a small rounding error. Normals can additionally be missing when the tangent loop accesses them, before the later normal-recalculation step; that risk was not executed.

## Animation and multiple clips

### What survives

The importer loops over all model animations and creates one native `SceneAnim` for each (`290–476`). It imports separate translation, rotation and scale channels, and preserves clip names and the union of key times. It creates one native `Anim` per sampler and binds these to nodes; multiple native bindings for a node are intentional and evaluated by the runtime.

| Asset | Nodes | Clips | Imported channel bindings | Translation/scale key error | Maximum quaternion key error |
| --- | ---: | ---: | ---: | ---: | ---: |
| Colossus glTF / GLB | 51 | 4 | 612, or 153 per clip | 0 | 7.80e-7 |
| car_engine | 126 | 1 | 87 | 0 | 1.33e-7 |

Quaternion errors are Euclidean distances between quaternion components, taking the smaller error under sign reversal, after basis conversion; they are not angular errors. Key-time differences are below `1e-9` second. Preserved keys do not imply preserved interpolation.

### Channel lookup and shared sampler bugs

The target-path search compares `channels[k].sampler` but reads `channels[j].target_path` (`338–340`), where `j` is a sampler index. Channel order is therefore incorrectly treated as sampler order.

In `reordered_channels`, input translation Z values `[2,4]` become native `[2,4]` instead of `[-2,-4]`, while scale Z values `[3,5]` become `[-3,-5]`. Neither our current exporter nor Colossus exposes this because their channel/sampler arrays happen to align.

The importer also creates a single mutable native track for each sampler and changes its target for every referencing channel (`439–471`). `shared_sampler` uses one VEC3 sampler for translation and scale. Both bindings end up referring to a `Scale` track, with already-reflected values; the translation target is lost. Sharing a sampler between nodes with the same target is a different case and is not inherently broken by this target overwrite.

### Interpolation is not preserved

`sampler.interpolation` is logged but never used. Vec3 keys become native Hermite tracks with default zero tension/bias; quaternion keys become native quaternion tracks evaluated by component interpolation and normalization. See [animation.h](../harfang/engine/animation.h) and `scene.cpp:2191–2203`.

- `CUBICSPLINE`: the importer reads `value[k]` instead of the value within each tangent/value/tangent triple. The fixture's intended native Z endpoints `[-2,-4]` become `[0,-2]`. Both values and tangents are mishandled.
- `STEP`: imported key data is identical to the LINEAR fixture, with no step flag. The native evaluator interpolates between them.
- `LINEAR`: Vec3 keys are not converted to a linear representation. Applying the native Hermite formula to this fixture at `t=0.25` gives Z `-2.40625`; the glTF linear value is `-2.5`. At `t=0.5`, native Z is `-3`, while the STEP value should still be `-2`. These two examples are calculations from the stored keys and runtime formula, not separate rendered measurements.
- Quaternion interpolation is not glTF spherical interpolation, and quaternion conversion unnecessarily passes through Euler angles. Colossus key errors are small, but this does not establish robustness around all rotations or large-angle intervals.

Morph-weight channels have SCALAR output and no implemented conversion case. Naming a target `Weights` in the binding loop does not supply a usable track or morph geometry. Material animation extensions are not handled either; this is a capability gap, not a proposal to revive the cancelled material-specific conversion.

## Skinning

### Working baseline and quantization

JOINTS_0 accepts integer attributes, WEIGHTS_0 accepts float/normalized integer attributes, native skin records are indexed by source vertex, and `ExportSkins` resolves bone-node references after node creation. The current Colossus output takes this path correctly.

Reimported Colossus preserves all 40 palette entries and all joint indices. Its inverse bind matrices differ from direct reflected input matrices by at most `2.69e-7` per component. Weight components are rounded independently to bytes (`1079–1082`): maximum component error is `1/510`, approximately `0.00196078`.

| Imported byte-weight sum | Vertices |
| ---: | ---: |
| 254 | 21 |
| 255 | 3,791 |
| 256 | 9 |

Even the simple equal-weight fixture produces `[128,128,0,0]`, sum 256. No residual correction enforces a sum of 255. The geometry builder decodes bytes by 255; the default skin shader does not explicitly renormalize weights.

### Missing matrices crash

At `1203`, `model.accessors[skin.inverseBindMatrices]` is used unconditionally. `skin_no_ibm` is valid, passes Khronos validation, and terminates the importer with SIGSEGV. It should use identity bind matrices when that accessor is absent.

### Instances and fallback materials lose skinning

Material skinning is enabled only when the local `geo.skin` vector is nonempty (`1276`). On a geometry-cache hit, the local geometry stays empty. In `skin_instance`, both objects have the correct bone references and share the geometry, but only the first material has `EnableSkinning`. Runtime program selection uses that flag, so the second object selects the unskinned shader variant.

The fallback-material branch never sets the flag. `skin_no_material` imports valid bones and vertex influences but writes an unskinned dummy material. Skinning enablement must come from the object's actual skin assignment, consistently for all material paths.

### Bind matrices should not be decomposed

The importer fast-inverts each matrix, extracts translation/Euler rotation/scale, reconstructs it, then fast-inverts again (`1220–1231`). This is not a general basis change and loses affine information. `skin_shear_ibm` supplies an XY shear of 0.5. The output's upper-left 2 × 2 matrix becomes approximately:

```text
Expected: [1.000000  0.500000]    Actual: [ 0.870388  0.323254]
          [0.000000  1.000000]            [-0.435194  1.171796]
```

Use direct `C * B * C`, with `C = diag(1,1,-1,1)`, rather than TRS decomposition. Colossus passing the current path does not cover general inverse bind matrices.

### Measured Colossus deformation

The existing C++ `gltf_exporter_tests --skin-reference` helper loaded the **reimported** scene, called native `BindAnim`, `EvaluateBoundAnim` and `ComputeNodeWorldMatrix`, and evaluated all 3,821 positions at nine times per clip. An independent Python glTF evaluator computed input animation with linear TRS translation/scale and spherical quaternion interpolation. This gives **137,556 vertex/time comparisons**.

The C++ helper divides by the actual byte-weight total. The normalized comparison includes quantization, bind-matrix conversion and animation differences. A second calculation restores the raw `/255` weight sum to inspect the unnormalized world-space XYZ values.

| Clip | Max normalized position error | RMS normalized error | Max raw-weight world XYZ error |
| --- | ---: | ---: | ---: |
| Idle | 1.29e-4 | 1.00e-5 | 5.97e-3 |
| Walking | 1.74e-4 | 2.18e-5 | 6.03e-3 |
| Heavy walking | 2.15e-4 | 5.15e-5 | 6.18e-3 |
| Crouched walking | 2.48e-4 | 1.46e-5 | 5.00e-3 |

Distances are in asset coordinates. **The raw-weight column is not a measured screen-space silhouette error:** the shader propagates homogeneous W into projection, where a common scale can cancel. However, it also exposes `world_pos.xyz` directly for lighting/shadows, so the unnormalized values remain relevant. No GPU render or asset-compiler roundtrip was performed. Sampling is not a continuous-time error bound. GLB imports produced identical native scene and geometry data; the CPU playback run used the glTF import.

## Materials and textures

Basic uniform values, texture references, double-sided state and opaque/blended state are mapped. All four Colossus images and all 14 `car_engine` images were copied byte-for-byte, verified by SHA-256; all output material texture paths resolve. This establishes resource preservation, not rendered material equivalence.

### Confirmed conversion problems

- **Textured material factors:** the importer stores base color, roughness, metallic and emissive factors in native uniforms. The default [PBR fragment shader](../core-package/core/shader/pbr_fs.sc) selects texture values *instead of* those uniforms (`128–146`). Thus the factors are present in the scene but do not multiply their textures. An arbitrary `-shader` override may behave differently; only the default path was reviewed.
- **Occlusion-only material:** at `678–680`, `(int)roughnessFactor * 255` and `(int)metallicFactor * 255` truncate before multiplication. The fixture with roughness 0.5 and metallic 0.7 generates metadata `construct: ["R",0,0]`. The intended factor-derived channels would be nonzero. This metadata was inspected; assetc was not run on it.
- **Alpha mask:** both `MASK` and `BLEND` become `BM_Alpha` (`724–725`). `mask_texture` produces `blend_mode: alpha`, no alpha-cut flag, and no retained cutoff 0.2. Cutouts therefore become blending rather than threshold masking.
- Texture UV selection, texture transforms, normal-map scale and occlusion strength are not mapped. Different occlusion/MR UV sets cannot be faithfully represented by the existing combined-map logic.
- Sampler conversion only handles REPEAT and CLAMP_TO_EDGE. Mirrored wrapping and minification/magnification filter choices are ignored.
- Standard material extensions such as unlit and emissive strength have no conversion path. There is no importer-level check rejecting unsupported required extensions. Compressed geometry/texture extension handling was not exercised.

### Attribution of Colossus losses

The input glTF already contains base color factor 0.8, metallic factor 0, roughness factor 0.5 and zero emissive factor, as recorded in the exporter audit. The importer preserves those values. It cannot reconstruct the original custom HARFANG material or `uSelf` behavior from them.

The default shader's texture-over-uniform behavior is an **additional importer-to-runtime mismatch**. It may visually conceal an exporter error by ignoring an already-wrong factor; that does not make the roundtrip faithful. Material conclusions here are from data and shader inspection, not image comparisons.

## Scene selection, cameras and operational behavior

### Multiple clips work; multiple scenes are unsafe

`ImportGltfScene` writes every glTF scene (`1539`), while node maps and geometry caches are global and not reset per scene. `ExportMotions` imports every animation into every scene and uses `idNode_to_NodeRef[target_node]`, without checking scene membership.

In `multiple_scenes`, scene A contains animated `Mesh`; scene B contains only `Other`. Scene B nevertheless receives the same animation bound to its node 0 (`Other`), using A's stale node reference. This is distinct from multi-clip support within a single scene. With `-name`, all scenes also request the same output filename, creating skip/overwrite conflicts depending on policy.

### Additional source-reviewed gaps

- Node matrix conversion decomposes to TRS using unsigned scale magnitudes; reflected/negative-scale matrices warrant dedicated tests. Camera/light facing under coordinate conversion was not validated visually.
- Orthographic camera `xmag`/`ymag` are not mapped. Perspective aspect ratio is not retained, and absent `zfar` is assigned from the loader value without an explicit infinite-far policy (`1314–1330`).
- `-geometry-scale` changes positions only, not node translations, animation translation keys or inverse bind matrices. It is a mesh-coordinate operation, not a safe whole-scene unit conversion for skinned assets.
- `-all-policy` is assigned and then overwritten for every category by individual-option defaults (`1667–1673`). Use explicit category options when testing overwrite behavior.
- `-anim-to-file`, `-fix-geometry-orientation` and `-detect-geometry-instances` are advertised but not read. Geometry caching runs independently of the last option.
- Finalizer scripting is commented out: a nonempty script can pass `LoadFinalizerScript` without executing. Material/geometry/node finalizer hooks are likewise inactive.
- Individual geometry/scene writes are not checked for failure before overall success is returned. Unsupported-content errors generally do not propagate to the process status.
- Partial per-primitive attribute arrays, unusual matrix transforms, missing external resources and filename collisions deserve additional fixtures. They were reviewed as risks, not established by the 22 executed probes.

## Recommended repair sequence

1. **Restore safe skin/accessor handling:** optional identity bind matrices; sparse accessor decoding; checked references; skinning flags for instances and dummy materials; direct bind-matrix basis conversion. Add regression tests for the demonstrated failures.
2. **Correct animation associations:** build native tracks per channel/target rather than mutating sampler-owned tracks; resolve actual channel targets and scene membership; preserve all clips.
3. **Make interpolation explicit:** correctly decode cubic triplets and provide a documented evaluation/baking strategy for LINEAR, STEP and CUBICSPLINE, including quaternion interpolation. Do not silently relabel curves as native Hermite.
4. **Complete basic geometry conversion:** non-indexed triangles, strip/fan triangulation, tangent reflection/corner indexing, consistent missing-attribute handling and vertex colors. Preserve meaningful vertex sharing and skin identity.
5. **Improve PBR fidelity and diagnostics:** factor/texture combination, MASK semantics, occlusion metadata arithmetic, UV/sampler choices, required-extension policy, and nonzero exit status for unsupported or failed output.

The first step follows the current priority on skinning. Colossus's favorable result should be retained as a regression baseline while adding inputs that actually expose the broken paths.

## Reproduction

Run from `/Users/fra/dev/harfang_build`. Use fresh output directories: the default policy skips existing files.

```sh
cmake --build build/studio-macos --target gltf_importer gltf_exporter_tests -j 6
python3 harfang3d/tools/gltf_converter/tests/audit_importer.py \
  --importer build/studio-macos/harfang/tools/gltf_converter/gltf_importer \
  --out build/gltf-import-audit/probes-recheck
```

The script records exit codes, native geometry arrays, scene bindings, material flags and texture preprocessing metadata. It intentionally records failures rather than asserting that current behavior is correct. It requires only Python's standard library and refuses a nonempty output directory. The SIGSEGV outcomes are isolated subprocess results, not Python failures.

To validate the generated fixtures, run this from the workspace root, with `GLTF_PROBES` set to that output directory:

```sh
GLTF_PROBES=build/gltf-import-audit/probes-recheck node <<'JS'
const fs = require('fs'), path = require('path');
const validator = require('./build/gltf-vertex-audit/validator/node_modules/gltf-validator');
(async () => {
  const dir = process.env.GLTF_PROBES;
  for (const file of fs.readdirSync(dir).filter(f => f.endsWith('.gltf'))) {
    const result = await validator.validateBytes(new Uint8Array(fs.readFileSync(path.join(dir, file))), {uri: file});
    console.log(file, result.issues.numErrors, result.issues.numWarnings);
    if (result.issues.numErrors || result.issues.numWarnings) process.exitCode = 1;
  }
})();
JS
```

Example real-asset import:

```sh
build/studio-macos/harfang/tools/gltf_converter/gltf_importer \
  build/gltf-skin-validation/colossus/colossus.gltf \
  -out build/gltf-import-audit/colossus-recheck \
  -base-resource-path build/gltf-import-audit/colossus-recheck -name colossus
```

Native CPU reference used for the recorded deformation run:

```sh
build/studio-macos/harfang/tools/gltf_converter/gltf_exporter_tests --skin-reference \
  build/gltf-import-audit/colossus/colossus.scn \
  build/gltf-import-audit/colossus/colossus_colossus_idle_Layer.geo \
  build/gltf-import-audit/native-reference.json
python3 build/gltf-import-audit/measure_roundtrip.py
```

The last measurement script is a local audit artifact (requires NumPy), using the checked-in glTF accessor and CPU interpolation helpers. The committed JSON retains the numeric results and input hashes independently of the build directory. Rendering, full extension conformance, exhaustive malformed-input testing and continuous-time deformation bounds remain outside this audit.
