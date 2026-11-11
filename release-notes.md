# [3.3.1] - 2026-11-11

This release adds native JavaScript bindings, a native Gaussian splatting pipeline, scene animation crossfades, portable texture compression, and LZMA archives. It also improves macOS rendering and input, refreshes the core asset package, and provides a single Windows rebuild entry point.

### JavaScript

* Added the native QuickJS language layer, generated HARFANG bindings, the `HarfangJs` CMake target and `hgjs` executable, enabled with `HG_BUILD_HG_JS`.
* Added module loading, exception reporting, promise processing, the `harfang-host` frame helper, and interoperability with the engine's existing Lua scene scripting. Embedded scene scripts remain Lua-based.
* Added 32 JavaScript tutorials covering rendering, scenes, materials, ImGui, physics, input, filesystems, and audio, with API parity and tutorial validation tools.
* Added `rebuild_hg_js_bullet.bat`. Windows builds use official QuickJS 2026-06-04 and Zig 0.14.1 for the embedded C runtime, with MSVC for HARFANG and its bindings.
* Documented native/Web compatibility priorities and WebGL delivery experiments. The separate Web work and feasibility studies do not imply native API parity in browsers.

### Gaussian splatting and compute

* Added native Gaussian model loading, GPU resource management, covariance projection, spherical harmonics through SH3, depth sorting, drawing, and reusable per-view render data.
* Added `gsplatc` and AssetC metadata profiles for `.splat`, supported binary little-endian PLY, and SPZ v2/v3 sources. Compiled HGS2 supports compact quantized storage and a float32 reference profile.
* Preserve every source splat by default; reduction requires an explicit `max-count`. Imports validate the entire source and reject unsupported formats or resource budgets instead of silently dropping data.
* Added GPU sorting optimizations, immutable uploaded inputs, camera-dependent caching, compute capability reporting, buffer/image bindings, dispatch helpers, and asynchronous frame statistics.
* Added a native `TrackballController` with orbit, pan, dolly, reset, and fractional wheel input.
* Consolidated the Lua Gaussian tutorial around the bundled Houseplant and Saari datasets, with model switching, per-model cameras, SH controls, an RGB orientation gizmo, packaging targets, and automated rendering checks.
* Fixed full-count rendering and model selection on Retina displays, pending-frame resource shutdown, and Metal MSAA color preservation when compute dispatches interrupt rendering.
* Windows validation during release preparation exercised the Saari model with all 1,307,339 splats, camera gestures, resize, focus recovery, slide revisits, MSAA composition, and clean shutdown. This does not qualify every graphics backend or hardware configuration.

### Animation

* Added the scene-owned `SceneAnimPlayer` for transform-animation crossfades with configurable duration and easing, updated blending semantics, scripting bindings, and regression coverage.
* Added clip metadata queries and transform-animation import between scenes, with hierarchy-based node mapping and optional completion of missing position, rotation, and scale channels.
* Documented two-track transitions and fixed animation loop-flag handling on macOS.

### Rendering, platform, and input

* Added drawable framebuffer-size queries and corrected render initialization and resize handling for Retina displays.
* Added native macOS gamepad support through GameController and preserved fractional mouse-wheel deltas in GLFW input and ImGui integration.
* Fixed macOS shadow-map behavior and ensured the global timer thread is joined at process exit.

### Asset tools and packaging

* Added the portable `texc` image-to-DDS compiler with BC1, BC3, BC4, BC5, BC7, RGBA8, and BGRA8 outputs. Fixed rectangular mip chains, uncompressed DDS layout, resize limits, and write-error reporting, with AssetC integration checks.
* Added LZMA compression and reading to legacy archives. Packing now defaults to LZMA; `--method zlib` preserves compatibility with older runtimes and launchers. Raw and existing Zlib archives remain supported.
* Vendored Assimp and consolidated the maintained core assets in `core-package`, including Gaussian shaders and synchronization checks for tutorial resources.
* Removed the in-tree Mini Studio prototype and reorganized archived specifications.
* Updated Windows rebuild helpers and added `rebuild_essentials.bat [Config]`, which runs AssetC, FFmpeg, Lua/Bullet, Python/Bullet, JavaScript/Bullet, static documentation, converters, and the XM player in order, stopping on the first failure.
* Regenerated static documentation for 3.3.1 and made its build depend on `harfang/version.txt`, so version changes invalidate the generated pages.

### Video fixes

* Fixed a race in the FFmpeg plugin where immediate stream closure could be overwritten by decoder-thread startup and block shutdown.
* Do not expose a video frame before decoding has produced one. Added a lifecycle regression check covering 100 immediate play/close cycles.
* Windows release validation decoded, sought, and closed all 21 videos in the interactive-book integration, and exercised the deployed console/no-console launchers and FBX/glTF tools.

### API additions since 3.3.0

The inventory below covers public engine additions and newly exposed scripting APIs, compared with the `v3.3.0` tag. Unless specified otherwise, engine entries are available in C++ and the generated Python, Lua, Squirrel, and native JavaScript bindings, subject to build options. Native-only codecs and language-specific additions are identified separately.

#### Scene animation playback and import

* `Scene.CreateAnimPlayer(clips)`, `Scene.CreateInstanceAnimPlayer(node)`, and `Scene.DestroyAnimPlayer(player)` manage scene-owned `SceneAnimPlayer` handles. Copies refer to the same player; playback updates through the scene's animation update.
* `SceneAnimPlayer` exposes `IsValid()`, `SetCrossFadeDuration(duration)`, `GetCrossFadeDuration()`, `SetCrossFadeEasing(easing)`, `GetCrossFadeEasing()`, `Play(name, loop_mode = ALM_Once, restart = false)`, `IsTransitioning()`, and `Stop()`. Durations use `time_ns`; supported easing values are `E_Linear` and `E_SmoothStep`. Players accept transform clips and reject conflicting channel ownership, discrete tracks, and nested animation tracks.
* `GetSceneAnimInfo(scene, ref)` returns `SceneAnimInfo` with `valid`, `name`, `t_start`, `t_end`, and `frame_duration`.
* `BuildSceneAnimNodeMap(source, destination)` matches nodes by their full hierarchy paths. `SceneAnimNodeMap` exposes `success`, `message`, `source_nodes`, and `destination_nodes`; ambiguous paths and missing destination nodes are reported as failures.
* `ImportSceneAnim(source, source_anim, destination, node_map, name, preserve_missing_trs = true)` copies a transform clip into another scene. By default, missing transform channels are completed from the source pose. `SceneAnimImportResult` exposes `success`, `message`, the imported `anim`, `copied_channels`, and `completed_channels`. Invalid mappings, unsupported tracks, and duplicate destination animation names are rejected before publishing the imported data.
* Exposed the existing C++ load flag `LSSF_DoNotLoadResources` in scripting's `LoadSaveSceneFlags`, for loading scene structure and animation data without loading its rendering resources.

