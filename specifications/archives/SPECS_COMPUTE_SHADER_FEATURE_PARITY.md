# Compute Shader Feature-Parity Implementation Spec

This document is a technical implementation brief.

Its purpose is to reproduce, in another branch or repository state, the compute shader support that now exists in this Harfang codebase, with feature parity at the public API level and enough validation to make the feature usable by end users.

The scope of this document is:

- core C++ runtime support
- Lua and Python bindings
- documentation
- tests
- end-user examples

This document is based on the implementation present in this repository and on the Lua examples located in the sibling repository `tutorials-hg2`.

## 1. Goal

Expose a minimal but practical compute shader API to Harfang end users, with the following properties:

- compute programs can be loaded from file or assets
- textures can be explicitly created as compute-writable
- images can be bound to compute stages with read/write access control
- compute dispatches can be submitted from Lua and Python
- compute output can be displayed immediately in a subsequent render view
- the API is generic and renderer-pipeline-level, not tied to a specific forward or AAA hook

The intended end-user pattern is:

1. create a texture
2. load a compute shader
3. bind the texture as an image
4. dispatch compute
5. display the generated texture with a regular fullscreen quad

This is the pattern used in:

- `tutorials-hg2\compute_fractal.lua`
- `tutorials-hg2\compute_raytrace_spheres.lua`

## 2. Target Public API

The public API that must exist at feature parity is:

### Program loading

- `LoadComputeProgramFromFile(path)`
- `LoadComputeProgramFromAssets(name)`

### Texture flags

- `TF_ComputeWrite = BGFX_TEXTURE_COMPUTE_WRITE`

### Image access enum

Expose `bgfx::Access::Enum` as:

- `Access`
- `AC_Read`
- `AC_Write`
- `AC_ReadWrite`

### Image binding value type

Expose a script-visible type:

- `ImageBinding`

With fields:

- `texture`
- `stage`
- `mip`
- `access`

And a helper:

- `MakeImageBinding(texture, stage, access, mip=0)`

### Dispatch function

Expose:

- `DispatchCompute(view_id, program, x, y=1, z=1, values=[], textures=[], images=[], debug_name="")`

Contract:

- applies `UniformSetValue`
- applies `UniformSetTexture`
- applies `ImageBinding`
- calls `bgfx::dispatch`
- increments `view_id`

Important:

- `DispatchCompute` must not configure the viewport
- if the compute shader uses `u_viewRect`, the caller must call `SetViewRect` before `DispatchCompute`

## 3. Non-Goals for v1

Do not expand the initial scope beyond the following:

- no direct exposure of internal AAA render targets
- no custom pass hook inside the forward pipeline submission path
- no storage buffers / structured buffers API
- no generalized 3D texture or buffer abstraction
- no attempt to redesign the renderer architecture around compute

This first version is intentionally limited to user-managed textures and compute dispatch submission.

## 4. Existing Engine Facts You Can Rely On

The implementation assumes the following facts are true or must be made true:

- `assetc` already compiles compute shaders
- compute shader outputs keep the `.sc` filename
- Harfang already had internal low-level compute usage, notably HiZ
- `bgfx::dispatch` and `bgfx::setImage` are available in C++

In this repository, those facts are true.

The relevant existing concepts are:

- `UniformSetValue`
- `UniformSetTexture`
- `CreateTexture(...)`
- `DrawTriangles(...)`
- `LoadProgramFromFile(...)`
- `LoadProgramFromAssets(...)`

## 5. Required Engine Changes

### 5.1 File targets

Main files:

- `harfang/engine/render_pipeline.h`
- `harfang/engine/render_pipeline.cpp`

### 5.2 Add compute program public declarations

The following declarations must exist in `render_pipeline.h`:

- `LoadComputeProgram(...)`
- `LoadComputeProgramFromFile(...)`
- `LoadComputeProgramFromAssets(...)`

### 5.3 Add image binding abstraction

Add:

```cpp
struct ImageBinding {
	Texture texture;
	uint8_t stage{0};
	uint8_t mip{0};
	bgfx::Access::Enum access{bgfx::Access::Read};
};
```

And:

