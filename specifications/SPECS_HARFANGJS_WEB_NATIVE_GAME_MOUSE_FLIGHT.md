# HARFANG JS Web: Native Mouse Flight Compatibility Experiment

Date: 2026-10-05

Status: implemented and exercised against native HG JS on Windows x64.

## Objective and result

Run `harfang3d/tutorials/game_mouse_flight.js` on the Web without changing its
public HARFANG calls or gameplay code. The packaged entry is byte-identical to
the native tutorial. The browser substitutes `js/window.js` and preloads compiled
assets before entering `main()`.

The experiment lives in
[`harfangjs/experiments/native-game-mouse-flight`](../../harfangjs/experiments/native-game-mouse-flight/README.md).
It extends the native-compatible API facade and standalone compiler introduced
by [Many Nodes](SPECS_HARFANGJS_WEB_NATIVE_SCENE_MANY_NODES_FEASIBILITY.md).
Both experiments use the same `src/compat/harfang.js` implementation.

The hard constraint remains public API compatibility with native HG JS for the
supported calls: names, signatures, enum values, defaults, return types and
observable update behavior. Browser size, presentation mode and requested
antialiasing remain host concessions. MSAA 8x produces a browser console warning.

## Problems and implemented solutions

| Problem | Solution |
| --- | --- |
| Synchronous native loading versus asynchronous HTTP | Preload and verify the manifest and all payloads in the browser host. Keep `LoadSceneFromAssets` synchronous and boolean. |
| Authored scene and model formats | Compile original JSON `.scn` and binary `.geo` into bounded Web scene/geometry payloads. Preserve transforms, material slots and triangle winding. |
| Scene instance composition | Implement the instance root, child parenting, scene view and owner relationship. Default instance flags load nodes/animations without replacing the world's environment. |
| World-transform timing | Match native cached world matrices: local edits are published by `Scene.Update()` or explicit matrix-update calls. Immediate recomputation changed the chase camera and was rejected by the comparison test. |
| Native node containers | Return `NodeList` with `get`, `at`, `length` and `size`, rather than a JS array. Preserve `BigInt` node counts and exclusion of instantiated children from `GetNodes`/`GetNodeCount`. |
| PBR source shaders and material variants | Compile pinned, reviewed WebGL 2 PBR/depth adapters. Keep authored values, base-color sampling, fog and lighting behavior. Reject unreviewed source changes. |
| HDR environment probes | Link CMFT into native `assetc-web`; generate irradiance and Phong-prefiltered radiance from the original HDR and metadata. Upload RGBA16F cubemaps and BRDF data in WebGL 2. |
| Directional shadows | Reproduce native four-split construction, light-space texel stabilization and PCF sampling in a depth atlas. |
| Mouse and 2D overlay | Implement native `Mouse` snapshots, bottom-left buffer-pixel coordinates, numeric render states, RGBA float vertices and the depth-cleared line view. |
| Cleanup and restart | Track scene/model/program/texture/line ownership. Stop pending frame waits and release resources on normal exit, restart and context loss. |

Two native behaviors deserve explicit retention. First, the camera's initial
frames reflect the cached world-matrix behavior of the original tutorial; the
port does not rewrite the gameplay to hide it. Second, an instance load failure
returns a valid root and `false`. Destroying an instance root with `DestroyNode`
alone does not recursively destroy its children in native HG JS; the Web
container/count behavior follows that rule. `Scene.Clear()` invalidates all nodes.

## Asset compiler and runtime boundary

The authoring input is shared with native HG JS: two scenes, three geometries,
the grid PNG, the BRDF DDS, the HDR probe, their metadata and reviewed shader
sources. A staging directory copies that subset unchanged; it is generated build
input, not a separately maintained Web asset tree.

`harfangjs/tools/native/assetc_web.cpp` now has a `web-native-scene/1` profile in
addition to the Many Nodes program profile. Its manifest contains logical IDs,
dependencies, byte sizes and SHA-256 hashes. Asset objects are content-addressed;
the manifest is published after conversion and validation. Failed compilation
preserves the previous manifest.

The implementation links CMFT statically and does not launch an external image
converter. A private build copy fixes CMFT's uninitialized memory-reader cursor,
missing cursor offset and memory/file-open check. The shared native dependency
checkout remains unchanged. This allows the compiler to consume already-read
bytes and supports Unicode asset paths.