#### Compute shaders and GPU resources

* `GetComputeCaps()` returns `ComputeCaps`: `supported`, `rgba32f_vertex_sampling`, `rgba32f_image_write`, `image_write_reporting_complete`, `max_bindings`, `max_views`, `max_texture_size`, and `renderer`. The reporting-completeness flag distinguishes backend reporting limitations from a confirmed lack of image-write support.
* `GetComputeFrameStats()` returns `ComputeFrameStats`: `gpu_timing_available`, `gpu_timestamp`, `gpu_ms`, `draw_calls`, and `compute_calls`. Timing describes the latest completed backend measurement and may lag the current frame.
* Added `ComputeBuffer` and `ComputeBufferType` (`CBT_Float4`, `CBT_UInt4`), with `CreateComputeBuffer(count, type = CBT_Float4)`, `CreateComputeBufferFromData(data, type = CBT_Float4, offset = 0)`, `LoadComputeBufferFromFile(path, type = CBT_Float4, offset = 0)`, and `LoadComputeBufferFromAssets(path, type = CBT_Float4, offset = 0)`. Uploaded data uses immutable GPU storage and accepts read bindings only.
* Added `IsValid(buffer)`, `GetComputeBufferCount(buffer)`, and `DestroyComputeBuffer(buffer)`. Buffer copies share ownership; explicit destruction invalidates their aliases and bindings.
* Exposed the existing C++ functions `LoadComputeProgramFromFile(path, silent = false)` and `LoadComputeProgramFromAssets(path, silent = false)` to scripting.
* Added scripting access constants `AC_Read`, `AC_Write`, and `AC_ReadWrite` in the `Access` enum, and the texture flag `TF_ComputeWrite`.
* Added `MakeComputeBufferBinding(buffer, stage, access)` and `MakeImageBinding(texture, stage, access, mip = 0)`, returning `ComputeBufferBinding` and `ImageBinding`. Their scripting collection types are `ComputeBufferBindingList` and `ImageBindingList`; these and `GaussianSplatInstanceList` follow the existing list constructor, sequence-access, `clear()`, `reserve(count)`, and `push_back(value)` conventions.
* `DispatchCompute(view_id, program, x, y = 1, z = 1)` submits a compute dispatch. The full overload additionally accepts uniform `values`, sampled `textures`, storage `images`, compute `buffers`, and an optional `debug_name`. Successful dispatch advances `view_id`; validation failure returns `false` without submission or view advancement.
* `CreateComputeQuadModel(count)` creates batched quads addressable by compute-generated data. `ComputePremultipliedRenderState()` creates a premultiplied-alpha render state with color writes and no depth test/write or face culling.
* C++ additionally declares `ComputeBufferMaxBytes` and `ComputeQuadMaxCount`. `ReleaseComputeResources()` supports renderer shutdown, while `DispatchComputeSameView(...)` is the internal scheduling variant that leaves view advancement to its caller.

#### Gaussian splatting runtime

* Added shared resource handles `GaussianSplatModel`, `GaussianSplatRenderer`, and `GaussianSplatRenderData`, with corresponding `IsValid(...)` overloads and `DestroyGaussianSplatModel(...)`, `DestroyGaussianSplatRenderer(...)`, and `DestroyGaussianSplatRenderData(...)`. Explicit destruction invalidates aliases; C++ renderer teardown uses `ReleaseGaussianSplatResources()`.
* `LoadGaussianSplatModelFromFile(path, options)`, `LoadGaussianSplatModelFromAssets(path, options)`, and `LoadGaussianSplatModelFromData(data, options)` load compiled HGS2 data. The optional `GaussianSplatLoadOptions` exposes `max_file_bytes`. `GaussianSplatLoadResult` exposes `ok`, `message`, and `model`.
* `CreateGaussianSplatRendererFromFile(shader_root)`, `CreateGaussianSplatRendererFromAssets(shader_root)`, and `CreateGaussianSplatRenderData()` create the renderer and reusable submission data.
* `GaussianSplatInstance(model, world = Mat4.Identity, id = 0, opacity = 1)` and `MakeGaussianSplatInstance(...)` describe an instance through `model`, `world`, `id`, and `opacity`. `GaussianSplatInstanceList` is the scripting collection type.
* Added model queries `GetGaussianSplatCount(model)`, `GetGaussianSplatSourceCount(model)`, `GetGaussianSplatSHDegree(model)`, `GetGaussianSplatBoundsMin(model)`, `GetGaussianSplatBoundsMax(model)`, and `IsGaussianSplatAntialiased(model)`.
* `SubmitGaussianSplats(next_view, rect, view_state, instances, renderer, render_data, settings, target)` submits compute and drawing work, with optional settings and target framebuffer. It never clears the target or calls `Frame()`. Reusing render data can avoid repeated projection/sorting; independently ordered cameras or passes need separate render data.
* `GaussianSplatRenderSettings` exposes `max_sh_degree`, `view_limit`, `max_splats`, `depth_test`, `compact_views`, and `max_pixel_radius`. The radius cap is an explicit appearance/overdraw approximation. Supported transforms are rigid camera views and instance transforms with positive uniform scale; projection must be perspective, including off-axis perspective.
* `GaussianSplatSubmitResult` exposes `ok`, `message`, `draw_view`, `count`, `dispatches`, `gpu_bytes`, and the effective `max_pixel_radius`. Cached submissions report zero compute dispatches while still drawing. Validation/allocation failures leave `next_view` unchanged and submit no work.

