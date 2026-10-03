# Hybrid HARFANG: Tutorial-Based Validation Matrix

Date: 2026-10-03

Status: validation specification, based on source and asset inspection. No JavaScript tutorial ports or runtime test results are claimed.

Related documents: [feasibility study](SPECS_HYBRID_CPP_JS_WEBGL_FEASIBILITY.md), [delivery slices](SPECS_HYBRID_CPP_JS_WEBGL_DELIVERY_SLICES.md), and [existing tutorial instructions](../tutorials/README.md).

Tutorial baseline: HARFANG `6a683714fa208d3791a76abc51aa88570e463373`. The top-level `tutorials/` directory contains **56 tutorial families and 158 Python/Lua/Squirrel entry files**. Language variants of the same family count as one scenario, not three independent coverage items. This inventory is separate from the earlier engine baseline recorded in the feasibility study.

## 1. Decision

**Use the existing HARFANG tutorials as the primary acceptance suite for the web port's delivery slices.** Port the relevant scenarios to shared JavaScript and run them on native HARFANG and the pure-JS/WebGL backend. Add focused fixtures only where the tutorials do not cover a promised feature or failure mode.

Keep their observable purpose, source assets, and important operations. A port may adapt host setup, asynchronous loading, named math operations, output values, and the portable UI API. It must not remove the feature being tested merely to make the scenario pass.

The desktop entry remains `main.js`, which explicitly calls `init`, `update`, `render`, and `dispose`. The browser entry uses the same tutorial/application module through animation-frame scheduling. Browser code and dependencies remain pure JS, without Wasm. Physics and video remain excluded.

“Excluded” means excluded from the hybrid acceptance suite. Existing native tutorials remain in the repository.

## 2. Classification And Counting Rules

| Classification | Count | Meaning |
| --- | ---: | --- |
| Retained | 25 | Mandatory for the specified slice/profile once its declared prerequisites ship; adaptation or staged variants may be necessary |
| Deferred | 12 | Relevant concepts, but an additional capability beyond the current minimum profile; not silently counted as validated |
| Excluded | 19 | Outside the selected product or dependent on explicitly excluded native services |
| **Total** | **56** | Every inspected top-level tutorial family is accounted for |

Execution status is separate from this classification. Record `not-ported`, `not-run`, `blocked`, `pass`, `fail`, or `not-applicable-to-profile`. An unsupported required feature is a failure, not a skip. A conditional case is not applicable only when the release profile explicitly omits its optional capability.

Lua is the preferred starting reference because it matches the existing `main.lua` workflow. Inspect the Python/Squirrel versions when resolving discrepancies; do not assume their return handling and behavior are identical. For example, `CreateInstanceFromAssets` results are unpacked differently across the existing language variants.

## 3. Retained Tutorials

Slice identifiers follow the delivery plan: W0 foundations, W1 static scenes/assets, W2 materials/lights, W3 shadows, W4 textures, W5 instances, W6 animation, W7 skinning, W8 audio, W9 UI, W10 hardening. Every retained JS scenario also contributes to the native N and shared-contract C workstreams.

