# Gaussian Splatting as a Native HARFANG Rendering Feature

Date: 2026-10-27. Source audit: branch `gsplat`, revision `dc0f42de206f2982d12c9a24e6ee94e9fefcafda`.

Status: architecture study and proposed contracts, not an implemented API. This document builds on the [Lua tutorial roadmap](SPECS_GAUSSIAN_SPLATTING_LUA_TUTORIAL_ROADMAP.md) and distinguishes the current implementation from the proposed engine feature. Backend results quoted below come from [GSPLAT_PROGRESS.md](GSPLAT_PROGRESS.md); no new GPU qualification was performed for this study.

## 1. Recommendation and intended use

**Build a native Gaussian rendering subsystem for scenes containing several Gaussian models and conventional meshes. Make the tutorial a small client of that subsystem.** Moving the current Lua implementation into C++ verbatim would preserve several limitations that are unacceptable for this use case.

The proposed division is:

| Concern | Recommendation |
|---|---|
| Public rendering API | Immutable Gaussian model resources, lightweight transformed instances, shared renderer programs, and separate per-camera render data. One high-level submission call, plus an explicit prepare/draw interface for pipeline integration. |
| Several Gaussian models | Project all contributing instances into one camera-specific working set and sort their splats together. Sorting models separately and drawing them in object order is insufficient. |
| Conventional meshes | Share the opaque/alpha-tested mesh depth attachment; depth-test splats without writing depth. Add a deliberate insertion point in the forward pipeline before final composition. |
| Transparent meshes | Treat joint ordering as a separate rendering problem. Document supported composition policies; do not imply that global splat sorting also sorts triangle fragments. |
| Loading | Use one validated, versioned native runtime asset through HARFANG readers. Initially import the existing `.splat` dialect and Graphdeco-style Gaussian PLY offline. Add SPZ and glTF Gaussian import as explicit subsequent adapters. |
| Spherical harmonics | Preserve degree 0–3 and their conventions in the canonical data contract immediately. Evaluate directional color on the GPU. Never make baked RGB the only native representation. |
| Implementation strategy | Design ownership, composition, resource budgets and correctness tests first. Reuse proven tutorial shaders and probes where appropriate, while replacing assumptions that prevent scene integration. |

The first complete engine feature must demonstrate **two overlapping Gaussian models, independently transformed, and an opaque/alpha-tested mesh that correctly occludes them**, from two camera views in one frame. A single-object viewer is a regression fixture, not the acceptance definition. Transparent mesh interpenetration, physically correct volume integration, streaming city-scale datasets, training, and relighting are additional capabilities with separate costs.

## 2. What exists, and what it proves

The entry point is already small: [`gaussian_splatting.lua`](../tutorials/gaussian_splatting.lua) controls the window, camera and frame loop. Most orchestration is in [`renderer.lua`](../tutorials/gaussian_splatting/renderer.lua). There is no per-Gaussian Lua loop during normal rendering; projection, culling, sorting and gathering already run on the GPU. Native code chiefly provides an opportunity for reliable ownership, scheduling and integration. A performance gain must be measured, not inferred from the language change.

| Existing evidence | Implication for the engine feature |
|---|---|
| [`compute.h`](../harfang/engine/compute.h) and [`compute.cpp`](../harfang/engine/compute.cpp): typed float4/uint4 buffers, bulk upload, image bindings, dispatch, capability reporting, persistent quad models | Keep these general rendering primitives. Add a Gaussian-specific subsystem above them rather than requiring application scripts to assemble a renderer. |
| [`bind_harfang.py`](../binding/bind_harfang.py), `bind_compute`: shared buffer handles, list conversion, `ViewId &` returned through `arg_in_out` | There is an established FabGen route. The original roadmap's missing-compute-binding audit predates the current implementation. |
| Full Houseplant: 113,648 Gaussians, padded to 131,072; 153 bitonic compares, 155 compute dispatches, seven quad batches | Useful correctness baseline, but expensive view consumption and quadratic-logarithmic sorting work are implementation choices, not public contracts. |
| [`project.sc`](../tutorials/resources/gaussian_splatting/shaders/project.sc): hard-coded perspective camera parameters, already transformed input, no object transform | Replace the tutorial camera protocol with `ViewState`, an explicit viewport and instance transforms. |
| [`splat_vs.sc`](../tutorials/resources/gaussian_splatting/shaders/splat_vs.sc): constant clip-space depth; `ComputePremultipliedRenderState`: no depth test/write | Mesh occlusion is not implemented. Merely drawing the existing model into a scene view cannot provide it. |
| Lua file I/O plus executable `metadata.luadata`, HGS0 header checks, then a second file read for upload | Replace this with a single native validation/decode path and passive metadata usable through file and mounted-asset providers. |
| [`prepare_gaussian_splat.py`](../tutorials/tools/prepare_gaussian_splat.py): one 32-byte `.splat` dialect, deterministic presets, 64-byte HGS0 records | Reuse the fixtures and conversion knowledge. This is not yet a general Gaussian asset pipeline or a PLY importer. |
| [`bloom.h`](../harfang/engine/bloom.h), [`hiz.h`](../harfang/engine/hiz.h), [`scene_forward_pipeline.h`](../harfang/engine/scene_forward_pipeline.h) | HARFANG already uses native effect objects with `Create…FromFile/Assets`, `Destroy…`, and view-ID-consuming submission. Follow these conventions. |

Recorded Metal results on M3 Pro include 28.64 MiB of persistent GPU payload and full-preset GPU p95 of 5.68 ms at 1280×720. These are the existing single-object workload's results. They do not predict multi-object, SH3, stereo, or mesh-composition costs. D3D11 and OpenGL remain unqualified for this feature in the recorded evidence, even though this study is being written on Windows.

