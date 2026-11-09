# Native Gaussian splatting: Saari implementation and validation

Date: 2026-11-09. Base revision: `2114a55`, branch `gsplat`.

This is an implemented, D3D11-tested slice of the [native API study](SPECS_GAUSSIAN_SPLATTING_NATIVE_API_STUDY.md). It provides [Saari](../tutorials/gaussian_splatting_native.lua) and [Houseplant](../tutorials/gaussian_splatting.lua) tutorials using the same native API. This does not complete every proposed engine feature. See the [preparation and launch guide](../tutorials/GAUSSIAN_SPLATTING_NATIVE.md).

## HGS2 quantized/compressed storage (2026-11-09)

HGS1 has been replaced outright, as requested: no legacy decoder is retained. Recompile existing assets. The [HGS2 specification](SPECS_GAUSSIAN_SPLATTING_HGS2.md) defines the new 128-byte header, checksummed block table, 256-splat adaptive quantization and per-block zlib compression through existing miniz. The default `storage` is `quantized`; `float32` is a lossless, compressed representation inside HGS2 for reference comparisons. Native loading reconstructs the same canonical float arrays and GPU buffers; the Lua rendering API is unchanged.

Copilot/Kimi implemented the codec and public write options/statistics. Codex reviewed it, handled CLI/assetc integration, added independent decoding and adversarial format tests, updated metadata/documentation, and performed builds and runtime comparisons.

### Actual sizes, at identical retained counts

Sizes below are exact bytes, measured on the local Saari SPZ and bundled plant source. Quantized source attributes are not clipped to fixed SPZ ranges: ranges adapt per block, including direct PLY conversion.

| Asset | Splats / degree | Old HGS1 | HGS2 float32 compressed | HGS2 quantized compressed |
|---|---:|---:|---:|---:|
| Saari from SPZ | 250,000 / SH3 | 76,000,128 | 15,884,337 | **8,493,320** |
| Houseplant | 113,648 / SH0 | 7,273,600 | 5,572,167 | **1,966,399** |

The compact files are 88.82% and 72.97% smaller respectively than the old HGS assets. The original Saari SPZ remains 38,221,014 bytes for 1,307,339 splats; that different-count file is not the basis of the reduction percentages above.

Direct PLY -> quantized HGS2 at 250,000 splats/SH3/normalized produced **14,678,839 bytes**. The PLY retains coefficient precision already lost by the SPZ source, so the two routes are not bit-equivalent. Direct PLY conversion reports maximum errors: position L2 2.1394e-5, relative scale 0.0205363, opacity 0.00196078, DC 0.0393994, higher SH 0.00352322, quaternion sign-equivalent L2 0.0020833. No fixed-range saturation is introduced.

### Verification

- Release `assetc`, `gsplatc`, and Lua module build successfully. All tutorial resources are recompiled; the default quantized profile is left active.
- Eleven native regression groups pass. Tests include independent HGS2 inspection, both storage modes, counts 1/255/256/257/513, SH0-3, zero-width ranges, numerical error bounds, deterministic output, existing source import checks, malformed HGS2 header/table/compression/payload rejection and HGS1 rejection.
- The existing Lua GPU composition/SH/lifetime probe runs against both float32 and quantized HGS2 fixtures. Invalid files are checked through file and Data loading routes; compressed trailing bytes and inflation overruns are rejected.
- Twelve assetc integration tests pass, including storage-profile inheritance, invalid storage values, cache invalidation and source conversion behavior.
- Both actual tutorials complete 60 D3D11 frames in both storage modes with two resizes, repeated GC, captures and clean shutdown. Native GPU memory requirements are unchanged.

Whole-frame RGB screenshot comparison (1280x720, identical initial camera): Houseplant mean absolute error 0.02291 on the 0-255 scale, p99 error 1, PSNR 62.52 dB; Saari mean error 0.52333, p99 error 3, PSNR 46.46 dB. Quantization is not pixel-identical: Saari's maximum local channel difference is 218, with 155 of 921,600 pixels differing by more than 8 in at least one channel. These are single-camera observations, not a guarantee for every viewpoint. Captures were visually inspected.

Evidence: local `.codex_tmp/gsplat-hgs2-duo/`, including converter/GPU/assetc logs, both-mode HGS assets, screenshots and `comparison.json`. Earlier HGS1 sizes/commands below remain historical evidence.

## SPZ import and Saari source migration (2026-11-09)

