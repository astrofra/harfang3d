# Gaussian splatting implementation progress

Branch: `gsplat`. Local commits only, authored as `Astrofra`. Updated 2026-10-27.

## Delivery order

The user confirmed macOS-first implementation: advance as far as possible on Metal, then qualify DirectX on Windows when a machine is available. Windows evidence is deferred, not fabricated and not a blocker for continued macOS work. HARFANG's backends in scope are DirectX, Metal and OpenGL; OpenGL compute qualification comes later.

| Slice | Status | Evidence / next step |
|---|---|---|
| M0 Metal native dependency chain | Validated on M3 Pro/macOS 14.1 | [Probe and exact commands](../harfang/tests/COMPUTE_PROBE.md); positive and negative pixel checks plus normal shutdown. |
| M0 DirectX | Deferred: no Windows runner available | Run the same probe with `--renderer dx11` on Windows. |
| M1 C++/FabGen/Lua compute bridge | Validated on Metal | Generated Lua module: four pixel seeds, premultiplied blending, rejected invalid dispatches, binding-owned buffers and shutdown invalidation. |
| M2 reproducible content | Validated | Bundled Houseplant (3,636,736 bytes, CC BY 4.0); pinned SHA-256, 19 converter tests, deterministic full/60k/30k presets. |
| M3–M5 renderer and Lua viewer | Validated on Metal | Anisotropic projection, all 131,072 sorted IDs checked against CPU ordering, pixel oracle, persistent batches, quaternion trackball and RGB DrawLines gizmo, resize and normal shutdown. |
| M6 performance/portability | Metal reference workload validated; Windows deferred | Full preset: 171 seconds at 1280×720; frame p95 10.26 ms, GPU p95 5.68 ms. Windows remains an unfulfilled cross-platform gate. |

## Interaction and implementation decisions

The user replaced the FPS requirement with a trackball-only camera. The implementation has left-drag quaternion rotation, wheel dolly, right/middle-drag pan, reset and Escape. There is no FPS mode. A world XYZ orientation gizmo uses RGB `DrawLines`, drawn over the splats. [Guide and launch commands](../tutorials/GAUSSIAN_SPLATTING.md).

The Metal prototype keeps GPU bitonic sorting (153 compare passes for full quality), exceeding the original radix scheduling target. Native/Lua dependencies, full-sized ordering and end-to-end color checks are validated. Radix is deferred while this measured small-data path meets the reference frame budget. The renderer does not claim exact volumetric ordering, SH beyond degree zero, or per-pass GPU timings.

## Duo review: GSPLAT-M0-AUDIT

Contract: bounded static backend audit at `48699a301862f893647a231435a59adeba5721ff`, isolated worktree, one allowed Markdown output, no shell/git/web/MCP. Copilot CLI 1.0.95 ran `deepseek-v4.1-flash:cloud`; runner exit 0, no reported contract violations. Delivery was **À vérifier**, then reviewed by Codex.

Accepted after checking source: explicit D3D11 typed-buffer flags and stride; separate scratch/readback resources; Metal image-capability reporting omission; checks for stale producer/consumer results. Corrected: a lack of explicit Metal barriers does not by itself prove missing synchronization for serial encoders and tracked, directly bound resources. The native experiment and Apple documentation support the exercised path, without generalizing to untested workloads. Raw logs and the unedited report remain outside the repository.

## Duo review: GSPLAT-CONTENT-001

DeepSeek (`deepseek-v4.1-flash:cloud`, Copilot CLI 1.0.95) delivered the converter and its unittest suite in an isolated worktree. The first attempt violated the output-path contract and was discarded. The bounded retry created exactly the two allowed files, returned exit 0 with no reported violations, and was delivered **À vérifier**. Codex reviewed the math, binary format and tests, corrected the manifest checksum to cover each complete file including its header, and ran all 19 tests successfully. Known quaternion/color/scale examples, malformed input rejection before writes, spatial subset selection and deterministic output are covered. Raw duo logs remain outside the repository.

## Metal reference results — 2026-10-27

Renderer/tutorial source: `32e1e37`. Release build, Apple M3 Pro (14 GPU cores), macOS 14.1, Metal, AC power, low-power mode disabled. The Metal driver is supplied by macOS; no separate driver version was collected. The native capability probe reports device/vendor `106b:03ef`. VSync is disabled. The default full preset has 113,648 SH0 Gaussians; drawable resolution is 1280×720 (640×360 client size on the tested Retina display).

