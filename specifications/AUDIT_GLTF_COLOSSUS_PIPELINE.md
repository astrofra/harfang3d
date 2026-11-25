# Colossus: native HARFANG to glTF pipeline audit

Date: 2026-11-25

Exporter revision: `87c361bc2ec191a15549a2301e6e5f9c97777827`, branch `main`.

Requested input: `/Users/fra/dev/demo-martian-melodies/src/assets/generated/anims/colossus.scn`.

Scope: actual resource compilation and glTF/GLB export, geometry and hierarchy preservation, animation, skinning, materials, and multiple clips. This audits the saved scene and its referenced shaders/assets, not the demo's entire Lua playback and rendering system. No exporter or original demo assets were modified.

## Verdict

**The exported Colossus cannot reproduce the native animated character.** Four named clips and the skeleton hierarchy survive, but the entire connection between bones and surface is missing. The exported mesh is a static root node in every clip. Fixing animation track selection alone would still leave it undeformed.

**Material texture transport works, but material appearance is not preserved.** All four PNG payloads survive byte for byte. However, fallback native colors are incorrectly used as glTF texture multipliers: the exported material has zero metalness, zero effective emission, a 0.5 roughness multiplier, and a 0.8 base-color multiplier. The project's custom AO/emission processing is also omitted.

**Multi-clip storage works in this case.** All four names, order, and duration ranges are present. The contents of each clip are incomplete, and the file does not encode the native runtime's crossfades or gait/controller logic.

| Area | Result | Practical consequence |
| --- | --- | --- |
| Geometry | Pass for emitted surface attributes | All 4,672 triangles retain their positions, normals, and UVs after coordinate conversion. |
| Scene hierarchy | Pass | All 51 nodes and all 48 parent-child edges survive. |
| Multiple clips | Structural pass | Four distinct named clips with correct sampled time ranges. |
| Animation fidelity | Fail | 408 of 612 source transform tracks are omitted, including moving hips translation in every clip. |
| Skinning | Critical failure | No skin, bone indices, weights, or bind matrices in output; the surface remains static. |
| Texture transport | Pass | Four 2,048 × 2,048 RGB PNGs preserved exactly. |
| Material semantics | Fail | Metalness and emission are suppressed; base color, roughness, and AO differ from native shader semantics. |
| glTF conformance | Fail | 223 errors in each output format, all reported as animation accessor-bound errors. |

Machine-readable measurements and input/output fingerprints are in [AUDIT_GLTF_COLOSSUS_MEASUREMENTS.json](AUDIT_GLTF_COLOSSUS_MEASUREMENTS.json).

## Test method and limits

The scene and its dependencies were copied into an isolated staging directory:

- `build/gltf-colossus-audit/source/generated/anims/colossus.scn`
- `build/gltf-colossus-audit/source/colossus/colossus_idle/`
- `build/gltf-colossus-audit/source/core/`

`assetc` compiled that staged source for Metal into `build/gltf-colossus-audit/compiled`. The exporter then generated `out/colossus.gltf` and `out/colossus.glb`. Compilation and both exports completed successfully, without missing-resource errors. These are real converter outputs; a successful exit code does not certify preservation of animation or materials.

Checks performed:

1. Parsed the native `.scn` and complete version-2 `.geo`, including skin records and bind matrices.
2. Decoded actual output accessors and image buffer views.
3. Compared expanded triangle positions, normals, and UVs against the native polygon bindings, with the exporter's winding and Z conversion.
4. Compared hierarchy edges and the selected animation tracks' node targets, key counts, and timestamps.
5. Compared glTF and GLB scene/animation structures and all animation input/output accessor values.
6. Compared embedded PNG bytes to their source files and inspected image channel ranges.
7. Ran Khronos `gltf-validator` version `2.0.0-dev.3.10` on both files, with `maxIssues: 20000`.
8. Read the actual project PBR shader to distinguish preserved texture files from preserved shader behavior.

There was no rendered native-versus-glTF image comparison, CPU reconstruction of animated skin positions, or interactive viewer playback test. Static-surface and material-loss findings below follow directly from decoded output and shader semantics; they are not claims about a particular viewer's screenshot. Skinning, interpolation fidelity, and material fixes will need numerical and visual validation.

## 1. Source assets and geometry

The scene has 51 nodes, one object (`Layer`), one material (`DefaultMaterial`), 204 native animation records, and four scene clips. Its object references `colossus/colossus_idle/Layer.geo` and enables skinning.

