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

Validate the five additional tutorials directly against the existing Lua
compiled assets (from the workspace root):

```powershell
python harfang3d/languages/hg_quickjs/test_tutorials.py install/js_bullet/hgjs/hgjs.exe
```

This runs each exported `main()` for 120 frames in a hidden window, including
both physics modes, and saves logs and PNG captures under
`build/hgjs-tutorials/additional-validation`. Use `--skip-physics` for a build
without Bullet, or `--renderer GL` when using GL-compiled assets. To check the
isolated fixtures, add `--tutorials build/hgjs-tutorials/source
--cwd build/hgjs-tutorials`. The preparation helper stages the physics example
separately from its renderer-only `TUTORIALS` list so existing validators can
still run with physics disabled.
