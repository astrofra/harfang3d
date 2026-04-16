# Compositing Shader Uniform API Implementation Spec

This document is a technical implementation brief.

Its purpose is to define the least invasive reliable way to pass predefined and user-defined uniforms to the final forward-pipeline compositing shader from C++, Lua, and Python.

The scope is limited to the AAA final compositing pass loaded from:

- `tutorials/resources/core/shader/compositing_fs.sc`
- `tutorials/resources/core/shader/compositing_vs.sc`
- `tutorials/resources/core/shader/compositing_varying.def`

The implementation should not change the overall forward pipeline architecture, the shader compiler flow, or the way scenes are submitted.

## 1. Goal

Expose a small API that lets applications drive custom post-processing parameters in the final compositing shader.

The API must support two use cases:

- predefined compositor parameters with stable engine-owned uniform names
- arbitrary user uniforms declared by a custom compositing shader

The intended end-user pattern is:

1. create or load a custom compositing shader under the regular pipeline resource path
2. set fixed compositor parameters on `ForwardPipelineAAAConfig`
3. optionally attach arbitrary `UniformSetValue` and `UniformSetTexture` lists to the same config
4. call `SubmitSceneToForwardPipeline` or `SubmitSceneToPipeline` as usual

The final compositing draw must receive:

- the existing forward-pipeline scalar and matrix uniforms, especially `uResolution` and `uAAAParams`
- the new predefined compositor parameter block
- the user-provided arbitrary uniforms and sampler uniforms

## 2. Current State

`ForwardPipelineAAAConfig` already contains compositor-related values:

- `exposure`
- `gamma`
- `sharpen`
- `use_tonemapping`
- `debug_buffer`

`UpdateForwardPipelineAAA` packs the runtime AAA values into `uAAAParams[3]`:

- `uAAAParams[1].x`: exposure
- `uAAAParams[1].y`: inverse gamma
- `uAAAParams[2].y`: sharpen

`compositing_fs.sc` already reads `uAAAParams` and `uResolution` through `forward_pipeline.sh`.

The final compositing pass in `scene_forward_pipeline.cpp` currently submits the fullscreen quad with empty uniform lists:

```cpp
DrawTriangles(view_id, {0, 1, 2, 0, 2, 3}, vtx, aaa.compositing_prg, {}, {}, ComputeRenderState(BM_Opaque, DT_Always));
```

That means the pass does not explicitly bind `pipeline.uniform_values` for this draw. A reliable implementation must stop relying on any previous draw state and must set the required uniforms immediately before the compositor submit.

There is also a naming mismatch to audit before landing the API:

- the C++ compositing path creates and binds `u_color` and `u_depth`
- the provided `tutorials/resources/core/shader/compositing_fs.sc` samples `u_copyColor` and `u_copyDepth`
- the copy shader also uses `u_copyColor` and `u_copyDepth`

The recommended cleanup is to keep `copy_fs.sc` on `u_copyColor` and `u_copyDepth`, and align `compositing_fs.sc` with the C++ compositor names `u_color` and `u_depth`. If the compiled assets already use `u_color` and `u_depth`, only the source shader needs updating.

## 3. Recommended Design

Use a two-tier API.

Tier 1 is a fixed compositor parameter block owned by the engine:

```glsl
uniform vec4 uCompositingParams[4];
```

This provides 16 stable float slots for shader authors without requiring custom bgfx uniform handle management from scripts.

Tier 2 reuses the existing generic render API:

- `UniformSetValue`
- `UniformSetTexture`
- `MakeUniformSetValue`
- `MakeUniformSetTexture`

This keeps arbitrary uniforms consistent with `DrawTriangles`, `DrawModel`, `DrawText`, and other existing draw APIs.

Do not extend `uAAAParams` for user data. It is already shared by temporal AA, motion blur, SSGI, SSR, PBR, tone mapping, and sharpen. Packing custom compositor values into it would create hidden coupling and make future AAA changes risky.

Do not introduce runtime shader reflection or name-based maps for this first implementation. bgfx requires the shader source to declare the uniform names, and Harfang already has a working explicit uniform list abstraction.

## 4. Public API