| Tutorial family / source | Acceptance slice | What must survive the JS adaptation |
| --- | --- | --- |
| [basic_loop](../tutorials/basic_loop.lua) | W0 | Visible clear color, frame progression, input/stop handling, explicit desktop loop, clean disposal |
| [render_resize_to_window](../tutorials/render_resize_to_window.lua) | W0-W1 | Resize/projection agreement and correct drawing-buffer dimensions; native window and browser canvas use their own host mechanisms |
| [input_read_keyboard_basic](../tutorials/input_read_keyboard_basic.lua) | W0 | Current key state for supported keys, focus behavior, orderly stop; replace the busy console loop with a scheduled sample |
| [input_read_keyboard_advanced](../tutorials/input_read_keyboard_advanced.lua) | W0 | Pressed/down/released transitions, no repeated one-shot events, focus-loss reset |
| [input_read_mouse_basic](../tutorials/input_read_mouse_basic.lua) | W0 | Supported buttons and pointer coordinates relative to the application surface |
| [input_read_mouse_advanced](../tutorials/input_read_mouse_advanced.lua) | W0 | Stateful transitions and deltas; explicitly document CSS-pixel/drawing-buffer conversion |
| [draw_lines](../tutorials/draw_lines.lua) | W0-W1 | Dynamic vertex upload, line drawing, color/clear behavior, correct time-based changes |
| [draw_lines_starfield](../tutorials/draw_lines_starfield.lua) | W0/W10 | Colored dynamic lines and repeatable motion; seed random generation and record actual vertex count |
| [draw_model_no_pipeline](../tutorials/draw_model_no_pipeline.lua) | W1 | Cube/plane creation, explicit transforms, basic program/vertex-layout mapping, draw submission without a scene pipeline |
| [filesystem_assets](../tutorials/filesystem_assets.lua) | W1/W4 | Resolve `pictures/owl.jpg` through the compiled asset namespace and report a valid texture with correct dimensions; use an async resource API |
| [picture_load](../tutorials/picture_load.lua) | W1/W4 | Decode an ordinary image and report its dimensions; replace the source-folder filesystem path with a compiled logical asset ID |
| [scene_pbr](../tutorials/scene_pbr.lua) | W1 then W2/W4 | Load `materials/materials.scn`, preserve scene/camera/material assignment, then validate the PBR appearance and selected texture path |
| [material_update_value](../tutorials/material_update_value.lua) | W2/W4, W3 for original shadow setting | Despite its name, this example toggles `uDiffuseMap` and updates the material program variant; retain that on/off transition and resource validity |
| [scene_light_priority](../tutorials/scene_light_priority.lua) | W2 | Select among 16 moving point lights according to priority and available slots; compare selection as well as appearance |
| [scene_spot_shadow_clip](../tutorials/scene_spot_shadow_clip.lua) | W3, conditional on spot-shadow capability | Preserve animated shadow near plane, fixed far plane, and caster exclusion; the diagnostic text may use a simple overlay |
| [scene_instances](../tutorials/scene_instances.lua) | W5 then W6 | Independent subscene state, add/remove actors, instance-local animation lookup, idle/walk/run transitions; staged variants are defined below |
| [mouse_scene_projection](../tutorials/mouse_scene_projection.lua) | W0/W1/W2/W5 | Unprojection and world-space targeting; the loaded scene itself references a nested `target_rectangle.scn` instance |
| [game_mouse_flight](../tutorials/game_mouse_flight.lua) | W5/W10, with W0-W2 | Input-controlled paper-plane instance, chase camera, transform math, and line overlay; this is kinematic JS behavior, not a physics tutorial |
| [scene_lua_script](../tutorials/scene_lua_script.lua) | C/N/W0 | Rework as a JS behavior-communication scenario: read/write state, invoke functions with arguments/handles, return values, missing-call errors; do not embed Lua |
| [scene_many_nodes](../tutorials/scene_many_nodes.lua) | W1/W2/W10; W3 for shadow variant | Shared models and many independently updated transforms; separate a bounded correctness case from the original large stress workload |
| [audio_play_sound_stereo](../tutorials/audio_play_sound_stereo.lua) | W8 | Load the compiled WAV asset, loop, vary panning, stop/dispose; browser activation is explicit |
| [audio_play_sound_spatialized](../tutorials/audio_play_sound_spatialized.lua) | W8, conditional on spatial-audio capability | Move a looping source and preserve the declared spatial convention; distinguish spatialization from stereo panning |
| [imgui_basic](../tutorials/imgui_basic.lua) | W9 | The same panel/text function runs through Dear ImGui on desktop and the JS DOM/CSS backend on the web |
| [imgui_edit](../tutorials/imgui_edit.lua) | W9 | Choice, reset button, color editing, numeric value/clamping, and visible application changes; native bgfx view IDs are not the portable UI contract |
| [imgui_mouse_capture](../tutorials/imgui_mouse_capture.lua) | W9/W10 | Text editing and pointer capture prevent simultaneous scene interaction; add keyboard-focus and composition assertions |

Retaining these low-level draw examples commits the profile to a **small explicit drawing subset**: dynamic lines, cube/plane primitives, the layouts they use, and reviewed versions of their simple shaders. It does not imply every bgfx program, vertex layout, or draw API is portable. Record that subset in C and implement it during W0-W1.

