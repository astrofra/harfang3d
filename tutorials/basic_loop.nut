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

	// Force the view to be processed even if nothing is drawn to it.
	hg.Touch(0);

	hg.Frame();
	hg.UpdateWindow(window);
}

hg.RenderShutdown();
hg.DestroyWindow(window);
