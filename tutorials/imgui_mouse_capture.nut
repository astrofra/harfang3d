// Detect ImGui mouse capture

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - ImGui Mouse Capture", res_x, res_y, hg.RF_VSync);

hg.AddAssetsFolder("resources_compiled");

hg.ImGuiInit(10, hg.LoadProgramFromAssets("core/shader/imgui"), hg.LoadProgramFromAssets("core/shader/imgui_image"));
local text_value = "Clicking into this field will not clear the screen in red.";

local mouse = hg.Mouse();
local keyboard = hg.Keyboard();

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local reset = hg.RenderResetToWindow(win, res_x, res_y, hg.RF_VSync);
	local render_was_reset = reset[0];
	res_x = reset[1];
	res_y = reset[2];

	mouse.Update();
	keyboard.Update();

	local dt = hg.TickClock();
	hg.ImGuiBeginFrame(res_x, res_y, dt, mouse.GetState(), keyboard.GetState());

	local clear_color = hg.Color.get_Black();
	if (!hg.ImGuiWantCaptureMouse() && mouse.Down(hg.MB_0)) {
		clear_color = hg.Color.get_Red();
	}

	hg.SetView2D(0, 0, 0, res_x, res_y, -1, 0, hg.CF_Color | hg.CF_Depth, clear_color, 1, 0);

	hg.ImGuiSetNextWindowPosCenter(hg.ImGuiCond_Once);
	hg.ImGuiSetNextWindowSize(hg.Vec2(700, 96), hg.ImGuiCond_Once);

	if (hg.ImGuiBegin("Detecting ImGui mouse capture")) {
		hg.ImGuiTextWrapped("Click outside of the GUI to clear the screen in red.");
		hg.ImGuiSeparator();
		local input_text_result = hg.ImGuiInputText("Text Input", text_value, 4096);
		text_value = input_text_result[1];
	}
	hg.ImGuiEnd();

	hg.ImGuiEndFrame(0);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
