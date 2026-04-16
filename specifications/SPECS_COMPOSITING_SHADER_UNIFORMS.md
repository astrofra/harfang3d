# Compositing Shader Parameters API Implementation Spec

This document is a technical implementation brief.

Its purpose is to define the least invasive reliable way to pass application-defined values to the final forward-pipeline compositing shader from C++, Lua, and Python.

The scope is limited to the AAA final compositing pass loaded from:

- `tutorials/resources/core/shader/compositing_fs.sc`
- `tutorials/resources/core/shader/compositing_vs.sc`
- `tutorials/resources/core/shader/compositing_varying.def`

The implementation should not change the overall forward pipeline architecture, the shader compiler flow, or the way scenes are submitted.

## 1. Goal

Expose a small API that lets applications drive custom post-processing parameters in the final compositing shader.

The API must fit the existing `ForwardPipelineAAAConfig` shape. That structure currently exposes purpose-oriented values such as `exposure`, `gamma`, `bloom_threshold`, `motion_blur`, and `dof_focus_point`; it does not expose low-level shader uniform handles or draw-time uniform lists.

The recommended API therefore adds a small number of generic `Vec4` fields to `ForwardPipelineAAAConfig`. These fields are transported to the compositing shader through one fixed engine-owned uniform array.

The intended end-user pattern is:

1. create or load a custom compositing shader under the regular pipeline resource path
2. set generic compositor vectors on `ForwardPipelineAAAConfig`
3. call `SubmitSceneToForwardPipeline` or `SubmitSceneToPipeline` as usual
4. read the vectors from the compositing shader through the fixed uniform contract

The final compositing draw must receive:

- the existing forward-pipeline scalar and matrix uniforms, especially `uResolution` and `uAAAParams`
- the new generic compositor parameter vector block

Arbitrary named uniforms, arbitrary samplers, and script-visible `UniformSetValue`/`UniformSetTexture` lists are intentionally out of scope for the minimal implementation.

## 2. Current State

The public Lua/Python documentation for `ForwardPipelineAAAConfig` currently exposes these members:

- `bloom_bias`
- `bloom_intensity`
- `bloom_threshold`
- `dof_focus_length`
- `dof_focus_point`
- `exposure`
- `gamma`
- `max_distance`
- `motion_blur`
- `sample_count`
- `temporal_aa_weight`
- `z_thickness`

The C++ structure also contains additional fields such as `sharpen`, `use_tonemapping`, `specular_weight`, and `debug_buffer`, but those are not currently listed in the public binding table shown above.

`UpdateForwardPipelineAAA` packs the runtime AAA values into `uAAAParams[3]`:

- `uAAAParams[1].x`: exposure
- `uAAAParams[1].y`: inverse gamma
- `uAAAParams[2].y`: sharpen

`compositing_fs.sc` reads `uAAAParams` and `uResolution` through `forward_pipeline.sh`.

The final compositing pass in `scene_forward_pipeline.cpp` currently submits the fullscreen quad with empty uniform lists:

```cpp
DrawTriangles(view_id, {0, 1, 2, 0, 2, 3}, vtx, aaa.compositing_prg, {}, {}, ComputeRenderState(BM_Opaque, DT_Always));
```

That means the pass does not explicitly bind `pipeline.uniform_values` for this draw. A reliable implementation must stop relying on previous draw state and must set the required uniforms immediately before the compositor submit.

There was also a sampler naming mismatch:

- the C++ compositing path creates and binds `u_color` and `u_depth`
- `compositing_fs.sc` used to sample `u_copyColor` and `u_copyDepth`
- the copy shader correctly uses `u_copyColor` and `u_copyDepth`

That mismatch should be fixed independently by aligning `compositing_fs.sc` with the C++ compositor names `u_color` and `u_depth`, while leaving the copy shader unchanged.

## 3. Recommended Design

Add generic compositor vectors to `ForwardPipelineAAAConfig`.

The public API remains field-based:

```cpp
Vec4 compositing_params0 = Vec4::Zero;
Vec4 compositing_params1 = Vec4::Zero;
Vec4 compositing_params2 = Vec4::Zero;
Vec4 compositing_params3 = Vec4::Zero;
```

The shader contract is a fixed uniform array:

```glsl
uniform vec4 uCompositingParams[4];
```

This provides 16 stable float slots for custom compositing shader authors without introducing low-level uniform list management into `ForwardPipelineAAAConfig`.

This design deliberately treats the values as generic compositor payload, not as arbitrary bgfx uniforms. The shader uniform name is fixed, and the application decides how to interpret each component.

Do not extend `uAAAParams` for custom compositor values. It is already shared by temporal AA, motion blur, SSGI, SSR, PBR, tone mapping, and sharpen. Packing user data into it would create hidden coupling and make future AAA changes risky.

Do not add `std::vector<UniformSetValue>` or `std::vector<UniformSetTexture>` members to `ForwardPipelineAAAConfig` for the first implementation. That would conflict with the current config style, make serialization unclear, and expose a lower-level draw API inside a purpose-oriented pipeline config.