| Native geometry property | Count |
| --- | ---: |
| Positions | 3,821 |
| Polygons | 2,348 |
| Polygon corners / normal entries / tangent frames | 9,368 each |
| Triangles after fan triangulation | 4,672 |
| UV sets populated | UV0 only, 9,368 entries |
| Skin records | 3,821 |
| Bind matrices | 80 |

The glTF has one mesh/primitive and 3,821 render vertices. Only `POSITION`, `NORMAL`, and `TEXCOORD_0` are exported. Expanding the actual output indices reproduces exactly the expected source triangle attribute sequence. The recent vertex-sharing correction is functioning on this asset; it does not cause the deformation loss.

The skeleton is retained as ordinary nodes. The `Layer` mesh node has no parent, identity rotation/scale, and zero translation. Every exported clip gives it only a single constant zero-translation sample. It has no `skin` reference. Therefore neither its own transform nor its ancestors animate the surface, and joint-node animation cannot deform it.

## 2. Animation and multi-clip support

Each native clip binds 51 nodes, with position, scale, and either Euler or quaternion rotation for each node: 153 transform tracks per clip, 612 overall.

The exporter writes one channel per node binding. A quaternion track takes precedence; otherwise the first Vec3 track is selected. In this asset, that first Vec3 track is position. This explains why the output mixes animated rotations with constant translation channels rather than preserving full transforms.

| Clip name | Native duration (s) | Rotation channels | Translation channels | Scale channels | Keys per rotation channel |
| --- | ---: | ---: | ---: | ---: | ---: |
| `colossus_idle` | 3.900000000 | 34 | 17 | 0 | 235 |
| `colossus_walking` | 5.570370674 | 30 | 21 | 0 | 189 |
| `colossus_heavy_walking` | 5.037036885 | 33 | 18 | 0 | 171 |
| `colossus_crouched_walking` | 4.029629781 | 22 | 29 | 0 | 137 |

Each clip has 51 output channels. All 85 exported translation channels across the four clips are single-key tracks. All 119 quaternion tracks are exported; all 85 Euler rotation tracks are omitted. Of the 204 source position tracks, 119 are omitted. All 204 scale tracks are omitted. Total: **408 omitted tracks**.

### Actual motion lost

Every clip loses its varying `mixamorig:Hips` translation because that same node also has quaternion rotation.

| Clip | Largest component change from its first hips-position key |
| --- | ---: |
| Idle | 0.009075 |
| Walking | 0.119225 |
| Heavy walking | 0.170059 |
| Crouched walking | 0.101736 |

These are native local-coordinate deltas, not world-space or physical-unit measurements. They establish that the missing position tracks are not redundant constants.

Exact comparison also detects varying scale keys on 119 tracks, but their maximum component changes are only about `6.56e-7` to `7.15e-7`. Those variations are consistent with numerical noise; this audit does not present them as visible scale animation. Constant rotation/scale tracks can nevertheless establish a clip pose and must not be dropped without proving equivalence to the base transform.

### What multi-clip testing confirms

- Four distinct animation objects retain the four source names and their order.
- Each contains its own 51 channels/samplers with source-matching target names.
- Selected-track key counts and timestamps agree with the source to float-time precision.
- Sampled ranges start at zero and end at the intended durations, with expected float rounding.
- glTF and GLB contain identical animation structures and decoded sample values.

This is a structural multi-clip pass, not a playback-fidelity pass. Native clip `frame_duration` is 50 ms for idle and approximately 88.89 ms for the three walking clips; the exporter copies original keys rather than using those values to bake native evaluation. It also does not export native controller state, crossfades, gait selection, footstep logic, or procedural motion. A consumer must select/play/blend the exported clips itself.

The previously documented interpolation, rotation-mode, filtered-node mapping, and invalid-track risks still apply; see [the animation audit](AUDIT_GLTF_EXPORTER_ANIMATION.md). In particular, copied quaternion keys do not prove identical motion between keys, and omitted Hermite position tracks cannot be restored by simply relabeling interpolation.

## 3. Skinning: complete data loss at export

The source contains everything needed to construct a skin:

- `Geometry::skin`: four native joint-slot indices and four byte weights per native vertex.
- `Geometry::bind_pose`: 80 affine bind matrices.
- The object's `bones`: 80 references mapping joint slots to scene nodes.
- The node hierarchy and four animation clips.

The output contains:

```text
skins:                 0
nodes with skin:       0
JOINTS_0 / WEIGHTS_0:   absent
inverseBindMatrices:   absent
```

`PreparePolygonVertex` never reads skin data, `ExportGeometry` never writes joint/weight attributes, and `ExportObject` never constructs or assigns a glTF skin. Exporting bone nodes as ordinary animated nodes cannot substitute for these structures. This is missing implementation, not merely an indexing error or a viewer playback setting.

