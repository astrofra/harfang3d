# Real-Time Gaussian Splatting in Lua — Tutorial Roadmap

Date: 2026-10-27. Audited HARFANG revision: `7f00a71632d16320585531f8f9fa1aa3e2c729d6`.

Status: the source audit below describes the pre-implementation revision. The macOS/Metal implementation now exists; current validation, decisions and measurements are recorded in [GSPLAT_PROGRESS.md](GSPLAT_PROGRESS.md). Windows qualification remains deferred.

Implementation updates and measured results are tracked in [GSPLAT_PROGRESS.md](GSPLAT_PROGRESS.md). The user subsequently confirmed macOS-first implementation, with Windows qualification in a second phase.

## 1. Deliverable and architecture decision

Deliver `tutorials/gaussian_splatting.lua`: a small, documented Gaussian splat viewer with a mouse-controlled quaternion trackball camera and an RGB XYZ orientation gizmo drawn with `DrawLines`, a bundled Creative Commons sample, and an essential GPU compute path. Lua owns initialization, camera controls, pass scheduling, settings, and shutdown. C++ supplies missing rendering primitives and bulk data operations through FabGen. Shaders perform the per-splat work. Other language tutorials come after the Lua version passes acceptance.

**Use a hybrid renderer for v1:** compute shaders project/cull splats, sort them by depth, and prepare sorted attributes; conventional vertex/fragment shaders draw Gaussian ellipses as ordered quads. This explicitly means compute-assisted Gaussian splatting, not rasterization entirely in compute. It satisfies the requested use of compute while keeping the first tutorial tractable. Section 10 describes a separate, optional fully compute rasterizer.

The first version displays one static object with view-independent color, anisotropic covariance, and ordered alpha blending. It does not require training, CUDA, a neural inference library, an AAA scene, or a separate graphics API context. Use the bgfx version already vendored by HARFANG.

## 2. Audit: what this checkout actually supports

| Area | Evidence in this revision | Consequence |
|---|---|---|
| Compute program loading | [`render_pipeline.h`](../harfang/engine/render_pipeline.h), [`render_pipeline.cpp`](../harfang/engine/render_pipeline.cpp): `LoadComputeProgram`, `LoadComputeProgramFromFile`, `LoadComputeProgramFromAssets` | Native loaders already exist; expose and exercise them instead of introducing another loader. |
| Native dispatch and image access | [`hiz.cpp`](../harfang/engine/hiz.cpp): `bgfx::setImage`, `bgfx::dispatch`, incremented views for successive mip passes | HARFANG already submits compute workloads internally. This is source evidence, not a successful test on the current machine. |
| Shader build chain | [`assetc.cpp`](../tools/assetc/assetc.cpp): `BuildComputeShader`, `ComputeShader`, `AssetType::ComputeShader` | `assetc` invokes `shaderc --type compute`; compiled compute files retain the `.sc` filename. Runtime must load compiled output, not shader source. |
| Shader profiles for the three HARFANG backends in use | `BuildComputeShader`: DirectX/D3D11 `cs_5_0`, Metal `metal`, OpenGL `130` | Compiler branches do not prove runtime support. In particular, inspect how pinned shaderc handles the OpenGL compute profile and test the resulting shader. |
| Generic public compute API | No `hg::DispatchCompute` or `ImageBinding` in the audited rendering implementation | Add a public submission wrapper and image bindings. Internal HiZ calls are not a Lua API. |
| Lua compute API | [`bind_harfang.py`](../binding/bind_harfang.py) lacks compute loaders, `DispatchCompute`, `ImageBinding`, and `TF_ComputeWrite` | These bindings are implementation work. Existing Lua program/texture/uniform facilities can be reused. |
| Compute buffers | Vendored [`bgfx.h`](../extern/bgfx/bgfx/include/bgfx/bgfx.h) has compute bindings for vertex/index buffer handles; [`defines.h`](../extern/bgfx/bgfx/include/bgfx/defines.h) has compute buffer flags | Implement a bounded HARFANG buffer abstraction. Do not assume a public storage-buffer API or graphics-stage buffer access already exists. |
| Shader language helpers | [`bgfx_compute.sh`](../tutorials/resources/core/shader/bgfx_compute.sh): `BUFFER_RO`, `BUFFER_RW`, thread declarations and atomics | Use pinned shader helpers; verify types and workgroup operations through the actual compiler. |
| FabGen generation | [`binding/CMakeLists.txt`](../binding/CMakeLists.txt) generates `binding/bind_Lua.cpp` using `HG_FABGEN_PATH` | Change the binding specification, regenerate, and rebuild `hg_lua`. Never hand-edit generated bindings. |
| Existing FPS input (unused by the final tutorial) | [`fps_controller.cpp`](../harfang/engine/fps_controller.cpp), `FpsController` bindings | Existing keyboard/mouse overload supports WASD/arrows and left-button look. The boolean overload allows tutorial-specific controls. |
| Quad drawing | `DrawTriangles` in `render_pipeline.cpp` allocates transient buffers; `Indices` is `std::vector<uint16_t>` and implicit indices also narrow to 16 bits | Approximately 454,000 quad vertices exceed both the index domain and a practical transient budget. A persistent, correctly indexed batch path is an enabling gate. |
| Premultiplied blending | `BlendMode`, `ComputeRenderState`, and Lua's opaque `RenderState` expose no `ONE / ONE_MINUS_SRC_ALPHA` mode | Add a supported premultiplied mode or a narrow state constructor; existing `BM_Alpha` is not equivalent. |

