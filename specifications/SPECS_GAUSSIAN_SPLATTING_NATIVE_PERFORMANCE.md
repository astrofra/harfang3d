# Native Gaussian splatting: full-count performance

Date: 2026-11-09. Implementation and validation: Codex / Copilot CLI with Kimi.

macOS follow-up: the optimized full-count path now passes native, sort/gather, cache and actual-tutorial tests on Apple M3 Pro/Metal. The same moving-camera benchmark measures 57.0 fps average at 1280x720 with all 1,307,339 Saari splats. See the [Mac measurements, Retina input fix and qualification scope](GSPLAT_MACOS_OPTIMIZED_VALIDATION.md). The measurements below remain the original Windows before/after run.

Copilot implemented the sorting kernels and reviewed the final buffer/cache/sort lifecycle without identifying a concrete blocker. Codex integrated and corrected the implementation, profiled the native path, fixed immutable-buffer allocation and ran the builds, GPU oracles and benchmarks. The static review does not substitute for device testing.

This implements the immediate performance work from the [Lua tutorial roadmap](SPECS_GAUSSIAN_SPLATTING_LUA_TUTORIAL_ROADMAP.md). Full Saari rendering keeps all **1,307,339 splats**, SH3, the existing projection, radius cap and blending equations. Applications keep their existing Lua submission calls. No import decimation, runtime particle limit, CPU sorting or synchronous readback was introduced.

## Measured result

Windows, Direct3D 11, NVIDIA GeForce RTX 4060 (adapter vendor `0x10de`, device `0x2882`), driver 610.88 / Windows 32.0.16.1088, Windows High performance power plan. Window framebuffer: 1280 × 720. Source: the default quantized Saari HGS2, SHA-256 `66257288d65d24d56e8ffcd08b4919f081d771c103fe4196b6d85a3927b8fee1`.

The same 240-frame moving camera route repeats after 60 warm-up frames. Each run measures at least 60 seconds and finishes a whole route cycle. The application requests reset flags zero (no VSync flag). Runs were sequential, without competing test renderers. The optimized window reaches the observed roughly 60 Hz presentation cadence; this is not a claim of an uncapped GPU throughput limit.

| Measurement | Before | After |
|---|---:|---:|
| Measured duration | 63.558 s | 60.119 s |
| Measured frames | 480 | 3,600 |
| CPU wall frame median / p95 | 133.571 / 133.942 ms | 16.685 / 16.956 ms |
| BGFX completed GPU frame median / p95 | 133.393 / 139.211 ms | 16.690 / 16.916 ms |
| Native submission CPU median | 0.238 ms | 0.068 ms |
| Compute dispatches per moving frame | 234 | 27 |
| Accounted model + scratch GPU bytes | 791,695,632 | 795,889,936 |

Observed median frame time improves by approximately **8×**, from about **7.5 to 60 frames/s**, on this route and device. BGFX completed-frame timestamps are asynchronous and can include gaps between GPU commands; do not equate these whole-frame numbers to summed kernel execution times.

A separate diagnostic run enabled BGFX per-view GPU timing and expanded compute views for 60 measured frames. With radix already enabled, changing only the source buffer allocation reduced median projection time from **63.746 ms to 1.795 ms**. In the optimized diagnostic run, all radix passes together took **3.517 ms**, gather **0.880 ms**, and drawing **1.309 ms**. These instrumented measurements describe a shorter route segment and have profiling overhead; they are not the 60-second benchmark above. Temporary profiling instrumentation was removed from the delivered engine.

A static-camera check reports zero compute dispatches and a 16.690 ms median wall frame time, while still drawing all splats against current scene depth. It is a separate diagnostic, not the moving-camera performance claim.

## 1. Keep immutable source data in GPU-oriented storage

`CreateComputeBufferFromData` previously uploaded to a BGFX dynamic vertex buffer with compute-read access. In the pinned D3D11 backend, creating that buffer without initial renderer-side data and without UAV access selects `D3D11_USAGE_DYNAMIC` with CPU write access. This allocation path is inappropriate for hundreds of megabytes of fixed source/SH data read every frame.

Uploaded compute inputs now use `bgfx::VertexBufferHandle` / `createVertexBuffer` with initial data. D3D11 creates immutable storage with no CPU access. GPU-written scratch still uses dynamic read/write compute buffers. `ComputeBuffer` retains its public API, element types, counts, shared ownership and explicit-destruction semantics. Binding, validity and destruction handle either internal storage type. Write and read/write bindings to uploaded inputs remain rejected before submission.

No BGFX fork, backend-specific Lua path, format change or extra upload dispatch is needed. The change also applies to other immutable compute inputs. Actual placement and performance on Metal remain backend/driver decisions.

## 2. Stable radix ordering in 24 dispatches

For more than 256 padded entries, the native renderer replaces global bitonic sorting with eight stable 4-bit radix digits. Each digit has three dependent dispatches:

1. Histogram: 256 keys per block, 128 threads, 16 initialized shared counters.
2. Prefix: one workgroup per bin computes exclusive block offsets plus the sum of preceding bins. It uses bounded, barrier-separated shared scans and covers every block up to the native two-million-entry limit.
3. Scatter: each block builds 16 × 256 membership bits in 512 bytes of shared memory. Popcounts of preceding bits give a deterministic local rank; block prefixes give the global destination. Atomics build membership only, so scheduling does not determine the tie order.

For positive float32 depths, the complemented bit representation sorts far to near without quantization. Invalid depth `-1` maps to the final unsigned key. Clear/project initialize slots in ascending global source ID; instances are assigned offsets in explicit instance-ID order. Stable LSD passes therefore preserve the exact ascending-ID tie rule across blocks and instances. All 32 depth bits participate.