```cpp
ImageBinding MakeImageBinding(const Texture &texture, uint8_t stage, bgfx::Access::Enum access, uint8_t mip = 0);
```

### 5.4 Add dispatch helper

Add:

```cpp
void DispatchCompute(
	bgfx::ViewId &view_id,
	bgfx::ProgramHandle program,
	uint32_t x,
	uint32_t y = 1,
	uint32_t z = 1,
	const std::vector<UniformSetValue> &values = {},
	const std::vector<UniformSetTexture> &textures = {},
	const std::vector<ImageBinding> &images = {},
	const std::string &debug_name = {}
);
```

### 5.5 Factor binding application

Do not duplicate logic between graphics draws and compute dispatch.

Create an internal helper in `render_pipeline.cpp` that applies:

- uniform values via `bgfx::setUniform`
- sampled textures via `bgfx::setTexture`
- image bindings via `bgfx::setImage`

Then:

- reuse this helper in `DrawDisplayList(...)`
- reuse this helper in `DispatchCompute(...)`

This avoids divergence between draw-time and compute-time resource binding.

### 5.6 Dispatch semantics

`DispatchCompute(...)` must:

1. optionally set the view name if `debug_name` is not empty
2. bind uniforms, sampled textures, and images
3. call `bgfx::dispatch(view_id, program, x, y, z)`
4. increment `view_id`

It must not:

- call `setViewRect`
- call `setViewTransform`
- touch the view

## 6. Required Binding Changes

### 6.1 File target

- `binding/bind_harfang.py`

### 6.2 Add enum exposure

Expose:

```python
gen.bind_named_enum('bgfx::Access::Enum', ['Read', 'Write', 'ReadWrite'], bound_name='Access', prefix='AC_')
```

### 6.3 Add texture flag exposure

Add:

- `TF_ComputeWrite`

to the script-visible `TextureFlags` constants group.

### 6.4 Add compute program bindings

Bind:

- `hg::LoadComputeProgramFromFile`
- `hg::LoadComputeProgramFromAssets`

### 6.5 Add ImageBinding bindings

Expose:

- class `hg::ImageBinding`
- its fields
- `std::vector<hg::ImageBinding>`
- `hg::MakeImageBinding(...)`

### 6.6 Add DispatchCompute bindings

Bind overloads matching the C++ API.

`view_id` must use `arg_in_out` so that Lua/Python can receive the incremented view id.

This is critical for scripts that chain:

1. compute dispatch
2. display pass
3. capture pass

### 6.7 Binding behavior notes

The current implementation pattern in Harfang bindings is:

- vectors are expanded through `expand_std_vector_proto(...)`
- list-like script types are accepted transparently

Follow the same pattern for:

- `UniformSetValue`
- `UniformSetTexture`
- `ImageBinding`

## 7. Shader Asset Pipeline Requirements

No new tool is required.

The existing `assetc` pipeline already supports compute shaders.

Important behavior:

- vertex/fragment programs compile to `.vsb` and `.fsb`
- compute shaders compile to a single compiled file that keeps the `.sc` extension

This means:

- `LoadProgramFromFile("path/name")` expects `path/name.vsb` and `path/name.fsb`
- `LoadComputeProgramFromFile("path/name.sc")` expects a compiled compute shader file

This asymmetry is intentional and must be documented because it is easy to misuse.

## 8. End-User Usage Pattern

The intended end-user flow is demonstrated by the following Lua examples:

- `tutorials-hg2\compute_fractal.lua`
- `tutorials-hg2\compute_raytrace_spheres.lua`

The canonical script pattern is:

1. initialize input/window/render
2. create a compute output texture with `TF_ComputeWrite`
3. load a compute program
4. load a regular display program
5. build `ImageBindingList`
6. build optional `UniformSetValueList`
7. call `DispatchCompute(...)`
8. display the texture using `DrawTriangles(...)`
9. present with `Frame()`

Minimal Lua structure:

