# Gaussian Splatting Backend Portability: Windows and macOS

Status: implementation study and proposed contracts, not an implementation claim.

Date: 2026-11-09. Source baseline: `f2a805b`, plus the working-tree shader dependency path normalization in `assetc`. Review: Codex with Copilot/Kimi. The study was conducted on Windows; no macOS GPU execution was performed for this document.

## 1. Decision and immediate delivery objective

Keep one native Gaussian implementation, one reference shader source set in `core-package/core/gaussian_splatting`, and one Lua application source. Build and package the runtime and compiled assets for the destination platform automatically.

The immediate release target is **Windows/D3D11 and macOS/Metal**, with identical application Lua and HGS2 files in both packages. “DX” in this first milestone means D3D11; the presence of a D3D12 enum or compiler branch is not D3D12 qualification.

The existing native design is suitable for this objective. A separate Metal splat implementation or an OS-specific Lua rendering path is not justified by the inspected code. The main work is to make renderer selection, shader compilation, packaging and GPU qualification agree on the same target.

Recommended sequence:

1. Qualify the current native renderer on a real Mac, using Metal assets and the unchanged existing tutorial.
2. Make build-selected defaults an explicit shared contract between `RenderInit`, `assetc` and application packaging. Automate the complete build/compile/package/test sequence.
3. Add an external renderer override for applications using automatic initialization; qualify Windows/OpenGL separately.
4. Add packages containing several backend variants only when an application needs to switch backend without rebuilding its package.

The first two steps must not wait for step 4. A platform-specific package with a stable `resources_compiled` layout already meets the Windows-to-Mac requirement.

## 2. Meaning of “unchanged Lua”

The application must keep the same model paths, renderer/model creation calls, instance data, camera calculation and `SubmitGaussianSplats` calls on both platforms. No Lua changes to sort passes, bindings, texture origins, shader profiles or asset suffixes are acceptable when changing platform.

Native binaries and GPU shader payloads are platform-specific. Unchanged Lua does **not** imply that a Windows distribution directory can run unchanged on macOS. The deployment operation replaces its native runtime and backend-dependent assets automatically.

The current [`tutorials/gaussian_splatting.lua`](../tutorials/gaussian_splatting.lua) already selects D3D11 on Windows and Metal otherwise. Its source can therefore be used unchanged for the immediate Windows/Mac qualification. However, this OS test is not a general default policy: it also selects Metal on Linux and prevents an external OpenGL override if explicit renderer arguments retain their meaning.

For the long-term reference tutorial, make one upstream simplification to use automatic native initialization. Thereafter distribute exactly that source on every supported platform. Applications already using the no-renderer `RenderInit` overload need no corresponding source change.

An application explicitly requesting `RT_Direct3D11` on every platform cannot both preserve that request and become a Metal application without a migration. Do not silently reinterpret explicit renderer enums. This is a compatibility boundary, not a reason to introduce OS branches into new Lua code.

## 3. Current implementation: evidence and gaps

