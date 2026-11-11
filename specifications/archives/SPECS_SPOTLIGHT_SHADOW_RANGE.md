# Spotlight Shadow Near/Far Implementation Spec

This document is a technical implementation brief for an autonomous coding agent.

Its purpose is to reproduce, in another branch or repository state, the spotlight shadow-range feature that now exists in this Harfang codebase.

The scope of this document is limited to:

- core C++ runtime support in Harfang
- Lua/Python binding exposure through Harfang bindings
- scene serialization and compatibility implications
- native validation
- AssetC rebuild requirements

This document intentionally excludes Studio-specific UI work.

## 1. Goal

Expose per-spotlight shadow frustum range control through the public `Scene` / `Light` API.

The feature must allow end users to control:

- spotlight shadow near plane
- spotlight shadow far plane

These values must:

- be stored on scene light components
- be serialized with scenes
- propagate to forward-pipeline spotlight data
- be consumed by spotlight shadow-map generation instead of hardcoded constants

The public intent is to let users tighten spotlight shadow-map depth range for better practical control and precision.

## 2. Target Public API

The public API that must exist at feature parity is:

### `Light`

- `GetShadowNear()`
- `SetShadowNear(float)`
- `GetShadowFar()`
- `SetShadowFar(float)`

### `Scene`

- `GetLightShadowNear(ComponentRef)`
- `SetLightShadowNear(ComponentRef, float)`
- `GetLightShadowFar(ComponentRef)`
- `SetLightShadowFar(ComponentRef, float)`

### Spotlight creation helpers

Extend spotlight creation entry points so they accept optional shadow range parameters at the end of the signature:

- `Scene::CreateSpotLight(...)`
- free helper `CreateSpotLight(scene, mtx, ...)`

These new arguments must be optional and appended at the end to preserve existing call sites.

## 3. Behavioral Contract

### Defaults

Default values must preserve previous behavior:

- `shadow_near = 0.1f`
- `shadow_far = 100.f`

### Scope

These values are meaningful only for spotlights.

Point and linear lights may carry the fields in serialized/component storage, but they do not use them in the forward spotlight shadow pass.

### Validation and clamping

Setter behavior must match the camera near/far pattern:

- `shadow_near` must be clamped to `>= 0.0001f`
- `shadow_near` must stay `< shadow_far`
- `shadow_far` must stay `>= shadow_near + 0.0001f`

In practice, the implementation should behave like:

- near setter clamps to `[0.0001f, shadow_far - 0.0001f]`
- far setter clamps to `>= shadow_near + 0.0001f`

## 4. Non-Goals

Do not extend the scope beyond the following:

- no public binding exposure of `ForwardPipelineLight.shadow_near`
- no public binding exposure of `ForwardPipelineLight.shadow_far`
- no change to public binding helpers such as `MakeForwardPipelineSpotLight(...)`
- no Studio/editor work in this implementation spec
- no redesign of spotlight radius or lighting attenuation behavior

The feature is strictly about the spotlight shadow frustum range used by the forward pipeline shadow pass.

## 5. Required Engine Changes

### 5.1 Main files

The implementation touches these core files:

- `harfang/engine/scene.h`
- `harfang/engine/component.cpp`
- `harfang/engine/node.h`
- `harfang/engine/scene.cpp`
- `harfang/engine/forward_pipeline.h`
- `harfang/engine/forward_pipeline.cpp`
- `harfang/engine/scene_forward_pipeline.cpp`
- `harfang/engine/scene_load_binary.cpp`
- `harfang/engine/scene_load_json.cpp`

### 5.2 Extend scene light storage

Add two fields to `Scene::Light_`:

- `float shadow_near{0.1f};`
- `float shadow_far{100.f};`

These fields are component data, not transient render-only values.

### 5.3 Add `Scene` accessors

Add getter/setter functions on `Scene` for light shadow range:

- `GetLightShadowNear`
- `SetLightShadowNear`
- `GetLightShadowFar`
- `SetLightShadowFar`

