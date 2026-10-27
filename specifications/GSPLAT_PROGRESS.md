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
| M3–M5 renderer and Lua viewer | Pending | Projection, GPU ordering, persistent draws, FPS camera. |
| M6 performance/portability | Pending | Measure Metal; Windows validation in the second phase. |

## Duo review: GSPLAT-M0-AUDIT

Contract: bounded static backend audit at `48699a301862f893647a231435a59adeba5721ff`, isolated worktree, one allowed Markdown output, no shell/git/web/MCP. Copilot CLI 1.0.95 ran `deepseek-v4.1-flash:cloud`; runner exit 0, no reported contract violations. Delivery was **À vérifier**, then reviewed by Codex.

Accepted after checking source: explicit D3D11 typed-buffer flags and stride; separate scratch/readback resources; Metal image-capability reporting omission; checks for stale producer/consumer results. Corrected: a lack of explicit Metal barriers does not by itself prove missing synchronization for serial encoders and tracked, directly bound resources. The native experiment and Apple documentation support the exercised path, without generalizing to untested workloads. Raw logs and the unedited report remain outside the repository.

## Duo review: GSPLAT-CONTENT-001

DeepSeek (`deepseek-v4.1-flash:cloud`, Copilot CLI 1.0.95) delivered the converter and its unittest suite in an isolated worktree. The first attempt violated the output-path contract and was discarded. The bounded retry created exactly the two allowed files, returned exit 0 with no reported violations, and was delivered **À vérifier**. Codex reviewed the math, binary format and tests, corrected the manifest checksum to cover each complete file including its header, and ran all 19 tests successfully. Known quaternion/color/scale examples, malformed input rejection before writes, spatial subset selection and deterministic output are covered. Raw duo logs remain outside the repository.
