# HarfangJs: native JavaScript binding

`HG_BUILD_HG_JS=ON` adds the `HarfangJs` CMake target, producing `hgjs.exe`,
and the static `hg_quickjs` binding library. QuickJS lives in the language layer.
The engine, its Lua VM, scene systems and embedded Lua binding stay unchanged.
Lua, Python and Squirrel targets can coexist in the same build configuration.
`hgjs` is also accepted as a build target for compatibility with the initial helper.

The `harfang` ES module is generated from `binding/bind_harfang.py`, as are the
other language bindings. The native target exposes the same engine API as Lua
and Squirrel for the same build options, including physics, navigation, audio,
rendering, file access and Lua scene systems. Three native tutorial ports are
included. JavaScript scene components and a portable native/Web
application facade are deferred. Existing Lua scene components remain available
through the ordinary `SceneLuaVM` and `Scene*Systems` APIs.

## Compatibility priorities

1. **Native HG JS prioritizes conformity with HG Lua**, for the same engine build
   options. HG Lua defines the reference functionality and engine behavior;
   JavaScript keeps its own language conventions. Squirrel and Python provide
   additional comparison points.
2. **Web HG JS adapts on a best-effort basis to run native HG JS projects.** Web
   restrictions, budgets and unsupported features must not reduce the native API.
   Required adaptations and approximations belong in the web compatibility
   documentation; complete browser compatibility is not guaranteed.