## 3. Native architecture and public surface

### 3.1 Separate data, resources, instances and camera work

Use four public concepts, with the instance being a small value rather than another resource owner:

| Proposed type | Responsibility and lifetime |
|---|---|
| `GaussianSplatModel` | Immutable decoded model, metadata, bounds and GPU source storage. Shareable across instances, cameras and renderer submissions. Independent of one shader program or camera. |
| `GaussianSplatInstance` | Model reference, object-to-world transform, stable instance ID and optional visibility/opacity multiplier. No copy of the model's splats. |
| `GaussianSplatRenderer` | Compiled programs, immutable common resources, capability decisions and algorithm configuration. Does not contain one globally reused camera scratch buffer. |
| `GaussianSplatRenderData` | Capacity-managed projection/key/sort/gather resources for one outstanding camera submission; prepared-state token, frame/generation information and statistics. |

Internally, separate CPU parsing/canonical data from GPU resource creation. The parser must be testable without `RenderInit`. Runtime model-loading convenience functions require an initialized renderer and perform both steps; an explicit load option controls retention of the CPU payload after upload. A later asynchronous loader can decode on a worker and upload on the render thread without redesigning the file format or exposing GPU calls to workers.

Avoid inheriting from `Model`: a Gaussian model is not a triangle display list with a material. Persistent quads are an internal rasterization technique. Likewise, keep the CPU canonical representation distinct from a GPU packing chosen for a particular renderer revision.

For multiple source models, the initial implementation need not bind every source buffer simultaneously. Project each instance into a disjoint range of a shared working set, binding only that model's source/SH data for the dispatch. Then sort references across the **union** and gather from that common projected data. This fits bounded compute-binding counts and scales CPU work with instances, not Gaussians. Cache source GPU allocations; repeated instances share them.

### 3.2 Proposed API shape

The following is a schematic interface; complete handle/result definitions and includes are omitted. Names, result types and signatures are proposals, not callable symbols in this checkout.

```cpp
// Model, renderer and render-data handles have shared, opaque storage.
struct GaussianSplatInstance {
    GaussianSplatModel model;
    Mat4 world;
    uint32_t id;                 // stable within this instance set
    float opacity = 1.f;
};

// Result records carry a status enum and a diagnostic message.
GaussianSplatLoadResult LoadGaussianSplatModelFromFile(
    const char *path, const GaussianSplatLoadOptions &options);
GaussianSplatLoadResult LoadGaussianSplatModelFromAssets(
    const char *name, const GaussianSplatLoadOptions &options);
GaussianSplatLoadResult LoadGaussianSplatModelFromData(
    const Data &data, const GaussianSplatLoadOptions &options);

GaussianSplatRenderer CreateGaussianSplatRendererFromFile(const char *shader_root);
GaussianSplatRenderer CreateGaussianSplatRendererFromAssets(const char *shader_root);
GaussianSplatRenderData CreateGaussianSplatRenderData();

GaussianSplatRequirements GetGaussianSplatRequirements(
    const GaussianSplatRenderer &renderer,
    const std::vector<GaussianSplatInstance> &instances,
    const GaussianSplatRenderSettings &settings);

GaussianSplatSubmitResult SubmitGaussianSplats(
    bgfx::ViewId &next_view, const iRect &rect, const ViewState &view_state,
    const std::vector<GaussianSplatInstance> &instances,
    GaussianSplatRenderer &renderer, GaussianSplatRenderData &render_data,
    const GaussianSplatRenderSettings &settings,
    bgfx::FrameBufferHandle target = BGFX_INVALID_HANDLE);

void DestroyGaussianSplatModel(GaussianSplatModel &model);
void DestroyGaussianSplatRenderer(GaussianSplatRenderer &renderer);
void DestroyGaussianSplatRenderData(GaussianSplatRenderData &render_data);
// IsValid overloads and metadata/statistics queries accompany these functions.
```

`GaussianSplatLoadResult` contains the model and its validation report. `GaussianSplatSubmitResult` contains status, a diagnostic and the actual draw view. Requirements report conservative GPU/CPU bytes, instance/splat limits, dispatch count and the number of views required by the selected schedule. Use checked wide arithmetic for these calculations; do not encode byte budgets in floats.

Settings should expose application decisions: maximum SH degree, supported color-composition policy, depth convention, memory/capacity budget, and an exclusive upper bound on the application's allocated view range. Keep workgroup sizes, ping-pong buffers, atlas dimensions, uniforms, sort strides and backend workarounds private. An advanced diagnostics structure can expose the selected algorithm without making it an application responsibility.

`SubmitGaussianSplats` owns the views it consumes, configures their target/rect/transforms, performs preparation and draws. It returns the first unconsumed view through `next_view`. It never calls `Frame`, changes the camera, resets the renderer or clears an existing scene target. The application/pipeline owns target clearing; an empty successful submission produces no draws and consumes no views.

Validate the entire submission, capacity and view range before enqueueing any commands. On ordinary validation/allocation failure, return an error and leave `next_view` unchanged. Since already submitted GPU commands cannot be rolled back, separate planning/allocation from command emission; do not discover predictable failures halfway through the sort. GPU/device failure after accepted submission is outside this transactional guarantee.

### 3.3 Advanced prepare/draw contract: rendering into an existing BGFX view

Provide a second level consistent with HARFANG's scene preparation functions:

1. `PrepareGaussianSplats(next_view, …, render_data)` consumes reserved compute views and freezes the camera, instances, settings and resource generations into prepared state.
2. `DrawGaussianSplats(draw_view, renderer, render_data)` submits the prepared ordered batches into a caller-configured graphics view, consuming no new view.