The existing [Compute Shader Feature-Parity specification](SPECS_COMPUTE_SHADER_FEATURE_PARITY.md) is a useful API design reference, but its claims about completed bindings/tests do **not** describe this checkout. Its initial scope deliberately excludes storage buffers. This roadmap therefore requires both the missing image/dispatch API and a separately specified buffer extension; image-only feature parity is insufficient.

### Backend feasibility gate

**HARFANG's three backends in use are DirectX, Metal and OpenGL.** This is the project's backend scope; additional compiler branches in vendored tooling do not expand it.

The user subsequently replaced the FPS camera with a trackball-only interaction model; do not retain an FPS toggle.

**Required initial backends: macOS/Metal first, then Windows/DirectX through Direct3D 11 (D3D11).** Both are acceptance requirements for v1. A Metal-only build is an intermediate milestone, not the completed delivery.

Run the small native feasibility probes on Metal first and continue the full viewer on macOS. Windows is unavailable for now: keep interfaces and shader data layouts suitable for D3D11, but defer its compiler/runtime qualification to the second phase. Keep backend-specific compiled assets in separate output directories so one build cannot overwrite the other's shader binaries. Require a Windows GPU runner before claiming D3D11 support; shader cross-compilation alone does not qualify that backend.

OpenGL is the third HARFANG backend. Qualification of this tutorial's compute path on OpenGL is deferred until Metal and DirectX pass; this does not question HARFANG's existing OpenGL rendering support. Keep shader sources portable and record the OpenGL compute compiler/profile checks needed for that later milestone. Browser and mobile delivery are outside this roadmap.

For each tested device/backend, record renderer, GPU, OS/driver, HARFANG/bgfx revision, and shader build commands. Check:

- `BGFX_CAPS_COMPUTE`, image read/write feature support, `limits.maxComputeBindings`, `maxTextureSize`, `maxViews`, and relevant allocation limits. The pinned public capability structure does not expose all workgroup limits: start with conservative 64/128-thread variants and validate compilation, shared memory and dispatch on each target.
- Per-format image-write and vertex-sampling support: `BGFX_CAPS_FORMAT_TEXTURE_IMAGE_WRITE` and `BGFX_CAPS_FORMAT_TEXTURE_VERTEX` for the proposed `RGBA32F` attribute atlases. Check format validity for the complete texture flags too.
- Implementation finding: pinned Metal omits the image-read/write format bits. Distinguish missing reporting from a proven unsupported format; the native probe explicitly tests RGBA32F on Metal. Do not globally mark all formats supported or silently bypass capability checks on other backends.
- `BGFX_CAPS_INDEX32` if using one large indexed quad batch; otherwise use ordered persistent 16-bit chunks.
- Compute-to-compute visibility with a producer/consumer sentinel, then compute-to-vertex-texture-fetch visibility with an observable rendered result. Ordered views schedule dependencies; access flags and workgroup barriers alone do not establish every required memory dependency.
- Actual pinned backend handling of buffer/image transitions. Fix a demonstrated backend gap in C++ rather than inventing a script-level barrier API.

Missing support must produce an actionable startup error naming the renderer and missing capability. A smaller asset cannot compensate for a missing compute feature.

## 3. Bundled asset: selected and verified

