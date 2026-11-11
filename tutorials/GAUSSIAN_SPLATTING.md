# Gaussian splatting: Houseplant and Saari

[gaussian_splatting.lua](gaussian_splatting.lua) is the single Lua Gaussian tutorial. It preloads Houseplant and Saari during setup, then switches between them through a small ImGui panel or the Space key. Each model keeps its own native trackball camera, SH setting and render data. Switching does not reload either file.

C++ owns HGS2 loading, GPU resources, covariance projection, spherical harmonics, depth sorting and drawing. Lua owns the interface, window, model selection and RGB orientation gizmo. No companion Lua modules or Python preparation/launch tools are needed. The tutorial contains no assertions; developer checks live under `harfang/tests`.

## Prepare, compile and run

Use a build containing the native Gaussian API, `TrackballController`, ImGui, Lua bindings and `assetc`. Both compiled models are required at startup. Houseplant and the Saari SPZ source are included in the checkout (Saari was added in `5cb3ca7`).

From `tutorials/`, compile the resources:

```powershell
assetc -api DX11 -platform windows resources resources_compiled
lua gaussian_splatting.lua
```

Both datasets are under `gaussian_splatting/source`. The Lua application takes no arguments and loads only compiled resources, including the shared Gaussian and ImGui shaders.

On macOS compile with `-api MTL -platform osx`; the tutorial selects Metal there. The unchanged application has been tested on macOS/arm64, Apple M3 Pro, including a 2560Ã—1440 framebuffer for a 1280Ã—720 client window. See the [Mac validation record](../specifications/GSPLAT_MACOS_VALIDATION.md) for the tested scope and remaining qualification work.

## Build and run on macOS

With the existing Lua build configured by [`rebuild_hg_lua.sh`](../rebuild_hg_lua.sh), run from the repository root:

```sh
cmake --build ../build/lua-cmake --config Release --target gsplat_tutorial_package --parallel 8
../build/lua-cmake/gaussian_splatting/Release/app/run.command
```

The package target builds Lua, its native module and the host asset tools, checks the tutorial core mirror, compiles the canonical Gaussian/ImGui shaders and tutorial gizmo for Metal, and converts both models through `assetc`. It stages the original Lua source and installed native dependencies under `../build/lua-cmake/gaussian_splatting/Release/app/`. `run.command` sets its own working directory and module path. This directory can be moved as a unit; no developer `DYLD_LIBRARY_PATH` or Python runtime is required.

`gsplat_tutorial_run` builds and opens the application directly. `gsplat_tutorial_smoke` builds and runs two automated 60-frame tutorials (GUI and Space switching), saving Houseplant/Saari captures beside `app/`. Both runs require all 1,307,339 Saari splats. GPU targets require an interactive graphics session:

```sh
cmake --build ../build/lua-cmake --config Release --target gsplat_tutorial_smoke
```

Assets and tool installations stay in the build tree. The package checks required outputs, writes `app/manifest.json` with input/output/runtime hashes and tool identities only after a successful build, and reuses unchanged assets. Changing the selected tool binaries invalidates this package's asset cache. Build/profile directories remain separate. The package helper currently supports native macOS/Metal and Windows/D3D11 configurations; the new package target has been executed on macOS only.

For the local, uninstalled Windows build, from `tutorials/`:

```powershell
$build = (Resolve-Path ../../build/gsplat-native).Path
$env:PATH = "$build/extern/glfw/src/Release;$build/extern/lua/Release;" + $env:PATH
$env:LUA_CPATH = "$build/languages/hg_lua/Release/?.dll;;"
& "$build/extern/lua/Release/lua.exe" gaussian_splatting.lua
```

The runtime or launcher supplies module search paths; the tutorial does not modify `package.cpath`.

## Models and initial poses

| Model | Compiled resource | Default retained data | Initial pose |
|---|---|---|---|
| Houseplant | `gaussian_splatting/source/plant.hgs` | All 113,648 splats, SH0 | Bounds-based framing; 180-degree X rotation converts -Y up |
| Saari | `gaussian_splatting/source/saari-probe.hgs` | All 1,307,339 splats, SH3 | Inside the sky shell, facing the island; -90-degree X rotation converts Z up |

The model transforms apply to positions, covariances and SH directions together. The GUI shows the retained count and stored SH degree. Both models' native buffers are loaded during setup. Per-view GPU scratch is allocated on first submission and retained independently for each model.

Houseplant is by Marcel Padilla, CC BY 4.0. Its unmodified `.splat` source is 3,636,736 bytes; attribution, source revision and checksum are in [ATTRIBUTION.md](resources/gaussian_splatting/ATTRIBUTION.md). The converter normalizes the full position bounds and encodes the stored RGB as SH0 without a gamma transform; it cannot recover higher harmonics absent from the source.

