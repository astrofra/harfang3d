// ImGui edit

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - ImGui Edit", res_x, res_y, hg.RF_VSync);

hg.AddAssetsFolder("resources_compiled");

// Initialize ImGui.
local imgui_prg = hg.LoadProgramFromAssets("core/shader/imgui");
local imgui_img_prg = hg.LoadProgramFromAssets("core/shader/imgui_image");

hg.ImGuiInit(10, imgui_prg, imgui_img_prg);

local imgui_output_view = 255;
local imgui_view_clear_color = hg.Color(0, 0, 0);
local imgui_clear_color_preset = 0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local reset = hg.RenderResetToWindow(win, res_x, res_y, hg.RF_VSync);
	local render_was_reset = reset[0];
	res_x = reset[1];
	res_y = reset[2];
	local dt = hg.TickClock();

	hg.ImGuiBeginFrame(res_x, res_y, dt, hg.ReadMouse(), hg.ReadKeyboard());
	hg.ImGuiSetNextWindowPosCenter(hg.ImGuiCond_Once);

	local begin_result = hg.ImGuiBegin("ImGui Controls", true, hg.ImGuiWindowFlags_AlwaysAutoResize);
	if (begin_result[0]) {
		local combo_result = hg.ImGuiCombo("Set Clear Color", imgui_clear_color_preset, ["Red", "Green", "Blue"]);
		local val_modified = combo_result[0];
		imgui_clear_color_preset = combo_result[1];

		if (val_modified) {
			if (imgui_clear_color_preset == 0) {
				imgui_view_clear_color = hg.Color(1, 0, 0);
			} else if (imgui_clear_color_preset == 1) {
				imgui_view_clear_color = hg.Color(0, 1, 0);
			} else {
				imgui_view_clear_color = hg.Color(0, 0, 1);
			}
		}

		if (hg.ImGuiButton("Reset Clear Color")) {
			imgui_view_clear_color = hg.Color.get_Black();
		}

		local color_edit_result = hg.ImGuiColorEdit("Edit Clear Color", imgui_view_clear_color);
		val_modified = color_edit_result[0];
		imgui_view_clear_color = color_edit_result[1];

		local input_int_result = hg.ImGuiInputInt("ImGui Output View", imgui_output_view);
		val_modified = input_int_result[0];
		imgui_output_view = input_int_result[1];
		if (val_modified) {
			if (imgui_output_view < 0) {
				imgui_output_view = 0;
			} else if (imgui_output_view > 255) {
				imgui_output_view = 255;
			}
		}
	}
	hg.ImGuiEnd();

	hg.SetView2D(imgui_output_view, 0, 0, res_x, res_y, -1, 0, hg.CF_Color | hg.CF_Depth, imgui_view_clear_color, 1, 0);
	hg.ImGuiEndFrame(imgui_output_view);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