The advanced draw call does not change clear state, framebuffer, viewport, view transform or view order. The caller must configure them consistently with preparation and use `VM_Sequential` before submitting any draws into that view. The caller must also guarantee that preparation executes before the draw in the final BGFX view order. Return a status for invalid prepared state; document external view-state/order obligations that the implementation cannot infer reliably.

An arbitrary view ID is therefore not enough information to promise correct integration. A dedicated Gaussian graphics view targeting the **same framebuffer and depth attachment** is usually easier to compose than changing the sort mode of a view already containing unrelated transparent draws. Both options should remain possible. Different cameras require different graphics views: one BGFX view has one set of rect/transform/target state for the frame.

### 3.4 View budget and scheduling

The current `DispatchCompute` increments the view for each dispatch. For one full tutorial dataset this implies 155 compute views plus one graphics view. Two such preparations would require 312 views; the pinned BGFX default is 256. Merely moving these calls to C++ would fail a basic two-camera use case.

There is a useful optimization opportunity in the pinned source: [`SortKey::encodeCompute`](../extern/bgfx/bgfx/src/bgfx_p.h) sorts compute by view and submission sequence, and [`EncoderImpl::dispatch`](../extern/bgfx/bgfx/src/bgfx.cpp) assigns that sequence. The draw bit places compute before graphics in a view; sequential graphics mode does **not** preserve arbitrary compute/draw interleaving. These observations justify testing several dependent dispatches in one compute view, but are not by themselves proof of resource visibility on every backend.

Recommended work order:

1. Build an internal compute schedule that can submit several passes to one reserved view, without changing the public semantics of the existing generic `DispatchCompute` wrapper.
2. Run producer/consumer and full-sort oracles for that schedule on each backend. Preserve explicit access bindings and ping-pong storage. A workgroup barrier is not a cross-dispatch dependency mechanism.
3. Keep the current separate-view schedule as a bounded reference path, with preflight rejection when it does not fit. Never wrap view IDs or overwrite another subsystem's views.
4. Develop stable GPU radix sorting for the production workload. Keep bitonic as a small-data/reference strategy until measurements establish a crossover. Radix still needs scans, bounded scratch and synchronization; it does not imply a fixed universal pass count.

The qualified compact path should aim for a small compute-view range plus a graphics view per camera. Dispatch count and view count are different budgets. Custom `setViewOrder` mappings must preserve dependencies; the subsystem must not silently rewrite an application's global view order.

Preflight must also validate compute support, format/image-write/vertex-sampling support, binding stages, dispatch dimensions, both atlas dimensions and every allocation size. The pinned Metal capability reporting is incomplete for image writes: use an explicitly qualified backend/version path, not a general rule that a missing capability flag means success. Report unsupported capability, exhausted capacity and invalid input as distinct statuses.

### 3.5 Ownership, Lua and frame lifetime

Follow the shared-storage precedent of `ComputeBuffer`, with generation/validity tracking for all resources. Copies and instance lists retain ownership. Explicit destruction invalidates aliases predictably; double destruction is harmless. Add renderer-shutdown invalidation so Lua userdata surviving `RenderShutdown` never destroys a stale GPU handle in a later renderer session.

Do not reuse camera A's projected buffer or atlas for camera B before A's draw has executed. BGFX execution order differs from host call order, and same-view compute-before-draw sorting makes this particularly easy to get wrong. Use distinct render data for outstanding camera submissions; retain source resources through the submitted frame and retire replaced allocations according to BGFX's lifetime rules. Reusing a camera's buffers in successive ordered frames is permitted when GPU read/write ordering is guaranteed; it must not introduce a blocking wait every frame. CPU upload memory must be copied or retained until BGFX has consumed it. Prepared tokens are frame-specific and invalidated by a new preparation or resource generation. The first API can be render-thread-only and explicitly reject concurrent use of a render-data object.

Bind opaque handles, small settings/result records, enums and vectors of instances through FabGen. Reuse `expand_std_vector_proto`; mark `next_view` as `arg_in_out`. Do not marshal a Lua table per Gaussian. Regenerate bindings from [`binding/CMakeLists.txt`](../binding/CMakeLists.txt), never edit generated C++.

An illustrative client becomes:

```lua
-- Proposed API; assets and shaders were compiled offline and mounted beforehand.
local renderer = hg.CreateGaussianSplatRendererFromAssets('core/gaussian_splatting')
local data = hg.CreateGaussianSplatRenderData()
local loaded = hg.LoadGaussianSplatModelFromAssets('plant.hgs', hg.GaussianSplatLoadOptions())
assert(loaded.ok, loaded.message)
local instances = {hg.MakeGaussianSplatInstance(loaded.model, hg.Mat4.Identity, 1)}

-- Per frame: the application/pipeline provides camera, target and view reservation.
local result
result, view_id = hg.SubmitGaussianSplats(
    view_id, rect, view_state, instances, renderer, data, settings, target)
assert(result.ok, result.message)
```

Camera controls, user interface and the XYZ gizmo remain tutorial/application responsibilities. Explicit cleanup remains visible in examples, with GC as a fallback. The normal frame path has no parsing, per-splat CPU traversal, blocking readback or stable-capacity GPU reallocation.

## 4. Several splat models and conventional meshes

### 4.1 One ordering domain for overlapping splats

All overlapping Gaussian instances in a camera's composition domain must contribute to one ordering operation. Use a deterministic total order: selected distance/depth key, stable instance ID, then stable source-splat ID. Cull invalid/padded entries to a trailing sentinel. Internal projected indices can identify the source instance and splat without encoding large integer IDs in floats.