## 4. Public API

### 4.1 C++ Data Model

Add these members to `ForwardPipelineAAAConfig`:

```cpp
Vec4 compositing_params0 = Vec4::Zero;
Vec4 compositing_params1 = Vec4::Zero;
Vec4 compositing_params2 = Vec4::Zero;
Vec4 compositing_params3 = Vec4::Zero;
```

The engine C++ side only transports these values. The default core compositing shader may still document conventions for how it interprets some slots.

Default compositing shader convention:

- `compositing_params0.x`: vignette strength, where `0` disables the effect
- `compositing_params0.y`: vignette radius
- `compositing_params0.z`: vignette softness
- `compositing_params0.w`: unused

Suggested remaining-slot convention for custom shaders:

- `compositing_params1`: secondary effect controls
- `compositing_params2`: color or curve controls
- `compositing_params3`: spare/debug controls

If a future compositor effect needs stable purpose-oriented public controls beyond this generic payload, it should get explicit fields such as `color_grading_intensity` instead of silently consuming more generic slots.

### 4.2 Shader Contract

Default and custom compositing shaders may declare:

```glsl
uniform vec4 uCompositingParams[4];
```

The mapping is direct:

- `uCompositingParams[0]` = `ForwardPipelineAAAConfig::compositing_params0`
- `uCompositingParams[1]` = `ForwardPipelineAAAConfig::compositing_params1`
- `uCompositingParams[2]` = `ForwardPipelineAAAConfig::compositing_params2`
- `uCompositingParams[3]` = `ForwardPipelineAAAConfig::compositing_params3`

The application and shader must agree on component meaning. The default core shader uses:

- `uCompositingParams[0].x`: vignette strength
- `uCompositingParams[0].y`: vignette radius
- `uCompositingParams[0].z`: vignette softness

Custom compositing shaders may define their own meaning for the remaining slots.

### 4.3 Non-Goals

The minimal API does not support:

- arbitrary uniform names
- arbitrary uniform types beyond `vec4` slots
- arbitrary sampler uniforms
- arbitrary texture binding stages
- runtime shader reflection
- script-visible bgfx uniform handles

Those can be considered later as a separate lower-level compositing override API if a concrete use case requires them.

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

Immediately before submitting `aaa.compositing_prg`, set the existing forward-pipeline uniforms and then set the compositor parameter array.

Recommended code shape:

```cpp
const Vec4 compositing_params[] = {
	aaa_config.compositing_params0,
	aaa_config.compositing_params1,
	aaa_config.compositing_params2,
	aaa_config.compositing_params3,
};
SetUniforms(pipeline.uniform_values, {});
bgfx::setUniform(aaa.u_compositingParams, compositing_params, 4);

DrawTriangles(view_id, {0, 1, 2, 0, 2, 3}, vtx, aaa.compositing_prg, {}, {}, ComputeRenderState(BM_Opaque, DT_Always));
```

This keeps the final pass explicit and avoids copying `UniformSetValue` objects every frame.

Setting `pipeline.uniform_values` matters because the compositing shader relies on `uResolution` and `uAAAParams`. Relying on stale bgfx state from previous draw submissions is fragile.

Do not bind `pipeline.uniform_textures` to the final compositor by default. The final compositor already binds its color and depth input explicitly through `u_color` and `u_depth`.

### 5.3 Non-Tonemapping Copy Path

The `use_tonemapping == false` copy path should remain unchanged for the minimal implementation.

Reason: the requested API targets the compositing shader. Applying compositor parameters to `copy_prg` would imply that custom effects run even when tone mapping/compositing is disabled, which changes the meaning of `use_tonemapping`.

If a later API wants custom compositing without tone mapping, add a separate explicit flag such as `use_compositing_shader`, not an implicit side effect.

### 5.4 Shader Source Changes

In `tutorials/resources/core/shader/compositing_fs.sc`:

1. make sure sampler names match the C++ compositor path:

```glsl
SAMPLER2D(u_color, 0);
SAMPLER2D(u_depth, 1);
```

2. add the generic parameter block:

```glsl
uniform vec4 uCompositingParams[4];
```

3. keep `compositing_varying.def` unchanged.

Uniform declarations belong in the shader source or an included `.sh` file, not in the varying definition. `compositing_varying.def` only describes vertex attributes and interpolated values.

## 6. Lua and Python Bindings

Update the `ForwardPipelineAAAConfig` binding to include the new vector fields:

```python
gen.bind_members(forward_pipeline_aaa_config, [
	'float temporal_aa_weight',
	'int sample_count', 'float max_distance', 'float z_thickness',
	'float bloom_threshold', 'float bloom_bias', 'float bloom_intensity',
	'float motion_blur',
	'float exposure', 'float gamma',
	'float dof_focus_point', 'float dof_focus_length',
	'hg::Vec4 compositing_params0',
	'hg::Vec4 compositing_params1',
	'hg::Vec4 compositing_params2',
	'hg::Vec4 compositing_params3',
])
```

