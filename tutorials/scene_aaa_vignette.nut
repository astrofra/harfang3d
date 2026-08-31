// Toyota 2JZ-GTE Engine model by Serhii Denysenko (CGTrader: serhiidenysenko8256)
// URL : https://www.cgtrader.com/3d-models/vehicle/part/toyota-2jz-gte-engine-2932b715-2f42-4ecd-93ce-df9507c67ce8

local hg = require("harfang");

function clamp(v, v_min, v_max) {
	if (v < v_min) {
		return v_min;
	}
	if (v > v_max) {
		return v_max;
	}
	return v;
}

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("AAA Scene - Compositing Vignette", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local scene = hg.Scene();
hg.LoadSceneFromAssets("car_engine/engine.scn", scene, res, hg.GetForwardPipelineInfo());

local pipeline_aaa_config = hg.ForwardPipelineAAAConfig();
local pipeline_aaa = hg.CreateForwardPipelineAAAFromAssets("core", pipeline_aaa_config, hg.BR_Equal, hg.BR_Equal);
pipeline_aaa_config.sample_count = 1;

local vignette_strength = 0.85;
local vignette_radius = 0.80;
local vignette_softness = 0.45;

local keyboard = hg.Keyboard();
local frame = 0;

while (!keyboard.Down(hg.K_Escape) && hg.IsWindowOpen(win)) {
	keyboard.Update();
	local dt = hg.TickClock();
	local dt_sec = hg.time_to_sec_f(dt);

	if (keyboard.Down(hg.K_Up)) {
		vignette_strength = clamp(vignette_strength + dt_sec * 0.75, 0, 1);
	} else if (keyboard.Down(hg.K_Down)) {
		vignette_strength = clamp(vignette_strength - dt_sec * 0.75, 0, 1);
	}

	if (keyboard.Down(hg.K_Right)) {
		vignette_radius = clamp(vignette_radius + dt_sec * 0.75, 0.1, 2.0);
	} else if (keyboard.Down(hg.K_Left)) {
		vignette_radius = clamp(vignette_radius - dt_sec * 0.75, 0.1, 2.0);
	}

	if (keyboard.Down(hg.K_Add)) {
		vignette_softness = clamp(vignette_softness + dt_sec * 0.75, 0.05, 2.0);
	} else if (keyboard.Down(hg.K_Sub)) {
		vignette_softness = clamp(vignette_softness - dt_sec * 0.75, 0.05, 2.0);
	}

	pipeline_aaa_config.compositing_params0 = hg.Vec4(vignette_strength, vignette_radius, vignette_softness, 0);
	hg.SetWindowTitle(win, "AAA Scene - Vignette strength " + format("%.2f", vignette_strength) + " radius " + format("%.2f", vignette_radius) + " softness " + format("%.2f", vignette_softness));

	local trs = scene.GetNode("engine_master").GetTransform();
	trs.SetRot(trs.GetRot() + hg.Vec3(0, hg.Deg(15) * dt_sec, 0));

	scene.Update(dt);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res, pipeline_aaa, pipeline_aaa_config, frame);

	frame = hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
