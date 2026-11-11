# Core package resource promotion

Date: 2026-11-09. `core-package/core` is now the reference implementation; `tutorials/resources/core` remains a synchronized copy for standalone tutorials.

## Initial comparison

The package contained 223 files, the tutorial core 238. No files existed only in the package. Among the 223 common paths, 222 were byte-identical and one shader differed semantically. Line endings were not responsible for that difference.

| Difference | Action and reason |
|---|---|
| Seven files under `gaussian_splatting/` | Promoted the native clear/project/sort/gather compute shaders, splat vertex/fragment shaders and varying definition. Their relative includes resolve against the package's existing `shader/` headers. |
| `shader/compositing_fs.sc` | Promoted tutorial version. It uses `u_color`/`u_depth`, matching the current AAA compositor bindings, and accepts the four `uCompositingParams` vectors already supplied by the engine. The first vector drives the vignette exposed in `scene_aaa_vignette.lua`. |
| Three `checkerboard_downsample` shader files | Preserved as optional utilities. The current engine uses the existing `aaa_downsample` program; this promotion does not switch pipeline algorithms. |
| Three `joint_bilateral_upsample` shader files | Preserved as optional utilities. No current engine load site uses this exact program name; the existing `aaa_upsample` and `joint_bilateral_blur` paths are separate. |
| `pbr/blue_sky.hdr` and its `.meta` | Promoted the existing bundled environment and probe-generation settings. `tutorials/resources/car_engine/engine.scn` references its generated irradiance/radiance maps. |

No shader algorithms were redesigned during this promotion. Shared BGFX headers, existing pipeline shaders and all other common resources were already identical. The runtime compositor still uses `u_copyColor`/`u_copyDepth` in its separate non-AAA copy path; only the final compositing shader needs `u_color`/`u_depth`.

## Reference workflow

Edit `core-package/core` first. `tools/sync_core_package.cmake` checks path/hash parity by default and copies reference changes to the tutorial tree with `-DMODE=UPDATE`. Unexpected tutorial-only files require review before synchronization; the helper performs no implicit deletions. Both trees contain the same 238 files after promotion.

The existing compute probe and Gaussian-sort probe now stage shared resources from `core-package/core`, so their tests exercise the reference implementation rather than the tutorial copy. No Python conversion/launch tooling is added.

## Validation

Validation logs and the exact promotion manifest are stored locally under `.codex_tmp/core-package-duo/`. Compile the package directly with installed `assetc`, preserving its `core/` prefix. The Gaussian GPU probes must load shaders from this package compilation, rather than a previous tutorial build. Tutorial-only mesh test shaders and generated HGS2 fixtures remain separate test inputs.

Completed Windows/D3D11 checks:

- Direct package compilation with installed assetc: 932 outputs processed, zero failures, including Gaussian compute/draw shaders, compositing, optional filter programs and HDR probes.
- Native Gaussian GPU probe passes using package-compiled shaders with both quantized and float32 HGS2 fixtures (multiple models, depth, SH, independent targets and ownership).
- All 131,072 sorted GPU IDs match the CPU total order using the reference sort shader.
- Existing C++ and Lua compute probes pass with the reference BGFX include files.
- The CMake parity check and update command both report 238 reference files and zero differences.
- The actual AAA vignette tutorial completes 60 frames through `CreateForwardPipelineAAAFromAssets`, with the package output mounted first in asset lookup order. The capture was inspected and no missing-resource warnings were reported.

Copilot/Kimi's read-only review found no blocking integration issue. Codex performed the compilation and runtime checks. Metal/OpenGL runtime support is not inferred from Windows/D3D11 checks.

## Additional engine observation

An exploratory AAA smoke test using an absolute path with `CreateForwardPipelineAAAFromFile` failed: `_CreateForwardPipelineAAA` loads several auxiliary programs through hard-coded `FromAssets` calls, even when its main reader/provider is the file route. The resulting absolute asset lookups fail. This pre-existing engine routing inconsistency is outside the shader/resource promotion; no engine code was changed here. The standard `FromAssets("core", ...)` route, with the compiled reference package mounted ahead of tutorial assets, passes the runtime check above. Logs: `aaa-smoke.log` (exploratory file-route failure), `aaa-assets-smoke.log` and `aaa-vignette.png` (validated assets route).
