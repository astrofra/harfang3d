-- Toyota 2JZ-GTE Engine model by Serhii Denysenko (CGTrader: serhiidenysenko8256)
-- URL : https://www.cgtrader.com/3d-models/vehicle/part/toyota-2jz-gte-engine-2932b715-2f42-4ecd-93ce-df9507c67ce8

hg = require("harfang")

local function clamp(v, v_min, v_max)
	if v < v_min then
		return v_min
	elseif v > v_max then
		return v_max
	end
	return v
end

hg.InputInit()
hg.WindowSystemInit()

res_x, res_y = 1280, 720
win = hg.RenderInit('AAA Scene - Compositing Vignette', res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X)

hg.AddAssetsFolder("resources_compiled")

--
pipeline = hg.CreateForwardPipeline()
res = hg.PipelineResources()

-- load scene
scene = hg.Scene()
hg.LoadSceneFromAssets("car_engine/engine.scn", scene, res, hg.GetForwardPipelineInfo())

-- AAA pipeline
pipeline_aaa_config = hg.ForwardPipelineAAAConfig()
pipeline_aaa = hg.CreateForwardPipelineAAAFromAssets("core", pipeline_aaa_config, hg.BR_Equal, hg.BR_Equal)
pipeline_aaa_config.sample_count = 1

-- uCompositingParams[0] is interpreted by the default compositing shader as:
-- x: vignette strength, y: vignette radius, z: vignette softness, w: unused
vignette_strength = 0.85
vignette_radius = 0.80
vignette_softness = 0.45

keyboard = hg.Keyboard()
frame = 0

while not keyboard:Down(hg.K_Escape) and hg.IsWindowOpen(win) do
	keyboard:Update()
	dt = hg.TickClock()
	dt_sec = hg.time_to_sec_f(dt)

	if keyboard:Down(hg.K_Up) then
		vignette_strength = clamp(vignette_strength + dt_sec * 0.75, 0, 1)
	elseif keyboard:Down(hg.K_Down) then
		vignette_strength = clamp(vignette_strength - dt_sec * 0.75, 0, 1)
	end

	if keyboard:Down(hg.K_Right) then
		vignette_radius = clamp(vignette_radius + dt_sec * 0.75, 0.1, 2.0)
	elseif keyboard:Down(hg.K_Left) then
		vignette_radius = clamp(vignette_radius - dt_sec * 0.75, 0.1, 2.0)
	end

	if keyboard:Down(hg.K_Add) then
		vignette_softness = clamp(vignette_softness + dt_sec * 0.75, 0.05, 2.0)
	elseif keyboard:Down(hg.K_Sub) then
		vignette_softness = clamp(vignette_softness - dt_sec * 0.75, 0.05, 2.0)
	end

	pipeline_aaa_config.compositing_params0 = hg.Vec4(vignette_strength, vignette_radius, vignette_softness, 0)
	hg.SetWindowTitle(win, string.format(
		'AAA Scene - Vignette strength %.2f radius %.2f softness %.2f',
		vignette_strength, vignette_radius, vignette_softness))

	trs = scene:GetNode('engine_master'):GetTransform()
	trs:SetRot(trs:GetRot() + hg.Vec3(0, hg.Deg(15) * dt_sec, 0))

	scene:Update(dt)
	hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res, pipeline_aaa, pipeline_aaa_config, frame)

	frame = hg.Frame()
	hg.UpdateWindow(win)
end

hg.RenderShutdown()
hg.DestroyWindow(win)