```lua
compute_prg = hg.LoadComputeProgramFromFile("resources_compiled/shaders/example.sc")
display_prg = hg.LoadProgramFromFile("resources_compiled/shaders/texture")

tex = hg.CreateTexture(width, height, "compute.output", hg.TF_ComputeWrite | hg.TF_UClamp | hg.TF_VClamp, hg.TF_RGBA8)

images = {hg.MakeImageBinding(tex, 0, hg.AC_Write)}
values = {hg.MakeUniformSetValue("u_params", hg.Vec4(t, 0, 0, 0))}

view_id = 0
view_id = hg.DispatchCompute(view_id, compute_prg, group_x, group_y, 1, values, {}, images, "Compute pass")

hg.SetView2D(view_id, 0, 0, width, height, -1, 1, hg.CF_Color | hg.CF_Depth, hg.Color.Black, 1, 0)
hg.DrawTriangles(view_id, quad_idx, quad_vtx, display_prg, display_values, display_textures, display_state)
```

## 9. Reference Examples to Match

### 9.1 Fractal example

Reference:

- `tutorials-hg2\compute_fractal.lua`
- `tutorials-hg2\resources\shaders\compute_fractal.sc`

Purpose:

- validate the simplest “write pixels in a compute texture, then display it” flow
- exercise time-varying uniform input
- exercise `TF_ComputeWrite`, `MakeImageBinding`, `DispatchCompute`

### 9.2 Raytraced spheres example

Reference:

- `tutorials-hg2\compute_raytrace_spheres.lua`
- `tutorials-hg2\resources\shaders\compute_raytrace_spheres.sc`

Purpose:

- validate a more serious workload fully executed in compute
- exercise multi-bounce logic inside a compute shader
- prove the API is usable for screen-space procedural rendering, not just trivial image fills

This example performs:

- ray/sphere intersections
- perfect reflection
- perfect refraction
- limited-range point light
- checkerboard floor shading
- animated orbiting spheres

## 10. Documentation Requirements

At feature parity, the following docs should exist:

- `doc/doc/LoadComputeProgramFromFile.md`
- `doc/doc/LoadComputeProgramFromAssets.md`
- `doc/doc/ImageBinding.md`
- `doc/doc/MakeImageBinding.md`
- `doc/doc/DispatchCompute.md`
- `doc/doc/Access.md`

Also update:

- `doc/doc/CreateTexture.md`

to mention `TF_ComputeWrite`.

Minimum documentation content:

- what the function/type does
- how it composes with the rest of the compute API
- key caveats

The most important caveat to document is:

- `DispatchCompute` does not set the viewport

## 11. Testing Requirements

### 11.1 Native smoke test

Add a C++ smoke test that:

1. initializes rendering
2. loads a compiled compute shader
3. creates a compute-writable output texture
4. dispatches the compute shader
5. renders the output texture to a color render target
6. captures that texture back to CPU memory
7. checks a few sentinel pixels

This repository implements that in:

- `harfang/tests/engine/compute.cpp`

And integrates it via:

- `harfang/tests/CMakeLists.txt`
- `harfang/tests/main.cpp`

### 11.2 Script smoke coverage

The script examples should also be runnable as smoke tests.

In this repository, CI-oriented smoke scripts were implemented by extending:

- `ci/dummy.lua`
- `ci/dummy.py`
- `ci/lua_test.*`
- `ci/wheel_test.*`

This is optional if your target branch does not have the same CI structure, but you still need at least one scripted runtime validation path.

## 12. Critical Behavioral Details

These details matter and should not be “simplified away”.

### 12.1 View id is stateful

`DispatchCompute` increments `view_id`.

In script bindings, the incremented value must be recoverable by the caller.

This is required because the intended workflow is:

- compute in one view
- compose/display in the next view

### 12.2 Compute textures are explicit

The texture used as a compute output must be created with:

- `TF_ComputeWrite`

Do not hide this requirement.

### 12.3 Image bindings are separate from sampler bindings

`UniformSetTexture` is for sampled textures.

`ImageBinding` is for compute image access.

Do not merge them into a single API.

They map to different bgfx concepts:

- `setTexture`
- `setImage`

### 12.4 Compute shaders are user-managed

The v1 API works on user-created textures.

It does not expose internal forward/AAA render targets.

This limitation is acceptable and intentional.