#### Gaussian import and HGS2 codec (C++ only)

These CPU APIs are declared in `engine/gaussian_splat_data.h`; they do not create GPU objects and are not exposed by the scripting bindings.

* `GaussianSplatData` contains `base`, `higher_sh`, `count`, `source_count`, `sh_degree`, `bounds_min`, `bounds_max`, `antialiased`, and `normalized`. `GaussianSplatDataResult` contains `ok`, `message`, and `data`.
* `GaussianSplatImportOptions` exposes `max_count`, `sh_degree`, and `normalize`. A zero `max_count` retains every source splat; a positive value explicitly requests reduction.
* Added `ImportGaussianSplatPLY(input, options)` / `ImportGaussianSplatPLYFromFile(path, options)`, `ImportGaussianSplatSPZ(input, options)` / `ImportGaussianSplatSPZFromFile(path, options)`, and `ImportGaussianSplatBinary(input, options)` / `ImportGaussianSplatBinaryFromFile(path, options)` for supported binary PLY, SPZ v2/v3, and headerless `.splat` data.
* `ReadGaussianSplatData(input)`, `ReadGaussianSplatDataFromFile(path)`, and `ReadGaussianSplatDataFromAssets(name)` decode HGS2 data.
* `WriteGaussianSplatData(data, output, error, options, stats)` and `WriteGaussianSplatDataToFile(data, path, error, options, stats)` encode HGS2. The error, options, and statistics arguments are optional. `GaussianSplatWriteOptions.storage` selects `GaussianSplatStorage::Quantized` (default) or `GaussianSplatStorage::Float32`.
* `GaussianSplatWriteStats` reports `uncompressed_bytes`, `stored_bytes`, `max_position_error`, `max_scale_relative_error`, `max_opacity_error`, `max_dc_error`, `max_sh_error`, and `max_quaternion_error`.
* Added budget/default constants `GaussianSplatMaxCount`, `GaussianSplatDefaultImportCount`, `GaussianSplatMaxNativeBytes`, `GaussianSplatMaxPLYBytes`, `GaussianSplatMaxBinaryBytes`, and `GaussianSplatMaxSPZBytes`.

#### Camera controls, input, and HiDPI

* `TrackballController` has default, `(target, distance, orientation = Quaternion.Identity)`, and copy constructors. Its editable fields are `target`, `orientation`, `distance`, `min_distance`, `max_distance`, and `dolly_speed`.
* `TrackballController.Reset()` restores its construction pose; `ComputeWorld()` returns the camera world transform. `Update(orbit, pan, x, y, dx, dy, wheel, width, height, fov)` accepts pointer input directly. `Update(mouse, width, height, fov, drag_valid)` uses left-button orbit, right/middle-button pan, and fractional-wheel dolly. Optional `fov` defaults to pi/3 radians; `drag_valid` defaults to `true` and can suppress drag when focus is reacquired.
* `MouseState` adds floating-point `wheel_delta` and `hwheel_delta`, plus `WheelDelta()` and `HWheelDelta()` accessors. `Mouse` adds the same two accessors. Legacy integer `Wheel()` / `HWheel()` remain available; high-resolution accessors fall back to integer values for legacy readers.
* `GetWindowFrameBufferSize(window, width, height)` returns drawable pixel dimensions, while `GetWindowClientSize` continues to return client coordinates, which are points on macOS. `RenderInit` and `RenderResetToWindow` now use drawable pixels.
* Added `ImGuiBeginFrame(..., mouse_scale)` overloads, with or without an explicit `DearImguiContext`, to scale client-coordinate mouse input to display dimensions. Existing overloads retain 1:1 input scaling.

#### Additional scripting and native JavaScript APIs

* Exposed existing quaternion-vector operations as `Rotate(quaternion, Vec3)` and `Rotate(quaternion, Vec4)` in the generated bindings.
* Added `RequestScreenShot(framebuffer_handle, path)` to Lua and native JavaScript, wrapping BGFX's screenshot request.
* The native JavaScript `harfang` ES module exposes the shared engine API. `SceneLuaVM.Pack(value)` and `SceneLuaVM.Unpack(luaObject)` bridge primitives and bound HARFANG objects to/from embedded Lua; Lua integers return as JavaScript `BigInt`. JavaScript `Script.SetParam(name, value)` accepts boolean, integer, float, and string values.
* JavaScript adds partial vector constructors with one or more components, `Color(gray)`, `Mat44(Mat44)`, and `Mat44(Mat4)`, plus value-comparison methods for `Vec2`, `iVec2`, `Vec4`, and `Mat44`. Default vectors are zeroed, default `Color` is opaque black, and default `Mat4` / `Mat44` are identity matrices. Mutable math constants are copied into JavaScript values.
* The `harfang-host` module adds `nextFrame(window?)`, resolving to `{closed, dtNs}` with a nanosecond `BigInt` delta. The launcher accepts ordinary top-level code, exported `main()`, or an exported `completion` promise, and supplies `print()`, `console.log()`, and `console.error()`.
* For C++ embedding, the optional QuickJS language layer adds the move-only `QuickJSValue` owner and `QuickJSVM`, with `ModuleReader` / `ModuleResolver` callback types, `GetContext()`, `GetRuntime()`, `SetModuleReader(...)`, `SetBindingCleanup(...)`, `Evaluate(...)`, `PumpJobs(...)`, `Check(...)`, `CheckPromise(...)`, `CheckUnhandledRejections()`, and `TakeException()`. Values must be released before their VM and cannot cross VM boundaries directly.

### Compatibility notes

* Gaussian assets must be rebuilt with the matching AssetC toolchain: older HGS0/HGS1 files are rejected by the HGS2 loader. SPZ v4, extension flags, and SH4 remain unsupported.
* LZMA archives require both an updated runtime and launcher. The `--legacy` archive-header option does not select an older compression codec.
* Native Gaussian packaging and rendering have documented validation scopes; consult `tutorials/GAUSSIAN_SPLATTING.md` and the platform validation records before assuming support on another backend.

