// Draw text

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Draw Text", res_x, res_y, hg.RF_VSync);

local font = hg.LoadFontFromFile("resources_compiled/font/default.ttf", 96);
local font_prg = hg.LoadProgramFromFile("resources_compiled/core/shader/font");

local text_uniform_values = [hg.MakeUniformSetValue("u_color", hg.Vec4(1, 1, 0, 1))];
local text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled);

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	hg.SetView2D(0, 0, 0, res_x, res_y, -1, 1, hg.CF_Color | hg.CF_Depth, hg.ColorI(32, 32, 32), 0, 1);

	hg.DrawText(0, font, "Hello world!", font_prg, "u_tex", 0, hg.Mat4.get_Identity(),
		hg.Vec3(res_x / 2, res_y / 2, 0), hg.DTHA_Center, hg.DTVA_Center, text_uniform_values, [], text_render_state);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