This mirrors the current binding style: simple public fields on the config object.

Do not add `UniformSetValueList` or `UniformSetTextureList` members to the config for this feature.

The existing unbound C++ fields such as `sharpen`, `use_tonemapping`, `specular_weight`, and `debug_buffer` can be exposed in a separate API cleanup if desired, but they are not required for compositor parameter vectors.

## 7. Usage Examples

### 7.1 Shader

```glsl
$input v_texcoord0

#include <forward_pipeline.sh>

SAMPLER2D(u_color, 0);
SAMPLER2D(u_depth, 1);

uniform vec4 uCompositingParams[4];

void main() {
	vec4 c = texture2D(u_color, v_texcoord0);

	float vignette_strength = uCompositingParams[0].x;
	float vignette_radius = uCompositingParams[0].y;
	float vignette_softness = uCompositingParams[0].z;

	vec2 centered_uv = v_texcoord0 * 2.0 - 1.0;
	float vignette = 1.0 - smoothstep(vignette_radius, vignette_radius + vignette_softness, length(centered_uv));
	c.rgb *= mix(vec3_splat(1.0), vec3_splat(vignette), vignette_strength);

	gl_FragColor = c;
	gl_FragDepth = texture2D(u_depth, v_texcoord0).r;
}
```

### 7.2 C++

```cpp
ForwardPipelineAAAConfig aaa_config;
aaa_config.compositing_params0 = {0.45f, 0.95f, 0.85f, 0.f};
```

### 7.3 Python

```python
aaa_config = hg.ForwardPipelineAAAConfig()
aaa_config.compositing_params0 = hg.Vec4(0.85, 0.80, 0.45, 0.0)
```

### 7.4 Lua

```lua
aaa_config = hg.ForwardPipelineAAAConfig()
aaa_config.compositing_params0 = hg.Vec4(0.85, 0.80, 0.45, 0.0)
```

## 8. Serialization

The generic compositor vectors are plain values and can be serialized safely.

Recommended JSON keys:

- `compositing_params0`
- `compositing_params1`
- `compositing_params2`
- `compositing_params3`

Use backward-compatible presence checks when loading JSON so older config files still work:

```cpp
if (js.contains("compositing_params0")) {
	// load Vec4
}
```

The minimal patch may leave these fields out of JSON serialization if the existing config file format should remain untouched. In that case, document that compositor vectors are runtime-only.

## 9. Validation Plan

Add one C++ render smoke test or tutorial-level sample that:

- loads a custom compositing shader
- sets `compositing_params0.x` to a visible value
- sets `compositing_params1` to a visible color/tint value
- verifies that the scene still renders with `use_tonemapping == true`

Add Lua and Python binding smoke tests that:

- construct `ForwardPipelineAAAConfig`
- assign `compositing_params0`
- assign `compositing_params1`
- call the existing submit function without requiring a new overload

Add a Lua tutorial derived from `scene_aaa.lua`:

- set `pipeline_aaa_config.compositing_params0` each frame
- use arrow keys to control vignette strength and radius
- use keypad plus/minus to control vignette softness

Manual shader validation:

- default compositor still performs exposure, gamma, sharpen, and depth copy
- custom compositor can read `uResolution`
- custom compositor can read `uAAAParams`
- custom compositor can read `uCompositingParams`
- changing `compositing_params0` changes the final image

## 10. Risks and Constraints

The API exposes arbitrary values, not arbitrary uniforms. This is intentional.

Limits:

- 16 custom float slots are available
- integer and boolean shader inputs should be encoded as floats
- custom texture/sampler inputs are not covered
- the shader must declare `uniform vec4 uCompositingParams[4]` if it wants to use the values

The number of slots should stay fixed for the first implementation. If a real project needs more, increase the array count before release rather than adding a dynamic uniform system.

Do not bind `pipeline.uniform_textures` to the final compositor by default. That would make sampler stage ownership ambiguous and could break the color/depth inputs.

## 11. Minimal Patch Checklist

1. Add `compositing_params0` through `compositing_params3` to `ForwardPipelineAAAConfig`.
2. Add `u_compositingParams` to `ForwardPipelineAAA`.
3. Create, validate, and destroy `u_compositingParams`.
4. Set `uCompositingParams` before the final compositing draw.
5. Call `SetUniforms(pipeline.uniform_values, {})` before the final compositing `DrawTriangles` call.
6. Update Lua and Python bindings for the new `Vec4` config fields.
7. Add `uniform vec4 uCompositingParams[4];` to the default compositing shader source.
8. Serialize the new fields with backward-compatible JSON loading, or explicitly document them as runtime-only.
9. Add one C++, one Lua, and one Python smoke path or tutorial snippet.

This keeps the implementation local to the AAA config, the AAA final pass, the existing shader source, and the existing binding generator. It avoids new submit overloads, avoids low-level uniform lists in `ForwardPipelineAAAConfig`, and preserves the current purpose-oriented API style.
