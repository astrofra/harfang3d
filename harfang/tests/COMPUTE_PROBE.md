# Native compute feasibility probe

This opt-in graphical test is the first Gaussian-splatting roadmap gate. It is separate from the ordinary headless unit tests. A small, static window opens and closes automatically; there is no interactive scene yet.

The test uploads 256 immutable `float4` records, dispatches four producer workgroups into a GPU-written buffer, dispatches four consumer workgroups with permuted reads into an RGBA32F texture, and samples that texture **in the vertex shader** of persistent quads. It captures the RGBA8 target and compares every cell against a CPU oracle. Four different seeds expose stale results. No `Frame()` separates the dependent GPU passes.

The runner also tests flip-before-render and two negative controls. Omitting the producer or consumer after a valid first frame must cause a pixel mismatch at the next seed. Those expected failures are required for the overall test to pass. Success also requires normal process exit, including renderer shutdown.

## Run

Use an existing configured HARFANG CMake build with `HG_BUILD_TESTS=ON` and `HG_BUILD_ASSETC=ON`. The runner builds `test_compute_probe`, `assetc` and `shaderc`, stages the pinned shader includes, compiles assets for the selected backend and runs the checks.

From the repository root on macOS:

```sh
python3 harfang/tests/run_compute_probe.py --build-dir ../build/lua-cmake --renderer metal
```

On Windows, after configuring the normal HARFANG dependencies and build:

```powershell
python harfang/tests/run_compute_probe.py --build-dir C:/path/to/harfang-build --renderer dx11 --config Release
```

`--skip-build` reuses existing executables. `--compile-only` compiles shaders without claiming runtime qualification. OpenGL is selectable with `--renderer gl` for later investigation, but is not currently qualified. Renderer fallback is rejected.

The latest run's evidence is under `<build-dir>/compute_probe/<renderer>/`: `report.json`, build/assetc logs, positive and negative-control logs, staged sources and compiled assets. Renderer outputs are separate. The report includes host, commands, exit codes, source hashes and executable hash. A failed process, setup error or timeout cannot count as an expected negative control. Runtime processes have a 60-second watchdog; no manual force quit should be needed.

## Observed on 2026-10-27

- Apple M3 Pro, macOS 14.1, Metal; reported vendor/device `106b:03ef`.
- All 256 records matched at seeds 0, 1, 3 and 7, both with HARFANG's default flip-after-render and with flip-before-render.
- Both negative controls produced the required pixel mismatch and exited normally.
- Capability limits: 16 compute bindings, 256 views, maximum texture dimension 16384.
- Windows/DirectX and OpenGL: **not run**. A Metal pass does not validate D3D11 buffer flags or driver behavior.

Two issues were exposed:

1. Pinned Metal never sets the per-format `IMAGE_READ/WRITE` capability bits. The probe reports this gap and permits an empirical RGBA32F write test **only on Metal**; it does not alter capability reporting or assert support on every Metal GPU. Vertex sampling, texture creation and pixel checks remain mandatory. The future public capability API must distinguish advertised from tested support.
2. HARFANG enables flip-after-render after bgfx initialization. The previous frame could remain pending, and Metal consumed another frame slot without a matching completion. Pixel checks passed, but shutdown waited forever for the missing slot. `Context::renderFrame` now flushes a pending previous frame even when the flip mode has changed. The positive tests exercise both modes and normal teardown; the baseline hang was captured in a local process sample before the fix.

No explicit Metal barrier was added: the pinned backend uses a serial compute encoder and directly bound device resources. Absence of explicit barriers alone does not establish a hazard. Apple's documentation describes automatic tracking for these resources and serial dispatch by default. [Metal resource synchronization](https://developer.apple.com/documentation/metal/resource-synchronization).

This probe qualifies the exercised dependency chain on the recorded device. It does not validate full-size radix sorting, all formats, performance targets, or every Metal device. Readback is deliberately confined to this diagnostic; the pinned Metal implementation can wait internally when reading a texture.

## Lua bridge (M1)

Add `--lua` to the command above to build the FabGen module and exercise the same dependency chain through Lua. The test checks all 256 pixels for four seeds with premultiplied blending, rejected invalid dispatches without advancing the view, buffer ownership across Lua garbage collection, explicit destruction and surviving userdata after renderer shutdown. It exits automatically under the runner's watchdog.

`DispatchCompute(view, program, x, y, z, values, textures, images, buffers[, name])` returns **`ok, next_view`** in Lua. Each successful dispatch consumes one view; failures leave it unchanged. Buffer stages and image/sampler stages must be unique within a dispatch. Input data consists of 16-byte float4/uint4 records, copied in bulk; `offset` skips a file header. Uploaded buffers are read-only; uninitialized scratch buffers permit GPU writes. Buffer copies and bindings share ownership, and explicit destruction invalidates all copies. Images borrow existing textures, which must outlive their submissions.

`GetComputeCaps()` reports actual backend flags. `image_write_reporting_complete` is false on this vendored Metal backend: a missing RGBA32F image-write flag does not negate the separately tested path. Do not silently treat that exception as portable support.

`CreateComputeQuadModel(count)` creates persistent ordered quads in batches of at most 16,384 quads with 16-bit indices. POSITION holds `(corner_x, corner_y, sorted_slot)`. Store the returned model in `PipelineResources` and destroy it through that owner. `ComputePremultipliedRenderState()` enables ONE/INV_SRC_ALPHA blending without depth writes or testing.

Validated on 2026-10-27: native positive/flip-before tests pass, both missing-dispatch controls fail with the expected pixel mismatch, and the Lua test passes and shuts down normally (Apple M3 Pro, macOS 14.1, Metal). DirectX remains untested without a Windows host.
