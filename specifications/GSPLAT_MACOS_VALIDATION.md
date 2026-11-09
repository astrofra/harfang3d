# Gaussian splatting: native Mac validation

Date: 2026-11-09. Platform: macOS/arm64, Apple M3 Pro, actual renderer **Metal**. Source: `5cb3ca7` plus this working-tree change. The existing `tutorials/gaussian_splatting.lua` is unchanged. The obsolete IDE tab `tutorials/gaussian_splatting/renderer.lua` refers to the former Lua implementation, removed before this work.

## Result and changes

Houseplant (113,648 splats, SH0) and Saari (250,000 of 1,307,339 splats, SH3) render through the existing shared native implementation. No Metal-specific Gaussian algorithm or Lua path was needed. Saari was retrieved by fast-forward pull at the user's request; its source SHA-256 matches the existing guide.

Two integration defects were reproduced and fixed:

- **Native shutdown with pending uploads.** Loading a model after the last frame, then shutting down, queued a Metal buffer transfer in the same command stream as BGFX's shutdown-begin. Submission was disabled before the transfer encoder was ended; autorelease-pool cleanup aborted in `MTLCommandEncoder dealloc`. `RenderShutdown()` now drains the queued frame before releasing native resources and starting BGFX shutdown. The existing surviving-model/lifetime GPU test reproduced the failure and passes after the fix. No extra Lua frame is required.
- **Installed asset tool dependencies.** Installed `gsplatc` linked `@rpath/liblua54.dylib`, but the toolchain directory lacked that dependency. Install the existing shared dependency set alongside the toolchain executables. Conversion now works from the installed toolchain.

`gsplat_tutorial_package` stages the canonical core Gaussian shaders, shared shader headers, ImGui programs, tutorial gizmo and both input models. It uses the platform tool installation, validates every required output and writes an input/output/runtime hash manifest. Native runtime installation gives the distribution relative library search paths. `gsplat_tutorial_run` and `gsplat_tutorial_smoke` use this distribution. Tool identity changes invalidate the package cache; ordinary unchanged builds do not compile shaders.

The tutorial runner now checks the actual renderer and image content outside the UI. The standalone GPU sorter now checks every sorted ID **and the reference gather output**, independently against CPU expectations, in expanded and compact scheduling. The native Gaussian probe also rejects renderer fallback.

## Observed checks

| Check | Observed result |
| --- | --- |
| Release build: Lua/module, assetc/gsplatc, compute probe | Passed |
| Canonical core vs tutorial mirror | 238 files, zero differences |
| Compute C++ and Lua, image writes and vertex fetch | Passed on actual Metal |
| Flip-before mode and skip-producer/skip-consumer negative controls | Passed; controls fail with the expected pixel mismatch |
| 131,072-slot sort, equal depths, invalid entries, padding | Every ID matches CPU order in both scheduling modes |
| Reference gather after the large sort | Every projected-ID pixel and invalid alpha matches CPU expectations in both modes |
| Native HGS2 suite | 11 groups passed; float32 and quantized GPU fixtures, malformed inputs, SH1/SH3, multi-instance ordering, mesh occlusion, offscreen targets and ownership |
| Unchanged actual tutorial, both models | 60 frames; two preloads, two GUI switches, SH1/SH3, two resizes, repeated GC, complete resource cleanup |
| Tutorial from installed package | Passed, including independent image-content checks on both captures |
| Relocated application, empty process environment | Passed: installed Lua/module/dylibs, both models and captures; no build-tree library or module environment |
| Framebuffer/client scaling | Captures 2560×1440 for the 1280×720 client; no fixed scale factor added |
| Shader dependency regression | All three tests passed with MTL/osx: content-hash caching, timestamp caching, guarded cycles/path aliases |
| Warm application package build | Zero assets processed, zero failed, no shader compiler invocation |

Houseplant and Saari captures were visually inspected. Houseplant is framed correctly; ImGui and the orientation gizmo are visible. Saari displays the island with the existing sparse distant sky caused by the documented 250,000-splat midpoint budget. Content checks are an empty-image guard, not a cross-backend image equivalence claim. Synthetic native pixel checks provide the deterministic rendering oracle.

## Reproduce

From a configured native Lua/assetc build, at the repository root:

```sh
cmake --build ../build/lua-cmake --config Release --target gsplat_tutorial_package --parallel 8
../build/lua-cmake/gaussian_splatting/Release/app/run.command
cmake --build ../build/lua-cmake --config Release --target gsplat_tutorial_smoke
python3 harfang/tests/run_compute_probe.py --build-dir ../build/lua-cmake --renderer metal --lua --skip-build
python3 harfang/tests/run_gaussian_sort_probe.py --build-dir ../build/lua-cmake --renderer metal
```

Enable `HG_BUILD_TESTS=ON` for the native probes. Python is for developer regression tests only. Build and application launch require a usable macOS graphics session. Runtime package: `<build>/gaussian_splatting/Release/app/`; captures: `<build>/gaussian_splatting/Release/tutorial*.png`; asset log and tool installation: the same package root. Local detailed qualification evidence is under `../build/gsplat-macos-duo/`, with native results in `native/summary.json`, compute results in `<build>/compute_probe/metal/report.json` and sort results in `<build>/gaussian_sort_probe/metal/`. Generated logs and binaries are not committed.

`../build/gsplat-macos-duo/qualification.json` collects check results, OS/GPU, requested/actual renderer, implementation hashes, the package manifest, capture hashes and qualification limits. `REVIEW.md` records Codex's review of the Duo delivery.

## Duo review and decisions

Copilot CLI **1.0.105**, `deepseek-v4.1-flash:cloud` through local Ollama, executed `GSPLAT-MACOS-AUDIT` at `303e080` in an isolated worktree. The runner returned exit 0 with one authorized Markdown write and no reported violations. Raw task, review, events and summary remain in `../build/gsplat-macos-duo/`; delivery was **À vérifier**, then reviewed by Codex. Codex owns implementation, builds, GPU execution and integration; the Copilot review is static evidence only.

Accepted findings: missing-source diagnostics/package completeness, absent actual-renderer checks, the large compact-sort/gather coverage gap, screenshots without content checks, installed tool/cache identity concerns, and incomplete Metal capability reporting. Saari's absence was resolved by the user's subsequent commit; the package rejects missing model inputs.

The suggestion to set a sequential mode on the compute view was rejected as a false positive: `EncoderImpl::dispatch` always assigns `getSeqIncr(view)` and `SortKey::encodeCompute` places the sequence ahead of the program bits. Draw `ViewMode` does not control this compute order in the pinned BGFX. The new large compact test also validates the required behavior. No speculative shader or scheduling rewrite was applied.

## Remaining qualification scope

This closes the immediate Mac tutorial execution goal on the tested Apple M3 Pro. It does not close every roadmap milestone. Intel/AMD Macs, other Apple GPU families, new Windows execution of these changes, MSAA variants, minimize/restore and interactive DPI/input behavior across screens remain unqualified. Steady-state performance/memory budgets are not established by the 60-frame smoke test.

Metal's RGBA32F image-write capability bit remains incompletely reported in the pinned backend; successful runtime image/write/vertex-fetch probes qualify this device, not every Metal device. Automatic renderer/build-policy alignment, environment/launcher overrides, generic assetc target/tool cache identities and multi-backend resolution remain future work. The new package's explicit target, binary identity guard and required-output checks apply to this tutorial distribution.