### Duplicate joint palette and weight normalization

The 80-entry native palette consists of two identical 40-entry halves. Their node references and bind matrices compare exactly equal. There are 40 distinct bone nodes, and 79 of the 80 slots have nonzero influences somewhere in the geometry.

A future implementation must map slot IDs deliberately. For this asset, collapsing identical `(node, bind matrix)` pairs to 40 joints is possible, but requires remapping every skin index and combining any repeated influence on the same joint. Do not collapse by name alone or assume that every asset's duplicate entries have identical bind matrices.

The byte-weight sums are not uniformly 255:

| Sum | Vertices |
| --- | ---: |
| 255 | 3,337 |
| 254 | 255 |
| 256 | 220 |
| 253 | 4 |
| 250 | 1 |
| 246 | 4 |

This is a source-data characteristic, not damage introduced by this exporter. A new skin exporter should define and test weight normalization, including zero-sum handling, and document any correction relative to the native runtime. Merely dividing every component by 255 does not guarantee a unit sum on this asset.

The native renderer forms matrices from bone world transforms multiplied by the corresponding bind matrices ([render_pipeline.cpp](../harfang/engine/render_pipeline.cpp), line 2175). HARFANG `Mat4` stores a 3 × 4 affine matrix; glTF matrix serialization and mesh-local/world-space conventions must be handled explicitly. Validate the Z-reflection basis change on both joints and bind matrices; copying raw matrix memory is not a valid conversion.

The skin should be assigned per object instance, because native bone-node mappings belong to the object even when geometry is shared. Preserve the vertex-to-source mapping through deduplication so each seam-split render vertex receives the correct skin record.

## 4. Materials: textures survive, semantics do not

The four embedded textures match their source PNGs byte for byte. Each is RGB, 2,048 × 2,048. Base color, packed occlusion/roughness/metalness, normal, and emission textures are all bound. ORM occlusion correctly reuses the same texture with UV0. The single opaque, culled material retains a valid mesh material assignment.

That transport success masks significant appearance changes. The project's shader is `src/assets/core/shader/pbr_fs.sc`, not an assumed stock shader. Its relevant branches are:

- Lines 193–197: use base texture **or** `uBaseOpacityColor`, then apply `uColorMul`.
- Lines 203–206: use ORM texture **or** `uOcclusionRoughnessMetalnessColor`.
- Lines 218–223: remap and exponentiate AO using `uAmbient`.
- Lines 226–234: use emission texture **or** `uSelfColor`, then apply AO/base-color mixing, a power, and a multiplier using `uSelf`.

The exporter instead copies both the texture and fallback color into glTF factors ([gltf_exporter.cpp](../tools/gltf_converter/gltf_exporter.cpp), lines 466–521).

| Property | Native textured shader behavior | Exported result |
| --- | --- | --- |
| Base color | Texture; fallback color is not multiplied into it | Texture multiplied by RGB `(0.8, 0.8, 0.8)` in linear space. |
| Metalness | ORM blue channel | `metallicFactor = 0`: the textured material becomes nonmetallic. |
| Roughness | ORM green channel, then roughness-level adjustment | `roughnessFactor = 0.5`: sampled roughness is halved. |
| Emission | Nonzero Self texture processed with `uSelf = (20, 2, 0.25, 1)` | Effective emissive factor is zero; the JSON omits the default-zero factor, so the texture contributes no emission. |
| Occlusion | AO remapped from `(0.1, 1)` and raised to power 3 | Raw ORM red channel; custom remap/power are absent. |
| Normal mapping | Native corner tangent frames and XY reconstruction of the sampled normal | Normal texture is present, but tangents are absent; the consumer generates tangent space. |

The ORM blue channel spans 0–255 and the emission image has nonzero RGB content. Thus metalness and emission suppression are not harmless multipliers on empty textures. For roughness, the shader program's default levels `(0, 0.5, 1)` give an identity adjustment; the saved scene does not override those levels. Runtime Lua changes, if any, are outside this saved-scene audit.

No explicit glTF samplers are written. Texture addressing/filtering and the native normal-map/tangent convention require dedicated checks before claiming visual parity. In particular, do not prescribe a green-channel inversion without demonstrating a convention mismatch.

### Material repair strategy

For a recognized shader profile, distinguish texture-active values from untextured fallback values. Use neutral glTF factors for branches that use textures directly, then map only genuine native multipliers. Do not apply a blanket rule to every HARFANG shader: this project has custom shader semantics.