The repeatable route includes a full orbit, closer views, changing elevation, and a turn toward empty background. First 60 frames are excluded. GPU measurements come from completed asynchronous bgfx timestamps; they are whole-frame times, not CPU submission estimates. No per-pass GPU breakdown is claimed. Camera/gizmo rendering is included; screenshot waiting is outside the measured samples.

| Preset | Sampled frames / duration | Wall frame median / p95 | GPU median / p95 | CPU submit median / p95 | Persistent GPU payload |
|---|---|---|---|---|---|
| Full (113,648) | 19,940 / 171.215 s | 6.802 / 10.264 ms | 3.833 / 5.679 ms | 0.386 / 1.184 ms | 28.64 MiB |
| 60,000 | 2,340 / 6.969 s | 2.332 / 6.638 ms | 1.968 / 2.748 ms | 0.323 / 0.855 ms | 14.84 MiB |
| 30,000 | 2,340 / 5.303 s | 1.535 / 6.237 ms | 1.030 / 1.800 ms | 0.352 / 0.865 ms | 7.42 MiB |

Only full quality received the sustained >60-second qualification; the smaller presets received shorter comparison runs. These are observations on one machine, not a 60 FPS guarantee for every Mac, resolution, view or thermal condition. The full preset uses 155 compute dispatches and eight draws including the gizmo. Memory counts explicit persistent buffers/atlases/quad geometry and excludes driver overhead, backbuffers, transient gizmo vertices and captures.

Commands and a screenshot are in [the tutorial guide](../tutorials/GAUSSIAN_SPLATTING.md). A compact [reference result artifact](GSPLAT_METAL_REFERENCE_RESULTS.json) preserves the reported numbers. Raw logs and PNGs remain under the local build tree; reruns may replace those files.

### Completed checks

- Native sentinel plus Lua pixel tests, including missing-producer/consumer negative controls, invalid submissions, shared buffer lifetimes and normal renderer shutdown.
- 19 deterministic offline-converter tests; source hash/count verified before preparing the actual Houseplant presets.
- Every one of 131,072 GPU-sorted IDs matches an independent CPU total order, using 113,648 input records, repeated depths, culled entries and padding.
- End-to-end GPU pixel oracle checks unequal/equal-depth overlapping colors, behind-camera/offscreen/zero-opacity culling and premultiplied compositing. An identity-sort negative control fails with the expected pixel mismatch.
- Quaternion trackball math: known rotation, inverse drag, focus gate, zoom limits, pan, reset and 10,000-drag orthonormality check. No FPS implementation remains.
- Two scripted client-size changes produce three drawable resets and return to 1280×720; rendering, capture and shutdown succeed. The default view and RGB line gizmo were visually inspected in the saved capture.
- Repeated bounded viewer runs with full/60k/30k datasets shut down normally. No manual force-quit is needed.

Physical mouse/trackpad feel and window-manager minimize/focus gestures still merit a human acceptance pass. Those manual interactions are not claimed to have been automated. The numerical focus gate and resize path are tested.

### Next phase: Windows

On a Windows machine, build this branch and run native/Lua compute, full-sort and pixel oracles with `--renderer dx11`, then validate the complete trackball viewer and measure the same route. Check D3D11 shader compilation, typed UAV/SRV buffers, RGBA32F image/vertex sampling, batch ordering and shutdown first. Do not mark the original two-backend M6 gate complete until that evidence exists. OpenGL qualification is separate and later; no additional backend is in scope.

## Tutorial launch simplification

The tutorial now follows the other Lua examples: launch `lua gaussian_splatting.lua` from `tutorials`, with no arguments. It loads Houseplant/full from the backend-specific `resources_compiled/gaussian_splatting` folder and locates HARFANG beside the installed Lua executable. The asset preparation helper populates that ignored folder from `assetc` output. Renderer internals are shared in `gaussian_splatting/renderer.lua`; command-line routes, captures and timing reports have moved to `harfang/tests/gaussian_splatting_runner.lua`.

Validated with the user's installed `install/lua/hg_lua/lua`: the actual no-argument tutorial loads all 113,648 records, renders 120 frames and shuts down normally under the test-only termination hook. The shared-renderer pixel oracle and two-resize/capture check also pass. No generated assets are committed.
