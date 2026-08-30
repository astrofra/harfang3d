// How to resize the render window.

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 512;
local res_y = 512;
local win = hg.RenderInit("Harfang - Render Resize to Window", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();
local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local cube_prg = hg.LoadProgramFromFile("resources_compiled/shaders/mdl");

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local reset = hg.RenderResetToWindow(win, res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X | hg.RF_MaxAnisotropy);
	local render_was_reset = reset[0];
	res_x = reset[1];
	res_y = reset[2];

	if (render_was_reset) {
		print("Render reset to " + res_x + "x" + res_y + "\n");
	}

	local viewpoint = hg.TransformationMat4(hg.Vec3(1, 1, -2), hg.Deg3(24, -27, 0));
	hg.SetViewPerspective(0, 0, 0, res_x, res_y, viewpoint, 0.01, 100, 1.8, hg.CF_Color | hg.CF_Depth, hg.ColorI(64, 64, 64), 1, 0);

	hg.DrawModel(0, cube_mdl, cube_prg, [], [], hg.TranslationMat4(hg.Vec3(0, 0, 0)));

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
