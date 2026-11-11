# GLFW HiDPI Runtime Policy Feasibility

Date: 2026-04-28

## Executive Summary

Adding a runtime-global C++ switch that disables HiDPI behavior for HARFANG
windows is feasible.

For the Windows problem described here, the lowest-risk and most robust first
step is not to make the whole process DPI-unaware. The better first target is:

- keep the process DPI-aware on Windows
- disable GLFW automatic window scaling when requested
- ensure render sizing uses the effective window size, not the originally
  requested size
- keep default mouse coordinates in the same client-space as the window size

This solves the requested "render + mouse + window size stay coherent when
Windows scaling is above 100%" use case with limited impact on the rest of the
runtime.

If you also want a true Windows "DPI-unaware" mode where the OS bitmap-scales
the whole app, that is also feasible, but it should be treated as a separate
Windows-only mode because it is process-global and has different behavior.

## Feasibility Verdict

Feasible with low risk for the Windows opt-out mode.

Feasible with medium risk for a fully explicit cross-platform HiDPI model
(logical size vs framebuffer size).

Feasible with medium risk for a true Windows process DPI-unaware mode, but that
mode should not be the default interpretation of "disable HiDPI".

## Current Code Findings

### 1. HARFANG currently forces GLFW scale-to-monitor

`harfang/platform/glfw/window_system.cpp` unconditionally enables:

- `GLFW_SCALE_TO_MONITOR` at window creation

This means that on Windows, the initial content area can be resized according to
monitor DPI instead of staying at the exact size requested by HARFANG.

Relevant file:

- `harfang/platform/glfw/window_system.cpp`

### 2. GLFW itself forces DPI-aware mode on Windows

The bundled GLFW Win32 backend calls one of:

- `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`
- `SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE)`
- `SetProcessDPIAware()`

inside `glfwInit()`.

Relevant file:

- `extern/glfw/src/win32_init.c`

This means that on Windows, DPI policy is already process-global and already
chosen during GLFW initialization.

### 3. Initial window creation and later resize do not have identical semantics

In the bundled GLFW Win32 backend:

- initial window creation multiplies the requested client size by monitor scale
  when `scaleToMonitor` is enabled
- later `glfwSetWindowSize()` does not perform that same multiplication

Relevant file:

- `extern/glfw/src/win32_window.c`

This is an important robustness issue. Today, `NewWindow(1280, 720)` and later
`SetWindowClientSize(1280, 720)` do not necessarily describe the same visible
client area when Windows scaling is above 100%.

### 4. HARFANG render init can overwrite the effective window size

`harfang/engine/render_pipeline.cpp` does two different things:

1. `RenderInit(Window *window, ...)` queries the actual window client size and
   initializes bgfx with it.
2. `RenderInit(const char *window_title, int width, int height, ...)` then
   calls `bgfx::reset(width, height, ...)` again with the originally requested
   size.

This can reintroduce a mismatch immediately after window creation when GLFW has
already scaled the actual client area.

Relevant file:

- `harfang/engine/render_pipeline.cpp`

This is a concrete explanation for "truncated render at startup" on Windows
with scaling above 100%.

### 5. Mouse input is coherent with window client coordinates, not with a
separate framebuffer model

`harfang/platform/glfw/input_system_glfw.cpp` reads:

- window size through `GetWindowClientSize()`
- cursor position through `glfwGetCursorPos()`

This is internally coherent as long as render size and client size are the same
space.

Relevant file:

- `harfang/platform/glfw/input_system_glfw.cpp`

The problem appears when code mixes mouse coordinates with render resolution.
For example, `harfang/engine/dear_imgui.cpp` passes `mouse.x` and `mouse.y`
directly against the frame width and height supplied by the caller.

### 6. `BGFX_RESET_HIDPI` is not the Windows fix here

The bundled bgfx source shows `BGFX_RESET_HIDPI` being used for macOS OpenGL
surface setup and Vulkan surface recreation logic. No Windows-specific use was
found in the bundled bgfx backend.

Relevant files:

- `extern/bgfx/bgfx/src/glcontext_nsgl.mm`
- `extern/bgfx/bgfx/src/renderer_vk.cpp`

Conclusion: `RF_HiDPI` should not be treated as the primary Windows solution.

### 7. On Win32, GLFW framebuffer size currently equals window size

In the bundled GLFW 3.3 Win32 backend, `_glfwPlatformGetFramebufferSize()`
forwards to `_glfwPlatformGetWindowSize()`.

Relevant file:

- `extern/glfw/src/win32_window.c`

So on current Windows builds, the main issue is not a hidden framebuffer size
different from the window size. The main issue is the window size itself being
implicitly DPI-scaled at creation time, plus HARFANG resetting bgfx to the
wrong size afterwards.

## What "Disable HiDPI" Should Mean

This needs to be defined precisely, because two different behaviors are
possible.

### Recommended meaning

"Disable HiDPI" should mean:

- do not let GLFW auto-resize the content area based on monitor DPI
- keep HARFANG window size, default mouse coordinates, and render size in one
  fixed client-space
- keep the process DPI-aware so Windows does not bitmap-scale the whole app

This produces stable and predictable engine coordinates.

### Optional Windows-only meaning

"Disable HiDPI" could also mean:

- make the process DPI-unaware
- let Windows scale the whole app automatically

This is a different mode. It is valid, but it changes how the entire process is
presented by the OS and should not be merged conceptually with the recommended
mode above.

## Recommended Design

### 1. Add a runtime-global policy, frozen before `WindowSystemInit()`

The cleanest future-proof API is an enum, even if only two values are used at
first:

```cpp
namespace hg {
	enum class HiDPIMode {
		Enabled,
		Disabled,
		DpiUnaware // optional Windows-only extension
	};

	extern HiDPIMode g_hi_dpi_mode;
}
```