# [3.3.0] - 2026-09-06

This release expands HARFANG with a selectable experimental Tau physics backend, first-class Squirrel support, application launchers, media plugins, rendering controls, and a much larger in-tree documentation and tutorial set. Bullet 3 remains the default scene-physics backend.

### Physics

* Added `HG_SCENE_PHYSICS_BACKEND` with `auto`, `none`, `bullet`, and `tau` choices. The new backend-neutral `ScenePhysics` class maps to the backend selected at build time, and `GetScenePhysicsBackendName` exposes that choice at runtime.
* Added the Tau rigid-body backend with dynamic, kinematic, and static bodies; discrete contacts for cuboids, spheres, capsules, and multi-shape compounds; first-hit and all-hits ray casting for analytic shapes and triangle meshes; 6DoF constraints; and pre-tick callbacks.
* Added continuous collision detection to rigid-body components, including JSON/binary serialization and scripting bindings. Tau CCD is currently limited to moving cuboids (including multi-cuboid compounds) against immovable cuboids and static triangle meshes; dynamic-against-dynamic, angular, sphere, and capsule CCD remain deferred.
* Added backend-neutral deactivation and sleep queries through `NodeSetDeactivation`, `NodeGetDeactivation`, and `NodeIsSleeping`.
* Added backend-neutral `.physics` collision geometry with BVH acceleration for triangle meshes and boundary edges.
* Improved Tau determinism, stability, and performance with fixed-step accumulation, persistent contact manifolds, a dynamic AABB broad phase, sleeping islands, reusable scratch storage, cached transforms/inertia, optimized constraint solving, and all-sleeping fast paths.
* Added an in-tree physics QA suite with deterministic dumps, Bullet/Tau comparisons, trajectory plots, ray-cast and collision tests, and performance benchmarks.
* Deprecated direct use of the legacy `SceneBullet3Physics` scripting constructor in favor of `ScenePhysics`; the former remains available for compatibility.

### Scripting and application launchers

* Added host-side Squirrel 3.2+ support with a vendored runtime, generated HARFANG bindings, `hg_squirrel`, `launcher_squirrel`, and a no-console Windows launcher. Embedded scene scripting remains Lua-only.
* Added Lua application launchers and shared Lua/Squirrel launcher infrastructure.
* Launchers can mount applications from a `data/` folder or `data.zip`, `data.gsa`, and `data.nac` archives, using `bootstrap.json` or `launcher.json` configuration files.
* Added the `legacy_archive` packing/unpacking tool and launcher regression tests for folder and archive deployments.
* Added backend-specific rebuild helpers for Lua, Squirrel, and Python. CPython support remains in-tree but is not guaranteed for every current platform/configuration.

### Rendering and platform

* Added configurable near/far clipping ranges for spotlight shadow maps through `GetShadowNear`, `SetShadowNear`, `GetShadowFar`, and `SetShadowFar`.
* Added four user-controlled `Vec4` compositing parameter slots to `ForwardPipelineAAAConfig`, serialized with the pipeline configuration and exposed to the compositing shader as `uCompositingParams`.
* Fixed compositing shader sampler names.
* Improved HiDPI behavior with an explicit runtime policy and `GetWindowContentScale`.
* Vendored the BGFX stack in the repository and documented the macOS Metal integration fix.

### Audio and video

* Fixed OGG streaming playback and added test coverage for audio streams.
* Added the XMP audio streamer plugin and helpers for MOD, XM, S3M, and IT module playback.
* Added `LoadLPCMSound` for loading raw PCM samples into the audio API.
* Added an optional FFmpeg `IVideoStreamer` plugin, enabled with `HG_BUILD_FFMPEG_PLUGIN`.

### Documentation, tutorials, and tools

* Moved the Python and Lua tutorials into the main repository, added Squirrel ports, and expanded coverage across rendering, input, filesystems, ImGui, audio, physics, VR/XR, scene management, and picture processing.
* Added a self-contained static HTML documentation build with C++, Python, Lua, and Squirrel API output.
* Added the first Lua-based Mini Studio prototype, including project opening, asset compilation, multi-scene management, a scene graph, viewport rendering, transform gizmos, and scene saving.
* Updated build scripts and documentation to use `HG_FABGEN_PATH` with the `astrofra/FABGen` fork.
* Reduced repetitive runtime warnings and corrected the PyPI download badges.

### Build notes

* Bullet remains selected by default (`HG_SCENE_PHYSICS_BACKEND=auto`). Select Tau explicitly with `-DHG_SCENE_PHYSICS_BACKEND=tau`.
* XMP audio support is enabled by default except on Emscripten, where dynamic audio plugins are disabled. The FFmpeg plugin remains opt-in and requires FFmpeg development files.
* Squirrel is the primary newly supported scripting target in this fork. Python build files are retained, but current Python artifacts are not guaranteed on all platforms.

# [3.2.7] - 2023-08-14

This minor release brings the support for the DOF post process.