| Area | Observed behavior | Consequence |
| --- | --- | --- |
| Build default | [`CMakeLists.txt`](../CMakeLists.txt) sets `HG_GRAPHIC_API` to `DX11` for MSVC, `MTL` for Apple, otherwise `GL`, unless supplied. | A useful existing default, but selected using different conditions from BGFX runtime selection. Make Windows targeting explicit rather than equating it with the MSVC compiler. |
| Asset compiler default | [`assetc.cpp`](../tools/assetc/assetc.cpp), `api` and `ASSSETC_DEFAULT_PLATFORM`, use `HG_GRAPHIC_API` and compile-time platform macros. `-api` and `-platform` already exist. | A native Mac `assetc` normally chooses `MTL`/`osx` without command-line options. Cross-target compilation must pass the destination platform, not infer it from the machine executing the tool. |
| Runtime default | [`render_pipeline.cpp`](../harfang/engine/render_pipeline.cpp), `RenderInit`, leaves `bgfx::Init::type` at its BGFX default for HARFANG's no-renderer route. It does not resolve `HG_GRAPHIC_API`. | Current DX11/Metal defaults coincide with BGFX preferences, but `-DHG_GRAPHIC_API=GL` alone does not guarantee an OpenGL runtime. |
| BGFX fallback | Vendored [`bgfx.cpp`](../extern/bgfx/bgfx/src/bgfx.cpp), `rendererCreate`, scores the requested renderer first, then tries alternatives. | Even a requested type must be checked against the actual renderer after initialization. |
| Lua surface | [`bind_harfang.py`](../binding/bind_harfang.py) exposes renderer-explicit and renderer-implicit `RenderInit` overloads and native Gaussian APIs. | No platform-specific Gaussian binding is needed. Avoid adding another ambiguous integer overload merely to express automatic selection. |
| Gaussian programs | [`gaussian_splat.cpp`](../harfang/engine/gaussian_splat.cpp), `CreateRenderer`, loads four compute programs and one graphics program from the supplied root. | All six compiled shader stages must match the active renderer. The `.sc` suffix of a compiled compute resource does not mean portable source. |
| Program loading | `LoadProgram` and `LoadComputeProgram` open the supplied paths directly; graphics stages use `.vsb` and `.fsb`. | There is no automatic DX11/Metal variant lookup today. File and asset-provider routes both need correct packaged inputs. |
| Portable data | [`gaussian_splat_data.cpp`](../harfang/engine/gaussian_splat_data.cpp) implements HGS2 and source decoding; GPU resources are created separately. | HGS2 remains backend-independent. There is no reason to create an HGS-Metal variant. |
| Compute capabilities | [`compute.cpp`](../harfang/engine/compute.cpp), `GetComputeCaps`, reports compute support, RGBA32F usage and limits. Metal image-write reporting is explicitly marked incomplete. | A missing Metal format bit is currently tolerated, which is not proof of support on every Metal device. |
| Native projection | Gaussian projection validates the BGFX depth convention and passes explicit matrix rows and `u_depth_range` to shaders. | This avoids a Lua matrix-layout workaround; correctness still needs GPU tests on both backends. |
| View scheduling | Native submission defaults to compact scheduling: one compute view and one sequential graphics view. | Dependent compute dispatches must remain ordered within one view; adding a view per dispatch is not a scalable portability fix. |
| Window dimensions | The tutorial uses framebuffer pixel dimensions for rendering and client dimensions for input conversion. | Preserve this distinction on Retina displays; do not add a fixed macOS scale factor. |
| Incremental compilation | The working-tree fix canonicalizes shader dependency paths before storing them. Shader build contexts currently contain profile, optimization flags and defines, but not a complete target/toolchain identity. | Retain the path fix and expand cache keys before sharing caches between configurations or toolchain revisions. |

### 3.1 Evidence collected for this study

Using the bundled Windows `assetc` and `shaderc`, a temporary source tree containing the reference Gaussian shaders, BGFX includes, tutorial gizmo and both ImGui programs was compiled with `-api MTL -platform osx`.

Result: **12 shader stages compiled, 14 total outputs including two copied headers, zero failures**. The Gaussian stages were `clear`, `project`, `sort`, `gather`, `splat_vs` and `splat_fs`. No tutorial or reference shader was edited for this experiment.

Reproduction shape, with a separate output directory:

```text
assetc -api MTL -platform osx <staged-source> <separate-metal-output>
```

Local evidence is under `C:/works/dev/harfang/.codex_tmp/gsplat-portability-study/compile-metal.log`. This temporary path is not a repository or CI dependency.

The three shader dependency regression tests also passed with `--api MTL --platform osx`: content-hash caching, timestamp-based caching, and guarded include cycles with path aliases. These tests execute real compiler processes but do not execute Metal GPU work; their log is `incremental-metal.log` in the same evidence directory.