If you want to stay closer to the original request, a simple global bool is also
feasible, but the enum avoids overloading one flag with two different Windows
behaviors.

Important rule:

- the policy must be set before `WindowSystemInit()`
- changing it after `glfwInit()` or after the first window should assert and be
  rejected

This matches Microsoft guidance: DPI awareness must be chosen before any HWND is
created, and once it is set, later calls fail.

### 2. Windows behavior for the recommended `Disabled` mode

When `HiDPIMode::Disabled` is selected:

- do not enable `GLFW_SCALE_TO_MONITOR`
- keep using standard GLFW window sizes and cursor positions as the HARFANG
  client-space
- keep render reset size equal to that same effective client-space

Expected result on Windows:

- requested window size stays stable regardless of 125%, 150%, 200% desktop
  scaling
- mouse coordinates stay stable in the same space
- render size stays stable in the same space
- moving the window across monitors with different DPI no longer changes the
  HARFANG client-space

This directly addresses the requested behavior.

### 3. Windows behavior for optional `DpiUnaware`

If a true OS-scaled mode is still desired, treat it as a separate
Windows-specific policy:

- before `glfwInit()`, set the process default DPI awareness to unaware
- on Windows 10+, use `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE)`
- on Windows 8.1, use `SetProcessDpiAwareness(PROCESS_DPI_UNAWARE)`
- on older Windows, behavior is less clean and may require relying on default
  process behavior or a manifest strategy

This mode should be documented as:

- process-global
- Windows-only
- not compatible with changing policy after startup
- likely blurrier because Windows will bitmap-scale the application

### 4. Rendering changes

Regardless of which HiDPI mode is chosen, the render path should be corrected.

Required change:

- after creating the window, do not call `bgfx::reset()` with the original
  requested width and height
- always use the effective window size that HARFANG has chosen for that mode

For the current Windows problem, this is mandatory.

Recommended rule:

- `RenderInit(title, width, height, ...)` should create the window
- it should then query the effective size from the window backend
- the same effective size should be used for both bgfx init and the first
  bgfx reset

This alone should remove the startup truncation case currently caused by the
double-size mismatch.

### 5. Mouse and client-space I/O

For the requested Windows opt-out mode, mouse handling is straightforward:

- keep default mouse coordinates in the same client-space returned by
  `GetWindowClientSize()`
- do not rescale default mouse input when HiDPI is disabled

This preserves current gameplay and UI assumptions in tutorials that combine:

- `RenderResetToWindow(...)`
- `ReadMouse()` or `Mouse::X()/Y()`
- ImGui frame size
- screen-space projection helpers

The `raw` mouse path on Windows should remain unchanged.

### 6. Optional API cleanup for long-term robustness

If you want full cross-platform HiDPI support later, add explicit APIs for the
two coordinate spaces instead of letting one API name carry both meanings.

Suggested long-term additions:

- `GetWindowLogicalSize(...)`
- `GetWindowFramebufferSize(...)`
- `GetWindowContentScale(...)`

Then:

- window management can use logical size
- rendering can use framebuffer size where relevant
- input can choose the correct mapping explicitly

This is more work, but it is the right direction for macOS and future GLFW
upgrades.

## Risk Assessment

### Low-risk items

- adding a startup-global HiDPI policy
- disabling `GLFW_SCALE_TO_MONITOR` in HARFANG window creation
- fixing the initial `bgfx::reset()` size mismatch

### Medium-risk items

- introducing a separate Windows `DpiUnaware` mode
- reworking APIs to expose both logical size and framebuffer size
- ensuring all tutorials and Dear ImGui paths use the intended coordinate space

### Main compatibility concern

Some existing applications may implicitly rely on the current behavior where
Windows scaling enlarges the initial client area. If that behavior matters, the
default should remain `Enabled`, and the new mode should be opt-in.

## Validation Plan

Minimum Windows validation matrix:

- 100%, 150%, 200% desktop scaling on a single monitor
- create a 1280x720 window and verify:
  - full render is visible
  - `GetWindowClientSize()` matches the intended policy
  - mouse corners map correctly to viewport corners
  - Dear ImGui hit-testing is correct
- call `SetWindowClientSize(1280, 720)` after creation and verify it matches the
  size semantics of `NewWindow(1280, 720)`
- move the window between 100% and 150% monitors and verify expected behavior
  for both `Enabled` and `Disabled`

Good smoke tests from the current tree:

- `tutorials/imgui_basic.py`
- `tutorials/imgui_mouse_capture.py`
- `tutorials/mouse_scene_projection.py`
- `tutorials/render_resize_to_window.py`

## Estimated Scope

Recommended Windows-first implementation:

- about 1 to 2 engineer-days for the policy, render init fix, and validation

Longer-term explicit logical/framebuffer cleanup:

- about 2 to 4 additional engineer-days depending on how many tutorials and
  utility paths must be updated

## Final Recommendation

Implement this in two phases.

Phase 1:

- add a startup-global HiDPI policy
- make `Disabled` mean "no GLFW scale-to-monitor"
- fix `RenderInit(...width, height...)` so bgfx always uses the effective window
  size
- keep default mouse coordinates in the same client-space

This phase is enough to satisfy the requested Windows behavior and should solve
the current truncation and wrong-client-size symptoms.

Phase 2:

- if needed, add a separate `DpiUnaware` Windows-only mode
- if cross-platform HiDPI correctness becomes a goal, split logical size from
  framebuffer size explicitly in the public API

In short: the requested feature is feasible, and the most robust first
implementation is a HARFANG runtime policy layered above GLFW, not a large
GLFW-specific patch.