### 12.5 Fullscreen display is a regular graphics pass

The expected presentation path is:

- compute writes a texture
- a standard vertex/fragment program displays that texture

This is the simplest and most robust pattern for volumetrics, raymarched views, raytraced demos, and other procedural screen-space outputs.

## 13. Known Pitfalls

### 13.1 Compute shader output naming

Compiled compute shaders keep the `.sc` extension.

Do not incorrectly expect `.csb`.

### 13.2 HLSL cross-compilation constructor rules

When authoring reference shaders intended to compile through `shaderc`, avoid scalar constructors like:

- `vec3(0.0)`
- `vec3(1.0)`

Some backends, especially DX11/HLSL codegen, may reject them.

Prefer:

- `vec3(0.0, 0.0, 0.0)`
- `vec3(1.0, 1.0, 1.0)`

This exact issue occurred while validating the raytraced spheres example.

### 13.3 View setup expectations

If a script uses `SetView2D(...)` before `DrawTriangles(...)`, then screen-space vertices are expected in 2D projected coordinates.

If the display shader expects clip-space directly, then the script must instead use explicit `SetViewRect(...)` and a compatible transform setup.

Do not mix the two mental models.

### 13.4 Old runtimes will not expose the new API

The tutorial examples can exist before the packaged Lua/Python runtime is updated.

In that situation:

- the examples are correct
- the local packaged runtime is simply too old to run them

This is not an implementation bug in the examples.

## 14. Recommended Implementation Order

If you are an autonomous coding agent, use this order:

1. inspect `render_pipeline.h` and `render_pipeline.cpp`
2. add `ImageBinding` and `DispatchCompute`
3. factor internal binding application logic
4. expose the new API in `binding/bind_harfang.py`
5. add documentation pages
6. add one trivial compute example
7. add one non-trivial compute example
8. add a native smoke test
9. validate shader compilation through `assetc`
10. validate script syntax and runtime behavior

Do not start from docs or examples first. The core runtime and binding layer must exist before the examples become meaningful.

## 15. Acceptance Criteria

The implementation is acceptable only if all of the following are true:

- compute shaders can be loaded from file and assets
- compute-writable textures can be created from script
- image access enum is exposed
- image bindings are exposed
- dispatch works from Lua
- dispatch works from Python
- a compute-generated texture can be displayed with a standard graphics pass
- at least one native smoke test exists
- documentation exists for the new public API
- the tutorials compile through `assetc`

Feature parity specifically requires that the implementation supports the same user flow as the two Lua tutorials:

- `compute_fractal.lua`
- `compute_raytrace_spheres.lua`

## 16. Concrete Reference Files

If you need concrete implementation references, inspect these exact files:

Core API:

- `harfang/engine/render_pipeline.h`
- `harfang/engine/render_pipeline.cpp`

Bindings:

- `binding/bind_harfang.py`

Docs:

- `doc/doc/DispatchCompute.md`
- `doc/doc/LoadComputeProgramFromFile.md`
- `doc/doc/LoadComputeProgramFromAssets.md`
- `doc/doc/ImageBinding.md`
- `doc/doc/MakeImageBinding.md`
- `doc/doc/Access.md`

Tests:

- `harfang/tests/engine/compute.cpp`
- `harfang/tests/CMakeLists.txt`
- `harfang/tests/main.cpp`

Examples:

- `tutorials-hg2\compute_fractal.lua`
- `tutorials-hg2\compute_raytrace_spheres.lua`
- `tutorials-hg2\resources\shaders\compute_fractal.sc`
- `tutorials-hg2\resources\shaders\compute_raytrace_spheres.sc`

## 17. Final Instruction to the Agent

Implement the compute shader API with exact feature parity, not an approximation.

Preserve the intended end-user model:

- create texture
- bind image
- dispatch compute
- display texture

Do not over-generalize the API in v1.
Do not couple it to AAA internals.
Do not introduce a second parallel abstraction for resource binding.

Deliver a small, sharp, stable compute API that is sufficient to support:

- procedural image generation
- screen-space volumetrics
- compute-driven post effects
- raytraced or raymarched tutorials rendered into textures

That is the correct implementation target.