This establishes source translation by the pinned toolchain, **not successful Metal pipeline creation, synchronization or rendered pixels**. The vendored Metal backend calls `newLibraryWithSource`; shaderc output contains generated Metal source and reflection data, rather than a universally validated, ready-to-execute Apple GPU binary. Compilation on Windows therefore cannot replace execution on macOS. See [`shaderc_metal.cpp`](../extern/bgfx/bgfx/tools/shaderc/shaderc_metal.cpp) and [`renderer_mtl.mm`](../extern/bgfx/bgfx/src/renderer_mtl.mm).

The earlier [Metal reference results](GSPLAT_METAL_REFERENCE_RESULTS.json) are encouraging evidence for the previous plant workload. They identify an older revision and must not be presented as qualification of the current native HGS2/SH3/multi-instance implementation.

## 4. One backend policy, owned by native code

### 4.1 Build default

Retain `HG_GRAPHIC_API` as the public build setting and define a validated mapping shared by the runtime, tools and packaging:

| Build target | Default asset API | Runtime renderer | shaderc platform | Initial qualification |
| --- | --- | --- | --- | --- |
| Windows | `DX11` | `Direct3D11` | `windows` | Required |
| macOS | `MTL` | `Metal` | `osx` | Required, Apple silicon first |
| Windows with explicit GL configuration | `GL` | `OpenGL` | `windows` | Later compute-capable GL qualification |

Validate the setting at configure time; reject unknown names and impossible platform/backend combinations. Generate one native configuration description rather than duplicating string-to-enum mappings in C++, Lua and CMake scripts. Distinguish the **default renderer** from the **renderers compiled into BGFX** and from **device support**.

The no-renderer `RenderInit` overloads resolve this default explicitly. Preserve the existing HARFANG behavior that uses `RT_NoOp` internally as the automatic sentinel; do not accidentally turn existing applications into headless/no-op rendering. A future genuine no-op API is a separate compatibility decision.

After BGFX initialization, compare the actual renderer with the resolved selection. For a package built for one backend, a mismatch is an initialization error with cleanup and a useful diagnostic. Do not continue with another renderer and load the original backend's shaders. A future fallback list is only valid if each alternative has matching packaged assets and satisfies the application's requirements.

### 4.2 Overrides without Lua edits

Proposed precedence, highest first:

1. An explicit renderer argument passed by the application to `RenderInit`.
2. An explicit launcher command-line selection, if supplied.
3. A process environment selection, proposed name `HG_RENDERER`.
4. A launcher configuration selection, proposed field `runtime.renderer`.
5. The default generated from `HG_GRAPHIC_API`.

`auto` means continue to the next source of policy. Invalid values fail visibly rather than falling back to another spelling or backend. Read policy before rendering starts; changing an environment variable during a frame loop must not switch renderer or invalidate live handles.

The environment mechanism belongs in native initialization so it also works when an application is started with the plain Lua interpreter. The launcher can supply configuration before running the script. The existing [`LauncherRuntimeConfig`](../languages/launcher_app_common.h) currently contains HiDPI configuration, not a renderer setting: extending it is proposed work.

This policy permits an unchanged automatic Lua application to run with Windows/OpenGL later. It intentionally does not override a script that explicitly requests D3D11. Report the selected renderer and its selection source once at startup.

Useful proposed introspection includes the build default, actual renderer, compiled backend set and a structured Gaussian capability result. Keep these optional for diagnostics; ordinary Lua rendering should not need to read them and branch.

### 4.3 Stable Lua-facing behavior

The existing window-based API already expresses automatic initialization without a new enum or overload:

```lua
hg.InputInit()
hg.WindowSystemInit()
local win = hg.NewWindow(1280, 720)
hg.RenderInit(win)

local renderer = hg.CreateGaussianSplatRendererFromFile(
	"resources_compiled/core/gaussian_splatting")
local loaded = hg.LoadGaussianSplatModelFromFile(
	"resources_compiled/gaussian_splatting/source/plant.hgs")
```