### 4.1 C++ Data Model

Add these members to `ForwardPipelineAAAConfig`:

```cpp
Vec4 compositing_params0 = Vec4::Zero;
Vec4 compositing_params1 = Vec4::Zero;
Vec4 compositing_params2 = Vec4::Zero;
Vec4 compositing_params3 = Vec4::Zero;

std::vector<UniformSetValue> compositing_uniform_values;
std::vector<UniformSetTexture> compositing_uniform_textures;
```

The fixed params are application-defined. The engine only transports them.

The arbitrary lists follow the same rules as the existing draw APIs:

- scalar, vector, and matrix uniforms use `UniformSetValue`
- sampler uniforms use `UniformSetTexture`
- the shader must declare a matching name and compatible type
- the application owns the meaning of each value

### 4.2 Fixed Uniform Contract

Default and custom compositing shaders may declare:

```glsl
uniform vec4 uCompositingParams[4];
```

Suggested convention for examples only:

- `uCompositingParams[0]`: primary effect controls
- `uCompositingParams[1]`: secondary effect controls
- `uCompositingParams[2]`: color or curve controls
- `uCompositingParams[3]`: spare/debug controls

The engine must not assign built-in semantics to these fields in the public API. If a future built-in effect needs named semantic fields, it should get explicit `ForwardPipelineAAAConfig` members instead of consuming generic slots silently.

### 4.3 Reserved Names and Stages

Reserved compositor uniform names:

- `u_color`
- `u_depth`
- `uResolution`
- `uAAAParams`
- `uCompositingParams`

Reserved sampler stages:

- stage 0: compositor color input
- stage 1: compositor depth input

For user sampler uniforms, use stage 2 or higher:

```glsl
SAMPLER2D(u_lut, 2);
```

The first implementation should document this rule. A debug warning for stage 0 or 1 in `compositing_uniform_textures` is useful, but not required for the minimal patch.

## 5. C++ Implementation

### 5.1 `ForwardPipelineAAA`

Add an engine-owned uniform handle:

```cpp
bgfx::UniformHandle u_compositingParams = BGFX_INVALID_HANDLE;
```

Create it with count 4 when the AAA pipeline is created:

```cpp
aaa.u_compositingParams = bgfx::createUniform("uCompositingParams", bgfx::UniformType::Vec4, 4);
```

Destroy it in `DestroyForwardPipelineAAA`.

Include it in `IsValid(const ForwardPipelineAAA&)`.

### 5.2 Final Compositing Submit

Immediately before submitting `aaa.compositing_prg`, set uniforms in this order:

1. existing forward-pipeline value uniforms
2. fixed compositor params
3. arbitrary compositor values and textures

Recommended code shape:

```cpp
SetUniforms(pipeline.uniform_values, {});

const Vec4 compositing_params[] = {
	aaa_config.compositing_params0,
	aaa_config.compositing_params1,
	aaa_config.compositing_params2,
	aaa_config.compositing_params3,
};
bgfx::setUniform(aaa.u_compositingParams, compositing_params, 4);

SetUniforms(aaa_config.compositing_uniform_values, aaa_config.compositing_uniform_textures);

DrawTriangles(view_id, {0, 1, 2, 0, 2, 3}, vtx, aaa.compositing_prg, {}, {}, ComputeRenderState(BM_Opaque, DT_Always));
```

This avoids copying `UniformSetValue` objects every frame. Copying these objects recreates bgfx uniform handles, so do not build a temporary combined vector in the render path.

`SetUniforms(pipeline.uniform_values, {})` intentionally does not bind `pipeline.uniform_textures`. The final compositor already binds its color and depth input explicitly, and arbitrary user textures should be controlled by `compositing_uniform_textures`.

### 5.3 Non-Tonemapping Copy Path

The `use_tonemapping == false` copy path should remain unchanged for the minimal implementation.

Reason: the requested API targets the compositing shader. Applying arbitrary uniforms to `copy_prg` would imply that custom effects run even when tone mapping/compositing is disabled, which changes the meaning of `use_tonemapping`.

If a later API wants custom compositing without tone mapping, add a separate explicit flag such as `use_compositing_shader`, not an implicit side effect.