## 4. Deferred Tutorials

These are relevant extensions, not invalid examples. Their capabilities must be estimated and declared before they become release gates. Listing them does not add them to the existing V1 estimate.

| Tutorial family / source | Potential slice/capability | Why it is deferred or needs a separate decision |
| --- | --- | --- |
| [audio_stream_ogg_stereo](../tutorials/audio_stream_ogg_stereo.lua) | W8 streaming extension | True streaming, codec support, buffering, and panning require their own contract; decoding the entire file is not proof of streaming |
| [draw_and_create_model_no_pipeline](../tutorials/draw_and_create_model_no_pipeline.lua) | W1 procedural-model extension | Uses `ModelBuilder` beyond the fixed cube/plane subset; useful once arbitrary procedural mesh construction is advertised |
| [model_builder](../tutorials/model_builder.lua) | W1 dynamic-model extension | Rebuilds/replaces procedural grid models; requires explicit buffer replacement and disposal semantics, beyond loading static models |
| [model_builder_iso_surface](../tutorials/model_builder_iso_surface.lua) | Procedural iso-surface extension | Combines iso-surface generation, AAA, and UI; a basic-forward derivative can test pure-JS geometry generation later, without restoring AAA scope |
| [draw_text](../tutorials/draw_text.lua) | Engine text/overlay extension | The existing example exercises font resources, text alignment, and render state; a DOM label alone cannot claim full `DrawText` conformance |
| [draw_text_over_models](../tutorials/draw_text_over_models.lua) | Engine text/compositing extension | Adds a second drawing view and text-over-3D ordering; separate from the current portable UI controls |
| [input_read_gamepad](../tutorials/input_read_gamepad.lua) | W0 gamepad extension | Useful if gamepad support is declared, with connection/mapping/device coverage; keyboard/pointer foundations do not establish it |
| [scene_draw_to_multiple_viewports](../tutorials/scene_draw_to_multiple_viewports.lua) | Multi-view renderer extension | Retain four views and one scene update per frame; native preparation/view-ID APIs need an explicit portable counterpart |
| [scene_draw_to_texture](../tutorials/scene_draw_to_texture.lua) | Render-target sampling extension | Render a scene into a texture then sample it on a model; the original requests RGBA32F and MSAA settings, which need a documented capability/fallback policy |
| [scene_capture_texture](../tutorials/scene_capture_texture.lua) | GPU readback/image-export extension | Tests capture completion and image export; this is a still-image feature, **not video**, but is outside the first profile |
| [picture_save](../tutorials/picture_save.lua) | Image-export extension | JPEG-to-PNG conversion is meaningful on the web, but a download/blob service differs from arbitrary local file writes; defer the export API explicitly |
| [screen_size_configurator](../tutorials/screen_size_configurator.lua) | W0/W9 display-settings extension | A browser derivative can configure render resolution and supported display modes; native monitor enumeration and borderless multi-monitor window modes are excluded |

Do not infer these extensions from internal implementation details. Having a shadow framebuffer does not automatically expose a public render-to-texture API, and having DOM text for UI does not implement HARFANG's engine text renderer.

## 5. Excluded Tutorials

