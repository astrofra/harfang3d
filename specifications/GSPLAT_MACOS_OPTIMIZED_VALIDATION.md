# Full-count Gaussian splatting on macOS

Date: 2026-11-09. Source: `436661c` plus the input/UI and shader-header fixes in this change. Device: Apple M3 Pro, macOS 14.1/arm64, actual renderer **Metal**, Release build. This qualifies the optimized native path from the [performance report](SPECS_GAUSSIAN_SPLATTING_NATIVE_PERFORMANCE.md) on this Mac.

## Full-count assets and correctness

The package and `tutorials/resources_compiled` were rebuilt with Metal shaders and the current default Saari metadata, which omits `max-count`. The real tutorial confirms **1,307,339 / 1,307,339 Saari splats**, SH3, and all 113,648 Houseplant splats. Submissions must preserve the loaded count. The locally compiled Saari HGS2 is 44,374,341 bytes, SHA-256 `42825252f01272de205230ce91e3535b4df96c5fe45a5b5b07f71227594ad250`.

Observed results:

| Check | Result |
| --- | --- |
| Native HGS2/import/GPU suite | All 12 groups passed; float32/quantized, SH, mesh composition, radix path and cache invalidation against fresh pixels |
| Reference/optimized sort and gather | Every result matches CPU order for sizes 1 through 2,097,152 in the existing case matrix, both compact and expanded schedules, fractional depth bits, ties, invalid entries and padding |
| Generic C++/Lua compute | Positive, flip-before and expected pixel-failure negative controls passed; immutable inputs and ownership checked |
| Actual tutorial, external display | GUI and injected Space selection each passed 60 frames, full-count submissions, SH1/SH3, two resizes, GC, captures and shutdown; framebuffer 1280x720, input scale 1 |
| Actual tutorial, built-in Retina display | Passed 60 frames with injected Space switching, full counts, SH1/SH3, resizes, content captures and cleanup; client 1280x720, framebuffer 2560x1440, input scale 2 |
| Native ImGui hit testing | Button hover/press/release passed at scale 1, scale 2 and nonuniform scale (1.25, 1.5), with resized display dimensions |
| Shader dependency tests | All three passed with MTL/osx |
| Complete tutorial asset build | Zero failed after replacing legacy shader header copies with canonical-header includes |

Saari and Houseplant captures were inspected. The full Saari image retains the distant sky coverage absent from the previous 250,000-splat subset. These captures are not a pixel-equivalence comparison against Windows; independent synthetic GPU tests supply the correctness oracle.

## Performance

The existing `gaussian_splat_benchmark.lua` ran sequentially, without another test renderer. The motion route uses 60 warm-up frames, at least 60 measured seconds and whole 240-frame route cycles. The benchmark requests no VSync flag; observed frame cadence is not an uncapped kernel-throughput claim.

| Measurement | Moving camera | Static camera diagnostic |
| --- | ---: | ---: |
| Framebuffer | 1280x720 | 1280x720 |
| Retained splats / SH | 1,307,339 / SH3 | 1,307,339 / SH3 |
| Measured frames / seconds | 3,600 / 63.165 | 240 / 1.492 |
| Average frame rate | 57.0 fps | 160.9 fps |
| Wall frame median / p95 | 16.720 / 30.478 ms | 6.499 / 11.317 ms |
| Native submission CPU median | 0.028 ms | 0.041 ms |
| Completed GPU frame median / p95 | 31.550 / 35.760 ms | 7.082 / 9.433 ms |
| Compute dispatches, median | 27 | 0 |
| Accounted model + scratch GPU bytes | 795,889,936 | 795,889,936 |

GPU timestamps are asynchronous whole-frame measurements, including scheduling/queue effects; they are not the sum of individual kernel times or the reciprocal of wall-frame throughput. The static run is a short cache diagnostic. No comparable pre-optimization full-count Mac baseline was measured, so these numbers establish current behavior rather than a Mac speedup factor. The external-display resolution matches the Windows report; GPU, driver and display conditions differ. Retina correctness captures use 2560x1440, but the table above is **1280x720**.

## Retina selection and keyboard switching

GLFW mouse positions are client coordinates, whereas the tutorial supplies framebuffer dimensions to ImGui. At 2x density this misaligns both X and the bottom-left to top-left Y conversion. New `ImGuiBeginFrame` overloads accept an explicit `mouse_scale`; the existing overloads retain their 1:1 behavior and native signatures. The tutorial passes the current framebuffer/client ratio every frame. The camera continues using client coordinates. The native unit test checks actual ImGui button hits, not just the conversion formula.

**Space switches the preloaded model once per key press**, including when the cursor is over the GUI. The shortcut requires window focus, preserves each model's camera/SH/render data and suppresses the first camera drag after switching. The tutorial displays the shortcut. The test runner injects two press edges and held intervals while allowing the real combo to run, independently of its GUI-forced selection mode.

The complete asset rebuild also exposed two old `bgfx_shader.sh` copies under tutorial shaders, without Metal sampler definitions. They now include `core/shader/bgfx_shader.sh`; bloom/depth/sprite/texture programs compile against the same canonical backend definitions.

## Reproduction and evidence

From the repository root with the native Lua build configured:

```sh
cmake --build ../build/lua-cmake --target gsplat_tutorial_smoke tests test_compute_probe --parallel 8
../build/lua-cmake/harfang/tests/tests engine.dear_imgui_mouse_scale
python3 harfang/tests/run_gaussian_sort_probe.py --build-dir ../build/lua-cmake --renderer metal
python3 harfang/tests/run_compute_probe.py --build-dir ../build/lua-cmake --renderer metal --lua --skip-build
../build/lua-cmake/extern/lua/lua harfang/tests/gaussian_splat_benchmark.lua \
  ../build/lua-cmake/languages/hg_lua tutorials/resources_compiled \
  ../build/gsplat-motion-metal.json motion 240 60 metal
```

The package target compiles its own application assets. To compile all tutorials, use its installed `assetc` from `tutorials/` with `-api MTL -platform osx resources resources_compiled`.

Detailed local evidence is under `../build/gsplat-macos-optimized/`: `native/summary.json`, `sort.log`, `compute.log`, `retina-unit.log`, `tutorial-smoke-final.log`, `retina-tutorial.log`, `benchmark-motion.json`, `benchmark-static.json`, `incremental.log`, asset logs and `qualification.json`. The local `retina_runner.lua` moves the real tutorial window onto the built-in Retina display before running the standard tutorial runner. Generated logs, captures and binaries are not committed. Other Mac GPUs, Windows execution of the new UI changes, and interactive movement between displays remain outside this run's qualification.
