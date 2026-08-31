// Toyota 2JZ-GTE Engine model by Serhii Denysenko (CGTrader: serhiidenysenko8256)
// URL : https://www.cgtrader.com/3d-models/vehicle/part/toyota-2jz-gte-engine-2932b715-2f42-4ecd-93ce-df9507c67ce8

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("AAA Scene", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

hg.AddAssetsFolder("resources_compiled");

local scene = hg.Scene();
hg.LoadSceneFromAssets("car_engine/engine.scn", scene, res, hg.GetForwardPipelineInfo());

local pipeline_aaa_config = hg.ForwardPipelineAAAConfig();
local pipeline_aaa = hg.CreateForwardPipelineAAAFromAssets("core", pipeline_aaa_config, hg.BR_Equal, hg.BR_Equal);
pipeline_aaa_config.sample_count = 1;

local target_dof_focus_point = 3.5;
local target_dof_focus_length = 2.0;
pipeline_aaa_config.dof_focus_point = target_dof_focus_point;
pipeline_aaa_config.dof_focus_length = target_dof_focus_length;

local frame = 0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();

	local trs = scene.GetNode("engine_master").GetTransform();
	trs.SetRot(trs.GetRot() + hg.Vec3(0, hg.Deg(15) * hg.time_to_sec_f(dt), 0));

	if ((frame % 250) == 0) {
		target_dof_focus_point = hg.FRRand(2.5, 3.5);
		target_dof_focus_length = hg.FRRand(0.5, 2.0);
	}

	pipeline_aaa_config.dof_focus_point = hg.Lerp(pipeline_aaa_config.dof_focus_point, target_dof_focus_point, 0.1);
	pipeline_aaa_config.dof_focus_length = hg.Lerp(pipeline_aaa_config.dof_focus_length, target_dof_focus_length, 0.1);

	scene.Update(dt);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res, pipeline_aaa, pipeline_aaa_config, frame);

	frame = hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
