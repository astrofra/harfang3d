// ImGui basics

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - ImGui Basics", res_x, res_y, hg.RF_VSync);

// Initialize ImGui.
hg.AddAssetsFolder("resources_compiled");

local imgui_prg = hg.LoadProgramFromAssets("core/shader/imgui");
local imgui_img_prg = hg.LoadProgramFromAssets("core/shader/imgui_image");

hg.ImGuiInit(10, imgui_prg, imgui_img_prg);

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local reset = hg.RenderResetToWindow(win, res_x, res_y, hg.RF_VSync);
	local render_was_reset = reset[0];
	res_x = reset[1];
	res_y = reset[2];

	hg.ImGuiBeginFrame(res_x, res_y, hg.TickClock(), hg.ReadMouse(), hg.ReadKeyboard());

	if (hg.ImGuiBegin("Window")) {
		hg.ImGuiText("Hello World!");
	}
	hg.ImGuiEnd();

	hg.SetView2D(0, 0, 0, res_x, res_y, -1, 1, hg.CF_Color | hg.CF_Depth, hg.Color.get_Black(), 1, 0);
	hg.ImGuiEndFrame(0);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