### 5.4 Shader Source Changes

In `tutorials/resources/core/shader/compositing_fs.sc`:

1. align sampler names with C++ if needed:

```glsl
SAMPLER2D(u_color, 0);
SAMPLER2D(u_depth, 1);
```

2. add the fixed parameter block:

```glsl
uniform vec4 uCompositingParams[4];
```

3. keep `compositing_varying.def` unchanged.

Uniform declarations belong in the shader source or an included `.sh` file, not in the varying definition. `compositing_varying.def` only describes vertex attributes and interpolated values.

## 6. Lua and Python Bindings

The binding layer already exposes:

- `UniformSetValue`
- `UniformSetValueList`
- `UniformSetTexture`
- `UniformSetTextureList`
- `MakeUniformSetValue`
- `MakeUniformSetTexture`

Update the `ForwardPipelineAAAConfig` binding to include existing missing fields and the new compositor fields:

```python
gen.bind_members(forward_pipeline_aaa_config, [
	'float temporal_aa_weight',
	'int sample_count', 'float max_distance', 'float z_thickness',
	'float bloom_threshold', 'float bloom_bias', 'float bloom_intensity',
	'float motion_blur',
	'float exposure', 'float gamma',
	'float sharpen',
	'float dof_focus_point', 'float dof_focus_length',
	'bool use_tonemapping',
	'float specular_weight',
	'hg::ForwardPipelineAAADebugBuffer debug_buffer',
	'hg::Vec4 compositing_params0',
	'hg::Vec4 compositing_params1',
	'hg::Vec4 compositing_params2',
	'hg::Vec4 compositing_params3',
	'std::vector<hg::UniformSetValue> compositing_uniform_values',
	'std::vector<hg::UniformSetTexture> compositing_uniform_textures',
])
```

If Fabgen member binding for vectors is not acceptable on every target language, add small helper functions instead:

```cpp
void SetForwardPipelineAAACompositingUniforms(ForwardPipelineAAAConfig &config,
	const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures);

void ClearForwardPipelineAAACompositingUniforms(ForwardPipelineAAAConfig &config);
```

Bind those helpers for C++, Lua, and Python. This fallback keeps the public script API explicit and avoids depending on direct vector member assignment.

## 7. Usage Examples

### 7.1 Shader

```glsl
$input v_texcoord0

#include <forward_pipeline.sh>

SAMPLER2D(u_color, 0);
SAMPLER2D(u_depth, 1);
SAMPLER2D(u_lut, 2);

uniform vec4 uCompositingParams[4];
uniform vec4 uVignette;

void main() {
	vec4 c = texture2D(u_color, v_texcoord0);
	float amount = uCompositingParams[0].x;
	c.rgb *= mix(vec3_splat(1.0), uVignette.rgb, amount);
	gl_FragColor = c;
	gl_FragDepth = texture2D(u_depth, v_texcoord0).r;
}
```

### 7.2 C++

```cpp
ForwardPipelineAAAConfig aaa_config;
aaa_config.compositing_params0 = {0.35f, 0.f, 0.f, 0.f};
aaa_config.compositing_uniform_values = {
	MakeUniformSetValue("uVignette", Vec4{0.9f, 0.85f, 0.75f, 1.f}),
};
aaa_config.compositing_uniform_textures = {
	MakeUniformSetTexture("u_lut", lut_texture, 2),
};
```

### 7.3 Python

```python
aaa_config = hg.ForwardPipelineAAAConfig()
aaa_config.compositing_params0 = hg.Vec4(0.35, 0.0, 0.0, 0.0)
aaa_config.compositing_uniform_values = hg.UniformSetValueList([
	hg.MakeUniformSetValue("uVignette", hg.Vec4(0.9, 0.85, 0.75, 1.0)),
])
aaa_config.compositing_uniform_textures = hg.UniformSetTextureList([
	hg.MakeUniformSetTexture("u_lut", lut_texture, 2),
])
```

### 7.4 Lua

