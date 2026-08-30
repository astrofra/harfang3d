// Draw models without a pipeline

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Draw Models no Pipeline", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local ground_mdl = hg.CreatePlaneModel(vtx_layout, 5, 5, 1, 1);

local shader = hg.LoadProgramFromFile("resources_compiled/shaders/mdl");

local angle = 0.0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();
	angle += hg.time_to_sec_f(dt);

	local viewpoint = hg.TranslationMat4(hg.Vec3(0, 1, -3));
	hg.SetViewPerspective(0, 0, 0, res_x, res_y, viewpoint);

	hg.DrawModel(0, cube_mdl, shader, [], [], hg.TransformationMat4(hg.Vec3(0, 1, 0), hg.Vec3(angle, angle, angle)));
	hg.DrawModel(0, ground_mdl, shader, [], [], hg.TranslationMat4(hg.Vec3(0, 0, 0)));

	hg.Frame();
	hg.UpdateWindow(win);

	collectgarbage();
}

hg.RenderShutdown();
