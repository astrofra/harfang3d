# Android Runtime Port Feasibility

Date: 2026-04-17

## Executive Summary

Porting the HARFANG runtime to Android is technically feasible, but it is not a
simple cross-compilation task. The repository already contains an Android
platform skeleton, and the rendering path already knows how to pass native
window handles to bgfx. However, the Android code currently appears stale
relative to the modern `hg` platform APIs and would need to be refreshed rather
than merely enabled.

The recommended first target is a limited native Android runtime:

- Android NDK + CMake + Gradle packaging.
- `arm64-v8a` first.
- bgfx `RendererType::OpenGLES`, no Vulkan in the first port.
- Native app lifecycle and `ANativeWindow` integration.
- Asset loading through staged files or an Android asset backend.
- Touch mapped to the existing mouse/input abstraction for the MVP.
- Lua and core C++ runtime validation before attempting CPython packaging.

This should be treated as a large integration project. A usable MVP is likely
in the 6 to 10 engineer-week range if scope is kept narrow. A production-grade
Android runtime with strong device coverage, polished lifecycle handling,
feature parity, packaging, CI, and scripting distribution is more realistically
a multi-month effort.

## Feasibility Verdict

Feasible with high integration risk.

The largest risk is not bgfx OpenGL ES itself. bgfx has Android and OpenGL ES
support, and HARFANG already has a render initialization path that accepts a
native window handle. The larger risks are build-system portability, the stale
Android platform layer, Android application lifecycle constraints, asset
packaging, input semantics, and renderer feature parity on mobile OpenGL ES
devices.

## Current Repository State

### Existing Android Skeleton

The repository contains `harfang/platform/android`, including JNI helpers,
native window code, logcat output, an `AAssetManager`-based I/O driver, and an
input implementation stub. This is useful prior art, but it does not look
compatible with the current platform APIs.

Observed issues:

- Several Android files still use the old `gs` namespace while the runtime now
  uses `hg`.
- `harfang/platform/android/input_system.cpp` references old headers and device
  types that no longer match the current reader-based input API.
- `harfang/platform/android/window_system.cpp` uses old value-based `Window`
  signatures, while the current public API uses `Window *` and exposes more
  functions than the Android file implements.
- `harfang/platform/android/platform.cpp` references old I/O driver concepts
  that do not match the current asset/file APIs.
- There is no complete Android app envelope: no Gradle module, manifest,
  Java/Kotlin activity, `android_main`, or packaging target was found in the
  main HARFANG tree.

Conclusion: the Android folder should be treated as a migration aid, not as a
working backend.

### Build System

The top-level `CMakeLists.txt` has a small `ANDROID` branch that only adds the
`ANDROID` compile definition. It does not currently force Android-safe defaults
such as:

- `HG_USE_GLFW=OFF`
- `HG_GRAPHIC_API=GLES`
- disabling desktop-only tools or window backends
- excluding desktop X11/Wayland bgfx discovery
- excluding readline on Android target builds
- separating host tools from Android target libraries

`harfang/platform/CMakeLists.txt` already has an Android source branch and links
`android` and `log`, but it also depends on `${ANDROID_NDK}/sources/android/native_app_glue`.
This should be modernized into a clear Android CMake path driven by the NDK
toolchain or by Gradle `externalNativeBuild`.

### Rendering

`harfang/engine/render_pipeline.cpp` initializes bgfx with `bgfx::PlatformData`
and passes:

- `GetDisplay()`
- `GetWindowHandle(window)`
- the requested `bgfx::RendererType`

`harfang/cmake/harfangConfig.cmake.in` already maps `HG_GRAPHIC_API=GLES` to
`bgfx::RendererType::OpenGLES`. This is a strong positive signal: if the Android
window system returns an `ANativeWindow *` and the build enables bgfx OpenGL ES,
the runtime render entry point should be adaptable without redesigning the
renderer.

The risk is feature parity, not startup. Advanced rendering features must be
gated by bgfx capabilities and verified on real OpenGL ES devices.