These must operate on `ComponentRef`.

### 5.4 Add `Light` accessors

Expose matching convenience methods on `Light`:

- `GetShadowNear`
- `SetShadowNear`
- `GetShadowFar`
- `SetShadowFar`

These should forward to the owning `Scene`.

### 5.5 Extend spotlight creation

Update spotlight creation overloads so that scene-created spotlights can initialize shadow range directly.

The new parameters must be optional and appended at the end of the signature, after existing spotlight parameters.

This preserves binary/source compatibility for existing call sites.

### 5.6 Extend forward-pipeline light data

Add internal fields to `ForwardPipelineLight`:

- `shadow_near`
- `shadow_far`

These are internal propagation fields.

They do not need new public script bindings.

### 5.7 Propagate scene values to render values

In `GetSceneForwardPipelineLights(...)`, when converting scene lights into `ForwardPipelineLight`, copy:

- `Light::GetShadowNear()`
- `Light::GetShadowFar()`

for spotlights.

### 5.8 Use the values in spotlight shadow-map generation

In the spotlight shadow generation path, replace the hardcoded projection range:

- near `0.1f`
- far `100.f`

with the spotlight-carried values.

The critical behavior change is in the call that builds the spotlight perspective projection matrix.

## 6. Serialization Requirements

### 6.1 JSON scene format

Scene JSON save/load must persist:

- `shadow_near`
- `shadow_far`

for lights.

### 6.2 Binary scene format

Scene binary save/load must also persist:

- `shadow_near`
- `shadow_far`

Because this extends the serialized layout of lights, the scene binary format version must be incremented.

In the current implementation, the binary scene format version was bumped:

- from `13`
- to `14`

### 6.3 Compatibility implication

Older binaries that only understand the previous scene binary version will not correctly consume scenes saved with the new version.

This matters for:

- runtime tools
- any packaged AssetC built against the previous version
- any workflow that saves and reloads scene binaries

## 7. Binding Requirements

### 7.1 File target

- `binding/bind_harfang.py`

### 7.2 Bind new `Light` methods

Expose:

- `Light.GetShadowNear`
- `Light.SetShadowNear`
- `Light.GetShadowFar`
- `Light.SetShadowFar`

### 7.3 Extend spotlight creation bindings

Extend the bound overloads for:

- `Scene.CreateSpotLight(...)`
- free `hg.CreateSpotLight(...)`

with optional trailing parameters:

- `?float shadow_near`
- `?float shadow_far`

### 7.4 Explicit non-goal in bindings

Do not expose the new internal `ForwardPipelineLight` fields in public script bindings.

Do not modify `MakeForwardPipelineSpotLight(...)` script API.

The feature is meant to be driven through `Scene` / `Light`, not manual forward-pipeline light construction.

## 8. AssetC Impact

### 8.1 AssetC source-level impact

This feature does not require a new AssetC feature or a new asset-processing code path.

There is no essential public AssetC API change tied to spotlight shadow near/far.

### 8.2 Why AssetC still matters

AssetC is tightly coupled to Harfang scene serialization because it writes scene binary assets and tracks the binary scene format version.

In practice:

- AssetC uses Harfang scene serialization
- AssetC records the scene binary format version

Therefore, once the scene binary format is bumped, the AssetC binary must be rebuilt from the updated source tree.

### 8.3 Agent instruction regarding AssetC

If reproducing this feature in another branch or repo state:

1. implement the Harfang runtime and serialization changes
2. rebuild `assetc`
3. rebuild its toolchain package if your distribution bundles it
4. ensure downstream packaged assets are produced by the rebuilt toolchain

Do not assume an old packaged `assetc` remains valid after the scene binary version bump.

## 9. Testing Requirements

### 9.1 Native light API test

Add a C++ test that validates:

- a spotlight starts with defaults `0.1f / 100.f`
- explicit `SetShadowNear` / `SetShadowFar` values persist
- clamp behavior is correct

At minimum, cover:

