# Houseplant: a single-file native Gaussian tutorial

[gaussian_splatting.lua](gaussian_splatting.lua) displays the bundled Houseplant by Marcel Padilla (CC BY 4.0). It is one Lua source: window, native trackball controller, RGB axis overlay and native Gaussian API calls. No companion Lua modules or Python preparation/launch tools are needed.

C++ owns HGS2 loading, GPU resources, covariance projection, global depth sorting and drawing. Lua submits one instance list per frame with `SubmitGaussianSplats`. The compiled shaders remain ordinary HARFANG resources; single-file refers to the Lua application, not an embedded asset bundle.

## Compile and run

Use a build containing the native Gaussian API, Lua binding and `assetc`. Installed `assetc` discovers its toolchain automatically; `gsplatc` lives beside `shaderc` and `texturec`. From `tutorials/`:

```powershell
assetc -api DX11 -platform windows resources resources_compiled
lua gaussian_splatting.lua
```

On macOS compile with `-api MTL -platform osx`; the tutorial selects Metal there. The migrated path has been exercised on Windows/D3D11; it has not yet been qualified on Metal.

For the local, uninstalled Windows build, from `tutorials/`:

```powershell
$build = (Resolve-Path ../../build/gsplat-native).Path
$env:PATH = "$build/extern/glfw/src/Release;$build/extern/lua/Release;" + $env:PATH
$env:LUA_CPATH = "$build/languages/hg_lua/Release/?.dll;;"
& "$build/extern/lua/Release/lua.exe" gaussian_splatting.lua
```

See the [Saari guide](GAUSSIAN_SPLATTING_NATIVE.md) for build configuration and installation commands. Preparing and running either tutorial requires no Python.

## Source conversion and budget

The bundled `resources/gaussian_splatting/source/plant.splat` contains **113,648** splats. Its adjacent `plant.splat.meta` opts into native conversion:

```json
{
  "profiles": {
    "default": {"type": "gaussian-splat", "max-count": 250000, "sh-degree": 0, "normalize": true},
    "preview": {"max-count": 30000}
  }
}
```

`assetc` invokes `gsplatc`, producing `resources_compiled/gaussian_splatting/source/plant.hgs`. The default preserves all records. `assetc -profile preview -api DX11 -platform windows resources resources_compiled` selects 30,000 records by deterministic midpoint sampling of the source order. This naive reduction may introduce holes; it does not merge splats or compensate opacity. Recompile with `-profile default` to restore the full asset.

The native importer supports the headerless 32-byte `.splat` layout: little-endian float32 XYZ position and linear XYZ scale, byte RGBA and packed byte WXYZ quaternion. It normalizes quaternions, preserves linear alpha (including zero), and encodes stored RGB as SH0 coefficients. It does not invent higher harmonics or perform a gamma conversion. Invalid records are rejected even when decimation would discard them.

Normalization centers the complete source position bounds and scales their largest extent to two units. The tutorial rotates the instance 180 degrees around X because the source is -Y up. The runtime transforms positions and covariances together. Source attribution and checksum are in [ATTRIBUTION.md](resources/gaussian_splatting/ATTRIBUTION.md).

The default compiled asset uses quantized, zlib-compressed HGS2 blocks. A `reference` profile with `"storage": "float32"` preserves imported values for comparison. `gsplatc` quantifies reconstruction errors; the native loader decompresses/dequantizes before GPU upload. GPU memory usage is unchanged. Recompile any HGS1 assets; no legacy reader is retained. See the [format specification](../specifications/SPECS_GAUSSIAN_SPLATTING_HGS2.md).

## Controls and rendering

Camera navigation uses [`hg.TrackballController`](../doc/doc/TrackballController.md). Lua chooses the initial pose from the model bounds, gates updates on window focus and handles the reset key. Orbit, dolly, pan and camera-world transform computation are native. The runtime or launcher supplies the module search paths; the tutorial does not modify `package.cpath`.

| Input | Action |
|---|---|
| Left drag | Quaternion trackball, including roll |
| Wheel | Dolly |
| Right/middle drag | Pan |
| R | Reset camera |
| Escape | Close and release resources |

The native submission uses two views (compute and draw). The application clears view 0 and draws the gizmo in the returned next view. It handles framebuffer resize and explicitly destroys Gaussian resources before renderer shutdown.

Splats use global center-depth sorting, premultiplied blending and depth testing without depth writes. Multiple instances can be passed in the same submission. Opaque meshes can share the target depth buffer; intersecting translucent meshes and exact per-pixel splat ordering remain outside this implementation.

## Regression checks

The headless native `engine.trackball_controller` unit test checks arcball rotation, dolly, pan, reset, stability and injected mouse input. The Lua binding smoke check runs without rendering:

```powershell
& "$build/extern/lua/Release/lua.exe" ../harfang/tests/gaussian_camera.lua "$build/languages/hg_lua/Release"
```

Python remains only in developer tests, outside `tutorials/tools`: `harfang/tests/test_gaussian_splat_native.py` validates conversion and optional Lua/GPU composition, and `tools/assetc/tests/test_gsplat.py` exercises metadata, incremental compilation and failures. `harfang/tests/run_gaussian_sort_probe.py` checks every GPU-sorted ID using the actual native sort shader.

The small Lua test runner executes the real tutorial, forces two resizes and repeated GC, checks successful native submissions and captures a screenshot. From `tutorials/`, after setting the build environment above:

```powershell
& "$build/extern/lua/Release/lua.exe" ../harfang/tests/gaussian_splatting_runner.lua `
  "$build/languages/hg_lua/Release" gaussian_splatting.lua 60 "$build/houseplant"
```

This produces `houseplant.png` and the BGFX intermediate `houseplant.tga`. The former Python HGS0 converter, launcher and legacy renderer/oracle have been removed; recompile resources to replace any old generated datasets.