| Tutorial family / source | Reason |
| --- | --- |
| [filesystem_local](../tutorials/filesystem_local.lua) | Arbitrary local file handles are outside the browser profile; asset-loading coverage comes from `filesystem_assets` |
| [filesystem_recursive_directory_listing](../tutorials/filesystem_recursive_directory_listing.lua) | Recursive native directory enumeration is outside the asset-manifest model |
| [input_list_devices](../tutorials/input_list_devices.lua) | Lists native keyboard/mouse/gamepad device names; browser input/capability discovery has different semantics |
| [audio_play_tts_say](../tutorials/audio_play_tts_say.lua) | Depends on `lib-say` synthesis and a native PCM bridge; speech synthesis is outside the audio subset |
| [audio_stream_mod_xm_stereo](../tutorials/audio_stream_mod_xm_stereo.lua) | Native module/tracker decoding is outside the initial audio subset; no Wasm plugin fallback |
| [physics_impulse](../tutorials/physics_impulse.lua) | Physics excluded |
| [physics_kapla](../tutorials/physics_kapla.lua) | Physics excluded |
| [physics_manual_setup](../tutorials/physics_manual_setup.lua) | Physics excluded |
| [physics_overrides_matrix](../tutorials/physics_overrides_matrix.lua) | Physics integration semantics are its purpose; UI presence does not make it a portable UI test |
| [physics_pool_benchmark](../tutorials/physics_pool_benchmark.lua) | Physics benchmark excluded |
| [physics_pool_of_objects](../tutorials/physics_pool_of_objects.lua) | Physics excluded; retain `scene_many_nodes` for non-physics scaling |
| [physics_pool_stress](../tutorials/physics_pool_stress.lua) | Physics stress excluded |
| [scene_aaa](../tutorials/scene_aaa.lua) | AAA rendering excluded; its assets may still be reused in a separately named basic-forward fixture |
| [scene_aaa_vignette](../tutorials/scene_aaa_vignette.lua) | AAA/vignette feature excluded |
| [scene_dof](../tutorials/scene_dof.lua) | AAA depth-of-field excluded |
| [scene_vr](../tutorials/scene_vr.lua) | Native OpenVR and stereo/VR integration excluded |
| [scene_vr_controllers](../tutorials/scene_vr_controllers.py) | Native VR controller integration excluded; this family currently has only a Python entry |
| [scene_vr_teleporter](../tutorials/scene_vr_teleporter.lua) | VR interaction/device requirements excluded |
| [scene_xr](../tutorials/scene_xr.lua) | Native OpenXR integration excluded |

No top-level video-playback tutorial was found in this inventory. This does not change the explicit exclusion of video from the product. Do not classify still-image capture/export as video merely because it reads a rendered frame.

## 6. Compound Tutorials Must Have Named Stages

### 6.1 scene_pbr: Scene Loading Before PBR Completeness

The inspected `materials/materials.scn` has 15 nodes, 13 object components, one camera, one light, and the PBR program family. Use its hierarchy, camera, and model/material assignments for an early W1 structural check. A declared unlit diagnostic rendering can help inspection, but must be recorded as `scene_pbr.structure`, not a passing PBR result.

`scene_pbr.materials` becomes a W2 gate. W4 reruns that scenario with each supported texture route. Preserve separate results so an early scene-loader success does not conceal missing normal maps or incorrect color conversion.

### 6.2 scene_instances: Separate Instances From Animation

The original creates 20 bipeds, randomly switches among `idle`, `walk`, and `run`, uses instance-specific animation lookup, and supports spawning/deleting actors. Split it into:

| Proposed case ID | Gate | Required behavior |
| --- | --- | --- |
| `scene_instances.static` | W5 | Multiple copies of a static subscene, independent transforms, scoped lookup, spawn/delete, resource sharing and cleanup |
| `scene_instances.animated` | W5+W6 | Preserve the original actor state machine and instance-local clip lookup, with a fixed seed and scripted add/remove events |
| `scene_instances.nested` | W5 | Add a deliberate nested-instance asset derived from existing tutorial content; validate recursive remapping and cycle errors |

For the static case, use the existing single-object paper-plane subscene or an explicit static derivative. Do not implicitly load an animated dependency and silently ignore all unsupported animation data. The derivative's asset recipe and omissions must be recorded.

**The biped tutorial is not currently a skinning proof.** Its inspected JSON has 187 nodes, 81 objects, 261 animation records, and 11 named scene animations, but no nonempty object `bones` references or `EnableSkinning` material markers were found. The example demonstrates hierarchical animation of its authored parts; a walking humanoid appearance does not establish weighted vertex deformation.

### 6.3 mouse_scene_projection: A Hidden Instance Dependency

The loaded scene references `mouse_scene_projection/target_rectangle.scn`. The full tutorial therefore depends on W5 even though its visible purpose is camera/pointer projection. A small standalone projection fixture can validate the math earlier; it does not count as passing the original nested-scene scenario.

### 6.4 scene_many_nodes: Correctness And Stress Are Different Results

The Lua source builds a 101-by-101 grid: **10,201 moving object nodes**, plus its other scene objects, and requests a 4096 shadow map. It also manually invokes Lua garbage collection.