The local Saari SPZ v3 source is 38,221,014 bytes, contains SH3 and has antialiasing enabled. SHA-256: `34b352905666508d92d19f5d97b7168588985177bb7992792c0d89f22bc77739`. Its matching PLY has SHA-256 `4646f8b9ead430be1d489f79700ed1c91d51c0b8735ce6f6044615cd9bb8bb01`. These files belong to the Forward/Komplex preservation project. Postshot antialias metadata enables determinant-ratio opacity compensation in native projection.

## Conversion, budgets and storage

`assetc` reads the adjacent `.meta` file and invokes toolchain `gsplatc`. A Saari metadata profile has this shape:

```json
{
  "profiles": {
    "default": {"type": "gaussian-splat", "sh-degree": 3, "normalize": true, "storage": "quantized"},
    "preview": {"max-count": 250000, "sh-degree": 1},
    "reference": {"storage": "float32"}
  }
}
```

Houseplant's metadata caps storage at SH0 and its `preview` profile selects 30,000 splats. Selected-profile fields override `default` fields, then built-in defaults. Files without `type: "gaussian-splat"` pass through unchanged. See the [Asset Compiler manual](../doc/doc/man.AssetCompiler.md#gaussian-ply-splat-and-spz-compilation) for the complete schema.

Omitting `max-count` preserves every source splat. Use `assetc -profile preview resources` for the reduced Saari profile above, or provide an explicit positive cap. A profile inherits any explicit cap from `default`; `preview` is never selected automatically. Explicit zero is invalid.

Conversion validates every source record, even when decimating. With an explicit cap, retained records are evenly spaced midpoint samples; this naive reduction can introduce holes and does not merge splats or compensate opacity. The supported maximum is 2,097,152 retained records, subject to a 512 MiB expanded-data budget per model. Exceeding either budget fails rather than silently decimating. `normalize: true` centers the full position bounds and scales their largest extent to two units.

The compiled format is quantized, zlib-compressed HGS2, in blocks of up to 256 splats. Adaptive ranges cover position, scale, DC and higher SH. `assetc -profile reference ...` selects compressed `float32` storage for lossless imported attributes; `-profile default` restores compact storage. The CLI equivalent is `gsplatc INPUT.ply OUTPUT.hgs --storage quantized` (or `float32`). The converter reports file size and reconstruction errors.

Loading decompresses/dequantizes into the native float buffers; GPU memory use is unchanged. HGS0/HGS1 are rejected and must be recompiled. See the [HGS2 specification](../specifications/SPECS_GAUSSIAN_SPLATTING_HGS2.md).