Native `gsplatc` and assetc now accept gzip SPZ **versions 2 and 3**, with the same `.meta` profiles, deterministic reduction, full-source normalization and HGS1 output as PLY. Source axes, SH0-3 and antialiasing are preserved. Runtime Lua continues to load HGS1 only.

- The local source was copied to `tutorials/resources/gaussian_splatting_native/saari-probe.spz` (gitignored), with tracked `saari-probe.spz.meta`. The verified former local PLY staging copy was removed; the original external PLY was untouched. The tutorial loads `gaussian_splatting_native/saari-probe.hgs`.
- Copilot/Kimi implemented the bounded gzip decoder, v2/v3 attribute import and CLI routing. Codex reviewed the code, added explicit plaintext-v4 diagnostics, assetc routing/collision handling, source staging, fixtures, documentation and validation. Existing miniz supplies raw DEFLATE and CRC32; no new dependency or Python conversion tool is introduced.
- Gzip optional fields/header CRC, footer CRC32/ISIZE, exact stream consumption, payload lengths and all source rotations are validated, including discarded records. Decompression is capped at 512 MiB; compressed input remains capped at 1 GiB. Unsupported v1/v4, extension flags and SH4 fail explicitly. This is not an unrestricted import of every SPZ revision.
- Source: 38,221,014 bytes, SHA-256 `34b352905666508d92d19f5d97b7168588985177bb7992792c0d89f22bc77739`, 1,307,339 splats, SH3, antialiasing. Default output: 250,000 splats, SH3, 76,000,128-byte HGS1, flags 7 (AA/normalization/higher SH).
- Release converter, assetc and Lua binding build successfully. Nine CPU test groups pass, including independent v2/v3 fixtures, all quaternion largest-component indices, signed positions, SH truncation/stride, alpha endpoints, normalization, optional gzip fields and malformed containers. Eleven assetc integration tests pass, including SPZ metadata/incremental/pass-through/profile handling and SPZ/PLY output collisions. Existing GPU composition/SH/lifetime regression passes.
- The actual Saari Lua tutorial completes 60 D3D11 frames, two resizes, repeated GC and clean shutdown with the SPZ-derived asset. Screenshot inspected. Evidence is in local `.codex_tmp/gsplat-spz-duo/`.

The full 250,000-record HGS payload was compared with the previous PLY-derived HGS. Maximum normalized position error is 1.44e-6; alpha error 0.001961; relative scale error 3.175%; sign-equivalent quaternion norm error 0.002084. These agree with the source's quantization. The retained PLY has 186 DC channel values outside SPZ's representable range; those are already clipped in the SPZ source. After accounting for this clipping, maximum DC error is 0.013073. Maximum higher-SH error is 0.066407, within the two-stage 4-bit quantization bound (1/16 + 1/256). Conversion cannot recover information removed by SPZ encoding.