Use a declared smaller grid for the mandatory correctness gate and retain the original workload as an independently named stress case. Record object counts, quality settings, shadow resolution, draw counts, frame-time percentiles, and memory. Do not compare a reduced browser workload against the full native example as a performance win. The JS port does not inherit Lua's `collectgarbage()` call or assume a browser force-GC API.

### 6.5 game_mouse_flight: Deterministic Time Policy

The existing flight logic uses several per-frame movement/smoothing constants even though it reads `dt`. For reference comparisons, replay both implementations with the same fixed update schedule. If converting the controls to elapsed-time-based behavior, apply the same documented change to the shared JS tutorial on both targets and record the difference from the original sample.

### 6.6 UI Tutorials Validate The Portable Facade

Port the three retained ImGui scenarios through `harfang/ui`. Keep choice/reset/color/text/capture behavior; do not import native ImGui flags or view IDs into the browser UI layer.

In `imgui_edit`, expose color editing as a small portable control or a documented composition of scalar controls. Replace its editable ImGui output-view number with an application-owned bounded integer in the portable derivative; keep the native view-routing demonstration native-only. Report this boundary instead of claiming identical low-level UI rendering behavior.

## 7. Implications For The Implementation Slices

The corpus makes several requirements concrete:

1. **W0-W1 need a small direct-drawing subset.** The line and cube/plane tutorials are useful foundations before scene loading. Support their known programs/layouts instead of inventing a general bgfx compatibility layer.
2. **W2 needs the historical default material family as well as PBR.** `material_update_value`, `scene_light_priority`, `scene_many_nodes`, and the spot-shadow example use `core/shader/default.hps`, including `uDiffuseColor`, `uSpecularColor`, `uSelfColor`, and `uDiffuseMap`. Implement its tested subset; silently substituting PBR would change the regression reference.
3. **W3 has a strong existing spot-shadow test but lacks a complete directional-cascade test.** Add the latter; a spot clipping pass alone cannot validate directional shadows.
4. **W5-W6 can reuse real scenes and animations.** Preserve the original content graph; qualify derivatives separately.
5. **W7 needs a weighted-mesh skinning fixture.** None of the 12 inspected tutorial scene JSON files supplied the object/material skinning markers needed to establish that coverage.
6. **W9 can reuse the existing UI interactions.** The objective is matching control behavior across Dear ImGui/DOM, not matching their internal rendering APIs.

These refinements replace vague acceptance examples with concrete content and expose coverage gaps. They do not authorize adding every deferred capability. Re-estimate the affected slices after the first tutorial ports establish the cost of the direct-drawing/default-material adapters; tutorial availability alone does not justify reducing the overall estimate.

## 8. Minimal Additional Fixtures

Tutorials are the primary suite, but the supported product still needs targeted checks that they do not contain.

| Gap | Supplement | Gate |
| --- | --- | --- |
| Shared JS value/lifetime semantics | Copy-versus-reference math values, exact integer boundaries, outputs, invalidation, shutdown callbacks | C/N/W0 |
| Required async browser lifecycle | Stop during loading, failure/cancellation, resume timing, repeated start/stop | W0/W10 |
| Directional shadow coverage | Camera motion, cascade boundaries, bias, alpha-cut casters | W3 |
| GPU texture codecs and color spaces | Known-size mip chains, ASTC/fallback selection, normals/masks/alpha corpus, context restoration | W4 |
| Deep scene-instance behavior | Nested references, duplicate names, cycles, partial-load rollback | W5 |
| Animation edge cases | Fixed-time samples, seek/loop boundaries, Hermite/quaternion cases, exact timestamps | W6 |
| Weighted skinning | Two-bone weighted mesh plus representative skinned character, two independently animated instances, shadow deformation | W7 |
| Audio restrictions/lifetime | Activation, suspend/resume, stop during load, source disposal, shared decoded resource lifetime | W8 |
| Robust UI editing | Stable IDs during reordering, focus/selection/composition, one action per frame | W9 |
| Sustained runtime behavior | Context loss, repeated unload/reload, cold-cache startup, zero-Wasm dependency audit | W10 |