### Assets

The current asset system primarily supports filesystem folders and packaged
assets. The stale Android `AAssetManager` driver is not wired into the current
asset APIs.

For an MVP, the safest route is:

1. Build Android-targeted compiled assets on the host.
2. Package them into the APK/AAB assets.
3. Extract or copy them to the app internal files directory on first launch.
4. Register the extracted directory through the existing HARFANG asset APIs.

A better long-term solution is a native Android asset backend that reads through
`AAssetManager` directly and can feed the existing asset/package abstractions
without requiring extraction.

### Input

The current input API is reader based (`AddMouseReader`, `AddKeyboardReader`,
`AddGamepadReader`, etc.). The Android implementation must be rewritten around
this API.

For the first usable runtime, touch should be mapped conservatively:

- Single-finger down/up/move maps to left mouse button and cursor position.
- Coordinates must follow HARFANG's existing convention. The SDL backend is a
  useful reference because it maps touch into mouse state and flips Y relative
  to window height.
- Multi-touch should be deferred or exposed through a new touch abstraction
  after the MVP.
- Physical keyboard and gamepad input can be added through Android key and
  motion events once the core touch path is stable.

Text input and IME support should not be part of the first milestone unless an
Android UI/editor use case requires it.

## Recommended MVP Scope

### In Scope

- Android app project able to build and install an APK.
- NDK C++ runtime build through CMake.
- `arm64-v8a` ABI first.
- Android minSdk chosen explicitly during implementation; a practical baseline
  such as API 24 is reasonable, while OpenGL ES 3.0 technically starts earlier.
- `HG_GRAPHIC_API=GLES`.
- `HG_USE_GLFW=OFF`.
- Native window backend returning `ANativeWindow *`.
- bgfx initialization with `RendererType::OpenGLES`.
- Basic forward rendering sample.
- Compiled asset sample built with `assetc -api GLES -platform android`.
- Touch-as-mouse MVP.
- Logcat output.
- Pause/resume/surface destroy handling.
- Basic OpenAL-soft sanity check if audio remains enabled.

### Out of Scope for MVP

- Vulkan.
- Full editor or Studio UI on Android.
- OpenXR/OpenVR/SRanipal.
- CPython Android packaging.
- Running asset converters on device.
- Full advanced/AAA rendering parity.
- Full multi-touch gesture API.
- IME and text composition.
- Google Play publishing pipeline.

## Android App Glue Strategy

Two approaches are viable.

### Option A: NativeActivity / Native App Glue

This is the shortest path to a native smoke test. Android creates the activity,
the NDK glue provides lifecycle callbacks and an `android_app` object, and the
runtime can initialize once `APP_CMD_INIT_WINDOW` exposes an `ANativeWindow`.

Benefits:

- Minimal Java/Kotlin.
- Direct access to `ANativeWindow`, `AInputQueue`, and `AAssetManager`.
- Good fit for the first rendering and input smoke test.

Costs:

- Less control over Java-side UI, permissions, IME, file picking, and platform
  integrations.
- Still requires manifest and Gradle packaging.

### Option B: Java/Kotlin Activity + JNI

This uses a regular Android activity with a `SurfaceView` or similar surface,
then passes the surface to native code through JNI.

Benefits:

- Better long-term control over permissions, lifecycle, UI overlays, IME,
  storage, asset extraction, and Android services.
- Easier to integrate with future Java/Kotlin-side helpers.

Costs:

- More glue code.
- More lifecycle states to bridge manually.

### Recommendation

Start with Option A for the first native rendering proof. Keep the C++ platform
layer independent enough that a custom Java/Kotlin activity can replace the app
envelope later without rewriting HARFANG's window, input, and asset backends.

## Build-System Plan

The Android build should be explicit rather than inferred from generic UNIX
paths.

Required changes:

- Add an Android CMake preset or Gradle `externalNativeBuild` configuration.
- Introduce an Android runtime target, for example `HG_BUILD_ANDROID_RUNTIME` or
  an equivalent sample app target.
