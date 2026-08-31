// Draw text over models

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Draw Text over Models", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local ground_mdl = hg.CreatePlaneModel(vtx_layout, 5, 5, 1, 1);

local shader = hg.LoadProgramFromFile("resources_compiled/shaders/mdl");

local font = hg.LoadFontFromFile("resources_compiled/font/default.ttf", 96);
local font_prg = hg.LoadProgramFromFile("resources_compiled/core/shader/font");

local text_uniform_values = [hg.MakeUniformSetValue("u_color", hg.Vec4(1, 1, 0, 1))];
local text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled);

local angle = 0.0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();
	angle += hg.time_to_sec_f(dt);

	local viewpoint = hg.TranslationMat4(hg.Vec3(0, 1, -3));
	hg.SetViewPerspective(0, 0, 0, res_x, res_y, viewpoint, 0.01, 5000);

	hg.DrawModel(0, cube_mdl, shader, [], [], hg.TransformationMat4(hg.Vec3(0, 1, 0), hg.Vec3(angle, angle, angle)));
	hg.DrawModel(0, ground_mdl, shader, [], [], hg.TranslationMat4(hg.Vec3(0, 0, 0)));

	hg.SetView2D(1, 0, 0, res_x, res_y, -1, 1, hg.CF_Depth, hg.ColorI(32, 32, 32), 1, 0);

	hg.DrawText(1, font, "Hello world!", font_prg, "u_tex", 0, hg.Mat4.get_Identity(),
		hg.Vec3(res_x / 2, res_y / 2, 0), hg.DTHA_Center, hg.DTVA_Center, text_uniform_values, [], text_render_state);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
