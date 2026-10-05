# HG JS Native / Web: Scene AAA Forward Port

## Outcome

`tutorials/scene_aaa.js` ports `scene_aaa.lua` using native HARFANG JS APIs. The
browser experiment in `harfangjs/experiments/native-scene-aaa/` copies that entry
unchanged and replaces the window helper. HG JS Web routes the AAA submission
overload to its forward renderer. Native HG JS retains real AAA and animation
support; `main({aaa:false})` selects native forward for comparison.

The original engine scene is retained, including its three cyclorama instances
and animation data. The script's manual rotation remains active. The Lua source
does not explicitly start its embedded `Take 001` animation.

## Public API and temporary stubs

The native bindings in `binding/bind_harfang.py` and engine headers are the
reference for names, argument order, defaults and return types. No Web-only
switches are added to the public HG submission call.

| API | Web behavior |
| --- | --- |
| `new ForwardPipelineAAAConfig()` | Native-bound fields and initial values, including four independent zero `Vec4` compositing parameters. Values are retained but have no rendering effect. |
| `CreateForwardPipelineAAAFromAssets(path, config, ssgi_ratio=BR_Half, ssr_ratio=BR_Half)` | Returns an opaque `ForwardPipelineAAA` stub, warns once, allocates no AAA GPU resources and loads no AAA shaders. |
| `ForwardPipelineAAA.Flip(view_state)` | Warned no-op. |
| `IsValid(aaa)` | False: there is no real AAA pipeline. The submission adapter recognizes its own live fallback handle separately. |
| `DestroyForwardPipelineAAA(aaa)` | Invalidates the fallback handle; repeat destruction is harmless. |
| `SubmitSceneToPipeline(view, scene, rect, true, pipeline, resources, aaa, config, frame)` | Validates the stub arguments and submits ordinary forward. Returns the same `[nextView, SceneForwardPipelinePassViewId]` shape as the existing forward overload. |
| `Scene.GetSceneAnim(name)` | `InvalidSceneAnimRef`, with a stub warning. |
| `Scene.GetSceneAnims()` | Empty `SceneAnimRefList`. |
| `Scene.PlayAnim(ref, loop_mode, easing, t_start, t_end, paused, t_scale)` | Accepts the native defaults and returns an invalid `ScenePlayAnimRef`; starts nothing. |
| `Scene.IsPlaying(ref)` | False. |
| `Scene.StopAnim(ref)`, `StopAllAnims()`, `UpdatePlayingAnims(dt)` | Warned no-ops; nanosecond arguments remain BigInt. |
| `Scene.GetPlayingAnimRefs()`, `GetPlayingAnimNames()` | Empty `ScenePlayAnimRefList` / `StringList`. |

Animation lists have native `get/at`, `length`, BigInt `size()`, mutation and
equality methods. `ALM_Once/Infinite/Loop`, `E_Linear`, `UnspecifiedAnimTime`,
`BR_Equal/Half/Quarter/Eighth/Sixteenth/Double` and AAA debug enum values use the
native values. Other AAA overloads/configuration I/O and animation players,
skinning or evaluation are not claimed as implemented.

Warnings are deduplicated per browser application: ignored MSAA, forward AAA
fallback and ignored animation playback. Animation data is not silently dropped
from compiled scenes. `assetc-web` requires `--animation-stubs` and records
`animationPlayback: "stub"` in the manifest before the runtime accepts tracks.

## Additional forward work

The previous Mouse Flight subset could not load the engine scene directly:

| Required content | Implementation |
| --- | --- |
| Normal and ORM textures | PBR adapter `pbr-scene-instanced/2`; optional normal/ORM samplers and native tangent-space normal reconstruction. |
| Tangent frames | Geometry compilation preserves tangent and bitangent vectors in a 14-float vertex layout where present; existing 8-float meshes still work. |
| Spot plus directional shadows | Separate spot depth target alongside the four-split directional atlas; highest-priority local spotlight casts the spot shadow. |
| Authored scene instances | Recursive loading, parent attachment, native node enumeration, instance views, disabled state and transactional rollback; cycles/depth overflow fail. |
| `environment.probe` | The engine's zero-parallax probe uses its HDR irradiance/radiance maps. Nonzero parallax remains rejected. |
| Residual Phong uniforms in PBR materials | The native facade retains `uDiffuseColor` / `uSpecularColor` as inactive values, matching the native PBR shader. The historical W2 material contract remains strict. |

Native shader sources are still pinned by the compiler's existing provenance
hashes. The new adapter is derived from those same sources. The old PBR adapter
descriptor remains readable by the runtime.

## Asset policy and layout

Original sources stay under `tutorials/resources/`. Staging and compiled Web
assets are in `harfangjs/build/experiments/native-scene-aaa/`; the HTTP package
is `harfangjs/dist/experiments/native-scene-aaa/`.

All compiled assets keep their original logical filenames and extensions.
Hashes remain in the manifest for integrity; no GUID/hash filenames are used.
The compiled scene JSON preserves the source's full parsed content.

The experiment uses a configurable **1024-pixel PNG limit**. Twelve original
2048-square material textures alone need about 256 MiB with mipmaps in the
current RGBA8 representation. The 1024 setting keeps the existing 128 MiB
asset/GPU limits. `--max-texture-size N` resizes inside native `assetc-web`, before
mip generation; omitting it preserves original dimensions. Native `BC3` texture
metadata is accepted with an explicit RGBA8 conversion warning and recorded in
the manifest. HDR/BRDF conversion remains floating point and is not resized by
this PNG option.

The compiled release contains 109 assets and approximately 118.4 MiB of payloads.
Rendering uses approximately 90.8 MiB of tracked GPU resources, 126 nodes,
91 object components (90 enabled render instances), 85 main draws and 425 shadow
draws. These counters describe this fixture, not a general performance promise.

## Validation and testing

From `harfangjs/`:

```powershell
python experiments/native-scene-aaa/build.py
python experiments/native-scene-aaa/serve.py
.venv/Scripts/python.exe experiments/native-scene-aaa/validate.py --skip-build --native ../install/js_bullet/hgjs/hgjs.exe
```

Open **http://localhost:8003/**. Escape stops; Pause/Resume and Restart are
available. Build scripts preserve the source entry byte-for-byte.

The initial Windows/Chrome run compares 60 fixed-step frames with native OpenGL
forward at 960x625, without MSAA. Engine rotation and the shared API probe pass
the native comparison. Image mean channel errors are approximately 0.864/255 at
frame 4 and 0.865/255 at frame 60; respectively 0.392% and 0.451% of pixels differ
by more than 16. Native uses original texture resolution/compression, while Web
uses the documented 1024 RGBA8 conversion. These images compare forward output,
not the native AAA effects.

Validation also covers compiler determinism, resizing, animation opt-in,
failed-publication preservation, three instance views, cycle rollback, stub
return types/warnings, pause/resume, resizing, restart and resource release to
zero. Many Nodes, Mouse Flight and the historical source suite are regression
checks. Reports and screenshots are generated in
`harfangjs/build/experiments/native-scene-aaa/reports/`.

AAA effects, animation playback, skinning and parallax-corrected probe volumes
remain deferred. No changes were made to native HARFANG's supported features.