**Primary: Houseplant, a captured object by Marcel Padilla, CC BY 4.0.** Pin source revision `c3e5ec0a8ddb3ab26f28a067a29d272fe1a23411`.

- [Direct `plant.splat` download](https://raw.githubusercontent.com/marcelpadilla/splats/c3e5ec0a8ddb3ab26f28a067a29d272fe1a23411/data/plant/plant.splat)
- [Object metadata and licence declaration](https://raw.githubusercontent.com/marcelpadilla/splats/c3e5ec0a8ddb3ab26f28a067a29d272fe1a23411/data/plant/meta.json)
- [Publisher's decoder and format description](https://raw.githubusercontent.com/marcelpadilla/splats/c3e5ec0a8ddb3ab26f28a067a29d272fe1a23411/python/src/splatset/_splat.py)

| Property | Verified value |
|---|---|
| Download size | **3,636,736 bytes**, approximately 3.64 MB / 3.47 MiB |
| Gaussian count | **113,648**, exactly file size / 32 |
| Color representation | SH0-equivalent baked RGB; directional SH already removed |
| SHA-256 | `ce9d490b18f16d6730d832227bf541431b8b1a5d167c5a3fe89af4f324c0d8fd` |
| Source orientation | Up is `-Y` |
| Units | Arbitrary reconstruction scale, not metres |

The complete file was downloaded into memory during this audit: byte count, SHA-256, record count, and finite position/scale values were checked. Rendering quality and GPU cost remain untested. This is a much better first asset than a multi-million-splat scene. The metadata explicitly identifies this item as a capture; it must not be confused with the separate `plant_generated` asset.

During implementation, commit the original small `.splat`, its metadata, attribution, and licence notice. Record modifications to derived presets and preserve source attribution. Suggested credit: “Houseplant by Marcel Padilla, CC BY 4.0; source linked in the asset manifest. HARFANG adaptations: [list actual changes].” [CC BY 4.0 permits redistribution and adaptation with attribution, a licence link, and identification of changes.](https://creativecommons.org/licenses/by/4.0/)

**Backup for PLY importer validation:** Waku Factory's [trimmed kitune sample](https://www.wakufactory.jp/wxr/splats/data/kitune1.ply), covered by the publisher's [CC0 statement for sample data](https://www.wakufactory.jp/wxr/splats/sample.html). An HTTP range request reported **65,892,441 bytes**; its binary little-endian header declares **279,199** vertices and 45 `f_rest_*` fields, consistent with degree-3 directional SH. Only its header and server-reported size were inspected, not the complete payload or checksum. It is below 100 MB but less appropriate for the default demo; convert to SH0 and reduce offline before considering it for distribution.

Use a conservative project limit of **less than 100,000,000 bytes per committed binary**, and aim for **less than 20 MB total sample data**, with no LFS required for the tutorial. GitHub's actual hard limit is 100 MiB per ordinary Git file, with a warning above 50 MiB; the project's decimal limit is intentionally stricter. [GitHub large-file documentation](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github).

## 4. Data preparation and runtime representation

Make preprocessing reproducible and separate from the render loop. A small offline Python utility is acceptable; the interactive application remains Lua. Runtime requires neither Python nor a network download.

For the selected `.splat`, decode its documented 32-byte little-endian record: position `float32 × 3`, **linear** scale `float32 × 3`, RGBA `uint8 × 4`, quaternion bytes in **w,x,y,z** order. Decode quaternion components with `(byte - 128) / 128`, then normalize; reject or explicitly repair zero norm. Normalize color/opacity bytes to `[0,1]`. Do not apply `exp` or sigmoid to these already decoded quantities. [Publisher's decoder](https://raw.githubusercontent.com/marcelpadilla/splats/c3e5ec0a8ddb3ab26f28a067a29d272fe1a23411/python/src/splatset/_splat.py).

If adding the optional PLY path, parse properties by name and validate the Gaussian schema; an ordinary XYZ/RGB point cloud is not a Gaussian model. For the Graphdeco-style dialect, scales are logarithmic, opacity is a logit, and `f_dc_*` represents SH DC coefficients. Convert using the documented dialect, retain DC only, and discard higher bands explicitly. Do not apply those transforms indiscriminately to every PLY exporter. Use the [reference model representation](https://github.com/graphdeco-inria/gaussian-splatting/blob/main/scene/gaussian_model.py) to verify the conversion.

Preparation must:

1. Validate complete records, finite values, positive scales, normalized rotations, byte/count limits, and nonempty bounds.
2. Establish a reproducible source-to-HARFANG transform. Transform centers **and covariance/orientation**, not just positions. Document handedness, up axis, bounding-box normalization, and initial camera pose.
3. Generate optional deterministic 30k/60k presets using spatial coverage and contribution criteria. Preserve original IDs and record the algorithm, parameters, and output checksums. Compare against the full asset; arbitrary prefix truncation can remove whole parts of the object.
4. Produce a manifest with source URL/revision/hash, attribution, counts, bounds, SH degree, transform, and conversion version.
5. Build a versioned runtime blob into ignored generated staging, then pass it and shader sources through `assetc`. Verify pass-through handling for the custom binary. Runtime reads only `resources_compiled`, or its mounted asset namespace.

Prefer a clear, aligned GPU layout over premature compression: four `vec4` lanes per splat (position/opacity, scale/padding, quaternion, RGB/padding), **64 bytes** total. Document CPU offsets, shader indexing, byte order, and element count. This is approximately 6.94 MiB for the full asset. Treat source packing and GPU packing as different formats. FP16, quaternion compression, and SH1 are later measured options, not initial requirements.

## 5. C++ and FabGen work packages

The names below are **proposed API**, except for existing loaders and facilities identified in section 2. Finalize contracts before implementation.

| Work package | Proposed surface / contract |
|---|---|
| Image compute parity | Bind existing compute loaders; add `Access`/`AC_*`, `TF_ComputeWrite`, `ImageBinding`, `MakeImageBinding`, and `DispatchCompute` following the existing parity specification. |
| Capability reporting | Expose a compact compute capability query containing supported features, formats, binding/view limits, and renderer identity. Avoid exposing unstable bgfx internals wholesale. |
| Buffer ownership | A noncopyable, owned `ComputeBuffer` with explicit creation/destruction, capacity, element format, access permissions, and validity. Map to verified bgfx handles internally. |
| Bulk upload | Accept existing HARFANG data/blob facilities, not an array of Lua objects. Copy or retain upload memory according to bgfx lifetime rules. Separate immutable input from compute-written scratch. |
| Buffer binding | `ComputeBufferBinding`, a constructor/helper, list conversion, and a buffer-capable dispatch overload. Preserve existing image-only call semantics and defaults. |
| Persistent drawing | Reuse a suitable persistent model path if verified; otherwise add a narrowly scoped reusable quad-batch facility. Geometry is created once and indexed by sorted slot. |
| Premultiplied blend state | Add a proposed `BM_AlphaPremultiplied` mode or dedicated state constructor through C++ and FabGen. If extending the shared enum, preserve existing numeric values and update render-state construction, material mapping/serialization and documentation consistently. |

Texture flags and buffer flags are separate types/concepts. The pinned bgfx header states that compute-write buffers cannot be updated from the CPU: upload immutable input to an appropriate read buffer; initialize/clear scratch through compute. Do not design an unrestricted CPU `Update` operation for all buffer kinds.

Extend `binding/bind_harfang.py` with noncopyable ownership, access enums, binding records, vector/list conversions, and overloads. For a dispatch accepting `ViewId &`, use `arg_in_out` so Lua receives the next view ID. Follow existing `expand_std_vector_proto` patterns. Regenerate with `binding/CMakeLists.txt` and the configured FabGen checkout, then rebuild the Lua executable/module actually used by the tutorial.

Specify resource lifetime explicitly: reject use after destroy, prevent double destruction, keep referenced resources alive through submission, and release all GPU objects before `RenderShutdown`. GC is a safety net, not the normal shutdown mechanism. Test a forced collection between creation and submission, explicit destruction followed by GC, and clean shutdown. Avoid hidden ownership through freely copied raw handles.

`DispatchCompute` should apply all bindings and uniforms for that dispatch, submit work, and return the advanced view ID. It must not silently set viewport/camera state. Validate group sizes, stage indices, capacity and handle validity. Bind resources again when needed after submission; do not assume previous dispatch state persists.

At startup, check the required Lua symbols and report that the runtime must be rebuilt if they are absent. Merely adding tutorial files beside an older packaged runtime cannot enable this API.

## 6. Rendering pipeline

```text
Source .splat + manifest
  -> offline conversion -> assetc -> compiled runtime blob
  -> one-time bulk upload

Lua camera / projection / viewport
  -> compute: project + cull + depth keys
  -> compute: GPU depth sort (measured bitonic prototype now; radix optimization later)
  -> compute: gather sorted attributes into atlases
  -> graphics: ordered persistent Gaussian quads
  -> optional overlay -> present
```

### Projection and culling

Build 3D covariance from scale and rotation. Transform covariance into camera space and project with the perspective Jacobian into a 2D covariance. Regularize degenerate footprints, derive two ellipse axes, and use a finite Gaussian support threshold. Verify matrix storage conventions, positive camera depth, projection Y orientation, and clip-depth conventions on each backend. The [original 3DGS paper](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/) and [reference projection implementation](https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/forward.cu) provide mathematical references; this roadmap does not authorize importing their code or assume its licence matches HARFANG.

Cull behind-camera/near-plane invalid cases, negligible opacity, and footprints completely outside the viewport. Do not cull solely because a center is outside the screen: its ellipse may still intersect. Define the near-camera policy for very large ellipses and test it; a radius clamp is a quality/performance approximation that must be visible in the settings/documentation.

### Ordering

Sort valid splats **back to front** using positive view-space depth, not Euclidean distance. Keep full float32 depth precision in a monotonic unsigned key and store the source index separately. Use stable source-index order for ties, and an explicit sentinel policy for invalid entries. Keep a fixed-size work domain for v1 so dispatch dimensions do not require a GPU-to-CPU visible-count readback.

A radix digit requires histogram, prefix allocation, and **stable** scatter, potentially with several scan dispatches. Global histogram offsets alone do not guarantee a stable scatter: include deterministic within-block ranks and block prefixes. Reset counters each pass. Do not claim a global sort can use a workgroup barrier as a device-wide barrier.

Use bitonic sorting only as a small-data correctness reference or an explicitly measured prototype. A naive global bitonic sort padded to 131,072 elements needs 153 compare dispatches; that consumes substantial view and submission budget. Target a measured radix schedule under 64 compute dispatches and under 128 total views, while respecting the actual runtime limits. If the implementation exceeds those budgets, revise the schedule; they are targets, not measured results.

Recompute ordering on every camera change. For a static camera and static splats, reuse cached projection/sort/gather results; invalidate them on viewport, projection, transform, or quality-setting changes. Camera motion must remain the benchmark workload.

### Gather and draw

Gather sorted data into three `RGBA32F` atlases: center/depth/validity, two 2D ellipse axes, and RGB/opacity. Pad unused texels and disable their quads. Choose atlas dimensions against `maxTextureSize`; sample exact texel centers with point filtering, no mipmaps, no sRGB conversion. Avoid assuming compute buffer bindings are available to vertex shaders in this pinned bgfx version.

Create persistent quad geometry once: each quad carries a corner and a sorted-slot index. The vertex shader fetches that slot's attributes and expands the ellipse. The fragment shader evaluates `alpha = opacity * exp(-0.5 * dot(local, local))` inside the selected support, outputs premultiplied RGB, and uses `ONE / ONE_MINUS_SRC_ALPHA` blending. Disable transparent depth writes. Keep the first tutorial on a simple background; opaque-scene depth integration can follow separately.

Preserve primitive order within each batch and submission order between batches; Lua already exposes `SetViewMode` and `VM_Sequential`. For 16-bit batches, keep each local vertex domain within range and pass the appropriate sorted-slot offset. Never submit one draw call per splat. Confirm color-space handling against a reference image to avoid double gamma conversion. For the standalone background-only viewer, explicitly disable depth testing as well as depth writes; a later opaque-scene integration must instead test against its depth buffer with matching conventions.

Center-depth sorting is a practical approximation: intersecting or very large splats can still produce ordering artifacts. SH0 also removes view-dependent appearance. Document these limits instead of promising exact volumetric visibility.

## 7. Lua tutorial and asset layout

Original proposed layout (the delivered paths and commands are documented in [the tutorial guide](../tutorials/GAUSSIAN_SPLATTING.md)), following this repository's existing `resources` / `resources_compiled` convention:

```text
tutorials/
  gaussian_splatting.lua
  gaussian_splatting/                 # Lua camera, resource and pass modules
  tools/prepare_gaussian_splat.py     # offline conversion and preset generation
  resources/gaussian_splatting/
    plant.splat
    source.json
    ATTRIBUTION.md
    LICENSE-CC-BY-4.0.txt
  resources/shaders/gaussian_splatting/
    project.sc
    radix_histogram.sc
    scan.sc
    radix_scatter.sc
    gather.sc
    splat_vs.sc
    splat_fs.sc
    splat_varying.def
  generated/gaussian_splatting/       # ignored conversion/build staging
  resources_compiled/                # ignored runtime assets from assetc
```

The preparation/build command assembles reproducible staging, including the generated blob, then invokes `assetc` for the selected renderer. Do not commit GPU binaries or load editable `resources` files at runtime. Document exact commands using the tools actually built in the implementation environment; validate `assetc` options rather than guessing them now.

Lua responsibilities:

- Start from a minimal window/input/render loop; initialize resources and report unsupported features before submitting any work.
- Use a quaternion trackball: left drag rotates around the target, wheel dollies, right/middle drag pans, R restores the initial object framing, and Escape exits. Draw a compact world XYZ gizmo over the scene with RGB `DrawLines`. No FPS mode.
- Normalize trackball quaternions, handle client/drawable scaling correctly, bound dolly distance, and suspend input when unfocused. Keep the cursor visible and ignore stale deltas at the start of a drag.
- Frame the supplied bounds at startup. Handle resize/minimize without zero-size dispatches; use actual framebuffer dimensions for projection and render resources, especially on high-DPI displays.
- Show CPU frame time, measured GPU time where available, preset, total/visible count when obtainable without a stall, resolution, renderer, sort time, and allocated GPU bytes. Asynchronous/debug counters are sufficient; no synchronous per-frame readback.
- Reuse resource lists/uniform containers; avoid per-splat Lua allocation and full garbage collection every frame. Destroy resources explicitly, then shut down rendering and window/input systems.

## 8. Performance contract

Targets below are acceptance objectives to measure, not predictions or results.

| Item | Initial target |
|---|---|
| Default asset | Full 113,648-splat Houseplant; SH0 only |
| Quality presets | Full / deterministic approximately 60k / approximately 30k |
| Default resolution | 1280 × 720 physical framebuffer pixels |
| Desktop target | 60 FPS / 16.7 ms frame budget during continuous camera motion on a recorded reference GPU |
| Reduced preset | At least 30 FPS / 33.3 ms on a separately recorded lower-power configuration |
| GPU memory | Less than 128 MiB incremental allocation for this viewer's splats, scratch, geometry, atlases, and explicit render targets |
| CPU boundary | Bulk upload at load time; no per-frame splat-table conversion, CPU depth sort, or blocking GPU readback |

An initial memory estimate at 113,648 splats is roughly 7 MiB input, up to 8 MiB projected records with padding, 2–4 MiB ping-pong key/index storage, 6 MiB for three padded attribute atlases, and approximately 8 MiB persistent quad geometry. Histograms, scan scratch, render targets, allocation padding, and backend buffering must be added and measured. Small source files do not imply small GPU cost.

Benchmark a fixed camera route including close-ups, grazing views and empty background; use a warm-up and at least 60 seconds of camera motion. Record median/p95 frame times, projection/sort/gather/draw times where measurable, resolution, count, memory, backend, GPU/driver and power configuration. Report GPU timing as unavailable rather than substituting CPU submission time. Check blending overdraw and very large projected splats before adding SH or more data.

## 9. Milestones and exit gates

Execute in this order. Estimates should be made after the backend spike, because shader/compiler and buffer-binding gaps dominate uncertainty.

| Milestone | Work and deliverable | Exit gate |
|---|---|---|
| M0 — native feasibility | Source audit, capability report, tiny image and buffer compute probes through `assetc`; test Metal now, D3D11 in the Windows phase | Metal sentinel and compute-to-vertex-texture rendering unblock macOS implementation. Full two-backend qualification still requires the same evidence on Windows. |
| M1 — Lua/FabGen bridge | Image parity plus bounded buffer API, capability query, ownership, bulk upload, premultiplied blend state and bindings; small Lua compute probe | Rebuilt `hg_lua` runs the probe, returns advanced view IDs, handles invalid resources, exposes the required blend state, and shuts down cleanly. |
| M2 — reproducible content | Pin/download/hash the chosen sample, preserve licence, prepare SH0 blob and optional presets | Offline build reproduces manifest/count/hash; runtime loads compiled assets without network or Python. |
| M3 — visible Gaussian slice | Synthetic ellipses, projection, atlas gather, persistent quad path; tiny CPU ordering oracle permitted for debugging | Known rotations, scales, opacity and overlapping colors match expected output; reference asset orientation is correct. |
| M4 — GPU ordering | GPU ordering, invalid entries, equal-depth ties, bounded buffers and pass scheduling; a measured bitonic prototype is implemented, with radix deferred | GPU result agrees with a CPU oracle for small/adversarial cases and 113,648 entries; camera motion triggers GPU sorting with no blocking CPU readback. |
| M5 — complete Lua tutorial | Trackball camera, RGB DrawLines gizmo, resize/focus, presets, cleanup, credits and launch guide | Fresh local checkout plus documented build steps produces a navigable viewer; repeated open/close and resize pass. |
| M6 — Metal and D3D11 qualification | Fixed camera benchmark, memory accounting, complete Lua viewer and correctness checks on macOS/Metal and Windows/D3D11; documentation | Both required backends pass rendering, GPU ordering, camera, resize and shutdown checks. Each has recorded performance on a named reference device and an explicit supported preset. Missing Windows runtime evidence leaves this milestone incomplete. |

M2 can proceed alongside M0/M1. M3 needs M1/M2; M4 needs the native buffer/visibility gate; M5/M6 require the complete path. Keep other-language ports outside these exit gates.

Validation should cover empty/malformed/truncated assets, invalid scale/quaternion, non-power-of-two counts, all-invisible splats, duplicate depths, near-plane crossings, overlapping red/green splats with a known blend result, camera reversal, atlas edges, draw-chunk boundaries, and sort stability. Use GPU capture/readback only in focused validation or asynchronous diagnostics. Never treat a screenshot alone as proof of sort correctness.

The lack of a public buffer-readback API does not prevent an ordering oracle: a debug compute pass can encode sorted indices into a verified readable texture, followed by asynchronous capture through the existing `CaptureTexture` path. Validate that staging path first and wait only in tests; do not add blocking readback to the normal frame loop.

Add new public API documentation under `doc/doc`, a tutorial section in `tutorials/README.md`, and the smallest appropriate native/Lua smoke checks. The release checklist must distinguish shader compilation, runtime correctness, performance, and backend coverage.

## 10. Optional extension: rasterization entirely in compute

If the educational objective later requires compute shaders to produce the final pixels, extend the validated foundations with a tiled pipeline:

1. Project/cull and count each Gaussian's tile overlaps.
2. Prefix-scan counts and scatter bounded `(tile ID, depth key, source ID)` references.
3. Sort references by tile, then front-to-back depth; use multiple 32-bit key words instead of assuming portable 64-bit atomics.
4. Build tile ranges; assign a workgroup to a tile and composite its contributors per pixel, with transmittance early termination.
5. Write an output image and display it through a fullscreen pass.

Choose tile size, shared-memory use and maximum references from measured budgets. Overflow must never produce out-of-bounds writes or silently retain an arbitrary subset selected by atomic scheduling. Detect it before unsafe writes and render a visible diagnostic or deliberately switch to a deterministic reduced preset; record the policy. Include near-camera large-footprint stress cases.

This path introduces extra scans, duplication, sort keys, scratch memory and occupancy tuning. It is a separate milestone, not a requirement for the hybrid v1. A fullscreen shader looping over every splat per pixel is not an acceptable real-time substitute, and a fixed global contributor prefix does not identify the splats affecting each pixel.

## 11. Review provenance

Codex owns the architecture decision, source/asset verification, final document and integration. The requested `duo-codex-copilot-ollama` protocol was executed as task `GSPLAT-ROADMAP-001` on an isolated worktree at the audited revision. DeepSeek ran through Copilot CLI `1.0.95` as `deepseek-v4.1-flash:cloud`; the runner finished successfully, reported only the permitted review-file writes, and recorded no contract violations. Its initial delivery status was “À vérifier”.

After checking the report against source, Codex selectively integrated the missing premultiplied-blend API, the 16-bit transient-index limitation, runtime-symbol checks, and more explicit capability gates. Recommendations for permanent CPU sorting, unsupported performance estimates, and the claim that GPU sort validation is impossible without buffer readback were rejected. The buffer extension and capability query were already planned; their absence in today's API is an implementation dependency, not a contradiction in the roadmap. The raw review and local runner logs remain outside the repository; model output is not a substitute for runtime validation.
