# Native JavaScript tutorials

These ports run in **HarfangJs**, the native HARFANG binding hosted by QuickJS.
HG Lua is the priority reference for native JS functionality and behavior; the
Squirrel variants provide additional examples. Web HG JS adapts on a best-effort
basis to these native projects, with explicit browser adaptations and limits.
Those limits do not restrict the native tutorials or API. See the
[compatibility priorities](../languages/hg_quickjs/README.md#compatibility-priorities).

**HarfangJs native consumes exactly the same compiled assets as HG Lua.**
Scenes, models, textures and shaders use the native HARFANG formats and the same
`assetc` compiler. An existing Lua `resources_compiled` directory can be reused
directly with the matching renderer. Only HG JS Web needs Web-format assets;
no Web conversion is involved in these tutorials.
The native scene loader accepts both binary scene versions 10 (the existing Lua
tutorial assets) and 11 (current `assetc` output); recompiling the Lua assets is
not required to run these JavaScript ports.

| JavaScript entry | Source tutorials | Demonstration |
| --- | --- | --- |
| `draw_lines.js` | `draw_lines.lua`, `draw_lines.nut` | 1,000 animated line segments in one draw call |
| `draw_model_no_pipeline.js` | `draw_model_no_pipeline.lua`, `.nut` | Rotating cube and ground plane, without a scene pipeline |
| `filesystem_assets.js` | `filesystem_assets.lua`, `.nut` | Load the owl texture and print its dimensions |
| `game_mouse_flight.js` | `game_mouse_flight.lua`, `.nut` | Mouse-controlled paper plane with a chase camera and a 2D cursor |
| `scene_many_nodes.js` | `scene_many_nodes.lua`, `.nut` | Animate 10,201 spheres with a spotlight and a 4096x4096 shadow map |
| `scene_pbr.js` | `scene_pbr.lua`, `.nut` | Display the original PBR material scene at 940x720 |
| `scene_aaa.js` | `scene_aaa.lua` | Rotate the Toyota engine with AAA; `main({aaa:false})` selects forward |
| `draw_lines_starfield.js` | `draw_lines_starfield.lua`, `.nut` | 1,000 moving stars with projected, gradient line trails |
| `draw_text.js` | `draw_text.lua`, `.nut` | Centered text, font atlas, color uniform and alpha blending |
| `imgui_basic.js` | `imgui_basic.lua`, `.nut` | ImGui frame lifecycle, a draggable window and resize handling |
| `scene_draw_to_multiple_viewports.js` | `scene_draw_to_multiple_viewports.lua`, `.nut` | Four views of one animated scene, sharing shadow-map preparation |
| `physics_impulse.js` | `physics_impulse.lua`, `.nut` | Bullet cube suspension using force or impulse; Space switches mode |
| `draw_and_create_model_no_pipeline.js` | `draw_and_create_model_no_pipeline.lua`, `.nut` | Build a cube vertex by vertex with ModelBuilder and draw it without a pipeline |
| `material_update_value.js` | `material_update_value.lua`, `.nut` | Toggle a material texture every second and update the pipeline shader variant |
| `scene_instances.js` | `scene_instances.lua`, `.nut` | Twenty animated biped instances; S spawns an actor, D removes one |
| `scene_draw_to_texture.js` | `scene_draw_to_texture.lua`, `.nut` | Render a PBR scene to a 512x512 texture displayed on a rotating cube |
| `scene_lua_script.js` | `scene_lua_script.lua`, `.nut` | Exchange values and call functions in a SceneLuaVM, without a window |
| `draw_text_over_models.js` | `draw_text_over_models.lua`, `.nut` | Draw centered 2D text over a rotating 3D cube, clearing only the overlay's depth buffer |
| `imgui_edit.js` | `imgui_edit.lua`, `.nut` | Edit the clear color with presets and a color picker, and change the ImGui output view |
| `scene_light_priority.js` | `scene_light_priority.lua`, `.nut` | Animate sixteen lights and prioritize those closest to a sphere |
| `physics_manual_setup.js` | `physics_manual_setup.lua`, `.nut` | Manually attach a dynamic rigid body and a cube collision shape |
| `model_builder.js` | `model_builder.lua`, `.nut` | Rebuild an animated 40x40 grid, computing triangle indices and vertex normals |
| `imgui_mouse_capture.js` | `imgui_mouse_capture.lua`, `.nut` | Distinguish ImGui mouse capture from clicks in the scene |
| `render_resize_to_window.js` | `render_resize_to_window.lua`, `.nut` | Resize the render buffer to the window and report its dimensions |
| `input_list_devices.js` | `input_list_devices.lua`, `.nut` | Print native mouse, keyboard and gamepad device names without a window |
| `physics_overrides_matrix.js` | `physics_overrides_matrix.lua`, `.nut` | Inspect transforms versus physics matrices, edit positions and remove/recreate a rigid body |
| `scene_spot_shadow_clip.js` | `scene_spot_shadow_clip.lua`, `.nut` | Animate a spot light's shadow near plane and display the near/far values |

Build instructions are in [HarfangJs](../languages/hg_quickjs/README.md).
Run directly from `tutorials`, using the same `resources_compiled` directory as
Lua. No JavaScript-specific asset preparation is needed. From the workspace root:

```powershell
Push-Location harfang3d/tutorials
../../install/js_bullet/hgjs/hgjs.exe game_mouse_flight.js
../../install/js_bullet/hgjs/hgjs.exe scene_many_nodes.js
../../install/js_bullet/hgjs/hgjs.exe scene_pbr.js
../../install/js_bullet/hgjs/hgjs.exe scene_aaa.js
../../install/js_bullet/hgjs/hgjs.exe draw_lines_starfield.js
../../install/js_bullet/hgjs/hgjs.exe draw_text.js
../../install/js_bullet/hgjs/hgjs.exe imgui_basic.js
../../install/js_bullet/hgjs/hgjs.exe scene_draw_to_multiple_viewports.js
../../install/js_bullet/hgjs/hgjs.exe physics_impulse.js
../../install/js_bullet/hgjs/hgjs.exe draw_and_create_model_no_pipeline.js
../../install/js_bullet/hgjs/hgjs.exe material_update_value.js
../../install/js_bullet/hgjs/hgjs.exe scene_instances.js
../../install/js_bullet/hgjs/hgjs.exe scene_draw_to_texture.js
../../install/js_bullet/hgjs/hgjs.exe scene_lua_script.js
../../install/js_bullet/hgjs/hgjs.exe draw_text_over_models.js
../../install/js_bullet/hgjs/hgjs.exe imgui_edit.js
../../install/js_bullet/hgjs/hgjs.exe scene_light_priority.js
../../install/js_bullet/hgjs/hgjs.exe physics_manual_setup.js
../../install/js_bullet/hgjs/hgjs.exe model_builder.js
../../install/js_bullet/hgjs/hgjs.exe imgui_mouse_capture.js
../../install/js_bullet/hgjs/hgjs.exe render_resize_to_window.js
../../install/js_bullet/hgjs/hgjs.exe input_list_devices.js
../../install/js_bullet/hgjs/hgjs.exe physics_overrides_matrix.js
../../install/js_bullet/hgjs/hgjs.exe scene_spot_shadow_clip.js
Pop-Location
```

The JS tutorials use the same default renderer selection as Lua. Replace the
entry name to run another example. Escape or closing the window stops the drawing
tutorials; the texture example prints `Texture dimensions: 512x512` and exits.

Move the mouse to steer in `game_mouse_flight.js`. The scene tutorials use the
original playground, paper plane, PBR materials, lighting probe and pipeline
shaders. `scene_many_nodes.js` preserves the Lua
grid size and animation phase, accounting for JavaScript's zero-based arrays.
The scene ports also keep the Lua window sizes and MSAA settings.
`scene_aaa.js` uses real AAA on native. Its separate Web experiment uses explicit
AAA and animation stubs with forward rendering; see the
[port specification](../specifications/SPECS_HARFANGJS_WEB_NATIVE_SCENE_AAA.md).

`draw_lines_starfield.js` keeps star depths strictly positive and wraps them even
after a long frame, avoiding division by zero in the Lua example's projection.
`draw_text.js` needs `font/default.ttf` and `core/shader/font` in the compiled
assets. `imgui_basic.js` uses `core/shader/imgui` and `imgui_image`; ImGui owns
these programs and releases them during shutdown. All these sources already
exist in `resources` and are compiled by the normal asset build.

`scene_draw_to_multiple_viewports.js` updates the scene once per frame, prepares
the shared render data once, then threads the returned view ID through each
viewport's preparation and submission. Each view uses its own aspect ratio.
`physics_impulse.js` requires the Bullet build, starts in force mode, and prints
the active mode when Space is pressed. The exported `main({useForce:false})`
starts in impulse mode for automated checks. Its fixed physics step is 1/60 s;
frame deltas are capped at 50 ms to limit catch-up after a pause.

The next five ports retain the original tutorials' variable names, section
order and comments. `draw_and_create_model_no_pipeline.js` spells out the six
cube faces, including positions, normals, UVs and triangle indices.
`material_update_value.js` uses the original `textures/squares.png` and refreshes
the shader variant whenever the texture is attached or removed.

`scene_instances.js` starts twenty independent instances of `biped/biped.scn`
in the playground. Each actor switches between idle, walk and run, with its own
animation and movement. Press S to add an actor and D to remove the oldest;
removal also stops its animation and destroys the instance content.
`scene_draw_to_texture.js` renders the materials scene into a 4x MSAA framebuffer
and samples its color texture using `shaders/texture` on a rotating cube.

`scene_lua_script.js` runs in the console without assets or a renderer. It reads
and writes a Lua variable, passes a native Script object to Lua, checks a missing
function, and retrieves a string result. `Pack`/`Unpack` transfer the values;
Lua integers return as JavaScript BigInt and native result lists use `at(0)`.

`draw_text_over_models.js` uses view 0 for the models and view 1 for text; the
second view preserves the first view's color buffer. `imgui_edit.js` retains the
original controls and unpacks native multiple returns as JavaScript arrays,
including the visibility result from `ImGuiBegin` with an open flag.

`scene_light_priority.js` preserves the Lua animation phase and assigns each
light a priority equal to the negative distance from the sphere. The native
pipeline selects lights for its available slots. `physics_manual_setup.js`
requires Bullet and demonstrates the RigidBody and Collision components directly;
its cube falls onto the static ground using a 1/60 s physics step.

`model_builder.js` uses the original `probe_scene/pbr.scn` lighting environment.
It builds 1,681 vertices and 3,200 triangles, accumulates area-weighted normals,
and replaces the model as its wave changes. Grid indices are zero-based in JS.
The animation accumulates the host's frame delta because `runWindow` does not
advance the global `TickClock` used by the Lua example. The original helper
functions, comments and optional rotation example remain visible in the port.

`imgui_mouse_capture.js` keeps the background black while ImGui captures the
mouse; holding the left button outside the GUI turns it red. The text field
demonstrates editing without triggering that scene interaction.
`render_resize_to_window.js` starts at 512x512 and calls `RenderResetToWindow`
directly, using the window passed to its setup callback by `runWindow`. Resize
the window to see the viewport adapt and the new dimensions printed.

`input_list_devices.js` prints the registered native device names and exits.
Gamepad slots may be listed even when no controller is connected; names are not
a connectivity test. Native string lists are read using `size()` and `at()`.
`physics_overrides_matrix.js` requires Bullet. Its ImGui controls compare the
Transform position to the physics-driven world matrix, reset the Transform,
and destroy or recreate the cube's rigid body. Collision outlines show the
actual physics shape. `scene_spot_shadow_clip.js` animates the shadow near plane
between 0.1 and 7.5 while keeping the far plane at 18; close objects progressively
stop casting shadows, and an overlay displays the current distances.

Shader binaries and the selected renderer must match. The comparison harness
explicitly compiles with `-api GL` and passes `renderer: hg.RT_OpenGL` to the
tutorial's `main()`; normal launches do not force OpenGL.

Ports use ES modules (`import * as hg from 'harfang'`), `new` for constructors,
JS arrays for vector arguments and multiple results, and BigInt for native
64-bit integers and nanoseconds (`0n`). Vector arithmetic uses methods such as
`a.add(b)`. The host awaits exported `main()`; `await nextFrame(window)` yields
between frames. `js/window.js` shares window setup and cleanup, while each entry
keeps its drawing code. The tutorial code calls
`hg.AddAssetsFolder('resources_compiled')`, as in Lua/Squirrel. The resource path
is relative to the current working directory; JS imports are relative to their
importing file. A JS program without assets can simply run as `hgjs script.js`.

The validator imports these exact `main()` functions with hidden windows and a
six-frame limit. These are native API tutorials; browser `harfangjs` has its own
window, asset and render interfaces. Scene scripts in the native engine remain
Lua scripts.

For automated validation in an isolated directory, the optional
`languages/hg_quickjs/prepare_tutorials.py` helper stages the examples and their
original resources under `build/hgjs-tutorials`, then invokes native `assetc`.
It creates test fixtures, not a separate native-JavaScript asset format, and is
not part of the normal launch workflow:

```powershell
python harfang3d/languages/hg_quickjs/prepare_tutorials.py --assetc install/js_bullet/assetc/assetc.exe
Push-Location build/hgjs-tutorials
../../install/js_bullet/hgjs/hgjs.exe source/scene_pbr.js
Pop-Location
```

Use `--api GL` only when testing with `renderer: hg.RT_OpenGL`; compiled shader
binaries must always match the selected renderer.

Validate the twenty additional tutorials directly against the existing Lua
compiled assets (from the workspace root):

```powershell
python harfang3d/languages/hg_quickjs/test_tutorials.py install/js_bullet/hgjs/hgjs.exe
```

This runs each graphical `main()` for 120 frames in a hidden window, including
both force/impulse modes, checks the Lua VM example's assertions and lists
input devices in the console,
and saves logs and PNG captures under
`build/hgjs-tutorials/additional-validation`. Use `--skip-physics` for a build
without Bullet, or `--renderer GL` when using GL-compiled assets. To check the
isolated fixtures, add `--tutorials build/hgjs-tutorials/source
--cwd build/hgjs-tutorials`. The preparation helper stages the physics example
separately from its renderer-only `TUTORIALS` list so existing validators can
still run with physics disabled. Console examples are staged through the
separate `CONSOLE_TUTORIALS` list and do not require a screenshot.
Captures are requested three frames before the end of each run so animations
and physics have time to progress. Direct callers can choose `captureFrame`;
the shared window helper keeps frame 3 as its default for other validators.

To validate only the second batch of five ports:

```powershell
python harfang3d/languages/hg_quickjs/test_tutorials.py install/js_bullet/hgjs/hgjs.exe --only draw_and_create_model_no_pipeline material_update_value scene_instances scene_draw_to_texture scene_lua_script
```

To validate only the third batch and the grid's geometric invariants:

```powershell
python harfang3d/languages/hg_quickjs/test_tutorials.py install/js_bullet/hgjs/hgjs.exe --only draw_text_over_models imgui_edit scene_light_priority physics_manual_setup model_builder
install/js_bullet/hgjs/hgjs.exe harfang3d/languages/hg_quickjs/test_model_builder.js
```

To validate only the latest five ports:

```powershell
python harfang3d/languages/hg_quickjs/test_tutorials.py install/js_bullet/hgjs/hgjs.exe --only imgui_mouse_capture render_resize_to_window input_list_devices physics_overrides_matrix scene_spot_shadow_clip
```
