// Scene using the PBR shader

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 940;
local res_y = 720;
local win = hg.RenderInit("PBR Scene", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

hg.AddAssetsFolder("resources_compiled");

local scene = hg.Scene();
hg.LoadSceneFromAssets("materials/materials.scn", scene, res, hg.GetForwardPipelineInfo());

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();

	scene.Update(dt);
	hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.DestroyForwardPipeline(pipeline);
hg.RenderShutdown();
hg.DestroyWindow(win);