Native conformity is validated against HG Lua first. Web compatibility is then
measured against those native projects and reported separately. See the
[normative compatibility precedence](../../specifications/SPECS_HYBRID_CPP_JS_WEBGL_DELIVERY_SLICES.md#compatibility-precedence).

## Build

On Windows, `rebuild_hg_js_bullet.bat [Release|Debug|RelWithDebInfo]` in the
repository root configures, builds and installs the Bullet version with AssetC,
its toolchain and the XMP audio plugin. It works from any current directory.
Defaults are `../build/js-cmake-bullet` and `../install/js_bullet`; the runtime
is installed as `../install/js_bullet/hgjs/hgjs.exe`, alongside its DLLs.
The compiler is `../install/js_bullet/assetc/assetc.exe`.

Set `HG_QUICKJS_SOURCE_DIR` and `HG_QUICKJS_ZIG` to use explicit dependencies.
Otherwise the batch looks in the workspace's `deps` directory and the existing
`.codex_tmp/quickjs` bootstrap cache; Zig can also be found on `PATH`.
`BUILD_DIR`, `INSTALL_DIR`, `FABGEN_DIR`, `PYTHON_EXE` and `BUILD_JOBS` can be
overridden; `HG_BATCH_NO_PAUSE=1` disables the final pause for automation.

Windows x64 requires Visual Studio 2022 C++ tools, CMake, Python with FABGen's
dependencies, the adjacent FABGen checkout with its QuickJS backend, official
QuickJS **2026-06-04**, and Zig **0.14.1**. QuickJS archive SHA-256:
`b376e839b322978313d929fd20663b11ba58b75df5a46c126dd19ea2fa70ad2a`.

From the workspace containing `harfang3d`:

```powershell
cmake -S harfang3d -B build/hgjs -DHG_BUILD_HG_JS=ON -DHG_QUICKJS_SOURCE_DIR=C:/deps/quickjs-2026-06-04 -DHG_QUICKJS_ZIG=C:/deps/zig-0.14.1/zig.exe
cmake --build build/hgjs --config Release --target HarfangJs --parallel 4
```

The other HARFANG options retain their usual meaning. For a small build with
physics, navigation and tools disabled, `harfangjs/tools/build_hgjs.py` supplies
that profile; it does not disable other language targets.

Zig compiles only the QuickJS C core with `-target x86_64-windows-msvc`. HARFANG,
the generated C++ binding and launcher use MSVC. Upstream sources are unchanged:
`extern/quickjs/prepare.py` stages the reviewed C++14 helpers, consistent 32-bit
BigInt limbs, unsigned closure-kind bitfield and Win32 clock shim in the build
tree. Blocking Atomics are disabled for this single-threaded host. The GNU C path
for Linux/macOS is present but unvalidated. No QuickJS bytecode is shipped.

## Tutorials

See [the JavaScript tutorial guide](../../tutorials/README_JS.md).
From the workspace root, with native assetc already built:

```powershell
python harfang3d/languages/hg_quickjs/prepare_tutorials.py
Push-Location build/hgjs-tutorials
../../install/js_bullet/hgjs/hgjs.exe source/draw_lines.js
../../install/js_bullet/hgjs/hgjs.exe source/draw_model_no_pipeline.js
../../install/js_bullet/hgjs/hgjs.exe source/filesystem_assets.js
Pop-Location
```

## Host and packaging

The normal CLI is `hgjs script.js [args...]`. It reads JavaScript directly from
the filesystem. Paths can be absolute or relative; imports resolve relative to
the importing module, including `../` imports. No asset directory is required
or mounted automatically, and the process working directory is preserved.
`scriptArgs` contains the resolved entry filename followed by application
arguments. Use `hgjs -h` / `--help` for usage and `-v` / `--version` for versions.

The application selects its assets, just as in Lua or Squirrel:

```js
import * as hg from 'harfang';
hg.AddAssetsFolder('resources_compiled');
```

Shaders, models and textures still use the native asset compiler. JavaScript
code itself does not need asset compilation. The tutorials mount
`resources_compiled` in `js/window.js`, relative to the caller's working directory.
An entry can run ordinary top-level code, export `async function main()`, or
export a `completion` Promise. If `main` exists, the host calls and awaits it;
otherwise it awaits `completion` if present.

Applications own initialization, update, render and cleanup. `harfang-host`
exports `nextFrame(window?)`, resolving to `{closed, dtNs}` with a BigInt delta
in nanoseconds. Ordinary synchronous native loops are also supported. There is
no execution deadline, frame-delta clamp or fixed JavaScript heap cap. The host
services up to 64 Promise jobs per turn, and drains outstanding jobs and frame
requests before exiting. It reports
missing modules, rejected completion and unhandled rejections with a nonzero
exit code. A pending entry Promise with no remaining job or frame request is
reported as stalled. A 256 KiB JS stack guard protects the native Windows thread
stack. `print()` and `console.log()` write to stdout; `console.error()` writes to
stderr. Native filesystem APIs are exposed. Node.js and QuickJS's optional
`std`/`os` modules are not provided by this launcher.

`SceneLuaVM.Pack(value)` and `Unpack(luaObject)` exchange nil, booleans, strings,
numbers and bound HARFANG objects with embedded Lua scripts. Lua integers return
as BigInt, including values outside JavaScript's safe Number range. Use these
with `GetScriptValue`, `SetScriptValue` and `Call`, as in Squirrel. Native values
are copied across VMs; Lua references and result lists keep their source VM
alive. Arbitrary JavaScript objects/functions are not converted to Lua tables.

```powershell
cmake --install build/hgjs --config Release --component quickjs --prefix install/hgjs
```

The installation contains `hgjs/hgjs.exe`, GLFW, Lua's runtime DLL and licenses.
MSVC's runtime is required. `harfangjs/tools/package_hgjs.py` additionally copies
compiled resources to `resources_compiled`, copies JavaScript files separately,
and writes `run.cmd`. Resource mounts remain in the application's JavaScript.

## Validation

Configure `HG_BUILD_TESTS=ON` and build `hg_quickjs_tests` for the isolated QuickJS
runtime tests. Existing engine and Lua tests still run through `tests` unchanged.
`python languages/hg_quickjs/test_launcher.py path/to/hgjs.exe` tests direct file
execution, arguments, module resolution/cache, explicit asset mounts, output
streams, synchronous execution beyond five seconds and allocation beyond the
old 256 MiB cap. It also executes `test_native_api.js`: Lua value exchange,
cross-VM object copies, reference lifetimes and Bullet physics when enabled.
These checks do not require assetc or a renderer.

From the workspace root, compare the Lua/Squirrel/JS declarations and check the
installed executable's exports:

```powershell
python harfang3d/languages/hg_quickjs/check_api_parity.py --hgjs install/js_bullet/hgjs/hgjs.exe --report build/hgjs-api-parity.json
```

The default comparison enables Bullet and Recast; pass `--defines` to match
another build. It covers names of classes, constructors, functions, constants,
methods, members and operators. This is API surface coverage, not proof that
every overload or subsystem behaves identically. JavaScript uses `new`, explicit
math methods (`add`, `mul`, etc.), BigInt for 64-bit integers and arrays for
multiple return values. Native list access uses `get`/`set` or `at`.
From `harfangjs`:

```powershell
.venv/Scripts/python.exe tools/validate_native_js.py --native-render
```

This checks 19 shared math/scene contract groups against Chromium, Lua scene
components driven from JavaScript, module/Promise errors, window handling, the
three tutorial ports, and native room/lighting/PBR rendering. The portable web
facade, strict portable conversion rules and complete cross-host compatibility
remain separate work; the native binding exposes the wider HARFANG API.
