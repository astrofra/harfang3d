// Screen size configurator

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_list = [
	[640, 360],
	[768, 432],
	[896, 504],
	[1024, 576],
	[1152, 648],
	[1280, 720],
	[1920, 1080],
	[1920, 1200],
	[2560, 1440],
	[3840, 2160],
	[5120, 2880]
];

local res_list_str = [];
foreach (res in res_list) {
	res_list_str.append(res[0].tostring() + "x" + res[1].tostring());
}

local mode_list = [hg.WV_Windowed, hg.WV_Fullscreen, hg.WV_Undecorated, hg.WV_FullscreenMonitor1, hg.WV_FullscreenMonitor2, hg.WV_FullscreenMonitor3];
local mode_list_str = ["Windowed", "Fullscreen", "Undecorated", "Fullscreen Monitor #1", "Fullscreen Monitor #2", "Fullscreen Monitor #3"];

local mon_list = hg.GetMonitors();
local mon_list_size = mon_list.size();
if (mon_list_size == 2) {
	mode_list.resize(5);
	mode_list_str.resize(5);
} else if (mon_list_size == 1) {
	mode_list.resize(4);
	mode_list_str.resize(4);
}

local res_preset = 5;
local window_mode_preset = 0;

local config_res_x = 600;
local config_res_y = 200;
local config_window_mode = hg.WV_Windowed;

local config_win = hg.NewWindow("Window Configurator", config_res_x, config_res_y, 32, config_window_mode);
hg.RenderInit(config_win);
hg.RenderReset(config_res_x, config_res_y, hg.RF_VSync);

hg.AddAssetsFolder("resources_compiled");
local imgui_prg = hg.LoadProgramFromAssets("core/shader/imgui");
local imgui_img_prg = hg.LoadProgramFromAssets("core/shader/imgui_image");

hg.ImGuiInit(10, imgui_prg, imgui_img_prg);

local press_apply = false;
while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(config_win) && !press_apply) {
	hg.ImGuiBeginFrame(config_res_x, config_res_y, hg.TickClock(), hg.ReadMouse(), hg.ReadKeyboard());

	local begin_result = hg.ImGuiBegin("Window Configuration", true, hg.ImGuiWindowFlags_NoMove | hg.ImGuiWindowFlags_NoResize);
	if (begin_result[0]) {
		hg.ImGuiSetWindowPos("Window Configuration", hg.Vec2(0, 0), hg.ImGuiCond_Once);
		hg.ImGuiSetWindowSize("Window Configuration", hg.Vec2(config_res_x, config_res_y), hg.ImGuiCond_Once);

		hg.ImGuiText("Screen");

		local res_result = hg.ImGuiCombo("Resolution", res_preset, res_list_str);
		res_preset = res_result[1];

		local mode_result = hg.ImGuiCombo("Mode", window_mode_preset, mode_list_str);
		window_mode_preset = mode_result[1];

		hg.ImGuiSpacing();
		hg.ImGuiPushStyleColor(hg.ImGuiCol_Button, hg.Color(0.0, 0.5, 1.0, 1.0));
		press_apply = hg.ImGuiButton("Apply");
		hg.ImGuiPopStyleColor();
	}
	hg.ImGuiEnd();

	hg.SetView2D(0, 0, 0, config_res_x, config_res_y, -1, 1, hg.CF_Color | hg.CF_Depth, hg.Color.get_Black(), 1, 0);
	hg.ImGuiEndFrame(0);

	hg.Frame();
	hg.UpdateWindow(config_win);
}

hg.RenderShutdown();
hg.DestroyWindow(config_win);

if (press_apply) {
	local selected_res = res_list[res_preset];
	local selected_res_x = selected_res[0];
	local selected_res_y = selected_res[1];
	local selected_window_mode = mode_list[window_mode_preset];

	local win = hg.NewWindow("Window preset", selected_res_x, selected_res_y, 32, selected_window_mode);
	hg.RenderInit(win);
	hg.RenderReset(selected_res_x, selected_res_y, hg.RF_VSync);

	imgui_prg = hg.LoadProgramFromAssets("core/shader/imgui");
	imgui_img_prg = hg.LoadProgramFromAssets("core/shader/imgui_image");

	hg.ImGuiInit(10, imgui_prg, imgui_img_prg);

	local imgui_window_size = hg.Vec2(190, 50);
	local imgui_window_pos = hg.Vec2((selected_res_x / 2) - (imgui_window_size.x / 2), (selected_res_y / 2) - (imgui_window_size.y / 2));

	while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
		hg.ImGuiBeginFrame(selected_res_x, selected_res_y, hg.TickClock(), hg.ReadMouse(), hg.ReadKeyboard());

		local begin_result = hg.ImGuiBegin("Window Parameters", true, hg.ImGuiWindowFlags_NoResize | hg.ImGuiWindowFlags_NoTitleBar);
		if (begin_result[0]) {
			hg.ImGuiSetWindowPos("Window Parameters", imgui_window_pos, hg.ImGuiCond_Once);
			hg.ImGuiSetWindowSize("Window Parameters", imgui_window_size, hg.ImGuiCond_Once);

			hg.ImGuiText(format("Window size : %d x %d", selected_res_x, selected_res_y));
			hg.ImGuiText("Window mode : " + mode_list_str[window_mode_preset]);
		}
		hg.ImGuiEnd();

		hg.SetView2D(0, 0, 0, selected_res_x, selected_res_y, -1, 1, hg.CF_Color | hg.CF_Depth, hg.Color.get_Red() / 2, 1, 0);
		hg.ImGuiEndFrame(0);

		hg.Frame();
		hg.UpdateWindow(win);
	}

	hg.RenderShutdown();
	hg.DestroyWindow(win);
}