This excerpt shows the existing API shape, not a complete application. The proposed change is how native automatic initialization resolves its default. Creation, submission and resource ownership remain the same on both platforms.

## 5. Assets and packaging

### 5.1 Immediate solution: one backend per distribution

Produce independent Windows and macOS application distributions with identical logical paths:

```text
application/
  application.lua                   # identical source hash
  <native Lua runtime and module>    # target OS and architecture
  resources_compiled/
    core/gaussian_splatting/
      clear.sc project.sc sort.sc gather.sc
      splat.vsb splat.fsb
    core/shader/                     # target ImGui and scene programs too
    gaussian_splatting/source/plant.hgs
    gaussian_splatting_native/saari-probe.hgs
    ...
```

Compile **all shaders used by the application**, including ImGui, gizmo, conventional models and post-processing, for the same target. Replacing only the six Gaussian stages is insufficient.

Use one output/cache directory per target and configuration in the build tree. Package the selected directory under the stable runtime name `resources_compiled`. Never compile Metal over the working DX11 tutorial output directory as part of a cross-compilation test.

For an immediate move to a Mac, either rebuild source resources with the Mac `assetc` defaults, or deploy a complete Metal-compiled package prepared by the build pipeline. Existing HGS2 files can be copied unchanged. Do not copy the Windows `assetc.cab` as authoritative state for a fresh Mac asset build.

The existing `FromFile` tutorial works with this layout. `FromAssets` clients also work when the same logical asset namespace is mounted. Do not require an application-wide conversion from file loading to mounted assets just to run on macOS.

### 5.2 Automate the whole dependency chain

Add a reusable CMake application-assets helper and use it for a proposed `gsplat_tutorial_package` target. These names are proposals; the target does not exist yet.

The target should:

1. Build or locate the target Lua runtime, module, launcher and required host tools. Reuse the existing `assetc` toolchain installation, which includes `gsplatc`, `shaderc` and texture tools.
2. Stage `core-package/core` as the canonical core input, then stage application resources. Fail on unexpected conflicting core copies; reuse `tools/sync_core_package.cmake` to check the tutorial mirror.
3. Resolve destination OS, architecture, backend and build profile once. Pass the same result to asset compilation and runtime packaging. A cross-build requires executable **host** tools, not target binaries that cannot run on the build machine.
4. Invoke `assetc` in a separate generated directory. For native builds, its defaults must agree with the generated configuration; for cross-target builds, pass explicit `-api` and `-platform`.
5. Verify every required output exists and is nonempty. Some current missing-tool branches warn and skip work, so exit status alone is not sufficient as a packaging gate.
6. Write a small versioned build manifest recording target/backend, engine and shaderc identities, profiles, input hashes and required output hashes. Publish it only after successful compilation and validation.
7. Stage a runnable distribution, including native dependencies, a stable working directory or equivalent path resolution, and the original Lua source. On macOS validate architecture, dylib/rpath resolution and executable permissions from a clean installation rather than a developer shell.
8. Run the platform's smoke test and produce a machine-readable report. A second build with unchanged inputs must not invoke shader compilation.

Use CMake and native tools for the production build/package path. Python can remain in developer verification; do not restore the removed Python Gaussian converters or make Python a runtime requirement.