- Force Android defaults:
  - `HG_USE_GLFW=OFF`
  - `HG_GRAPHIC_API=GLES`
  - OpenVR/OpenXR/SRanipal disabled
  - host-only tools disabled in target builds
- Add a first-class Android branch in `extern/bgfx/bgfx.cmake` so Android does
  not go through desktop X11/Wayland discovery.
- Ensure bgfx is compiled with OpenGL ES support, not desktop OpenGL.
- Link NDK libraries explicitly where needed: `android`, `log`, `EGL`, `GLESv3`
  or the version selected by bgfx/CMake.
- Decide whether `native_app_glue` is compiled as a source file or linked as a
  static helper target.
- Exclude readline from Android target builds.
- Build asset tools for the host, not for Android, then feed their outputs into
  the APK packaging step.

Suggested first configure shape:

```bash
cmake -S . -B build/android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DHG_GRAPHIC_API=GLES \
  -DHG_USE_GLFW=OFF \
  -DHG_BUILD_ASSETC=OFF \
  -DHG_BUILD_HG_LUA=OFF \
  -DHG_BUILD_HG_PYTHON=OFF
```

The exact option set should be adjusted after the first configure pass because
some dependencies may still be pulled indirectly.

## Rendering Plan

First target: render a clear color, then a triangle/cube, then a small HARFANG
scene using compiled assets.

Implementation notes:

- Android `Window` should wrap `ANativeWindow *`.
- `GetWindowHandle(Window *)` should return that `ANativeWindow *`.
- `GetDisplay()` can likely return `nullptr` for Android/EGL unless bgfx's
  Android path requires otherwise.
- `RenderInit(window, bgfx::RendererType::OpenGLES, callback)` should be used
  explicitly in the Android sample.
- Initialize bgfx only after the native window exists.
- On surface size/orientation changes, call the HARFANG resize path and
  `bgfx::reset`.
- On surface destruction, stop submitting frames and release the bgfx/device
  resources in a controlled order.
- Treat context loss and resume as first-class test cases.

Feature policy:

- GLES 3.0 is the recommended minimum rendering target for the MVP.
- GLES 3.1 features, compute dispatch, image load/store, and advanced effects
  should be capability-gated and deferred until after basic scene rendering.
- Do not assume desktop texture formats. BCn formats are not a mobile baseline;
  prefer ETC2/ASTC strategy later, or raw/uncompressed textures for the first
  smoke tests.

## Asset Pipeline Plan

The host asset pipeline must produce Android/GLES-compatible assets.

Required tasks:

- Validate `assetc -api GLES -platform android` on a small sample.
- Confirm shader profiles produced by bgfx shaderc are valid for the chosen
  OpenGL ES baseline.
- Package compiled assets into the APK/AAB.
- MVP path: extract assets to internal files storage and call existing folder or
  package asset APIs.
- Long-term path: implement a current `AAssetManager` backend compatible with
  HARFANG's current asset abstractions.
- Add clear documentation for Android asset build commands.

Open question: whether HARFANG should expose Android assets as a new asset
mount type or hide them behind the existing package/folder APIs.

## Input Plan

Required tasks:

- Replace the stale Android input implementation with reader callbacks matching
  the current input system.
- Pump Android input events from `AInputQueue` or the native app glue callback.
- Map:
  - touch down/up/move to `MouseState`
  - back/menu/keyboard keys to `KeyboardState`
  - gamepad buttons and axes to `GamepadState` after the touch MVP
- Preserve HARFANG coordinate conventions.
- Decide whether touch pressure, multiple pointers, and gestures deserve a new
  HARFANG `TouchState` API after the MVP.

MVP acceptance: one finger can control an existing mouse-driven camera or UI
interaction consistently across portrait and landscape.

## Filesystem and Platform Services

Android has distinct locations for read-only packaged assets and writable app
data. The runtime should not assume the process working directory is useful.

Required tasks:

- Define app writable directories from Android context paths.
- Provide Android implementations or no-op stubs for unsupported file dialogs.
- Route logs to logcat.
- Decide how screenshots, saves, downloaded files, and user-generated content
  should be stored.
- Avoid relying on direct access to arbitrary external storage in the MVP.

## Audio Plan

OpenAL-soft includes Android-oriented backends, so audio is probably feasible.
It should still be validated separately because Android audio routing, latency,
pause/resume, and focus changes are platform-specific.

MVP target:

- Build with audio enabled if OpenAL-soft configures cleanly.
- Play one short sound after renderer and asset loading are stable.
- Defer capture, low-latency tuning, Bluetooth routing, and audio focus polish.

## Language Bindings

Lua embedded in the runtime is a plausible early target, assuming the target
build avoids desktop-only readline assumptions. CPython on Android should be
treated as a separate project because it introduces packaging, ABI, module,
filesystem, and distribution constraints beyond the core runtime port.

Recommended order:

1. C++ sample app.
2. Lua script loaded from packaged assets.
3. Evaluate Python only after the native runtime is stable.

## Risk Register

| Area | Risk | Mitigation |
| --- | --- | --- |
| Android platform layer | Existing code is stale and may not compile | Rewrite against current `hg` APIs using old code only as reference |
| CMake dependencies | Desktop assumptions leak into Android target | Add explicit Android branches and disable host-only targets |
| bgfx setup | Desktop GL/X11 path selected accidentally | Force `HG_GRAPHIC_API=GLES` and Android bgfx configuration |
| Lifecycle | Surface loss, pause/resume, and orientation break rendering | Make lifecycle smoke tests mandatory early |
| Assets | APK assets are not normal writable files | Start with extraction to internal storage; design native asset backend later |
| Shaders | Desktop shader assumptions fail on GLES | Compile with `assetc -api GLES -platform android`; run device shader tests |
| Texture formats | Desktop compressed formats are unavailable | Use raw/ETC2/ASTC policy per asset class |
| Input | Touch does not map cleanly to existing mouse APIs | Ship touch-as-mouse MVP; design real touch API separately |
| Advanced rendering | Compute/HiZ/AAA features may require GLES 3.1+ | Capability-gate advanced paths and defer parity |
| Scripting | Python packaging expands scope sharply | Keep CPython out of MVP |
| Device coverage | GLES drivers vary across vendors | Test at least one Adreno, one Mali, and one emulator/device baseline |

## Plan of Attack

### Phase 0: Decisions and Baseline

Duration: 2 to 4 days.

Outputs:

- Select app envelope for MVP: NativeActivity/native app glue recommended.
- Select first ABI: `arm64-v8a`.
- Select Android platform level and document rationale.
- Select first rendering baseline: GLES 3.0, no Vulkan.
- Select sample content small enough to debug quickly.

Exit criteria:

- Written Android target scope.
- One known device or emulator selected for smoke testing.
- Host asset build path identified.

### Phase 1: Android Build Bootstrap

Duration: 1 to 2 weeks.

Outputs:

- Android CMake preset or Gradle module.
- Target build can configure with the NDK toolchain.
- Desktop-only dependencies are excluded from the Android target.
- bgfx compiles with OpenGL ES support.
- HARFANG core libraries compile for `arm64-v8a`.

Exit criteria:

- Clean configure and compile of a minimal native Android library.
- No accidental GLFW, X11, Wayland, desktop OpenGL, or readline dependency in
  the Android target.

### Phase 2: App Shell and Lifecycle

Duration: 1 to 2 weeks.

Outputs:

- Manifest and Gradle packaging.
- Native entry point.
- Logcat output.
- Native window creation and destruction callbacks.
- Clear-screen bgfx initialization using `RendererType::OpenGLES`.

Exit criteria:

- APK installs and starts on device.
- App clears the screen.
- Pause/resume and rotate/surface recreation do not crash.

### Phase 3: Platform Layer Refresh

Duration: 1 to 2 weeks.

Outputs:

- Android `Window` implementation updated to current HARFANG API.
- Android input implementation updated to current reader API.
- JNI helpers moved to `hg` namespace and cleaned up.
- Unsupported platform services stubbed explicitly.
- App writable paths exposed to runtime code.

Exit criteria:

- Android platform backend builds without legacy API references.
- Single-touch mouse emulation works in a sample.
- Back key and basic keyboard events are visible.

### Phase 4: Assets and Sample Content

Duration: 1 to 2 weeks.

Outputs:

- Host-side Android/GLES asset build command.
- APK packaging for compiled assets.
- MVP extraction or package mounting strategy.
- Sample scene/shaders/textures loaded through HARFANG APIs.

Exit criteria:

- Android app renders content loaded from packaged assets.
- Missing asset failures produce actionable logcat messages.

### Phase 5: Rendering Capability Pass

Duration: 2 to 4 weeks.

Outputs:

- Basic forward pipeline verified.
- Capability gates for unsupported mobile features.
- Texture format policy for Android.
- Optional ImGui/debug overlay validation.
- List of unsupported or deferred effects.

Exit criteria:

- Small HARFANG scene renders correctly on at least two physical GPU families,
  or one physical GPU plus emulator if hardware is limited.
- Unsupported features fail gracefully instead of crashing.

### Phase 6: Audio, Gamepad, and Usability

Duration: 1 to 2 weeks.

Outputs:

- Basic sound playback.
- Android gamepad mapping.
- Improved touch controls or sample camera controls.
- App lifecycle handling for audio pause/resume.

Exit criteria:

- Sample can be controlled without desktop peripherals.
- Audio does not continue incorrectly through pause/resume.

### Phase 7: Hardening and CI

Duration: 2+ weeks.

Outputs:

- Repeatable Android build documentation.
- CI cross-compile job.
- Optional device/emulator smoke test.
- Crash/log collection instructions.
- Android runtime limitations documented.

Exit criteria:

- Fresh checkout can build the APK using documented commands.
- Smoke test verifies launch, render, input, asset load, and pause/resume.

## Success Criteria for the MVP

- A documented command builds an Android APK from a fresh checkout.
- The APK installs on an `arm64-v8a` device.
- The app initializes HARFANG and bgfx with OpenGL ES.
- A simple scene renders from packaged compiled assets.
- One-finger touch controls a mouse-style interaction.
- The app survives pause/resume and orientation/surface recreation.
- Logs are visible in logcat.
- The implementation does not depend on Vulkan.
- Known limitations are documented.

## Immediate Next Steps

1. Add a minimal Android Gradle app or CMake preset dedicated to a native smoke
   test.
2. Force `HG_GRAPHIC_API=GLES` and `HG_USE_GLFW=OFF` for Android.
3. Patch bgfx CMake integration so Android does not enter desktop Linux window
   discovery.
4. Replace the stale Android window backend with a current `Window *` API
   implementation around `ANativeWindow *`.
5. Build a clear-screen APK.
6. Add touch-as-mouse input.
7. Add Android/GLES asset compilation and package a minimal scene.

## References

- Android NDK CMake guide:
  <https://developer.android.com/ndk/guides/cmake>
- Android NativeActivity reference:
  <https://developer.android.com/ndk/reference/struct/a-native-activity>
- Android native app glue reference:
  <https://developer.android.com/reference/games/game-activity/group/android-native-app-glue>
- Android NDK input reference:
  <https://developer.android.com/ndk/reference/group/input>
- Android NDK native window reference:
  <https://developer.android.com/ndk/reference/group/a-native-window>
- Android NDK asset manager reference:
  <https://developer.android.com/ndk/reference/group/asset>
- Android OpenGL ES guide:
  <https://developer.android.com/develop/ui/views/graphics/opengl/about-opengl>
- bgfx API reference:
  <https://bkaradzic.github.io/bgfx/bgfx.html>
- bgfx overview:
  <https://bkaradzic.github.io/bgfx/overview.html>