```lua
aaa_config = hg.ForwardPipelineAAAConfig()
aaa_config.compositing_params0 = hg.Vec4(0.35, 0.0, 0.0, 0.0)
aaa_config.compositing_uniform_values = hg.UniformSetValueList({
	hg.MakeUniformSetValue("uVignette", hg.Vec4(0.9, 0.85, 0.75, 1.0))
})
aaa_config.compositing_uniform_textures = hg.UniformSetTextureList({
	hg.MakeUniformSetTexture("u_lut", lut_texture, 2)
})
```

If direct list assignment is not supported by the generated bindings, use the helper functions from section 6 instead.

## 8. Serialization

Do not serialize arbitrary `UniformSetValue` or `UniformSetTexture` lists in `LoadForwardPipelineAAAConfig` or `SaveForwardPipelineAAAConfig`. They contain runtime handles and texture references.

Fixed `compositing_params0` through `compositing_params3` can be serialized because they are plain values. Use backward-compatible presence checks when loading JSON so older config files still work:

```cpp
if (js.contains("compositing_params0")) {
	// load Vec4
}
```

The minimal patch may leave fixed compositor params out of JSON serialization if the existing config file format should remain untouched. In that case, document that these fields are runtime-only.

## 9. Validation Plan

Add one C++ render smoke test or tutorial-level sample that:

- loads a custom compositing shader
- sets `compositing_params0.x` to a visible value
- sets an arbitrary `uVignette` uniform through `compositing_uniform_values`
- optionally binds a sampler at stage 2 through `compositing_uniform_textures`
- verifies that the scene still renders with `use_tonemapping == true`

Add Lua and Python binding smoke tests that:

- construct `ForwardPipelineAAAConfig`
- assign `compositing_params0`
- assign one `UniformSetValue`
- assign one `UniformSetTexture`
- call the existing submit function without requiring a new overload

Manual shader validation:

- default compositor still performs exposure, gamma, sharpen, and depth copy
- custom compositor can read `uResolution`
- custom compositor can read `uAAAParams`
- custom compositor can read `uCompositingParams`
- custom compositor can read an arbitrary `vec4` uniform
- custom compositor can sample an arbitrary texture on stage 2

## 10. Risks and Constraints

Uniform type support remains limited to what `UniformSetValue` already supports:

- float values are packed as vec4
- `Vec2`, `Vec3`, and `Vec4` are backed by bgfx `Vec4` uniforms
- matrices are backed by bgfx `Mat4` uniforms
- integer and boolean shader inputs should be encoded as floats

The shader must declare every arbitrary uniform. This API does not make undeclared shader uniforms magically available.

Avoid creating arbitrary `UniformSetValue` objects every frame in hot code when the fixed `uCompositingParams` slots are sufficient. `UniformSetValue` owns a bgfx uniform handle, so repeated creation has a cost.

Do not let arbitrary sampler uniforms use stage 0 or 1. Those stages are required by the compositor input color and depth textures.

Do not bind `pipeline.uniform_textures` to the final compositor by default. That would make sampler stage ownership ambiguous and could break user-provided compositor samplers.

## 11. Minimal Patch Checklist

1. Add `compositing_params0` through `compositing_params3` to `ForwardPipelineAAAConfig`.
2. Add `compositing_uniform_values` and `compositing_uniform_textures` to `ForwardPipelineAAAConfig`.
3. Add `u_compositingParams` to `ForwardPipelineAAA`.
4. Create, validate, and destroy `u_compositingParams`.
5. Before the final compositing `DrawTriangles`, call `SetUniforms(pipeline.uniform_values, {})`.
6. Set `uCompositingParams`.
7. Call `SetUniforms(aaa_config.compositing_uniform_values, aaa_config.compositing_uniform_textures)`.
8. Update Lua and Python bindings for the new config fields, or bind the helper functions if vector member binding is not portable enough.
9. Add `uniform vec4 uCompositingParams[4];` to the default compositing shader source.
10. Audit and align `u_color`/`u_depth` versus `u_copyColor`/`u_copyDepth` naming in the compositing shader source.
11. Add one C++, one Lua, and one Python smoke path or tutorial snippet.

This keeps the implementation local to the AAA config, the AAA final pass, the existing shader source, and the existing binding generator. It avoids new submit overloads and reuses the uniform abstractions already exposed to users.