Native import accepts headerless 32-byte `.splat`, Graphdeco/Postshot binary little-endian float PLY, and gzip SPZ v2/v3 with SH0-3 and the antialias flag. SPZ v4, extension flags and SH4 are unsupported. SPZ decoding uses the existing miniz dependency and occurs in `gsplatc`, outside the render loop. [SPZ format reference](https://github.com/nianticlabs/spz/tree/11312e4ad836ed06e3da2f3d5b633688d17ebebb).

SPZ quantization can already clip color/SH in the input: the local Saari comparison found 186 of 750,000 retained DC channels beyond its representable range. HGS2 cannot restore those source values. Use the original PLY with `float32` storage for a reference. To prepare Saari from PLY, stage `saari-probe.ply` and rename the sidecar to `saari-probe.ply.meta`; keep only one opted-in source per basename, since both inputs produce `saari-probe.hgs`.

## GUI and controls

| Control | Action |
|---|---|
| Model combo | Switch between preloaded Houseplant and Saari |
| Space | Switch model once per press, including while hovering the GUI |
| SH degree slider | Select degree 0..3 for Saari; Houseplant displays its fixed SH0 |
| Reset camera button / R | Restore the active model's initial pose |
| Left drag outside the GUI | Quaternion trackball, including roll |
| Wheel outside the GUI | Dolly |
| Right/middle drag outside the GUI | Pan |
| 1 / 2 / 3 / 4 outside the GUI | Select SH degree 0 / 1 / 2 / 3, capped at the stored degree |
| Escape | Close and release resources |

Camera navigation uses [`hg.TrackballController`](../doc/doc/TrackballController.md). ImGui mouse capture blocks camera input, including the wheel; the first drag after returning from the GUI or reacquiring focus is suppressed. Camera poses and SH selections survive model changes. Runtime SH selection changes evaluation only; the importer's `sh-degree` cap controls stored data.

On Retina/HiDPI displays, the tutorial passes the current framebuffer/client ratio to the `ImGuiBeginFrame` mouse-scale overload. Mouse hit testing then uses the same pixel coordinates as the GUI; the camera continues using client coordinates. The ratio is recomputed every frame and is not a fixed macOS multiplier.

The application clears view 0, submits the selected model through two native views, draws the orientation overlay in the returned next view, then draws ImGui after the overlay. It handles resize and explicitly releases both models, both render-data objects, the shared renderer and GUI resources before render shutdown.

Splats use global center-depth sorting, premultiplied blending and depth testing without depth writes. Submit overlapping Gaussian instances together for a global sort. The native renderer supports translation, rotation and positive uniform scale. Opaque meshes can share the target depth buffer; arbitrary translucent mesh composition and exact per-pixel splat ordering remain outside this implementation.

## Full-count rendering performance

The native renderer keeps uploaded source/SH data in immutable GPU buffers, uses a stable 24-dispatch radix sort above 256 padded entries, and reuses projection/sort/gather when camera and scene inputs are unchanged. The Lua call sequence and SH quality are unchanged. One full Saari instance requires 27 compute dispatches while moving and zero on a cache hit; both cases still draw every frame and use the current target/depth buffer. Compact scheduling continues to reserve two views.

Retain one `GaussianSplatRenderData` per independently rendered camera/pass. Changes to camera, projection, viewport size, model/instance IDs, transforms, opacity, SH degree, radius cap or renderer invalidate cached compute results. Reordering an otherwise identical instance list does not. Multiple submissions sharing render data must execute in increasing BGFX view order; do not reorder their compute/draw dependencies afterward.

Rebuild the Lua module **and compile the updated core shaders** together. The canonical package now also includes `sort_local.sc`, `radix_histogram.sc`, `radix_prefix.sc` and `radix_scatter.sc`; `sort.sc` remains the correctness reference. Existing compiled HGS2 assets need no reconversion. See the [performance report and reproducible benchmark](../specifications/SPECS_GAUSSIAN_SPLATTING_NATIVE_PERFORMANCE.md) for measured results and backend coverage.

The full-count route is also tested on Apple M3 Pro/Metal: 57.0 fps average on the 60-second moving-camera benchmark at 1280x720, 27 compute dispatches while moving and zero on cache hits. See the [Mac validation and Retina input checks](../specifications/GSPLAT_MACOS_OPTIMIZED_VALIDATION.md).

## Building locally on Windows

From the repository root (`harfang3d/`), with the installed Visual Studio generator and FABGen path:

```powershell
cmake -S . -B ../build/gsplat-native -G "Visual Studio 18 2026" -A x64 `
  "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" `
  -DHG_BUILD_HG_LUA=ON -DHG_BUILD_ASSETC=ON -DHG_BUILD_TESTS=ON `
  -DHG_BUILD_ASSIMP_CONVERTER=OFF -DHG_BUILD_FBX_CONVERTER=OFF `
  -DHG_BUILD_GLTF_IMPORTER=OFF -DHG_BUILD_GLTF_EXPORTER=OFF `
  -DHG_BUILD_LEGACY_ARCHIVE=OFF -DHG_BUILD_DOCS=OFF `
  -DHG_FABGEN_PATH=C:/works/dev/harfang/FABGen
cmake --build ../build/gsplat-native --config Release --target assetc hg_lua recastc bulletc --parallel 6
cmake --install ../build/gsplat-native --config Release --component assetc --prefix ../install/gsplat-native-assets
& ../install/gsplat-native-assets/assetc/assetc.exe -api DX11 -platform windows tutorials/resources tutorials/resources_compiled
```

Installed `assetc` discovers `gsplatc`, `shaderc` and `texturec` in its platform toolchain. Use `assetc -toolchain PATH ...` for a custom toolchain.

## Regression checks

The headless native `engine.trackball_controller` test covers arcball rotation, dolly, pan, reset, stability and injected mouse input. Its Lua binding check runs without rendering. From `tutorials/`, with the build environment above:

```powershell
& "$build/extern/lua/Release/lua.exe" ../harfang/tests/gaussian_camera.lua "$build/languages/hg_lua/Release"
& "$build/extern/lua/Release/lua.exe" ../harfang/tests/gaussian_splatting_runner.lua `
  "$build/languages/hg_lua/Release" gaussian_splatting.lua 60 "$build/gsplat-unified"
```

The real-tutorial runner verifies the actual initialized renderer, that both models load before the loop, forces Houseplant -> Saari -> Houseplant through the GUI selection path, exercises SH1/SH3, forces two resizes and repeated GC, and checks native submissions and complete shutdown. Captures are `gsplat-unified.png` (Houseplant) and `gsplat-unified-saari.png`, with BGFX intermediate `.tga` files. Content checks exclude the GUI and reject empty or GUI-only captures. Assertions remain in the tests, not in the tutorial.

`harfang/tests/test_gaussian_splat_native.py` checks conversion, malformed input, HGS2 loading, multiple instances, mesh occlusion, SH pixels and lifetime. `tools/assetc/tests/test_gsplat.py` checks metadata and incremental compilation; `harfang/tests/run_gaussian_sort_probe.py` checks GPU-sorted IDs. Python is used only for developer tests.

The [native API study](../specifications/SPECS_GAUSSIAN_SPLATTING_NATIVE_API_STUDY.md) and [validation record](../specifications/GSPLAT_NATIVE_PROGRESS.md) retain the earlier implementation history.
