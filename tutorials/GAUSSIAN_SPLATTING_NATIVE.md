# Saari through the native Gaussian API

[gaussian_splatting_native.lua](gaussian_splatting_native.lua) is a single Lua source using native C++ loading, resource ownership, GPU projection, spherical harmonics, global sorting and drawing. It embeds its trackball camera. The [Houseplant tutorial](GAUSSIAN_SPLATTING.md) uses the same native implementation with a bundled SH0 sample.

## Prepare and run

The local Saari source is not bundled. The default source is the Postshot **SPZ version 3** export. Native import also accepts gzip SPZ version 2 and Graphdeco/Postshot binary little-endian PLY. From `tutorials/`:

```powershell
Copy-Item C:/works/projects/preservation-forward-by-komplex/postshot/saari-probe.spz resources/gaussian_splatting_native/saari-probe.spz
assetc -api DX11 -platform windows resources resources_compiled
lua gaussian_splatting_native.lua
```

The source destination is gitignored. Its tracked `saari-probe.spz.meta` selects 250,000 splats, SH3 and normalization. `assetc` delegates to toolchain `gsplatc` and writes `resources_compiled/gaussian_splatting_native/saari-probe.hgs`. A `preview` profile selects 50,000 splats and SH1. The Lua program takes no arguments and loads only compiled resources. Neither preparation nor launching requires Python.

