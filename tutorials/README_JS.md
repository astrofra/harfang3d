# Native JavaScript tutorials

These ports run in **HarfangJs**, the native HARFANG binding hosted by QuickJS.
HG Lua is the priority reference for native JS functionality and behavior; the
Squirrel variants provide additional examples. Web HG JS adapts on a best-effort
basis to these native projects, with explicit browser adaptations and limits.
Those limits do not restrict the native tutorials or API. See the
[compatibility priorities](../languages/hg_quickjs/README.md#compatibility-priorities).

| JavaScript entry | Source tutorials | Demonstration |
| --- | --- | --- |
| `draw_lines.js` | `draw_lines.lua`, `draw_lines.nut` | 1,000 animated line segments in one draw call |
| `draw_model_no_pipeline.js` | `draw_model_no_pipeline.lua`, `.nut` | Rotating cube and ground plane, without a scene pipeline |
| `filesystem_assets.js` | `filesystem_assets.lua`, `.nut` | Load the owl texture and print its dimensions |

Build instructions are in [HarfangJs](../languages/hg_quickjs/README.md).
From the workspace root, prepare the assets and run an entry:

```powershell
python harfang3d/languages/hg_quickjs/prepare_tutorials.py
Push-Location build/hgjs-tutorials
../../install/js_bullet/hgjs/hgjs.exe source/draw_lines.js
Pop-Location
```

Use `--assetc PATH` if the compiler is outside `install/assetc/assetc.exe`.
The preparation script stages only these examples and their dependencies, then
compiles their original shaders with assetc's native graphics API default.
The JS tutorials use the same default renderer selection as Lua. Authored sources, staging and
compiled assets stay separate. Replace the final entry name to run another
example. Escape or closing the window stops the drawing tutorials; the texture
example prints `Texture dimensions: 512x512` and exits.

If `tutorials/resources_compiled` is already compiled for your native Lua
tutorials, run the JS entries directly from `tutorials` with the same assets:

```powershell
../../install/js_bullet/hgjs/hgjs.exe draw_lines.js
```

Shader binaries and the selected renderer must match. The comparison harness
explicitly compiles with `-api GL` and passes `renderer: hg.RT_OpenGL` to the
tutorial's `main()`; normal launches do not force OpenGL. For a custom prepared
backend, use `prepare_tutorials.py --api GL` (or another API) and select that
same renderer in the calling JavaScript.

Ports use ES modules (`import * as hg from 'harfang'`), `new` for constructors,
JS arrays for vector arguments and multiple results, and BigInt for native
64-bit integers and nanoseconds (`0n`). Vector arithmetic uses methods such as
`a.add(b)`. The host awaits exported `main()`; `await nextFrame(window)` yields
between frames. `js/window.js` shares window setup and cleanup, while each entry
keeps its drawing code. The tutorial code calls
`hg.AddAssetsFolder('resources_compiled')`, as in Lua/Squirrel. The resource path
is relative to the current working directory; JS imports are relative to their
importing file. The preparation script creates that compiled folder next to
`source`. A JS program without assets can simply run as `hgjs script.js`.

The validator imports these exact `main()` functions with hidden windows and a
six-frame limit. These are native API tutorials; browser `harfangjs` has its own
window, asset and render interfaces. Scene scripts in the native engine remain
Lua scripts.