Inputs of 2–256 padded keys use one shared-memory bitonic dispatch; a single key needs no sort. `sort.sc` remains an independent global-bitonic reference for tests. The native path requires `sort_local.sc`, `radix_histogram.sc`, `radix_prefix.sc` and `radix_scatter.sc` in addition to clear/project/gather/draw shaders. Canonical sources are in `core-package`; tutorial resources are synchronized copies.

A full Saari frame uses clear + project + 24 sort + gather = **27 dispatches**. Multiple instances add projection dispatches but share one global sort. Compact submission still consumes two views. The radix histogram and offset buffers add **4 MiB** at the maximum padded capacity of 2,097,152. There is no device-wide workgroup-barrier assumption, subgroup intrinsic, floating-point atomic or hidden CPU/GPU synchronization.

## 3. Cache compute work, redraw composition

Each retained `GaussianSplatRenderData` keeps its projected/sorted/gathered results. A cache hit reports `dispatches == 0` and still issues the graphics draws. The application must continue clearing/drawing its target normally; opaque depth, target selection, viewport origin and depth-test enablement are applied on every draw.

The key compares validated inputs exactly: view/projection matrices, viewport dimensions, model storage identity, instance IDs, world transforms, opacity, maximum SH degree, radius cap, renderer identity, depth convention and compact/expanded scheduling. Input lists are canonicalized by instance ID. Scratch replacement invalidates the cache; a miss invalidates it before any compute command and publishes a new entry only after successful gather submission. All input/handle/budget validation still runs on hits. Shutdown invalidates render data, preventing reuse across renderer contexts.

Compact hits retain the two-view contract; expanded hits consume only their graphics view. View-budget preflight remains conservative and reserves enough capacity for the uncached path. No Lua dirty flags are required.

Use separate render data for independently ordered camera passes. Submissions sharing one render-data object must execute in increasing view order: compute, draw, then the next submission's compute/draw. Do not apply a BGFX view remapping that breaks these shared-resource dependencies. Overlapping splat models must still be submitted together for global ordering.

## Validation and reproduction

The compact local evidence for this run is retained in `../.codex_tmp/gsplat-performance/` relative to the repository: final measurements, validation logs, review and before/after PNG captures. Temporary diagnostic scripts, baseline binaries, generated fixtures, compiled test shaders and redundant captures were removed after validation. The test runners regenerate their fixtures and shaders. These local artifacts are not versioned; the results and reproduction commands below are the repository record.

- The GPU order/gather oracle compares **every output ID** with independent CPU ordering for padded sizes 1, 2, 128, 256, 512, 1,024, 131,072 and 2,097,152, plus equal-depth and all-invisible cases. Fractional float32 depths exercise low mantissa bits. Both compact and expanded schedules, reference and optimized algorithms are checked.
- The native GPU probe compares cached results to fresh render-data pixels after camera, projection, viewport, transform, opacity, model/ID, SH, radius and renderer changes; it also covers scratch growth/shrink reuse, new targets, changed depth contents/test state, rejected inputs, global ordering and mesh depth composition. A 257-record fixture forces the native radix branch. Both float32 and quantized HGS2 fixtures are used.
- The generic Lua compute probe covers input reads, scratch writes, pixel output, immutable-write rejection, retained bindings through GC, explicit destruction and shutdown.
- Before/after tutorial captures differ by at most one 8-bit RGB level in the scene region `x >= 380` (GUI excluded), for both Houseplant and Saari SH3. Real tutorial checks cover full-count submissions, Houseplant/Saari switching, SH1/SH3, resize, captures and cleanup. Shader dependency checks cover cold compilation and unchanged second builds.
- All canonical Gaussian shaders compile to D3D11 and Metal. **New Metal runtime correctness and performance are not qualified by this Windows run.** Run the same GPU oracle, native probe and moving-camera benchmark on macOS before claiming equivalent Metal performance.

From `tutorials/`, using an updated module and compiled default assets:

```powershell
./hg_lua/harfang/assetc/assetc.exe resources
./hg_lua/lua.exe ../harfang/tests/gaussian_splat_benchmark.lua ./hg_lua ./resources_compiled ./gsplat-motion.json motion 240 60 dx11
./hg_lua/lua.exe ../harfang/tests/gaussian_splat_benchmark.lua ./hg_lua ./resources_compiled ./gsplat-static.json static 240 0 dx11
```

On macOS, supply the module directory containing `harfang.so`, the native Lua executable and `metal` as the final argument, using assets compiled for Metal. The benchmark asserts the actual renderer, keeps the full loaded count, excludes load/warm-up time and records asynchronous GPU timing only when available. Its fixed route is useful for regression comparison; it does not qualify every camera pose, resolution or overdraw case.

From the repository root:

```powershell
python harfang/tests/run_gaussian_sort_probe.py --build-dir ../build/lua-cmake-bullet --renderer dx11
python tools/assetc/tests/test_shader_dependencies.py --assetc tutorials/hg_lua/harfang/assetc/assetc.exe --toolchain tutorials/hg_lua/harfang/assetc/toolchains/host-windows-x64-target-windows-x64
cmake -DMODE=CHECK -P tools/sync_core_package.cmake
```

Rebuild and distribute the native module together with the newly compiled core shaders. HGS2 input files remain compatible and do not need reconversion. Center-depth transparency ordering, radius-cap approximations, arbitrary translucent mesh composition and potentially expensive close-up overdraw remain the existing rendering limits.