Installed `assetc` finds `gsplatc`, `shaderc` and `texturec` together in its platform toolchain directory. For a custom toolchain use `assetc -toolchain PATH ...`. Metadata fields, defaults and validation are documented in the [Asset Compiler manual](../doc/doc/man.AssetCompiler.md#gaussian-ply-splat-and-spz-compilation). For example:

```json
{
  "profiles": {
    "default": {"type": "gaussian-splat", "max-count": 250000, "sh-degree": 3, "normalize": true},
    "preview": {"max-count": 50000, "sh-degree": 1}
  }
}
```

Selected-profile fields individually override `default` fields, then built-in defaults. `.ply`, `.splat` and `.spz` files without `type: "gaussian-splat"` pass through unchanged. Conversion writes versioned HGS1; the runtime rejects legacy HGS0.

To use PLY instead, copy the source as `resources/gaussian_splatting_native/saari-probe.ply` and rename the sidecar from `saari-probe.spz.meta` to `saari-probe.ply.meta`. Keep only one opted-in source for a given basename: both formats produce `saari-probe.hgs` and assetc rejects output collisions.

## SPZ scope

The native importer supports the gzip-based **SPZ v2/v3**, SH0-3 and the antialias flag. It validates the gzip header, CRC32, uncompressed size, exact attribute lengths and source records, with bounded decompression. Unsupported versions, extension flags and SH4 are rejected explicitly. SPZ v4 uses a different ZSTD container and is not supported by this importer; the supplied Saari file is v3.

SPZ is quantized: positions use signed 24-bit fixed point, scales use quantized logarithms, opacity is linear byte alpha, and color/SH coefficients have reduced precision and representable ranges. In the local Saari comparison, 186 of 750,000 retained DC channel values exceed the SPZ range and are already clipped in the source SPZ; HGS conversion cannot restore those PLY values. The decoder preserves stored coordinate axes, decodes v2 first-three or v3 smallest-three quaternions, and expands attributes to HGS1. No gamma transform or invented SH bands are applied. Source coordinates match the local PLY within quantization precision; the tutorial keeps its existing X rotation.

Format reference: [Niantic SPZ](https://github.com/nianticlabs/spz/tree/11312e4ad836ed06e3da2f3d5b633688d17ebebb), inspected at this revision. No upstream decoder or new library is bundled; gzip uses HARFANG's existing miniz dependency. SPZ decoding occurs in `gsplatc`, not in Lua or the render loop.

## Building locally on Windows

From the repository root, the tested Visual Studio 2026/CMake 4 configuration is:

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

Select the installed generator and FabGen location on other machines. For an uninstalled Lua runtime, set `PATH` and `LUA_CPATH` as shown in the [Houseplant guide](GAUSSIAN_SPLATTING.md#compile-and-run), then run `gaussian_splatting_native.lua` from `tutorials/`. Metal uses `-api MTL -platform osx`; this native path has only been validated on Windows/D3D11 so far.

## Data and controls

The SPZ source has **1,307,339 splats**, SH3 and antialiasing enabled, compressed into 38,221,014 bytes. SHA-256: `34b352905666508d92d19f5d97b7168588985177bb7992792c0d89f22bc77739`. The matching PLY has SHA-256 `4646f8b9ead430be1d489f79700ed1c91d51c0b8735ce6f6044615cd9bb8bb01`. These sources belong to the local Forward/Komplex preservation project and remain gitignored.

The converter validates all source records and retains evenly spaced midpoint indices when the budget is smaller than the source. This naive decimation can reduce coverage and introduce holes. Set `max-count` in the metadata (maximum 1,048,576) and recompile to change the budget. Higher budgets increase GPU sorting and drawing costs.

`normalize: true` centers the full source bounds and scales their largest extent to two units. It is enabled for this tutorial camera; ordinary conversion defaults to preserving coordinates. The instance rotates -90 degrees around X to convert the capture's Z-up orientation. Native projection transforms covariance and SH directions consistently. Postshot antialias metadata enables determinant-ratio opacity compensation.

| Input | Action |
|---|---|
| Left drag / wheel / right or middle drag | Trackball / dolly / pan |
| R | Reset camera |
| 1 / 2 / 3 / 4 | Evaluate SH degree 0 / 1 / 2 / 3 |
| Escape | Close and release resources |

Runtime SH selection only changes evaluation; it does not free stored bands. The importer's `sh-degree` cap controls storage. Reconstructed RGB is lower-clamped, without gamma conversion.

## Native interface and checks

```lua
local loaded = hg.LoadGaussianSplatModelFromFile(compiled_asset)
assert(loaded.ok, loaded.message)
local renderer = hg.CreateGaussianSplatRendererFromFile(compiled_shader_directory)
local data = hg.CreateGaussianSplatRenderData()
local instance = hg.GaussianSplatInstance(loaded.model)
local settings = hg.GaussianSplatRenderSettings()
local result, next_view = hg.SubmitGaussianSplats(first_view, viewport, view_state,
    {instance}, renderer, data, settings)
assert(result.ok, result.message)
```

The caller owns clearing and view allocation. Submit all overlapping Gaussian instances together for a global sort. The current renderer supports translation, rotation and positive uniform scale. Opaque meshes can share target depth; splats use projected center depth without depth writes. This does not solve arbitrary translucent mesh composition.

From the repository root, after compiling resources:

```powershell
$build = (Resolve-Path ../build/gsplat-native).Path
$env:PATH = "$build/extern/glfw/src/Release;$build/extern/lua/Release;" + $env:PATH
python harfang/tests/test_gaussian_splat_native.py `
  --gsplatc "$build/tools/gsplatc/Release/gsplatc.exe" --output "$build/native-tests" `
  --lua "$build/extern/lua/Release/lua.exe" --module "$build/languages/hg_lua/Release" `
  --shaders tutorials/resources_compiled/core/gaussian_splatting `
  --mesh-shaders tutorials/resources_compiled/shaders --renderer dx11
```

These developer tests cover PLY/`.splat`/SPZ conversion, malformed records, HGS1 loading, multiple Gaussian instances, an opaque triangle between splats, SH1/SH3 pixels, camera targets and lifetime through GC/shutdown. The [Houseplant guide](GAUSSIAN_SPLATTING.md#regression-checks) describes the real-tutorial smoke runner; substitute `gaussian_splatting_native.lua` to test Saari.

The [native API study](../specifications/SPECS_GAUSSIAN_SPLATTING_NATIVE_API_STUDY.md) and [validation record](../specifications/GSPLAT_NATIVE_PROGRESS.md) describe the implementation and remaining scope.