The tutorial uses camera-space center depth. Preserve that as an explicit native/reference policy; carry the source model's requested sort policy as metadata. Reject incompatible policies within a strict composition domain, or require a deliberate conversion/approximation choice. Changing a camera's sort policy is not a file-loader optimization.

Compute object transforms before projection and key generation. For a linear object transform `A`, transform covariance as `A * Sigma * transpose(A)` and centers with the full affine transform. Multiplying the three scale components is insufficient after a general nonuniform transform. Start with a documented positive uniform-scale/rotation/translation instance contract; add positive nonuniform scale with tests before claiming that broader domain. Explicitly reject unsupported shear, singular transforms and runtime reflections. Offline coordinate-system conversion is a distinct operation.

Sorting centers remains an approximation for extended/intersecting Gaussians. Global sorting fixes model-boundary errors but does not make the renderer an exact volume integrator. Reserve the option of tiled depth sorting or another rasterizer behind the same public resource/instance API.

### 4.2 Opaque and alpha-tested geometry: required integration

For the initial integrated path:

```text
Gaussian source models + instance transforms + camera
    -> projection/culling -> common keys -> global GPU sort -> gathered attributes

Meshes -> opaque / alpha-tested color and depth
    -> ordered splat draw into the same color/depth target
    -> explicitly supported transparent-mesh policy
    -> final composition / post-processing / overlays
```

The splat graphics pass must read depth via the depth test, keep depth writes disabled, and use premultiplied source-over blending. Add projected center depth to the gathered attributes; the current constant `gl_Position.z = 0.5` cannot be reused. Derive clip depth from the actual view/projection matrices and account for backend depth range, projection jitter, viewport origin, render-target orientation and the chosen forward/reversed depth comparison. Do not equate positive camera distance with hardware depth.

Constant **per-splat center depth**, correctly projected, is a useful initial mesh-occlusion approximation. It can cut a large Gaussian incorrectly where an opaque surface intersects its extent. It should be identified as the initial depth model and tested with an intersecting plane. More accurate per-fragment depth or volumetric integration is additional work. Writing a Gaussian's center to the depth buffer as though it were an opaque polygon would break translucent accumulation and is not the remedy.

Alpha-tested meshes participate in the opaque depth path using their normal coverage rules. Splats in front of a mesh blend over its color; splats behind it fail the depth test. If a separate splat framebuffer is used, it must share a compatible depth attachment or use an explicitly supported depth transfer/sampling path; copying just mesh color is insufficient. MSAA and resolved targets require matching depth/sample-count contracts.

### 4.3 Forward pipeline, AAA and transparent meshes

Expose a pipeline insertion point after opaque color/depth production and before the relevant composition/resolve step. [`scene_forward_pipeline.cpp`](../harfang/engine/scene_forward_pipeline.cpp) already separates preparation, opaque/transparent passes and AAA work; adding a call after the existing high-level function returns is not equivalent to integrating at the right internal stage.

For AAA, use the same jittered projection and define whether splats contribute to motion vectors, temporal history and depth-dependent effects. The existing forward attributes do not automatically describe splats. Until velocity/history handling is implemented, select a documented composition path that avoids feeding unsupported data to TAA/SSR rather than claiming full AAA compatibility. Native scene support can add a Gaussian component and a dedicated resource cache later; the required first integration can explicitly gather model instances and call the new pipeline stage.

For conventional transparent meshes, distinguish:

| Policy | Result and suitability |
|---|---|
| Transparent mesh pass entirely before or after splats | Simple, deterministic layer ordering; acceptable for known backgrounds/foregrounds, incorrect for arbitrary interpenetration. |
| Shared object-depth sorting | Can help separated objects; cannot interleave one mesh's fragments with the many splats inside its depth extent. |
| Unified tiled contributor lists or per-pixel fragment storage | Potentially better joint ordering; significant storage, overflow, geometry and rasterization work. Requires a separate design. |
| Weighted blended OIT | Bounded-cost approximation; changes Gaussian compositing and may lose the appearance learned by the asset. Not a transparent substitute for sorted source-over. |

The first release should guarantee splat/splat ordering and opaque/alpha-tested mesh occlusion, and expose the first policy explicitly for transparent meshes. If arbitrary glass/particles crossing splat volumes is mandatory, elevate unified transparency to a release gate; the present hybrid prototype does not solve it. Splats also do not automatically become shadow casters, collision surfaces or PBR materials.

### 4.4 Color and compositing are part of the integration contract

Record the asset's reconstructed color encoding, image state and compositing policy. The current tutorial directly blends decoded RGB without additional gamma conversion; that is a useful compatibility mode, not evidence that every capture stores scene-linear radiance.

Two practical engine modes are needed:

- **Linear scene integration:** evaluate each splat's color, convert it into the chosen linear working domain, then blend with conventional meshes. This is an intentional approximation when the model was trained/composited in an encoded display domain. Never apply a transfer function independently to SH coefficients; evaluate the directional color first.
- **Reference appearance composition:** composite splats in their declared training/display domain, with the opaque background transformed into that domain, then convert the completed image for output. This may require a native intermediate target/composition pass. Converting an isolated premultiplied splat layer after blending generally cannot reproduce the result over an arbitrary background through a nonlinear transfer function.

Do not silently mix models with different reference composition domains. Validate compatibility or require an explicit common-domain conversion policy. Expose this choice through a small setting/profile; keep actual render-target management in C++.

## 5. Loading and format support

### 5.1 Separate interchange, compilation and runtime loading

The production path should be:

