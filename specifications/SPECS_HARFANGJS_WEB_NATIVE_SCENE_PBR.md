# HG JS Native / Web: PBR Scene

Date: 2026-10-05

## Outcome and shared API

`tutorials/scene_pbr.js` is copied byte-for-byte into the Web release. The experiment
lives in `harfangjs/experiments/native-scene-pbr/`. Its browser window helper
re-exports `runWindow` from `harfang/browser`; the native helper remains unchanged.
The original `materials/materials.scn` is preserved as parsed JSON, including
material state and all fifteen nodes.

The entry uses the already implemented native-compatible surface:

| API | Web behavior |
| --- | --- |
| `CreateForwardPipeline()` | Forward rendering, default 1024 spotlight shadow target. |
| `new PipelineResources()`, `new Scene()` | Explicit resource and scene ownership. |
| `LoadSceneFromAssets(path, scene, resources, GetForwardPipelineInfo())` | Synchronous loading from assets preloaded by the browser host. |
| `Scene.Update(dt)` | BigInt nanosecond time step and native transform update semantics. |
| `SubmitSceneToPipeline(0, scene, IntRect, true, pipeline, resources)` | Current-camera forward submission and the existing native-shaped return value. |
| `Scene.Clear()`, `DestroyAllTextures/Models/Programs()`, `DestroyForwardPipeline()` | Explicit teardown, with browser host cleanup on stop or failure. |

No Web-specific arguments were added to public HG calls. Browser dimensions
override the requested window dimensions. The MSAA flag emits one warning and is
ignored. This scene uses neither AAA nor animation playback; no additional stubs
were needed.

## Compiler changes

The standalone native `assetc-web` now decodes `.jpg` and `.jpeg` alongside PNG
source textures, through its linked CMFT/stb implementation. JPEG headers and
dimensions are checked before image allocation. The existing input limit is
64 MiB; JPEG dimensions must be positive, at most 4096 on either axis and at most
4,194,304 pixels in total.

The authored fixture has 27 JPEG source textures and one PNG. LDR images become
RGBA8 with an offline-generated mip chain. NormalMap/Standard metadata is accepted,
and authored `max-size` combines with the optional compiler `--max-texture-size N`
by taking the smaller limit. Resizing preserves aspect ratio and never enlarges
images. The command-line limit applies to PNG/JPEG, not HDR/BRDF data.

Native BC2/BC3/BC5/ETC1 compression requests are accepted with a conversion warning
and retained as `sourceCompression`; the actual Web representation is RGBA8.
`sourceTextureType` records supplied texture type metadata. The compiler does not
claim to reproduce native compressed-block artifacts. HDR irradiance/radiance
and the BRDF lookup retain the existing RGBA16F path.

Every compiled asset preserves its original logical path and extension. A file
named `materials/Metal24_basecolor.jpg` in compiled output therefore contains
RGBA8 mip data, not JPEG bytes. The browser reads the manifest's encoding and
level offsets and uploads those bytes. JPEG decoding occurs only in the offline
compiler. The native assetc likewise retains source names for compiled textures.
Source and compiled trees must never be interchanged.

Empty `uSelfMap` slots in the scene are accepted. Nonempty emissive texture maps
remain unsupported in this native scene profile; the existing emissive color
uniform remains available.

## Alpha rendering

The PBR descriptor advances to `pbr-scene-instanced/3`, declaring
`render.alpha-blend` in addition to the existing PBR requirements. Reviewed shader
source hashes are unchanged; versions 1 and 2 remain readable for existing
packages. Newly compiled version 3 assets need the matching updated runtime.

Opaque geometry remains batched. Each alpha instance is a separate draw, even
when several nodes share the same mesh and material. Alpha draws follow opaque
draws and sort back-to-front using the existing native-compatible nearest-bound
camera depth, quantized to millimeters. Authored depth writes, depth test, color
writes and culling remain in effect. Alpha objects do not enter shadow caster
passes, matching the native forward pipeline.

The fixture submits twelve opaque draws, one alpha draw and twelve spot-shadow
draws. Other blend modes, alpha-cut variants and skinning are not introduced by
this change. Intersecting transparent geometry retains the limitations of
object-level sorting.

## Memory and package layout

The default build preserves texture dimensions. Its 37 compiled assets occupy
169,802,050 bytes (161.94 MiB). At 960x625, tracked GPU resources occupy about
163.11 MiB, including textures, geometry, instance buffers and the shadow map.
The PBR browser host explicitly configures both asset and GPU budgets to 256 MiB.
Other applications retain the 128 MiB defaults. Browser-only `maxAssetBytes` and
`maxGPUBytes` options are positive integer byte counts, bounded at 512 MiB; they
do not change public native HG APIs or guarantee a total browser memory limit.

The loader rejects a bundle above its budget before downloading payloads. Asset
progress remains byte-based from 0 to 100%, including partial texture downloads,
integrity verification and decoding. Restart resets progress.

| Role | Directory relative to `harfangjs/` |
| --- | --- |
| Shared source assets | `../harfang3d/tutorials/resources/` |
| Staged inputs | `build/experiments/native-scene-pbr/asset-input/` |
| Compiled assets | `build/experiments/native-scene-pbr/resources_compiled/` |
| Complete static HTTP package | `dist/experiments/native-scene-pbr/` |
| Validation artifacts | `build/experiments/native-scene-pbr/reports/` |

## Run and validate

From `harfangjs/`:

```powershell
python experiments/native-scene-pbr/build.py
python experiments/native-scene-pbr/serve.py
.venv/Scripts/python.exe experiments/native-scene-pbr/validate.py --skip-build --native ../install/js_bullet/hgjs/hgjs.exe
```

Open **http://localhost:8004/**. Upload the complete release directory contents
to Apache, including the browser helper, runtime sources, manifest and compiled
payloads. The package supports deployment in a URL subdirectory. Build with
`--max-texture-size 512` for a smaller optional variant; the reference validation
expects the default original-resolution build.

Native OpenGL and browser WebGL 2 render the unchanged tutorial for twelve fixed
16,666,667 ns frames, at 960x625 without MSAA. Captures at frames 4 and 12 have
mean channel error 1.509/255; 1.128% of pixels differ by more than 16. The gate is
below 2/255 mean error and below 3% over 16. Native compressed textures versus Web
RGBA8 produce visible differences in some fine normal/reflection detail. The
camera state and node counts also match the native reference.

Automated checks cover original entry/scene preservation, payload names/hashes,
JPEG conversion and resizing, malformed-header rejection with previous-output
preservation, memory budgets, two transparent instances sharing a material,
depth sorting, opaque-only shadow casting, pause/resume, resize, Escape, restart
and zero tracked GPU resources after stop. The Many Nodes, Mouse Flight, Engine
Scene and historical W2 suites are regression checks for the shared changes.
