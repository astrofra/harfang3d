.title Harfang for Squirrel

Harfang for Squirrel is distributed as a vendored Squirrel runtime package with native-module loading enabled.

## Installation

The standard package layout keeps the runtime files side by side:

- `hg_squirrel.exe` for the public interpreter
- `launcher_squirrel.exe` and `launcher_squirrel_noconsole.exe` for packaged applications
- `harfang.dll`, `squirrel.dll`, `sqstdlib.dll`, `glfw3.dll`, `lua54.dll`
- `harfang/assetc/assetc.exe` and its toolchain files

On Windows, launching `hg_squirrel.exe` from that folder is the supported setup.

## Testing your Installation

Create a file named `test.nut` and run it with `hg_squirrel.exe`.

```nut
local hg = require("harfang");
print("harfang loaded");
```

## First Program

```nut
// Basic loop
local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local width = 1280;
local height = 720;
local window = hg.RenderInit("Harfang - Basic Loop", width, height, hg.RF_VSync);

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(window)) {
	hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color.get_Green(), 1, 0);
	hg.SetViewRect(0, 0, 0, width, height);

	hg.Touch(0);

	hg.Frame();
	hg.UpdateWindow(window);
}

hg.RenderShutdown();
hg.DestroyWindow(window);
```

`require("harfang")` loads the native Harfang module. `include("script.nut")` can be used to execute additional Squirrel source files from the current script search path.

The public Squirrel package currently targets host-side scripting through `hg_squirrel.exe` and `launcher_squirrel.exe`. Embedded scene scripting remains Lua-only for now.