Use a Windows builder for the DX11 package and a macOS builder for the Mac native runtime. The successful Windows-to-Metal shader translation does not imply symmetric support for building D3D shaders on macOS: the pinned HLSL compiler path is Windows-specific. Cross-compiling shader payloads and building executable runtimes are separate capabilities. [BGFX shader build documentation](https://bkaradzic.github.io/bgfx/tools.html#building-shaders).

For reproducible CI, use the redistributable plant and small generated SH/multi-instance fixtures. Saari's source is local/ignored and may not exist on a clean clone. A package that promises the current two-model tutorial must require an explicitly supplied Saari asset and report its absence clearly; it must not silently publish an incomplete tutorial. Record the asset's content hash rather than baking `C:/works/...` into build logic.

### 5.3 Cache identity and diagnostics

Extend shader compilation cache identity to include the canonical API, destination platform, stage/profile, defines, optimization/debug settings, compiler identity and dependency contents. Preserve canonical dependency paths. Changing backend, platform or shaderc revision must invalidate affected shader outputs; changing nothing must be a no-op.

Separate reusable HGS2 conversion results from backend-dependent shader outputs where practical. A renderer override does not imply that models need new quantization or different SH coefficients. Geometry and texture compilation must retain their own format/profile keys; do not assume every non-shader asset is universally interchangeable.

Initially, packaging and launcher validation should check the build manifest against the resolved runtime target. Plain-Lua/direct-file deployments must receive the same validated target-specific package; launcher-only validation cannot be claimed as protection for those applications. Improve program creation errors to include the resource path and actual renderer for all loader routes.

Do not invoke `assetc` implicitly in a shipped application's render loop or silently search for Windows shader payloads after a Metal load fails. Compilation is a build/development operation, and deployment errors should be actionable.

### 5.4 Later: several backend variants in one package

For Windows users selecting DX11 or GL at launch, begin with two separately built packages selected by a launcher/build preset. This meets the unchanged-Lua objective with minimal loader changes.

If a single package must contain both, extend the **generic shader loading layer**, not only `CreateGaussianSplatRenderer`. Resolve logical program names using the actual renderer and package manifest. The rule must cover compute programs, both graphics stages, pipeline variants, ImGui and custom shaders, through both file and asset providers.

Possible layout:

```text
resources_compiled/
  manifest.json
  common/...
  backends/DX11/...
  backends/GL/...
```

The layout is a future physical storage choice; it must not appear in application Lua. Define an explicit root registration/discovery contract for direct-file readers as well as mounted assets before implementing it. Merely adding an assets-folder overlay does not redirect `FromFile` calls. Cache resolved programs by renderer and package identity, validate that all stages belong to the same target, and fail when the selected variant is absent. Do not transparently fall back across backend payloads.

## 6. Qualifying and hardening the native Metal route

### 6.1 Required capabilities

The current route requires compute dispatch, float4 structured buffers, at least five compute binding stages, RGBA32F writable textures, RGBA32F vertex sampling, sufficient texture dimensions and available views. The gather pass reads two buffers at stages 0/1 and writes three images at stages 2/3/4. Other passes use 64-thread workgroups; gather uses 8 x 8 threads.

Keep these requirements in native preflight and expose a structured failure reason. Unsupported capability, failed program creation, invalid input and allocation exhaustion should be distinguishable. A successful handle allocation is not evidence that a GPU pipeline eventually executed successfully.

The current blanket exception for incomplete Metal image-write reporting needs a bounded qualification policy. First verify the precise format path on the supported Mac hardware. Prefer fixing the vendored format reporting when it can be derived accurately; otherwise retain a documented, tested compatibility exception with its limits. Do not remove checks for all Metal devices on the assumption that the renderer name guarantees the operation.

A developer qualification probe can test dispatch-to-image-to-vertex-sampling behavior. It must not add hidden `Frame` calls, readback stalls or renderer reinitialization to ordinary `SubmitGaussianSplats` calls.

### 6.2 Shader ABI and numerical behavior

Keep the shared BGFX `.sc` sources and float4 buffer layout. Validate translated Metal resource bindings and uniform reflection, including unused SH0 buffers and SH3 input. The vendored Metal backend reserves buffer slot 0 for uniforms and maps compute buffer stages to `stage + 1`; application code must continue using BGFX stages, not Metal-specific indices.

Compare projection, covariance, near/far rejection, off-axis perspective, opacity and SH evaluation on both platforms. Explicit matrix rows in the current shader interface should remain authoritative. Do not add a Metal-only transpose in Lua.

Use deterministic IDs and synthetic depths to test the total-order sort exactly. For projection and final pixels, use explicit numerical tolerances and semantic checks rather than requiring bit-identical floating-point images across GPU vendors. Quantization-error checks and cross-GPU rendering tolerances are different tests.

### 6.3 Ordering and synchronization

Required dependency chain:

```text
clear keys -> project all instances -> bitonic sort passes -> gather atlases
           -> draw sorted splats -> following application views
```

Qualify both compact and expanded schedules, with compact scheduling the production default. Test repeated dispatches in the same view, ping-pong key buffers, compute-to-texture visibility and subsequent vertex sampling. View count alone does not establish a memory-ordering guarantee.

The inspected Metal backend uses a compute encoder for consecutive compute work and ends encoding when switching to rendering. Check the pinned implementation and actual resource behavior before adding synchronization. If a correctness defect is reproduced, fix the native/BGFX layer; do not insert Lua flushes, extra frames or CPU sorting as a platform workaround.

Preserve the local Metal depth-attachment fix and 32 MiB uniform-ring mitigation documented in [`extern/bgfx/VENDORED.md`](../extern/bgfx/VENDORED.md). The latter is a larger fixed capacity, not an overflow guard. Mixed scenes and multiple cameras still need stress tests and ideally diagnostic bounds checks. The historical [Metal command-stack note](SPECS_BGFX_METAL_COMMAND_STACK_WEAKNESS.md) contains evolving hypotheses; the vendored source is authoritative for this study. A broad BGFX upgrade is not a prerequisite unless qualification demonstrates a blocker that warrants one.

### 6.4 Coordinate conventions, Retina and mixed geometry

Keep using BGFX's `homogeneousDepth` for projection/depth conventions. Validate framebuffer origin separately. The Gaussian atlases are indexed data textures, not images to flip by OS convention: do not apply an unconditional Y flip. Test the complete image-write/sampling path, render-target composition and screenshot orientation before deciding where a conversion belongs.

On Retina, use drawable pixels for projection, viewport, targets and reset; preserve client-coordinate conversion for input and ImGui. Exercise resize, minimization/restore and display-scale changes.

For opaque mesh composition, render opaque color/depth first, preserve the target depth attachment, then draw splats with depth testing and without clearing or writing depth. Verify on a shared offscreen target and the backbuffer, with and without MSAA. Several Gaussian instances that need mutual transparency ordering must enter the same native submission for global sorting.

This milestone does not promise physically exact intersections between transparent meshes and Gaussian volumes. Preserve the current center-depth approximation and document ordering limitations rather than letting the Metal port change rendering semantics.

## 7. Preparing Windows/OpenGL without delaying macOS

The future override needs more than selecting `RT_OpenGL`:

- Compile the GL backend into the runtime and verify device compute/image/buffer support.
- Correct the compute shader profile policy: `BuildComputeShader` currently uses `130` for GL. The vendored shaderc compute path emits the requested GLSL version, so this is not a valid production compute profile merely because assetc exits successfully. Use an appropriately qualified compute profile, initially GLSL 430, without unnecessarily raising every existing graphics shader's minimum.
- Compile GL shader variants with `-platform windows` when targeting Windows. The current generic compute runner associates GL with Linux; decouple backend from destination OS in the runner.
- Qualify depth range, texture origin, image/SSBO barriers and vertex sampling on real Windows GL drivers. Extend the native Lua and sort probes, which currently only accept DX11/Metal selections.
- Reject an unavailable backend or missing variant with a clear error. Keep DX11 as the default Windows package until GL passes the same acceptance gates.

OpenGL compute shaders are core in OpenGL 4.3; this is a distinct baseline from traditional raster-only GL support. [Khronos compute shader reference](https://wikis.khronos.org/opengl/Compute_Shader).

macOS OpenGL is not the fallback target for this compute pipeline: Apple's available core profile is 4.1 and its OpenGL API is deprecated. Use Metal for the immediate Mac path. [Apple OpenGL profile documentation](https://developer.apple.com/documentation/appkit/nsopenglprofileversion4_1core?language=objc), [GLFW macOS context documentation](https://www.glfw.org/faq.html).

## 8. Automation and acceptance tests

Extend the existing developer probes rather than maintaining a second renderer just for tests:

| Existing entry point | Required use or extension |
| --- | --- |
| [`run_compute_probe.py`](../harfang/tests/run_compute_probe.py) | Qualify C++ and Lua buffer/image ordering, including existing negative controls. Keep compile-only results distinct from runtime passes. |
| [`run_gaussian_sort_probe.py`](../harfang/tests/run_gaussian_sort_probe.py) | Compare every sorted ID for the 131,072-slot fixture on DX11 and Metal; retain ties and invalid entries. |
| [`test_gaussian_splat_native.py`](../harfang/tests/test_gaussian_splat_native.py) | Run HGS2 conversion/loader checks and GPU tests for both quantized and float32 storage, including SH and mixed-mesh cases. Supply mesh shaders so composition is actually exercised. |
| [`gaussian_splatting_runner.lua`](../harfang/tests/gaussian_splatting_runner.lua) | Run the actual unchanged tutorial, test preloading, model selection, SH selection, resizing, garbage collection and shutdown. |
| [`test_shader_dependencies.py`](../tools/assetc/tests/test_shader_dependencies.py) | Run incremental dependency tests with `--api DX11 --platform windows` and `--api MTL --platform osx`; add target/toolchain-switch invalidation when cache keys are extended. |
| [`sync_core_package.cmake`](../tools/sync_core_package.cmake) | Keep canonical core shaders and the tutorial mirror identical. |

The Duo review identified three gaps that must be closed before calling these tests a complete qualification:

- The full 131,072-slot sort probe currently dispatches through separate views. The native probe compares compact and expanded scheduling on a small case. Add a large deterministic compact-mode test, including gather, and compare its result with the independent expected order and the expanded route.
- Assert the **actual** initialized renderer in native Gaussian and sort probes. A requested renderer string in a report is insufficient because the pinned BGFX implementation can fall back.
- The tutorial runner's screenshots and successful submissions are not an independent image oracle. Add image-content checks for both models and deterministic pixel/region checks for a synthetic mixed scene; retain captures for human review.

The top-level qualification target should discover tool/module paths for the selected build configuration, prepare inputs, compile shaders, execute probes and collect results. It should not require the user to manually construct long `--lua`, `--module`, `--shaders` and toolchain paths on each OS.

Required acceptance matrix:

| Check | Windows DX11 | macOS Metal |
| --- | --- | --- |
| Automatic renderer equals the build/package selection | Required | Required |
| Same application Lua and shared HGS2 content hashes | Required | Required |
| Cold compile, warm no-op, include edit and compiler/target change | Required | Required |
| Native shader creation and compute producer/consumer checks | Required | Required |
| Exact synthetic global sort; transformed overlapping instances | Required | Required |
| SH0 and SH1–3, including quantized and float32 HGS2 | Required | Required |
| Mesh depth, offscreen composition, resize and multiple cameras | Required | Required |
| Compact schedule, lifetime/GC and orderly shutdown | Required | Required |
| Actual tutorial, ImGui/gizmo and both supplied models | Required for the two-model package | Required for the two-model package |
| DPI/input consistency and window restore | Required | Required, including Retina |

Run GPU qualification on machines with actual usable graphics sessions. Do not count a headless CI job that skipped GPU execution as a pass. Start with one Apple silicon Mac and the current Windows DX11 machine; list Intel/AMD Macs as unqualified until tested, rather than promising all macOS hardware based on the API name.

Each report should record source revision, dirty-state identity when relevant, tool hashes, OS/architecture/GPU, requested and actual renderer, capability details, shader/asset hashes, executed tests, captures and failures. Record steady-state GPU/CPU frame times and memory for the plant and a budgeted SH3 scene after warm-up; set performance thresholds from the native baseline measured on each device, not the old tutorial timing file.

For the immediate Mac qualification, the existing entry points already support this shape after configuring a suitable build:

```text
cmake --build <mac-build> --config Release --target hg_lua assetc test_compute_probe
python harfang/tests/run_compute_probe.py --build-dir <mac-build> --renderer metal --lua --skip-build
python harfang/tests/run_gaussian_sort_probe.py --build-dir <mac-build> --renderer metal
```

These commands are building blocks, not the proposed one-command package target and not evidence that macOS execution has already passed. Complete the native HGS2 GPU suite and actual tutorial runner as part of the same qualification job.

## 9. Implementation work packages and exit criteria

| Priority | Work package | Primary files | Exit criterion |
| --- | --- | --- | --- |
| P0 | Establish current Mac correctness | `core-package/core/gaussian_splatting`, `engine/compute.cpp`, `engine/gaussian_splat.cpp`, vendored Metal backend, existing probes | Current native HGS2/SH route and unchanged tutorial pass on a real Mac; any reproduced defects are fixed natively. |
| P0 | Produce complete Mac application assets | Application CMake helper, `tools/assetc`, Lua installation rules | A clean Mac distribution runs with the same Lua/model paths; no manual shader copying or OS-specific Lua changes. |
| P1 | Align automatic renderer selection with the build | Top-level CMake, `engine/render_pipeline.cpp`, bindings/tests | Runtime and assetc defaults agree, selected/actual renderer mismatch is detected, explicit enums remain explicit. |
| P1 | Automate package and qualification targets | CMake helper, existing test runners, package manifest | One target builds, compiles, stages and verifies the application; warm rebuild is a no-op; evidence is retained. |
| P1 | Harden capability reporting and cache identity | `engine/compute.cpp`, `engine/gaussian_splat.cpp`, `tools/assetc/assetc.cpp` | Unsupported hardware is diagnosed accurately; changing shader target/toolchain cannot reuse stale outputs. |
| P2 | Add external overrides and Windows GL qualification | Native policy, launcher configuration, assetc profiles, probes | Automatic Lua application can select a qualified GL package without source edits. |
| P3 | Optional multi-backend package resolver | Generic program loaders and file/assets providers | One physical package resolves all program types consistently from the active renderer, with no Lua path branching. |

P0 fixes may be unnecessary in the shader algorithms if native Mac qualification passes as-is. Do not estimate a Metal rewrite before running the existing probes. Conversely, shaderc translation alone is not sufficient to close P0.

The immediate delivery is complete when a user can obtain the Windows or macOS package, run the **same Lua source**, display the same Gaussian content and conventional geometry correctly, and rebuild assets using the platform defaults without choosing shader profiles by hand.

## 10. Reference scope

This study complements the [native API study](SPECS_GAUSSIAN_SPLATTING_NATIVE_API_STUDY.md), [HGS2 specification](SPECS_GAUSSIAN_SPLATTING_HGS2.md) and [compute feature-parity specification](SPECS_COMPUTE_SHADER_FEATURE_PARITY.md). Statements about current behavior come from the linked checkout sources; older studies and benchmark files retain their historical scope.

BGFX documents renderer selection/capabilities and the shaderc backend profiles in its [API reference](https://bkaradzic.github.io/bgfx/bgfx.html) and [tool documentation](https://bkaradzic.github.io/bgfx/tools.html). Those live documents describe current upstream BGFX. HARFANG vendors an older revision with local patches, so verify implementation details against `extern/bgfx` before adopting upstream examples or new API assumptions.