The experimental output uses float32 geometry, RGBA8 PNG mip chains and RGBA16F
BRDF/cubemap payloads. The current DDS decoder contract accepts uncompressed
RGBA16F/RGBA32F sources. HDR processing uses the authored 256-pixel face limit,
irradiance SH filtering and nine radiance mips. Shader compilation is an explicit
adapter mapping, not arbitrary shader translation.

The release has 12 compiled payloads totaling approximately 13.6 MB before HTTP
compression. The browser contains JavaScript/WebGL 2 only. Python is used for
development packaging and validation; native `assetc`/`hgjs` are used only for
the reference comparison.

## Verification

The original tutorial executes in both hosts with fixed `16666667n` deltas,
identical injected pointer coordinates, 960 × 625 render buffers and no AA.
Input injection and state observation live in the test host, outside the copied
entry. A separate identical fixture exercises API return shapes, numeric enums,
math including vertical look directions, scene flags, texture references, node
lists, cached transforms, instance enablement/destruction and pass IDs.

Validated on Chrome 154, Windows x64, NVIDIA RTX 4060 through ANGLE D3D11:

| Gate | Observed result |
| --- | --- |
| Entry bytes and SHA-256 | Identical to native source |
| Plane/camera state over 60 frames | Every component within `1e-5` of native |
| Shared API fixture | Pass |
| Capture at frame 4 | Mean RGB error 0.0073 / 255 |
| Capture at frame 60 | Mean RGB error 0.2807 / 255; 0.172% pixels exceed 16 in any channel |
| Native compiler checks | 9 pass |
| Browser lifecycle/input/integrity checks | 8 pass |
| Final tracked GPU allocations | Zero, including line buffers/programs |
| Many Nodes regression | Native contract, image comparison, 11 compiler checks and 10 lifecycle checks pass |
| Existing W0/W1/W2 regression | 66 browser cases and 10 writer tests pass |

Image acceptance uses a mean RGB error below 2/255 and fewer than 3% of pixels
exceeding 16 in any channel. This is measured equivalence for these views and
inputs, not a promise of pixel identity across browsers or graphics drivers.

The rendered scene contains 21 total nodes and 18 objects. Batching produces five
main draws, 20 directional-shadow draws and one 2D line draw per frame, with 372
main-pass triangles. Tracked allocations are about 21.9 MB plus a 1,848-byte line
buffer. An isolated hardware run measured approximately 1.2 ms median CPU draw
submission; GPU frame time and other machines require separate measurement.

Generated evidence is retained under
`harfangjs/build/experiments/native-game-mouse-flight/reports/`.

## Test and package

From `harfangjs/`:

```powershell
python experiments/native-game-mouse-flight/build.py
python experiments/native-game-mouse-flight/serve.py
```

Open **http://localhost:8002/** and move the pointer inside the canvas. Escape
stops the application; the page provides pause/resume and restart. Browser
drawing-buffer dimensions determine the viewport, and flight speed remains the
original per-frame speed.

For the native comparison:

```powershell
.venv/Scripts/python.exe experiments/native-game-mouse-flight/validate.py --skip-build --native ../install/js_bullet/hgjs/hgjs.exe
```

The independent release is `harfangjs/dist/experiments/native-game-mouse-flight/`.
It can be served by an ordinary HTTP(S) server.

## Remaining boundaries

The supported scene slice is static, opaque, unskinned PBR with base-color maps,
legacy global environment probes and directional shadows. Nested authored
instances, animation, physics, skinning, additional PBR texture families,
transparency, probe volumes, arbitrary shaders and other texture metadata remain
outside this profile. The older W1/W2 gallery retains its documented adaptations.

Windows x64 validates the standalone compiler in this experiment. Packaging and
testing the complete Windows/macOS/Linux × x64/ARM64 distribution remain part of
the [assetc-web product contract](SPECS_HARFANG_WEB_ASSETC.md).

The demonstrated common surface is unchanged native application code plus a
host-specific window lifecycle and an offline target-specific asset compiler.
Further tutorials should extend this shared facade and its native comparison
fixture, keeping unsupported features explicit.