Custom AO curves and nonlinear boosted emission need a defined baking or extension policy. They are not represented by merely copying four sampler paths. Test with fixed lighting/exposure, inspect metalness/roughness debug views, and compare emission without relying on bloom. The material problem is independent of the skinning problem.

Core glTF requires explicit skin joint/weight bindings for deformation. Its material factors multiply sampled textures, and its default emissive factor is zero. These representation rules explain the observed failures; see the [Khronos skin specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#skins) and [material specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials).

## 5. Validator results

| Output | Errors | Warnings |
| --- | ---: | ---: |
| `colossus.gltf` | 223 | 1 |
| `colossus.glb` | 223 | 416 |

Errors are `ACCESSOR_MAX_MISMATCH` (119) and `ACCESSOR_ELEMENT_OUT_OF_MAX_BOUND` (104). They are consistent with the already identified quaternion maximum-W assignment defect. There are 119 exported quaternion tracks; not every incorrect declared maximum also places a sample above the bound, explaining the different counts.

The common warning concerns generated tangent space. GLB adds 415 URI warnings because its resources are embedded as data URIs rather than consolidated into the binary chunk. Other informational messages include missing buffer-view targets, empty nodes, and GLB data URIs. No report was truncated.

**Validation alone cannot detect the principal fidelity losses.** A mesh with no skin can be valid static glTF, and a zero-metalness or zero-emission material is legal. Those failures are established by comparison to native intent, not by validator error count.

## 6. Prioritized repair and acceptance criteria

1. **Implement skin export and full transform-channel export together.** Preserve all joint mappings, weights, bind matrices, and required position/rotation/scale tracks. Correct animation bounds and target conversion. Test hips movement and surface deformation in all four clips.
2. **Preserve native curve evaluation.** Bake or otherwise convert Hermite translation and rotation interpolation with an error tolerance, verify at intermediate times, and handle clip boundaries deliberately.
3. **Correct shader-aware material conversion.** Restore textured metalness and emission, remove unintended tint/roughness factors, and define custom AO/emission handling.
4. **Improve packaging and diagnostics.** Report unsupported deformation/material behavior rather than silently presenting a successful fidelity-preserving export; emit conventional GLB buffers and validate tangent space.

Acceptance checks should include:

- All four named clips remain independently selectable with correct durations, start poses, and intermediate motion.
- All 40 unique bone nodes are mapped consistently; no invalid or duplicate palette references remain after the chosen canonicalization policy.
- The mesh has a valid skin and in-range joint/weight attributes for every render vertex.
- CPU or native-runtime reference samples match exported deformed positions/normals at several times in every clip, including hips translation and a bind/reference pose.
- The source's weight normalization discrepancy is handled explicitly and quantified rather than hidden.
- Materials retain textured metalness and nonzero emission; measured lighting-controlled renders support the appearance claims.
- Both formats have zero errors and no unintended animation-channel omissions. Clip storage must be tested separately from viewer autoplay or blending.

## Reproduction and artifacts

Workspace-relative generated artifacts:

```text
build/gltf-colossus-audit/
  source/                       isolated source snapshot
  compiled/                     assetc output
  assetc.log
  export-gltf.log
  export-glb.log
  inspect.py                    native/output comparison probe
  measurements.json
  validate.cjs
  validation-summary.txt
  out/colossus.gltf
  out/colossus.glb
  out/colossus.gltf.validation.json
  out/colossus.glb.validation.json
```

After staging the three source locations listed above, the commands used were:

```sh
build/studio-macos/harfang/tools/assetc/assetc \
  build/gltf-colossus-audit/source build/gltf-colossus-audit/compiled \
  -toolchain 'build/studio-review-package/Harfang Studio.app/Contents/Resources/data/assetc/toolchains/host-darwin-arm64-target-darwin-arm64' \
  -api MTL

build/studio-macos/harfang/tools/gltf_converter/gltf_exporter \
  build/gltf-colossus-audit/compiled/generated/anims/colossus.scn \
  -compiled-resource-path build/gltf-colossus-audit/compiled \
  -source-resource-path build/gltf-colossus-audit/source \
  -out build/gltf-colossus-audit/out -name colossus.gltf
```

Repeat with `-name colossus.glb`, then run `python3 build/gltf-colossus-audit/inspect.py` and `node build/gltf-colossus-audit/validate.cjs`. The probe uses the existing accessor decoder under `tools/gltf_converter/tests/` and Pillow for image inspection. The validator script uses the locally installed validator from the earlier vertex audit. Generated files and tools under `build/` are local evidence, not committed source changes.