* Added the support for the depth of field  post process (CoC spiral gather method)
  * Adds `dof_focus_point` and `dof_focus_length` (see [ForwardPipelineAAAConfig()](https://dev.harfang3d.com/api/3.2.7/cpython/classes/#forwardpipelineaaaconfig))
  * Available in the AAA rendering pipeline only.
* Fix CMake warnings.

# [3.2.6] - 2023-06-05

This minor release provides several fixes and brings a functionning API to capture the framebuffer and save it as a picture.

### Framework integration and source code maintenance
* Fixed the missing `DisableCursor` on SDL (by @PMP-P). 
* Fixed Linux Golang module build script.

### Rendering
* Added the ability to set the pixel center offset for the projection matrix: `SetCameraCenterOffset` and `GetCameraCenterOffset` (by @RobEwbank1).
* Resolved issue #50 (fix CaptureTexture() for Python / Lua)":
  * **OLD:** `uint32_t CaptureTexture(const PipelineResources &resources, const TextureRef &t, Picture &pic)`
  * **NEW:** `uint32_t CaptureTexture(bgfx::ViewId &view_id, const PipelineResources &resources, const TextureRef &t, const Texture &readback, Picture &pic)`
  * Fixed Picture Set/Get RGBA: `GetPixelRGBA` and `SetPixelRGBA`.
  * How does the framebuffer capture work ?
    * In order to grab the framebuffer, create an empty picture: `picture = hg.Picture(512, 512, hg.PF_RGBA32)`
    * The texture framebuffer is created as in [`draw to texture`](https://github.com/harfang3d/tutorials-hg2/blob/da92f5dc96099dfc315c90d3ea188fc30f18f28f/scene_draw_to_texture.lua)
    * Then, set framebuffer to `TF_ReadBack`: `tex_readback = hg.CreateTexture(512, 512, "readback", hg.TF_ReadBack | hg.TF_BlitDestination, hg.TF_RGBA8)`
    * When submitting the scene, target the framebuffer: `hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, 512, 512), true, pipeline, res, frame_buffer.handle)`
    * The capture will be made asynchronously: `frame_count_capture, view_id = hg.CaptureTexture(view_id, res, tex_color_ref, tex_readback, picture)`
    * Then, only when the current `frame` counter is greater or equal to `frame_count_capture`, save the picture: `hg.SavePNG(picture, "capture.png")`

### Misc bug fix
* Fixed `!=` color operator.
* Brought back the old `minmax` transform to fix #49. 
* Removed warning message ("Invalid node instance"). 
* Resolve "Tutorials issues on Linux Ubuntu" (Fixed `VertexLayoutPosFloatNormUInt8TexCoord0UInt8`).
* Studio related fixes
  * `RBT_Static` wasn't saved properly in the scene file.
  * Fixed `GetAnimableNodePropertyFloat` and `SetAnimableNodePropertyFloat` to handle the camera FOV.

# [3.2.5] - 2022-12-09

This minor release provides several fixes and new features in the VR/XR and Physics areas. Platform compatibility was slightly improved as well on OS X and WASM (still experimental):

### Framework integration and source code maintenance
* Improved the support of WASM (@PMP-P).
* Improved the MacOS support (@Tommo).
* Improved the unit testing & code coverage of `Foundation` and `Engine`.
* Various fixes to improve the compatibility of [Harfang Studio](https://www.harfang3d.com/en_US/studio) on Linux.
* CMake Fixes.

### VR/XR
* OpenXR:
  * Added the support of OpenXR to Harfang (see `OpenXRInit`, `OpenXRShutdown`, `OpenXREyeFrameBuffer`, `OpenXRCreateEyeFrameBuffer`, `OpenXRGetHeadPose`...).
  * Support of the most common features.
  * Support of the hand tracking.
  * Support of the passthrough.
  * Support of the eye gaze tracking.
  * Added the extensions `VARJO_QUADVIEWS` and `COMPOSITION_LAYER_DEPTH` to support the [Varjo XR-3 headset](https://varjo.com/products/xr-3/).
* OpenVR:
  * Fixed #34, added a function that tells if the HMD is mounted or not.
  * Fixed #35, added the support for OpenGL and DX12 renderers.

### Physics
* Added the support for physics geometries (**Bullet** and **Assetc**).
* Added the support for **6DOF** physics constraints (see `Add6DofConstraint`, `Remove6DofConstraint`).
* Added pre-tick callback to the Physics system (see `SetPreTickCallback`).
   * The `SetPreTickCallback` allows the user to define a function that will be automatically invoked by the Physics solver. This function is provided with 2 parameters, the **physics system** and the **delta of time** within the current solver step:
       ```python
       # scene physics
       physics = hg.SceneBullet3Physics()
       physics.SceneCreatePhysicsFromAssets(scene)
       physics_step = hg.time_from_sec_f(1 / 60)
       
       function foo(ph, dt)
           # do physics stuff (AddForce, AddImpulse...) 
       end
       
       physics.SetPreTickCallback(foo)
       ```

### Misc
* Fixed #33, added a function to disable/grab mouse cursor (see `DisableCursor`).
* Fixed incorrect warning message (see `Scene::SetObjectModel` and `Object::IsValid`).
* Fixed X11 display retrieval and propagate GFLW backend to parent project.

### Rendering
* Added a blending mode: `BM_AlphaRGB_AddAlpha` (required by [Harfang GUI](https://github.com/harfang3d/harfang-gui)).

### Harfang Python
* Packaged **Assetc** into the bdist wheel and allow it to be called as a function of harfang.bin module.
   * Can be used from the command line: 
     ```bash
     python3 -m harfang.bin assetc resources_path -api GL
     ```
   * Or as a python module: 
     ```python
     import harfang.bin
     harfang.bin.assetc('resources', '-api', 'GL')
     ```

# [3.2.4] - 2022-09-27

This minor release provides minor corrections and fixes to specific issues:
 - Fixed the OpenGL support of the rendering pipeline (see https://github.com/harfang3d/harfang-core-package).
 - Improved the support of the Go language (see https://pkg.go.dev/github.com/harfang3d/harfang-go).
 - Improved the Emscripten build.

### Framework integration and source code maintenance

- Ongoing effort to support the WASM / Emscripten target.
  - Added a series of flags improve the compilation to WASM.
  - Updated the SDL calls to support the latest inputs/windows system.
  - :warning: Audio is disabled for now.

### Binding / Golang support

- Proper package of the Go binding.
- Added a _mingw_ build stage for the Windows lib part of HARFANG.
- Added a cmake flag `HG_BUILD_HARFANG_STATIC` to build HAFANG in static mode.
- Added a Go directive (based on FabGen merge request https://github.com/ejulien/FABGen/pull/60).
- Removed the non-mingw Go build from the Windows target.
- Updated cmake to build HARFANG Go as a monolithic lib.
- Reinstated the script support (to embed Lua in a Go project).
- The support for the _OpenVR_ API was (temporarily) removed.

### Toolchain

- **GLTF importer:**
  - support for camera and lights (:warning: experimental)
    - support for _point_, _directional_ and _spot_ types.
    - support for the diffuse color and intensity (specular is not supported by the GLTF standard).
  - Fixed a mislabelled _usage_.
- **Assetc:** added `jpeg` to the textures checklist.
- Fix #16 (lua53.dll should be lua54.dll).

### Binding

- Added a constructor to `FileFilter`.
- Fixed the arg out for `CollectCollisionEvents` to return a `NodePairContacts` properly.

### Physics

- Improved the ability of a node to change its collision shape component multiple times during runtime.
- Fix #17 Capsule / Cone model fix.

### Documentation

- Fixed a mention to `ViewMode` enums in the manual.
- Fixed the reference to 3 physics functions in the manual, `Bullet3Physics`, `SyncTransformsFromScene` and `SyncTransformsToScene`.

# [3.2.3] - 2022-07-13

This minor release brings several fixes to the rendering, physics, engine, foundation and tools.

### Framework integration and source code maintenance

- Added a CMake option to force MSVC to use updated __cplusplus macro.
- Build fixes for GCC 12.

### Toolchain

- Added a way to select the input channel from an input texture within a `construct` when processing textures in **Assetc**.
- Changed  `BC6H_UF16` into `BC6H_SF16` to produce a valid DDS from a HDR file.
- **Assetc** now parses the shaders to check their dependencies for any modification and triggers a rebuild if needed.
- Properly quote **Luac** and **Recastc** invocations to support space in arguments.

### Binding

- Fixed `ImGuiMouseButton` enums (`ImGuiMouseButton_Left`, `ImGuiMouseButton_Right`, `ImGuiMouseButton_Middle`).
- Added a `SetProbe` function to set the radiance and irradiance map to a scene.
- :warning: Deprecated `SubmitModelToForwardPipeline`.

### Engine

- Added `GetMaterialsWithName`.
- Added `GetFullPathAsset`
- Added `get_log_level`, `get_log_detailed` functions.
- Fixed the ray/cone intersection.
- Ensure an extension is specified before returning output path from `SaveFileDialog`.
- Fixed the reserved texture units used by the **AAA** pipeline (see https://dev.harfang3d.com/docs/3.2.3/man.pipelineshader/). As a consequence, the `core\` folder will need to be updated if your project is using the **Forward** or **AAA** rendering pipelines.
- Fixed a nasty issue in forward pipeline texture table.
- :warning: Deprecated `UpdateForwardPipelineAO` and `UT_AmbientOcclusion`.

### Physics

- Fixed #14, Bullet uses half extend for cylinders.

### Audio

- Fixed #13, properly reset OpenAL source velocity when starting a stereo sound.

### Documentation

- Fixed a dead link in the API documentation.
- URLs updates (Quickstart, Wheel description, Readme file).

# [3.2.2] - 2022-06-03

This minor release brings several fixes, a better implementation of the AAA rendering pipeline including probe reprojection and a more stable screen space raytracer.<br>
Improvements were made in the usability area, for Python development, as HARFANG will now output warnings as much as possible when users are calling for invalid API operations.<br>
The Python build script was worked out to make the wheel available on Pypi for Linux OSes.<br>

### Framework integration and source code maintenance

- Improved the Python source package creation, to allow a `pip install` from the source package and address [this issue](https://github.com/harfang3d/harfang3d/issues/5).
  - The following development packages are necessary to rebuild Harfang:
    - ubuntu: `uuid-dev`, `libreadline-dev`, `libxml2-dev`, `libgtk-3-dev`
    - centos/fedora: `uuid-devel`, `readline-devel`, `libxml2-devel`, `gtk3-devel`
  - See `languages/hg_python/pip/setup.py`.
- Allowed the tools to be called from a Python script.
- Allowed the tools to be called from a Lua script.
- External libraries update.
- Moved _mikktspace_ and _stb_image_ to extern.
- Fixed the License URL, removed a useless URL indirection.
- Removed the external libraries samples and tests from source package.
- Removed `AssetsSource` from _assets.h_.

### Toolchain

- Fixed **Assetc** to prevent it from processing invalid geometries.
- **GLTF importer**:
  - Better management of geometry instances
  - Improved material translation.
- Sanitized the filenames when outputting files from the **FBX** and **GLTF** converters.
- **FBX converter** now exports materials as PBR by default.
- **Assimp converter** now exports materials as PBR by default.
- **GLTF exporter**:
  - Added a `filter_disabled_node` options to avoid exporting node.
  - Fixed several bugs. 

### Engine

- :warning: Multiplication and maths API fixes:
  - Removed **vec * mat** multiplications.
  - Swapped row/column in the API to match the correct mathematical order.
  - Removed mixed `Vec4`/`Vec3` operations as the result was ambiguous.
- Added a binding for `Vec3` on `CubicInterpolate`.
- :beginner: Introduced a defensive programming approach in HARFANG Python, assuming the programmer is learning the API by trial and error:
  - This is done using the debug log method so it can be completely deactivated when working on C++ projects.
  - Demote most errors to warning. Errors are now strictly reserved for conditions from which a program written using HG cannot recover by itself/is not aware of.
- Fixed the `AddQuad` method in the model builder. Added an helper method to quickly build common vertex configurations, see `MakeVertex`.
- Animations support:
  - Code cleanup to support the animation editor cleanup.
  - Remove duplicate keys from animation track when calling `SortAnimTrackKeys`.
  - Added a function to quantize scene animation.
  - Additional scene animation APIs.
  - Added the support of camera fov animations.
- Text input callback is now a signal, see `TextInputSignal`.
- Added a size field to `ListDir` output.

### Rendering

- Added a sharpen post-process.
- Added functions for orthographic projection to clip and screen space. See `ProjectOrthoToClipSpace`, `ProjectOrthoToScreenSpace`.
- Added functions for orthographic unprojection. See `UnprojectOrthoFromClipSpace`, `UnprojectOrthoFromScreenSpace`.
- Implemented a light probe reprojection in the **Forward** and **AAA** pipelines.
- Fixed the orientation of the cubemap in the probe generation.

### Physics

- Load/save scene collision components.
- Added a missing cone collision component creation.
- Setup physics for instantiated nodes.
- Fixed a bug with local transformation for single collision shape nodes.

### Documentation

- Documented the coordinates system.
- Doxygen documentation update

# [3.2.1] - 2022-04-10

This minor release brings both code and submodules maintenance, several fixes in the toolchain, in the scenegraph and physics interchange and in the rendering pipeline.

### Source code maintenance
- Updated GLFW (3.3.6).
- Updated BGFX to the latest version.
- Updated OpenAL (v.1.6.10b).
- Updated OpenVR (v1.16.8).
- Fixed the debug build with Visual Studio Build Tools 2022.
- Fixed the installation of PDB files.
- CMake misc. fixes and updates.

### Toolchain

- FBX Importer: changed the unit of command line argument `max smoothing angle` to degrees and fixed the internal vertex/tangent computation.
- GLTF Importer: improved the way non-Windows-compliant filenames are handled.

### Engine

- Fixed an issue in the Wavefront OBJ export that flipped the model on the X axis.
- Improve the error message issued by OpenAssets in case of missing file.
- Fixed a crash when parsing a corrupted .HPS file.
- Added `hg::Picture::SaveBC7` and `hg::Picture::SaveBC6H` functions.
- Switched to a time limit based resource queue processing
- Return the size on disk of a folder
- Implemented a _ComputeNodeWorldMatrix_ to compute a node world matrix on the fly (`ComputeWorld`, `GetWorld`, `SetWorld`). :warning: This function is slow but useful when scene matrices are not yet up-to-date
- Improved the way unicode paths are handled on Windows
- Preliminary support for scene properties animation (`AmbientColor`, `FogNear`, `FogFar`, `FogColor`) while preserving the legacy file format.
- Add an `LSSF_Silent` scene flag, add many boolean flags to silence errors in the rendering IO.
- Allow 32 bit indices in geometries (In the future we might introduce a _force 16 bit indice flag_ if required).
- Validate index and vertex buffer handles when creating geometries.
- Flag instantiated nodes as `NF_InstanceDisabled` if the host node is disabled.
- Added file filter descriptions to the file selector dialogs (`OpenFileDialog`, `SaveFileDialog`). 

### Rendering

 - Added a _depth only_ pass to render shadow maps (`DEPTH_ONLY=1`).
 - Set a higher probe resolution by default (from 64 to 512, using the `--glossScale 20` `--glossBias 0` parameters sent to CMFT).
 - Load/save AAA config (`LoadForwardPipelineAAAConfigFromFile`, `LoadForwardPipelineAAAConfigFromAssets`, `SaveForwardPipelineAAAConfigToFile`)
 - Tweaked the default light values (_default_shadow_bias_ set to 0.0001, _pssm_split_ of a the linear light set to 200m).
 - Switched the jitter sequence to Halton (2,3).
 - Added a function to retrieve the size of the OpenVR framebuffers
 - Added a `RGBA32` pixel format to improve the performance when playing a video stream.

### Physics

- Fixed an issue in synchronizing state from scene to kinematic body

### Documentation

- Added an explanation of why using integer representation for time is important.
- Documented the profiler API.
- Added a HARFANG logo in the Doxygen documentation

# [3.2.0] - 2022-02-21

This minor release brings several fixes, performance improvements and new features to the rendering, physics and audio APIs.

### Source code maintenance
- Cleanup sources, remove spurious hg namespace specifiers and run clang format on affected sources.
- Updated the GLFW CMake to improve the resolution of the library path on Linux.
- Updated ImGui to v1.87.

### Engine
- Implement model load queuing (see `ProcessModelLoadQueue`, `ProcessLoadQueues`, `LSSF_QueueTextureLoads`, `LSSF_QueueModelLoad`).
- Support replay and streaming of OGG files (see `LoadOGGSoundFile`, `LoadOGGSoundAsset`, `StreamOGGFileStereo`, `StreamOGGAssetStereo`, `StreamOGGFileSpatialized`, `StreamOGGAssetSpatialized`).
- Added a `Mat4` copy constructor.
- Added missing declarations for `LoadImage*` functions.
- Added a flag to prevent changing the current camera when loading a scene, if the current camera points to a valid node (see `LSSF_DoNotChangeCurrentCameraIfValid`).
- Added an `is_file` field to the structure returned by `GetFileInfo`.
- Simplify scene binary loader, removed versioning code:
  - Removed unused members from the `RigidBody_` struct.
  - Reduced memory footprint of `RigidBody_` to 6 bytes.
- Performance improvements on multiple scene loading (through instances).
- Added a profiling API (see `BeginProfilerSection`, `EndProfilerSection`, `EndProfilerFrame`, `CaptureProfilerFrame`, `PrintProfilerFrame`).
- Added a Videostream plugin interface (see `MakeVideoStreamer`).
- Fixed scene animation garbage collection.
- Fixed a bug with Unicode support in assetc.
- Fixed an issue with trailing slashes on Linux in the assetc command line.

### Rendering

 - Fixed an issue in the viewport computation when upscaling half buffers in the AAA rendering pipeline.
 - Added a `z_thickness` param to the AAA rendering pipeline.
 - Added a series of cubemap render functions.
 - Implemented and documented [all supported pipeline program features](https://www.harfang3d.com/docs/3.2.0/man.pipelineshader/).

### Physics

- New functions to lock translations and rotations in a more consistent way with the Bullet API:
  - `NodeSetLinearLockAxes`, `NodeSetAngularLockAxes` replaced by `NodeSetLinearFactor`, `NodeSetAngularFactor`.
  - `NodeGetLinearLockAxes`, `NodeGetAngularLockAxes` replaced by `NodeGetLinearFactor`, `NodeGetAngularFactor`.
- Added AddTorque/AddTorqueImpulse to physics API:
  - `NodeAddTorque`, `NodeAddTorqueImpulse`
- Simplified the physics collision/contact query code, unified collision query API for `NodeCollideWorld` and `StepSimulation` (see `CollectCollisionEvents`).
- Implemented `NodeTeleport` in Bullet physics.
- Improved the transform synchronization logic, set node world transform using the fast scene path (see `SyncTransformsFromScene`, `SyncTransformsToScene`).
- Added proper motion interpolation in Bullet physics synchronize to scene.
- Added missing functions to create all supported collision shape types.

### Documentation

- Added a missing reference to the requirements page in the main index.
- Improved the `LoadSceneXXX` functions documentation.
- Fixed both Lua and Python code snippets.
- Improved the clarity of functions using bitflags by adding support for constants group in the documentation generator.
- Update of ownership and views manual pages.

# [3.1.1] - 2021-12-31

This minor release brings several fixes, mainly in the Bullet Physics API.

### Engine

- Added a missing texture render target flag (`TF_RenderTarget`).
- Added  `SetSourceStreamTimecode()` to the audio API.

### Physics

- Updated the Physics unit tests.
- Fixed an issue in the `Kinematic` physics matrix.
- `SetRigidBodyAngularDamping()` now takes a `float` instead of a `Vec3`.

### Documentation

- Fixed several typos in the documentation.

# [3.1.0] - 2021-12-13

This minor release brings several improvements and fixes, mainly in the Bullet Physics API.

### Engine

- Added new HLS colorspace functions.
- Improved `CreateInstanceFromFile`/`CreateInstanceFromAssets` by returning a boolean to inform the caller that the hosted scene could be instantiated.
- Fixed various bindings (`CreateInstanceFromFile`, `CreateInstanceFromAssets`, `GetColorTexture`, `GetDepthTexture`, `GetTextures`)
- Added a function to center a window (GLFW). 
- Added the support for Windows refresh callback (GLFW). 
- Bind read back and blit destination texture flags. 
- Simplify the frame buffer API store less content in Harfang objects. 
- Added fog to render data.
- Validate AAA forward pipeline and associated post processes. 
- Fixed an issue where setting up the imgui font destroyed the cursors. 
- Added `time_from_xxx_d` functions and fixed a precision issue with `time_from_string` 

### Physics

- Fixed the initial transform setup code of Bullet rigid bodies. 
- Changed the sync kinematic and dynamic mechanisme.
- Added `ResetWorld`,  `DisableDeactivation`, add a way to lock axes.
- Added a getter/setter for rolling friction.
- Added a raycast functions to return all collisions found between two points. 
- Fixed a raycast issue with geometry hierarchy.
- Added the support for multi collision shapes with matrices

### Toolchain

- Assetc cleanup error messages, do not output unless truly reporting on an error.
- Assetc will now delete the outputs for deleted inputs. A flag was added to opt-out from this mechanism (`-no_clean_removed_inputs`).
- Assetc will now delete the previous compiled lua scripts if a compilation error arises.
- Assimp importer: fixed a mention to the wrong converter. 
- Assimp importer: added a `-merge-mesh` option.
- Assimp importer: updated to Assimp v5.1.2.
- GLTF importer: fixed the import in case of many instances.
- Fixed the toolchain integrity check.

### Documentation

- Documentation improvements and cleanup.
- Fixed a typo in `GetTextures` documentation filename.
- Fixed the reference to `SceneBullet3Physics` in the man Physics page. 
- Added a man.Requirements page.
- Improved the Pypi wheel description ('Quickstart' was lacking a download URL).

### Misc

- Updated the license.
- Fixed some cppcheck issues. 

# [3.0.0] - 2021-10-12

This major release mainly replaces Newton Dynamics with Bullet Dynamics for physics and improves interoperability between supported languages.

### Engine

- Add diffuse and specular intensity to lights.
- Performance and filtering improvements of the AAA rendering pipeline.
- Replace Newton Dynamics with Bullet 3 Dynamics, SceneBullet3Physics uses the same API as SceneNewtonPhysics.
- Add SceneBullet3Physics.NodeWake to wake a sleeping body.
- Implement restitution and friction in rigid bodies.
- Provide support for value transfer between Lua VMs when calling Get/Set/Call on SceneLuaVM.
- Implement full value transfer between Python and Lua VMs.
- Implement Call on SceneLuaVM.
- Light default specular value is now (1.0, 1.0, 1.0).
- Support for specular color in core light models (point, spot, linear) in the PBR shader.
  This is not compliant with a strict definition of a PBR Pipeline, but this approach tends to be a consensus in the industry (Blender, Redshift, ...) and is artist-friendly.
- Add Joystick support (Joystick, JoystickState, ReadJoystick, GetJoystickNames, GetJoystickDeviceNames).
- Improve OpenAsset and LoadResourceMeta performance.
- Improve Vertices.End performance by removing systematic default validation.
- Rename x_aspect_ratio parameter to fov_axis_is_horizontal in SceneSubmitToForwardRenderPipeline.
- SetView2D and SetViewPerspective now takes x,y,w,h instead of w,h.
- Fix redundant loads of texture meta JSON when loading materials.
- Fix ImGui Enter/Return key distinction.
- Fix broken linear texture filtering.
- Fix GLFW window system not sending window signals and breaking IsWindowOpen as a consequence.
- Fix spurious error message when opening asset with several asset folders registered.

### Toolchain

- New AssImp converter.
- GLTF Importer: Add occlusion texture if no metal/roughness texture is declared in a source material.
- FBX, GLTF importer: Fix on the importation of animations in some specific rotations situations.

### Documentation

- Quickstart page improvements.
- Index class members in the search index.

### Misc

- Update STB Truetype.
- Update STB Vorbis.
- Update STB Image.
- Update STB Image Write.
- Update bgfx.
- Update bimg.
- Update tiny process.

# [2.0.111] - 2021-06-21

This release is a rewrite of the Harfang API toward a data-oriented approach.
As a developer, you will find that many of the old classes are now gone and replaced by function calls that you string together in order to implement complex functionalities. The improved API provides greater flexibility and delivers higher performance than the Harfang 1.x API.
Of major interest in this first release of Harfang 2.x are the new resource/asset pipeline (see Resources & Assets) as well as the new AAA renderer beta enabling screen-space global illumation with correct approximation of both the radiance and irradiance terms of the PBR equation.

**NOTE:**

The AAA rendering pipeline is currently a **WORK IN PROGRESS** and has important issues to address.