```text
Source PLY / .splat / later SPZ or glTF
    -> native importer + validation + explicit transforms/quality settings
    -> versioned HARFANG Gaussian asset + passive provenance metadata
    -> assetc staging/compilation
    -> compiled assets or asset package
    -> native File / Assets / Data loader -> upload once
```

Use HARFANG's [`Reader`/`ReadProvider`](../harfang/foundation/rw_interface.h) internally, with file and [`assets`](../harfang/engine/assets_rw_interface.h) wrappers. `FromFile` refers to a filesystem location, normally compiled output; `FromAssets` resolves the mounted namespace. A `FromData` entry point takes a serialized native asset, not an undocumented GPU byte layout. Optional editor/procedural creation can use a separate checked bulk-data builder.

The shader root is engine content compiled for the active backend, independent of model paths. Keep backend shader outputs separate. Native model data should remain backend-neutral until an optimization demonstrably requires distinct compiled variants.

[`assetc.cpp`](../tools/assetc/assetc.cpp) currently copies unprocessed files and builds compute/graphics shaders. It does not import Gaussian sources. First add an offline `gsplatc`-style tool backed by reusable C++ parsers; subsequently integrate its dependency tracking into `assetc`. A copy of a PLY to the compiled directory is not a compiled Gaussian asset. Existing Python preparation can remain a deterministic reference fixture during migration.

Direct PLY/SPZ loading is useful for editors and ad hoc viewers, but should be an explicit `Import…` service using the same decoder. It has different latency and memory costs from native runtime loading. Do not hide decompression, normalization or file conversion inside a draw call.

### 5.2 Prioritized compatibility matrix

The external sources in this table were checked on 2026-10-27. Pin importer dependencies and conformance fixtures to a revision when implementing them.

