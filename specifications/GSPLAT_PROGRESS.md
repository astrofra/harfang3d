# Gaussian splatting implementation progress

Branch: `gsplat`. Local commits only, authored as `Astrofra`. Updated 2026-10-27.

## Delivery order

The user confirmed macOS-first implementation: advance as far as possible on Metal, then qualify DirectX on Windows when a machine is available. Windows evidence is deferred, not fabricated and not a blocker for continued macOS work. HARFANG's backends in scope are DirectX, Metal and OpenGL; OpenGL compute qualification comes later.

| Slice | Status | Evidence / next step |
|---|---|---|
| M0 Metal native dependency chain | Validated on M3 Pro/macOS 14.1 | [Probe and exact commands](../harfang/tests/COMPUTE_PROBE.md); positive and negative pixel checks plus normal shutdown. |
| M0 DirectX | Deferred: no Windows runner available | Run the same probe with `--renderer dx11` on Windows. |
| M1 C++/FabGen/Lua compute bridge | Next | Image/buffer bindings, capability reporting, lifetime and Lua smoke coverage. |
| M2 reproducible content | Pending | Bundle verified CC BY sample, validate/convert SH0 data and presets. |
| M3–M5 renderer and Lua viewer | Pending | Projection, GPU ordering, persistent draws, FPS camera. |
| M6 performance/portability | Pending | Measure Metal; Windows validation in the second phase. |

## Duo review: GSPLAT-M0-AUDIT

Contract: bounded static backend audit at `48699a301862f893647a231435a59adeba5721ff`, isolated worktree, one allowed Markdown output, no shell/git/web/MCP. Copilot CLI 1.0.95 ran `deepseek-v4.1-flash:cloud`; runner exit 0, no reported contract violations. Delivery was **À vérifier**, then reviewed by Codex.

Accepted after checking source: explicit D3D11 typed-buffer flags and stride; separate scratch/readback resources; Metal image-capability reporting omission; checks for stale producer/consumer results. Corrected: a lack of explicit Metal barriers does not by itself prove missing synchronization for serial encoders and tracked, directly bound resources. The native experiment and Apple documentation support the exercised path, without generalizing to untested workloads. Raw logs and the unedited report remain outside the repository.