- near too small
- near greater than or equal to far
- far lower than near

### 9.2 Forward-pipeline propagation test

Add a C++ test that:

1. creates a spotlight with explicit shadow near/far
2. generates forward-pipeline light data from the scene
3. verifies the resulting `ForwardPipelineLight` carries the same values

### 9.3 Serialization persistence test

Add persistence coverage so that both JSON and binary scene save/load keep:

- `shadow_near`
- `shadow_far`

### 9.4 Binding smoke coverage

If the existing test layout allows it, add at least a minimal smoke path that:

- creates a spotlight from bindings
- sets shadow near/far
- reads them back

## 10. Critical Details

These points must not be simplified away.

### 10.1 This is shadow-frustum control, not light-radius control

`shadow_near` and `shadow_far` affect the spotlight shadow-map projection range.

They do not redefine:

- spotlight radius
- attenuation model
- light influence volume

### 10.2 Public control belongs on `Scene` / `Light`

The feature should be reachable through regular scene/component APIs.

Do not force users to rebuild equivalent manual `ForwardPipelineLight` objects just to control spotlight shadow range.

### 10.3 Defaults must preserve old behavior

If callers do nothing, the runtime behavior should remain equivalent to the old hardcoded values.

This is why the defaults must remain:

- `0.1f`
- `100.f`

### 10.4 Serialization is part of the feature

This feature is incomplete if the values only affect runtime objects but disappear on save/load.

Persisting the values is mandatory.

### 10.5 AssetC must be rebuilt after the binary format bump

Even though AssetC does not gain new user-facing spotlight logic, it is part of the compatibility surface.

Treat rebuilding AssetC as part of feature delivery.

## 11. Recommended Implementation Order

If you are an autonomous coding agent, use this order:

1. inspect `scene.h`, `component.cpp`, and `node.h`
2. add component storage and `Scene` / `Light` accessors
3. extend spotlight creation helpers
4. extend `ForwardPipelineLight`
5. propagate values in scene-to-forward conversion
6. replace hardcoded spotlight shadow projection range
7. extend JSON and binary serialization
8. expose the new API in `binding/bind_harfang.py`
9. add native tests
10. rebuild and validate `assetc`

Do not start from bindings or tests first. The runtime and serialization behavior must exist first.

## 12. Acceptance Criteria

The implementation is acceptable only if all of the following are true:

- spotlights expose `GetShadowNear/Far` and `SetShadowNear/Far`
- `Scene` exposes matching light accessors
- spotlight creation helpers accept optional shadow near/far arguments
- spotlight shadow-map generation uses per-light near/far instead of hardcoded values
- values persist through JSON scene save/load
- values persist through binary scene save/load
- binary scene format version is updated appropriately
- forward-pipeline internal light data carries the per-spotlight values
- native tests cover defaults, clamping, propagation, and persistence
- AssetC is rebuilt against the updated scene binary format

## 13. Concrete Reference Files

If you need implementation references, inspect these exact files:

Core runtime:

- `harfang/engine/scene.h`
- `harfang/engine/component.cpp`
- `harfang/engine/node.h`
- `harfang/engine/scene.cpp`
- `harfang/engine/forward_pipeline.h`
- `harfang/engine/forward_pipeline.cpp`
- `harfang/engine/scene_forward_pipeline.cpp`
- `harfang/engine/scene_load_binary.cpp`
- `harfang/engine/scene_load_json.cpp`

Bindings:

- `binding/bind_harfang.py`

Tests:

- `harfang/tests/engine/scene.cpp`

## 14. Final Instruction to the Agent

Implement exact feature parity for spotlight shadow near/far control through Harfang scene lights.

Do not reframe the problem as a general light redesign.
Do not push public control into manual forward-pipeline light construction.
Do not omit serialization.
Do not forget the AssetC rebuild requirement introduced by the binary scene format change.

Deliver a narrow, stable feature that lets users tighten spotlight shadow range through normal Harfang scene APIs while preserving previous behavior by default.