| Format/dialect | SH and practical properties | Recommendation |
|---|---|---|
| Existing HGS0 v1 | Baked RGB only, fixed 64-byte records; currently used by the tutorial | Maintain a read-only compatibility adapter and conversion fixture. Do not extend it ambiguously. |
| New native `.hgs` | Explicit schema, metadata, sections and SH storage under engine control | Required runtime format. Optimize loading and validation without binding its on-disk ABI to a C++ struct or GPU layout. |
| Existing 32-byte `.splat` dialect | Small, simple, view-independent RGB/opacity; no higher SH | Required initial import, retaining Houseplant. Headerless input needs explicit dialect selection and strict length/semantic checks. |
| Graphdeco-style Gaussian `.ply` | Named attributes and full directional coefficients; verbose but useful interchange | Required initial import. Binary little-endian first, ASCII supported for tooling/fixtures; reject unsupported encodings explicitly. Schema validation is essential. [Reference producer](https://github.com/graphdeco-inria/gaussian-splatting/blob/main/scene/gaussian_model.py). |
| SPZ | Quantized compressed representation with directional SH; reference C++ decoder | Next compact import adapter. Current v4 uses independently Zstandard-compressed streams; v1–3 use gzip. Current source permits degree 4, beyond the proposed initial renderer. [SPZ specification](https://github.com/nianticlabs/spz/blob/main/README.md). |
| glTF/GLB with `KHR_gaussian_splatting` | Standardized Gaussian attributes and scene placement; baseline is ratified | Strategic scene-interchange adapter after the canonical model/scene contract. Extend the existing glTF tool; the presence of a glTF importer alone does not provide splat support. [Khronos specification](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_gaussian_splatting/README.md). |
| PlayCanvas compressed PLY / SOG | Different packed schemas; SOG v2 uses metadata, quantized image data and optional SH palettes | Optional offline adapters when actual assets require them. SOG can be bundled or separate files; it needs a dedicated decoder. [SOG specification](https://developer.playcanvas.com/user-manual/gaussian-splatting/formats/sog/). |
| KSPLAT and other viewer-specific containers | Packing/version conventions tied to their producers | Defer; use explicit offline conversion when needed. Do not define HARFANG's renderer around a viewer's file layout. [KSPLAT implementation](https://github.com/mkkellogg/GaussianSplats3D/blob/main/src/loaders/SplatBuffer.js). |
| Ordinary XYZ/RGB PLY, LAS, OBJ | No Gaussian covariance, opacity and directional appearance contract | Not Gaussian input. Point-to-Gaussian synthesis would be a separately named approximation/import operation. |

PLY is a container, not a sufficient format identifier. A compressed PLY, an ordinary mesh PLY and a Graphdeco PLY require different decoders; filename-based guessing is inadequate. The [PlayCanvas PLY documentation](https://developer.playcanvas.com/user-manual/gaussian-splatting/formats/ply/) also distinguishes the additional Gaussian properties.

SPZ support must identify accepted versions and model flags. Its reference library's unpacked `GaussianCloud` still represents scales logarithmically, alpha as logits, rotations as XYZW, and base color as DC coefficients; it is not the proposed runtime representation. Normalize exactly once in the adapter. Degree-4 data must be rejected by default until supported, or explicitly reduced to degree 3 with a report. Preserve antialiasing/training flags and reject unsupported rendering semantics in strict mode. [Reference data contract](https://github.com/nianticlabs/spz/blob/main/src/cc/splat-types.h).

The glTF adapter needs rendering conformance as well as attribute decoding: the checked baseline specifies camera-distance sorting, declared color domains with blending before transfer conversion, and display-referred appearance. These differ from a generic depth-sorted linear-PBR assumption. Implement a matching profile or label conversion as approximate. The existing [`gltf_importer.cpp`](../tools/gltf_converter/gltf_importer.cpp) ignores point primitives and has no Gaussian extension handling. Compression extensions must be qualified separately from the baseline. [Khronos rendering contract](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_gaussian_splatting/README.md#splat-reconstruction).

### 5.3 Initial source decoder contracts

For the bundled `.splat` dialect, retain the verified decoding: XYZ and **linear** XYZ scales as floats, byte RGB/alpha, and packed WXYZ quaternion normalized after decoding. Do not apply an exponential or sigmoid. Higher SH cannot be recovered from this representation.

For Graphdeco PLY, require named position, scale, rotation, opacity and DC fields. Interpret scale as logarithmic, opacity as a logit, rotation as WXYZ, and retain DC coefficients rather than prematurely converting them to clamped RGB. Numerically sort `f_rest_*` suffixes, validate contiguous complete bands, then transpose the source channel-major layout to the canonical coefficient-major RGB layout. These semantics belong to this dialect only. [Graphdeco model serialization and activations](https://github.com/graphdeco-inria/gaussian-splatting/blob/main/scene/gaussian_model.py).

For every decoder, validate property types, duplicate/missing names, element counts, stride/length arithmetic, offsets, finite values, positive supported scales and nonzero quaternion norm. Bound headers, auxiliary elements and list-property lengths. Safely skip understood irrelevant properties/elements, or reject unsupported structures with a diagnostic. Handle exponential overflow/underflow and stable sigmoid evaluation; validate converted values before allocating GPU memory. Report the property and record responsible for an error. Do not silently repair degenerate scales: future adapters for dialects permitting them need an explicit supported degeneracy/filter policy.

Do not normalize every asset to a two-unit height. Preserve authored placement/units by default; framing and tutorial normalization are explicit import options. Record transformations and source identity. Compute center bounds separately from conservative render bounds: Gaussian support extends beyond its center. Use the configured support threshold to derive useful bounds, while disclosing that an ideal Gaussian has infinite support.

### 5.4 Proposed native format evolution

Define a new magic/version rather than appending undocumented bytes to HGS0. The initial contract should include:

| Header/section information | Purpose |
|---|---|
| Magic, major/minor version, endian marker, header size, total byte length, required-feature flags | Detect incompatible files and distinguish safely skippable additions from required semantics. |
| Splat count, bounds and units/coordinate convention | Validate allocations, place instances and frame a camera without executing metadata. |
| Color encoding/image state, SH degree/basis/direction convention, kernel/filter and sort profile | Preserve rendering meaning across importers. |
| Section table with kind, encoding, offset, length, count and stride | Separate base Gaussian attributes from optional SH and future chunk/LOD sections. |
| Base data and optional higher-SH section | Degree-0 content does not pay for higher coefficients; coefficients remain available for quality changes. |
| Source/converter hashes and passive provenance metadata | Reproduce conversion and retain attribution; metadata is not Lua code. |

Serialize fields explicitly; do not dump a platform-dependent C++ struct. Use checked 64-bit file-size arithmetic even if the first runtime uses 32-bit indices. Validate section overlaps, alignment, payload lengths and unsupported required encodings. Use an explicitly defined integrity mechanism/version if checksums are included. Future optional sections may be skipped; unsupported required semantics must fail.

Prefer uncompressed FP32 sections for the first correctness baseline. File compression, FP16 or palette quantization can then be independently versioned and measured. Preserve source coefficients unless the user selects an explicit reduction policy. A versioned native format is a runtime cache and packaging choice, not a proposed replacement interchange standard.

### 5.5 Loading robustness and large assets

Check budgets before expanding compressed data and before upload. Report peak CPU decode/staging bytes, immutable GPU bytes and per-camera scratch separately. Compressed size says little about runtime cost. Build the new model completely before replacing an existing handle so a malformed hot reload does not destroy the current scene.

An initial synchronous load is acceptable when documented; add a cancellable read/decode/upload state machine for large/editor workloads later. Do not claim streaming merely because parsing reads chunks. Useful spatial streaming needs an index, chunk bounds, a residency/LOD policy, stable IDs, consistent ordering and a strategy for visible transitions. Independent sorted chunks still need correct composition where they overlap.

## 6. Spherical harmonics support

### 6.1 Preserve the data now; evaluate on the GPU

SH store view-dependent appearance, not scene lights or a complete relightable material. Degree 0 is constant color. Degree 3 contains 16 RGB coefficient vectors including DC. The current Houseplant `.splat` has lost its higher bands; a new renderer flag cannot recreate them. Use a separate known SH fixture and a real directional asset for acceptance.

For the initial canonical convention, use the real Graphdeco-compatible basis/order, RGB coefficient vectors and the camera-to-center unit direction. Evaluate:

```text
K = (degree + 1)^2
direction = normalize(mean - camera_position)       # in the SH basis frame
color = max(0, 0.5 + sum(coeff[k] * Y[k](direction), k = 0..K-1))
Y[0] = 0.28209479177387814
```

The reference evaluator clamps the final result below zero; it does not clamp every coefficient or impose a universal upper bound of one. Reversing the direction flips odd-degree terms. Record that sign convention explicitly and keep it independent of the quaternion used to orient the covariance. [Reference GPU evaluator](https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/forward.cu).

For legacy baked RGB, convert to `DC = (RGB - 0.5) / Y[0]`, with no higher coefficients, or retain an explicitly tagged baked-color representation through the compatibility adapter. Never reinterpret HGS0 RGB bytes/floats as DC. Use independent reference values from the [SH basis utilities](https://github.com/graphdeco-inria/gaussian-splatting/blob/main/utils/sh_utils.py) for CPU/GPU conformance.

### 6.2 Placement in the rendering pipeline

Evaluate SH after cheap visibility rejection in the projection pass and write one RGB result into the existing projected color lane. Gather/sort only IDs and evaluated attributes; leave source coefficients immutable and in original order. The quad fragment shader still evaluates Gaussian coverage and premultiplies color by alpha. This avoids repeated SH work for four vertices or many covered pixels and does not enlarge the final color atlas.

Use degree-specialized shader variants for 0/1/2/3, or another measured strategy with a bounded coefficient loop. `max_sh_degree` limits evaluation to `min(stored_degree, requested_degree)`; retain the stored degree in metadata. Reducing evaluation cost does not free loaded higher-band memory unless an explicit load/compile policy discarded it.

Color invalidates when eye position relative to the model, model orientation, coefficients or quality settings change. A pure camera rotation at the same eye changes projection/sorting but not each fixed splat's SH direction. Cache only after correctness is established and with separate invalidation for projection, sorting and color. Each eye of a stereo camera needs appropriate evaluation.

### 6.3 Transforming the angular basis

For a rigid instance rotation `R`, evaluate the original coefficients at `transpose(R) * direction_world`; this is equivalent to rotating the SH function with the object, without rewriting every coefficient every frame. Translation affects the eye-to-center direction; positive uniform scale does not rotate the angular basis. The per-Gaussian covariance quaternion is generally **not** the SH basis rotation.

Offline coordinate changes must transform positions, covariance and the angular basis consistently. Either rotate/reflect coefficients into the chosen canonical basis or retain an orthogonal basis transform and apply its inverse to directions. For a reflection, a quaternion alone cannot represent the basis change; axis/sign transformations need a tested full orthogonal mapping. Do not both transform coefficients and transform directions a second time.

Nonuniform scaling/shear changes geometry but does not determine a unique physical deformation of captured radiance. A general inverse affine direction map and a rotation-only SH policy give different results. Therefore restrict the first runtime transform domain as specified above. Add positive nonuniform scale with an explicit rotation-only appearance policy and its conformance tests; do not silently treat that as an exact radiance-field deformation. Source importers must also handle their own coordinate conversion exactly once.

### 6.4 Memory and precision

The following are calculated payload sizes, not GPU measurements. A conservative GPU sidecar uses one float4 per non-DC RGB coefficient to reuse the currently proven typed-buffer path; the fourth component is padding.

| Maximum degree | RGB coefficient vectors including DC | Higher-band scalar floats | Packed FP32 higher-band bytes/splat | Float4 GPU sidecar bytes/splat |
|---|---:|---:|---:|---:|
| 0 | 1 | 0 | 0 | 0 |
| 1 | 4 | 9 | 36 | 48 |
| 2 | 9 | 24 | 96 | 128 |
| 3 | 16 | 45 | 180 | 240 |

With an initially compatible 64-byte base GPU record, SH3 source storage becomes 304 bytes/splat. For 113,648 splats the sidecar adds about 26.01 MiB; adding it to the current 28.64 MiB baseline gives about 54.65 MiB before any other redesign costs. At 1,048,576 splats, the current SH0 payload formula gives 252 MiB and this SH3 sidecar adds 240 MiB. These are single-camera working-set illustrations, excluding driver overhead, meshes, framebuffers, instances and upload staging.

Sharing source data avoids duplicating coefficients per camera, but projection/sort/gather scratch still grows with outstanding cameras and total instances. Repeated instances can therefore be cheap to store yet expensive to render. Current compute buffers are capped at 256 MiB per allocation and quad generation at 1,048,576 splats; a robust planner must acknowledge those limits or replace them with tested paging/chunk strategies.

FP16, tight packing into float4 lanes and quantized SH can reduce bandwidth, but each needs a defined decoder, backend validation and image-error measurements. Keep FP32 as the conformance reference. File compression alone does not shrink the decoded GPU sidecar. SH support is chiefly an additional attribute-bandwidth/ALU cost; it does not inherently require a different sorting algorithm or draw primitive.

## 7. Implementation stages and acceptance gates

These stages prioritize the intended mixed-scene feature. A direct C++ transcription of the tutorial is not a milestone by itself.

| Stage | Native work | Exit evidence |
|---|---|---|
| A — contracts and CPU data | Canonical model, new native format, HGS0 compatibility, checked readers/importers, bounds, diagnostics and budgets; SH degree/basis reserved and preserved | Malformed/truncated/overflow fixtures fail before allocation; deterministic source conversions; headless CPU tests; no executable metadata. |
| B — multi-instance native renderer | Immutable resources, explicit render data, instance transforms, common projection and global sort, native scheduling, lifecycle and FabGen | Two overlapping transformed splat models match an independent combined ordering oracle; native and Lua calls agree; allocation/view exhaustion leaves clean state. |
| C — required mixed-scene integration | Correct projected depth, opaque/alpha-tested mesh integration, color policies, compact view schedule and separate camera scratch | Near/behind/intersecting opaque plane scenes; two cameras in one frame; shared framebuffer depth preserved; resize/reset/destruction pass; declared approximation behavior visible. |
| D — directional appearance | Degree 1–3 GPU evaluation, basis/coordinate conversion, metadata and quality controls | Known coefficients/directions, odd-degree sign checks, rotations/reflections, zero-higher-band equivalence to SH0 and real SH asset comparisons. |
| E — portable product path | Engine shader packaging, native docs, backend probes, measured radix path where needed; tutorial rewritten as a client | Metal and D3D11 qualification of the mixed scene and lifecycle; memory/frame-time reports for several counts and cameras; small tutorial with no pass-management code. OpenGL separately qualified. |
| F — broader content/integration | SPZ versioned import, glTF Gaussian scene import/profile, scene component/cache/serialization, optional additional formats | Fixtures per accepted dialect/version and explicit unsupported-feature errors; scene placement and appearance preserved under documented policies. |
| G — scale and advanced composition | Streaming/LOD, improved depth model, arbitrary transparent meshes, full AAA support | Dedicated quality/performance/overflow criteria. No inference of support from earlier stages. |

Stages C and D may progress independently once the common data/submission contracts exist. **Stages B, C and D define the intended first native feature; stage E qualifies and exposes it.** Importing SH-capable content should not wait until after a baked-RGB ABI has been frozen. Do not claim multi-camera usability while the only available schedule exhausts the view budget.

Recommended source organization: `engine/gaussian_splat_data.*` for validated CPU data/readers; `engine/gaussian_splat.*` for native resources and submission; shared importer code used by an offline tool; engine shader resources outside tutorial-only content; additions to FabGen and `doc/doc`; a focused scene-pipeline adapter. These are proposed locations, not files created by this study.

## 8. Validation that distinguishes a robust feature from a viewer

Reuse the existing [sort probe](../harfang/tests/run_gaussian_sort_probe.py), [pipeline oracle](../tutorials/tools/check_gaussian_pipeline.py), converter fixtures and [compute probe](../harfang/tests/COMPUTE_PROBE.md), then extend their scope:

| Area | Necessary adversarial checks |
|---|---|
| Multi-model ordering | Interleaved red/green splats from different models, duplicate depths, repeated instances, reversed camera, deterministic IDs, non-power-of-two counts, all-invisible input. Compare the full combined permutation to an independent CPU oracle. |
| View/resource scheduling | Two cameras and targets in one frame; compact schedule positive and missing-producer negative controls; custom view order; boundary/exhausted reservations; repeated resize/reset; delayed GPU completion and stale prepared tokens. |
| Mesh composition | Splats in front of and behind an opaque plane, alpha-cutout coverage, large ellipses intersecting meshes, viewport offsets, offscreen targets, depth convention and sample count. Assert expected pixels and known approximation boundaries. |
| Transforms/projection | Positive uniform transforms, then qualified nonuniform transforms; off-axis perspective; large coordinates; near/far clipping; rejection of unsupported transforms/projections. Orthographic rendering is a deliberate later native extension, not an accidental use of perspective math. |
| SH | Degrees 0–3; basis impulses for each coefficient; axes and oblique directions; channel reordering; camera translation versus rotation; rigid object rotation; offline reflection; negative and above-one reconstructed values; active-degree truncation. |
| Loading | Empty/bad/huge counts, duplicate PLY properties, malformed bands, unsupported dialect/endian/version, truncated compression, overlapping native sections, decompression limits, NaN/Inf, zero quaternion, failed replacement and complete error diagnostics. |
| Ownership/bindings | Forced GC between load/prepare/draw, copied instances, explicit destroy then GC, surviving userdata after shutdown, renderer recreation, failed allocation cleanup and no duplicate release. |
| Performance | Separate CPU submission, asynchronous GPU timing, immutable GPU storage, per-camera scratch, peak CPU upload and actual draw/dispatch/view counts. Compare SH0–3 and multiple instances/cameras on named hardware. |

Keep negative controls and numerical oracles: a plausible screenshot cannot establish sort correctness, SH sign correctness or absence of stale camera data. Preserve the existing tutorial image/appearance as one compatibility test, while accepting deliberate differences only under an explicitly selected rendering profile.

## 9. Decisions to carry into implementation

Adopt the native resource/instance/per-camera split, global sorting domain, native validated runtime asset, SH-aware canonical representation and explicit mixed-scene composition contract. Keep source formats out of renderer internals. Make opaque/alpha-tested mesh coexistence and two cameras acceptance requirements. Treat transparent mesh interpenetration and full AAA behavior as separately specified capabilities.

Resolve the exact compact scheduling strategy, radix crossover, source/GPU packing and performance limits through implementation probes. Those are internal engineering decisions; they should not enlarge the Lua API or force application scripts to track hundreds of passes. The eventual tutorial should demonstrate scene use, camera interaction and quality controls, with the engine owning rendering correctness and resource management.

## 10. Duo review provenance

Codex performed the repository audit, checked the external primary sources, calculated payload budgets and wrote the integrated study. The requested second opinion was obtained as task `GSPLAT-NATIVE-STUDY-001` through GitHub Copilot CLI **1.0.103**, using **`deepseek-v4.1-flash:cloud`** through the local Ollama provider. Source excerpts and an architecture brief were supplied. The successful invocation returned exit code 0 and a substantive architecture report; tool availability was restricted to read/search operations. Raw prompts, output and runner logs remain outside the repository in the workspace's temporary review directory.

Accepted after checking the code: the compute-before-draw ordering caveat; whole-submission view-budget preflight; separate camera scratch; projection into shared ranges followed by global instance sorting; replacing constant depth for mesh integration; explicit SH basis/sign/storage; and decoded-memory and lifecycle accounting. The study also incorporates the user's subsequent clarification that a robust mixed-scene engine feature, rather than tutorial translation, is the objective.

Corrections and limits: HGS0 stores float32 RGB, not byte RGB; the HARFANG repository is the `harfang3d` subdirectory, whose revision Codex verified; absence of an explicit backend barrier alone does not prove a visibility failure; universal linear compositing does not preserve every trained appearance. The review's additional camera/scene/view-plan wrapper types and single-camera extraction-first milestones were not adopted as requirements. Existing `ViewState`, a vector of instances and explicit requirements/settings keep the proposed surface smaller, with multi-instance/mixed-scene behavior in the core acceptance gates. A later CLI follow-up returned no review content and is not counted as additional validation. Neither the review nor this study establishes new runtime or backend support.