Format reference: [Niantic SPZ source](https://github.com/nianticlabs/spz/tree/11312e4ad836ed06e3da2f3d5b633688d17ebebb). Current preparation/launch instructions are in the [Saari guide](../tutorials/GAUSSIAN_SPLATTING_NATIVE.md). Earlier PLY-only commands below remain historical evidence.

## Single-file tutorials and Python cleanup (2026-11-09)

The current workflow is documented in the [Houseplant guide](../tutorials/GAUSSIAN_SPLATTING.md) and [Saari guide](../tutorials/GAUSSIAN_SPLATTING_NATIVE.md). Earlier commands and timings below describe the previous staging/launcher workflow and remain historical evidence.

- Both applications are self-contained Lua sources with embedded trackball controls. Houseplant also embeds its RGB axis gizmo. Loading, projection, sort scheduling, resource ownership and splat drawing use the native API.
- Removed all Gaussian Python tools under `tutorials/tools`: the HGS0 converter and its tests, both launchers, and the legacy pipeline checker. Removed the former Lua renderer/camera/gizmo modules and duplicate Gaussian shaders. Native shaders live under `resources/core/gaussian_splatting`; relative includes work with ordinary assetc compilation.
- Copilot/Kimi implemented native headerless 32-byte `.splat` import and CLI/assetc routing. Codex integrated the tutorials, cleanup and documentation, reviewed the importer, and added/run validation. The importer shares PLY normalization/finalization, validates discarded records, decodes packed WXYZ rotations and linear scale/alpha, and represents stored RGB as SH0. Transparent alpha zero is valid canonical data.
- `plant.splat.meta` preserves all 113,648 plant records by default; `preview` selects 30,000. The single native asset is HGS1. `saari.ply.meta` retains 250,000 splats/SH3; its external source is explicitly ignored by Git. No Python is required to prepare or launch either tutorial.
- Python is retained for developer regression tests under `harfang/tests` and `tools/assetc/tests`. The large GPU sort test now compiles the actual native shader and has no dependency on removed tools. The real-tutorial Lua smoke runner exercises both files rather than maintaining another renderer.

Validation on Windows/D3D11:

- Release `assetc`, `gsplatc` and Lua module rebuilt; installed toolchain used for all tutorial resource compilation.
- Eight native regression groups pass, including the new binary `.splat` fixtures, invalid dropped records, normalization, SH0 RGB/alpha/quaternion decoding, PLY regressions, HGS1 loading and GPU composition/SH/lifetime checks.
- Ten assetc integration tests pass, including `.splat` opt-in/pass-through, metadata profiles, daemon changes, incremental rebuilds, failure handling and same-output source collisions.
- Full 131,072-entry GPU sort agrees with the independent CPU total order.
- Houseplant and Saari each complete 60 frames through their actual tutorial entry point, two window resizes, repeated Lua GC and explicit shutdown. Screenshots visually checked; Houseplant preserves the pot, foliage, colors and RGB gizmo.
- Logs and captures: local `.codex_tmp/gsplat-cleanup-duo/`; generated assets: `tutorials/resources_compiled/` (ignored).

Metal qualification remains outstanding for this migrated path. The API's composition and transform limitations below still apply.

## Delivered architecture

| Layer | Implementation |
|---|---|
| Offline conversion | `assetc` reads PLY `.meta` profiles and invokes toolchain `gsplatc`: strict Graphdeco/Postshot binary little-endian scalar-float PLY or headerless 32-byte `.splat`, deterministic midpoint decimation, optional normalization, SH0–3 preservation, Postshot antialias flag |
| CPU codec | `engine/gaussian_splat_data.{h,cpp}`: canonical CPU data and a versioned HGS1 serializer/parser; usable without `RenderInit` |
| Engine | `engine/gaussian_splat.{h,cpp}`: shared model/program resources, transformed instances, camera-specific scratch, capability/budget/transform/view checks, load results and submission statistics |
| GPU | Per-instance projection into disjoint ranges, one global far-to-near bitonic sort with stable ID/source-index ties, gather, premultiplied drawing; SH evaluation after visibility culling |
| Lua | FabGen opaque handles, instances/settings/results, metadata getters and `SubmitGaussianSplats`; model-valued properties copy the shared handle so their owner can be collected safely |
| Application | Window, trackball, viewport, clear, quality selection, one native submission and `Frame`; no source parsing, per-splat traversal or sort orchestration in Lua |

The compact schedule uses one compute view and one graphics view. `DispatchComputeSameView` shares the existing validator with `DispatchCompute`, whose public advancing-view semantics are preserved. A separate-view reference schedule remains available for comparison.

The renderer accepts positive uniform scale, rotation and translation. It uses the supplied perspective `ViewState`, including off-axis terms, and stores projected center depth. Opaque geometry can share the framebuffer's depth attachment; splats test depth and do not write it. Instance SH directions are transformed back by the inverse instance rotation. Runtime SH degree selection preserves the stored coefficient stride.

Explicit destruction invalidates shared aliases. Weak registries release surviving native resources during `RenderShutdown`, before generic compute teardown. Models are immutable after upload; there is no CPU splat traversal during submission. Per-camera scratch grows to a power-of-two capacity and is retained, while graphics submissions draw only the active prefix.

## Source and preparation

Validated source: `C:/works/projects/preservation-forward-by-komplex/postshot/saari-probe.ply`.

- SHA-256: `4646f8b9ead430be1d489f79700ed1c91d51c0b8735ce6f6044615cd9bb8bb01`.
- 1,307,339 records, 59 float properties, SH degree 3, `postshot.anti_aliasing=1`.
- Every source record is validated, including records discarded by decimation.
- Default budget: 250,000 retained splats. Selection is `floor((2*i+1)*source_count/(2*retained_count))` in source order.
- `--normalize` centers the complete source position bounds and maps their largest extent to two units. Bounds stored in HGS1 enclose the retained centers. Coordinates and linear scales change together; SH does not change.
- The tutorial applies a proper −90° X rotation from the capture's Z-up coordinates to HARFANG's Y-up presentation. Its initial camera is inside the learned sky shell, aimed at the island.

No source dataset, native binary asset or compiled shader is checked into the repository. The Python launcher orchestrates native tools and records hashes/counts/settings; it does not convert individual splats. Python 3.11 or newer is required for this helper.

## HGS1 runtime format

All fields are little endian. The format is deliberately distinct from the old tutorial's HGS0. It is an initial internal runtime format, not an interchange standard or a promise to accept future versions silently.

| Byte offset | Type | Meaning |
|---|---|---|
| 0 | char[4] | `HGS1` |
| 4, 6 | u16, u16 | Major 1, minor 0 |
| 8, 12 | u32, u32 | Endian marker `0x01020304`, header size 128 |
| 16 | u64 | Exact file byte length |
| 24 | u32 | Flags: bit 0 antialiased, bit 1 normalized, bit 2 higher SH present |
| 28, 32, 36 | u32 each | SH degree, retained count, source count |
| 40, 48 | u64 each | Base offset (128), base length (`count*64`) |
| 56, 64 | u64 each | SH offset (end of base), SH length (`count*nonDC*16`) |
| 72, 84 | f32[3] each | Retained-center bounds minimum and maximum |
| 96, 100 | u32 each | Base stride 64, SH coefficient stride 16 |
| 104, 108 | u32 each | Non-DC coefficient count (0/3/8/15), color encoding 0 (original Graphdeco domain) |
| 112 | byte[16] | Reserved, zero |

Each base record is four float4 values: `(position.xyz, opacity)`, `(linear_scale.xyz, 0)`, `(quaternion.wxyz)`, `(DC.rgb, 0)`. Higher SH follows as coefficient-major float4 `(r,g,b,0)` records within each splat. PLY channel-major `f_rest_*` values are transposed on import. Opacity is sigmoid-decoded, scales are exponentiated, and quaternions normalized. Reconstructed color is `max(0, 0.5 + sum(SH*basis))` without an upper clamp or implicit color-space conversion.

The parser rejects unsupported versions/flags, overlapping or noncanonical sections, incorrect lengths/strides/bounds, nonzero reserved fields, nonfinite values, nonpositive scales and nonnormalized quaternions. Limits are 1 GiB for source PLY, 512 MiB for HGS1, and 1,048,576 retained/submitted splats. Source conversion validates and serializes before opening its output; validation failures preserve an existing destination. Filesystem write failures are not claimed to be transactional.

## Executed validation

Windows x64, Visual Studio 2026 Release build, D3D11. The engine, FabGen Lua module, Lua interpreter, `gsplatc`, `assetc`, `shaderc` and compute probe built successfully.

`harfang/tests/test_gaussian_splat_native.py` passed all seven groups:

1. Independent HGS decoding, midpoint selection, source/retained counts, opacity/scale/quaternion/DC values and bounds.
2. SH degrees 0–3, channel/coefficient mapping and malformed bands.
3. Explicit normalization based on full source bounds, with unchanged SH.
4. Reordered named PLY properties.
5. Invalid schemas/payloads rejected without replacing a pre-existing destination.
6. Invalid discarded records rejected too.
7. Native Lua/GPU integration: malformed File/FromData loads, global and stable ordering, compact/separate schedule pixel equality, padding/all-invisible cases, depth rejection controls, a conventional opaque triangle between two Gaussian models, independent camera targets, SH1 sign, SH3 k=12 evaluation, lower-degree evaluation from stored SH3, failed preflight preserving the view ID, ownership across Lua GC, alias destruction and shutdown invalidation.

The triangle test renders opaque blue at z=2.5 between red at z=3 and green at z=2. The center pixel is approximately `(0, 0.5, 0.5, 1)`: red is occluded, green blends over blue. This distinguishes projected center depth from the old constant-depth splat shader. It does not establish general transparent-mesh interpenetration or AAA integration.

The existing C++ and Lua compute probes also passed after rebuilding, including their invalid-input controls. Evidence is under `<build>/native-tests/summary.json` and `<build>/compute_probe/dx11/report.json`.

The real tutorial ran to normal cleanup with screenshots at both budgets below, at 1280×720, SH3, using the final initial camera. Timing samples are available BGFX GPU-frame timestamps after 20 warm-up frames, not a cross-device benchmark. Payload estimates exclude driver allocation overhead and other engine resources.

| Retained splats | Compute dispatches/frame | Native views/frame | GPU payload bytes | GPU median / p95 |
|---|---:|---:|---:|---:|
| 250,000 | 174 | 2 | 125,283,072 | 16.710 / 17.923 ms, 99 samples |
| 1,048,576 | 213 | 2 | 515,899,392 | 83.388 / 86.954 ms, 69 samples |

The application additionally uses one clear view. Local screenshots/logs/manifests are under `<build>/gaussian_splatting_native/dx11/`; `saari.png` is the default-budget capture and `saari-1048576.png` the higher-budget capture. Naive decimation visibly reduces coverage, especially in the sky. The default favors interactivity; the larger budget gives a denser image at substantially greater cost. No opacity rescaling conceals the lost samples.

## Remaining scope

SPZ v1/v4, extension records, SH4 and glTF adapters, direct GPU decoding of quantized storage, streaming, radix/tile sorting, orthographic/reversed-Z projections, nonuniform/reflected instance transforms, explicit prepare/draw tokens, scene serialization, full forward/AAA insertion and arbitrary transparent-mesh composition remain unimplemented. The renderer uses center-depth ordering/occlusion and a configurable per-axis standard-deviation cap, so it is still an approximation for large overlapping Gaussians. The cap defaults to 256 pixels; quads extend to three standard deviations.

Metal/OpenGL/Vulkan execution has not been qualified for this native slice. The PLY importer is intentionally strict, not a generic polygon PLY reader. Postshot antialias compensation is implemented, but pixel identity against Postshot is not claimed. Mounted-asset loading is implemented through HARFANG providers; this validation exercised File and FromData.

## Duo contribution

Actual Copilot CLI sessions used `kimi-k3.1-pro:cloud` through the local Ollama provider. Kimi authored the CPU codec/converter, renderer/shaders, initial CMake/FabGen integration and independent test harness/probe in an isolated worktree. Codex integrated the delivery, fixed compilation and cross-language shader issues, corrected FabGen handle-property ownership and the independent test decoder/SH sign expectation, added full-source normalization, the opaque-mesh and SH3 regression cases, and implemented/run the thin Lua tutorial and asset launcher. All runtime results above were executed during integration; Kimi's initial static review was not counted as test evidence.

## Asset compiler integration follow-up

The same-day follow-up adds `type: "gaussian-splat"` to the existing `profiles` metadata convention. `assetc` converts `name.ply` plus `name.ply.meta` into `name.hgs` by launching external `gsplatc`; the converter is installed in the same platform toolchain directory as `texturec` and `shaderc`. PLY files without the declaration retain pass-through behavior. The [asset compiler manual](../doc/doc/man.AssetCompiler.md) and [Saari guide](../tutorials/GAUSSIAN_SPLATTING_NATIVE.md) document the schema, defaults and `-profile` overrides.

The build context includes effective conversion settings and a hash of the converter binary. Source/sidecar dependencies, metadata creation/removal, and profile changes participate in incremental processing. Compiler-owned alternate PLY/HGS outputs are cleaned when the conversion mode changes. Name collisions with independent HGS sources fail explicitly. Invalid metadata produces diagnostics and a nonzero one-shot exit status; failed compiler outputs are removed so retries cannot become false incremental successes.

Validation executed on Windows Release:

- All **8 assetc Gaussian integration tests** passed against the installed executables: direct-converter byte equivalence, nested paths/spaces, no-op reuse, source/meta edits, per-field profile fallback, pass-through transitions, malformed metadata including numeric overflow, missing compiler, failed conversion/retry, source removal, output collisions, and daemon metadata edit/removal.
- All **10 existing texture compiler/assetc tests** passed.
- Installing the `assetc` component placed `gsplatc.exe` at `assetc/toolchains/host-windows-x64-target-windows-x64/gsplatc.exe`, with no duplicate at the assetc root. A conversion using installed `assetc` without a `-toolchain` override found and ran that converter successfully.
- The real Saari tutorial was prepared via its generated PLY `.meta` and ran for 45 frames to normal cleanup with a screenshot. The compiled 250,000-splat SH3 asset retained its prior SHA-256, `25fcec566c8689b91f1e4ce91248accd2c4dcfa295e459d0d758e5a4d1b83e7f`.

The new tests are `tools/assetc/tests/test_gsplat.py`; evidence is under `<build>/assetc-gsplat-installed-tests/summary.json`. The launcher now stages source PLY/metadata and common toolchain executables, invokes only `assetc` for compilation, and writes its evidence manifest at `<build>/gaussian_splatting_native/<backend>/manifest.json`. Runtime code is unchanged.

Copilot/Kimi authored the assetc/CMake integration. Codex reviewed it, tightened unsigned range and JSON-overflow validation, added the integration tests, updated the launcher/docs, and ran build, installation, regressions and Saari validation.