Prefer deriving these from existing tutorial resources and hosts. The skinning mesh and missing interpolation cases may require new authored data. Synthetic fixtures should remain small enough to diagnose a failure precisely.

## 9. Native/Web Comparison Protocol

### 9.1 Three References

For each retained family, keep:

1. The pinned original native Lua/Python/Squirrel source and its intended behavior.
2. The shared JS port running against native C++ HARFANG, with explicit desktop `main.js` lifecycle calls.
3. The same shared JS port running against the pure-JS/WebGL engine.

First establish that the JS-native port preserves the original intent. Then compare JS-native with JS-web under the same profile. Agreement between two identically simplified ports is not enough if both dropped the tutorial's defining feature.

Use the portable profile on the native run, including matching light limits, supported material families, and shadow quality. Keep any full-native comparison as a separate reference. Rendering reductions are documented expected differences; scene transforms, material state transitions, resource IDs, and lifecycle counts are behavioral assertions.

### 9.2 Assets And Determinism

Build the tutorial source resources through `assetc` into separate native and web output trees. The existing `_build_assets.bat` invokes a native compiler and is not a web build path. A proposed harness can use `build/tutorials/assets-native` and `build/tutorials/assets-web`, without reusing a stale `resources_compiled` directory.

Record the engine/tutorial revision, source asset hashes, compiler/encoder configuration, API/profile/schema versions, random seed, update times, input sequence, viewport, pixel ratio, and quality tier. All runtime asset access uses compiled logical IDs, including examples whose originals directly open `resources/...` or `resources_compiled/...` filesystem paths.

Warm required shaders/assets before timed steady-state comparisons; measure cold loading separately. Pin random initial positions and clip choices in `scene_instances` and `draw_lines_starfield`. For interactive cases, provide a deterministic input/command sequence plus a short manual browser check for physical focus/audio/device behavior.

### 9.3 Evidence Required For A Pass

- No unhandled exceptions, missing dependencies, shader failures, or unsupported-feature fallbacks outside the declared profile.
- Numeric/state assertions for the tutorial's purpose: counts, transforms, active lights, selected animation, texture variant changes, input transitions, or UI actions.
- Captures at fixed checkpoints where visuals matter, with the same camera and render dimensions. Compare regions/features using per-case tolerances; do not require identical antialiasing or raster output across GPUs.
- Audio cases use control-state/timing assertions and an audible check where applicable. Screenshots cannot establish audio correctness.
- Resource/lifecycle accounting before and after disposal; no callbacks into unloaded scenes or UI panels.
- Performance reports for stress cases, with workload/quality explicitly attached. A screenshot does not establish frame-time or memory budgets.

The acceptance manifest should record `sourceFamily`, `sourceRevision`, `caseId`, `classification`, `slice`, `requires`, `adaptations`, `assetRecipe`, `seed`, `checkpoints`, `assertions`, `visualTolerance`, and `executionStatus`. The runner and manifest are proposed implementation deliverables, not files created by this study.

### 9.4 Per-Slice Release Rule

A slice passes when all retained cases applicable to its advertised profile pass on native JS and the chosen browser/device matrix, together with the small supplemental fixtures for that slice. Dependencies from earlier slices remain regression gates.

Deferred and excluded rows remain visible in reports with their reasons and never contribute to a success percentage. Report pass counts per case and capability, separately from the 56-family inventory. The full `scene_instances.animated` case can only pass once both W5 and W6 are available; its static variant does not stand in for it.

## 10. Inspection And Next Implementation Step

Performed for this specification: enumeration of all 56 top-level tutorial families and 158 language files, API/asset-reference inspection, focused source review of compound examples, and JSON inspection of the 12 available tutorial scene files. The classification is based on that evidence; no tutorial was executed or converted to JavaScript in this documentation update.

Start implementation with `basic_loop`, keyboard/mouse input, `draw_lines`, `draw_model_no_pipeline`, `filesystem_assets`, and `scene_pbr.structure`. Then add the default/PBR material scenarios, shadows, instances/animation, skinning fixtures, audio, and UI according to the dependency graph. This gives every delivery slice a visible, familiar HARFANG acceptance target.
