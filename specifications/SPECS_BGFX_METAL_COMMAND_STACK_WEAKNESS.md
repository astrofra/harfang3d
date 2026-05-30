---
name: bgfx-metal-command-stack-weakness
description: "harfang3d's pinned bgfx (Feb 2022) Metal backend originally used an 8MB-per-frame uniform-buffer ring with no overflow guard; heavy multi-draw loads corrupted geometry until Harfang's local 32MB mitigation"
metadata: 
  node_type: memory
  type: project
  originSessionId: 41a29389-b874-4633-9e86-fbc7a02fbdb0
---

In `harfang_build/harfang3d`, Harfang vendors a BGFX stack pinned to
`bgfx` `3d4bd88` (2022-02-12), `bimg` `663f724`, and `bx` `ad018d4`
under `extern/bgfx/`, still ~1130 commits behind upstream on the bgfx side.
Before 2026-05-30 these sources lived as nested submodules; they are now
tracked directly in the Harfang repository so local backend fixes survive
normal checkout/update flows. Its Metal backend
(`extern/bgfx/bgfx/src/renderer_mtl.mm`) has one **real** weakness under
heavy load — and it's **not** what I first suspected.

**REVISED ROOT CAUSE (2026-05-19, after source review on `physics_pool_of_objects` repro):**
The dominant bug is **uniform-buffer ring overflow with no bounds check**.

- In the original upstream pin, `UNIFORM_BUFFER_SIZE` was hard-coded to
  **8 MB** (renderer_mtl.mm:19). Harfang's current vendored fork carries a
  local bump to **32 MB** at the same site. The
  `m_uniformBuffers[BGFX_CONFIG_MAX_FRAME_LATENCY]` ring still rotates one
  buffer per frame — triple-buffered, no CPU/GPU race.
- BUT inside a single frame, `m_uniformBufferVertexOffset` /
  `m_uniformBufferFragmentOffset` grow linearly per draw
  (renderer_mtl.mm:4641-4642) and `setShaderUniform`
  (renderer_mtl.mm:1495-1503) does `dst[offset + _loc] = ...` with **no
  overflow check**. When draws-per-frame × per-draw-uniform-bytes exceeds the
  ring size (8 MB upstream, 32 MB in Harfang's current fork), later writes
  either spill past the MTLBuffer (UB) or alias the slots of previous draws on
  the same frame, corrupting their world matrices.
- Visible signature: degenerate triangles (one vertex at NaN/inf) →
  sharp black wedges + 1-pixel-wide bright vertical/horizontal "needle"
  triangles, scaling with object count. NOT a stall or black frame.

**What is in fact fine** (had to disprove these by reading the source):

- Command-queue cap: `m_framesSemaphore.post(BGFX_CONFIG_MAX_FRAME_LATENCY)`
  + `m_cmd.finish(false)` at submit start (renderer_mtl.mm:3520, 3803) properly
  paces frames. The "command-buffer pool saturation" hypothesis of the previous
  version of this memory was wrong.
- Transient vertex/index buffers: updated via `BlitCommandEncoder` on the
  same `MTLCommandQueue` (renderer_mtl.mm:3876-3888 → 2716-2741); Metal
  serialises queue order, so no cross-frame aliasing of m_ptr.

**Mitigations / current fork status:** For heavy-draw-count macOS/Metal scenes
on an unpatched fork of this BGFX era, expect uniform-ring overflow long before
command-queue or transient-buffer issues. Harfang's current vendored fork
pushes the threshold higher with a 32 MB ring, but the bug class still exists
until the backend grows or fences the ring properly. Mitigations, easiest
first:

1. **Already applied locally:** bump `UNIFORM_BUFFER_SIZE` from 8 MB → 32 MB
   in renderer_mtl.mm:19. One-line patch, rebuild bgfx + harfang.so, +72 MB
   unified-memory cost across the 3-frame ring. Pushes the threshold well past
   any sensible scene.
2. **Add `BX_ASSERT(offset + _loc + _numRegs*16 <= UNIFORM_BUFFER_SIZE, …)`**
   in `setShaderUniform` to trace future hits.
3. **Proper fix:** cherry-pick the post-2023 upstream rewrite of the Metal
   uniform ring (it grows + rolls over with a fence). Non-trivial — keep for a
   Phase-2 industrialisation pass.

**Vendorization strategy (applied 2026-05-30):**

1. Keep the existing `extern/bgfx/{bgfx,bimg,bx}` layout and Harfang CMake
   glue unchanged so the engine, binding, and asset pipeline do not need path
   churn.
2. Replace the three nested gitlinks with regular tracked directories and drop
   the inner `.git` metadata, turning Harfang-specific renderer fixes into
   ordinary repository history.
3. Record upstream provenance and local deltas in `extern/bgfx/VENDORED.md`
   instead of relying on submodule SHA pins.
4. Keep BGFX upgrades separate from Harfang patches. Harfang still uses other
   submodules, so `git clone --recursive` remains the correct bootstrap path.

**EMPIRICAL HISTORY:**
- 2026-05-19, Milestone 1: `interactive-book` cavern raymarch ran 3000 frames
  @~115 fps clean. Single-pass shader-heavy load, low draws/frame → never
  approached the 8 MB limit. False negative for the real bug.
- 2026-05-19, follow-up: `harfang3d/tutorials/physics_pool_of_objects.lua`
  spawning ~hundreds of dynamic physics cubes/spheres reproduced the
  geometry-corruption pattern reliably; symptom matches the
  uniform-ring-overflow analysis above. See
  [[interactive-book-macos-arm-feasibility]].
- 2026-05-21, **PATCH APPLIED + VERIFIED**: bumped `UNIFORM_BUFFER_SIZE`
  from 8 MB → 32 MB in `extern/bgfx/bgfx/src/renderer_mtl.mm:19`. As of
  2026-05-30 the BGFX stack is vendored directly in Harfang, so this fix now
  lives in the main repository instead of as a dirty nested submodule delta.
  Rebuilt via `ninja harfang.so` in
  `build/lua-cmake/` (Release/arm64, Metal); only deprecation warnings, no
  build breakage. Deployed `harfang.so` to
  `interactive-book/app/bin/hg_lua-macos-arm64/`. Re-ran the physics_pool
  stress (auto-spawn variant `physics_pool_stress.lua` next to the
  tutorial): **1500 objects clean, 2000 objects clean** (vs. ~300 before).
  3000 objects produces a black window — but that's CPU saturation in
  Bullet3 broadphase (98-101 % CPU, frame rate collapses to ~10 fps), a
  separate problem orthogonal to the Metal patch.

  Confirmed upstream bgfx (`origin/master` Oct 2025, 1169 commits ahead of
  our pin) still has the same 8 MB hard limit and the same unguarded
  `setShaderUniform` in `src/renderer_mtl.cpp`. A bgfx bump would NOT fix
  the bug class — the local patch is the right call. The renamed file
  (`.mm` → `.cpp` post-metal-cpp switch) plus bx/bimg drift confirms a
  bump is still a multi-week engine project, not a one-line lift.
