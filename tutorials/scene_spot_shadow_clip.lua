-- Spot light shadow clip

hg = require("harfang")

hg.InputInit()
hg.WindowSystemInit()

res_x, res_y = 1280, 720
win = hg.RenderInit("Harfang - Spot Shadow Clip", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X)

hg.AddAssetsFolder("resources_compiled")

pipeline = hg.CreateForwardPipeline()
res = hg.PipelineResources()

-- create models
vtx_layout = hg.VertexLayoutPosFloatNormUInt8()

cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1)
cube_ref = res:AddModel("cube", cube_mdl)
blocker_mdl = hg.CreateCubeModel(vtx_layout, 0.7, 1.6, 0.7)
blocker_ref = res:AddModel("blocker", blocker_mdl)
ground_mdl = hg.CreateCubeModel(vtx_layout, 12, 0.02, 14)
ground_ref = res:AddModel("ground", ground_mdl)
wall_mdl = hg.CreateCubeModel(vtx_layout, 8, 3, 0.08)
wall_ref = res:AddModel("wall", wall_mdl)

-- create materials
shader = hg.LoadPipelineProgramRefFromAssets("core/shader/default.hps", res, hg.GetForwardPipelineInfo())

mat_yellow_cube = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(255, 220, 64), "uSpecularColor", hg.Vec4I(255, 220, 64))
mat_red_cube = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(255, 72, 56), "uSpecularColor", hg.Vec4I(255, 72, 56))
mat_blue_cube = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(64, 128, 255), "uSpecularColor", hg.Vec4I(64, 128, 255))
mat_ground = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(160, 160, 160), "uSpecularColor", hg.Vec4I(80, 80, 80))
mat_wall = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(190, 190, 190), "uSpecularColor", hg.Vec4I(80, 80, 80))

font = hg.LoadFontFromAssets("font/default.ttf", 32)
font_program = hg.LoadProgramFromAssets("core/shader/font")
text_uniform_values = {hg.MakeUniformSetValue("u_color", hg.Vec4(1, 1, 1))}
text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled)

-- setup scene
scene = hg.Scene()
scene.canvas.color = hg.ColorI(20, 24, 30)
scene.environment.ambient = hg.Color(0.025, 0.025, 0.025)

cam = hg.CreateCamera(scene, hg.Mat4LookAt(hg.Vec3(5.5, 4.0, -8.0), hg.Vec3(0.1, 1.0, 0.6)), 0.01, 100)
scene:SetCurrentCamera(cam)

spot_pos = hg.Vec3(0, 4.2, -5.0)
spot_target = hg.Vec3(0, 0.9, 2.8)
shadow_near_min = 0.1
shadow_near_max = 7.5
shadow_far = 18.0

spot_node = hg.CreateSpotLight(scene, hg.Mat4LookAt(spot_pos, spot_target), 0, hg.Deg(4), hg.Deg(34), hg.Color.White, 1, hg.Color.White, 1, 10, hg.LST_Map, 0.00005, shadow_near_min, shadow_far)
spot_light = spot_node:GetLight()

hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, -0.01, 0.8)), ground_ref, {mat_ground})
hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 1.5, 4.6)), wall_ref, {mat_wall})

blockers = {
	{pos = hg.Vec3(-1.4, 0.8, -2.0), mat = mat_yellow_cube},
	{pos = hg.Vec3(-0.65, 0.8, -0.8), mat = mat_red_cube},
	{pos = hg.Vec3(0.0, 0.8, 0.4), mat = mat_blue_cube},
	{pos = hg.Vec3(0.65, 0.8, 1.6), mat = mat_red_cube},
	{pos = hg.Vec3(1.4, 0.8, 2.8), mat = mat_yellow_cube}
}

for i, blocker in ipairs(blockers) do
	hg.CreateObject(scene, hg.TranslationMat4(blocker.pos), blocker_ref, {blocker.mat})
end

rotating_cube = hg.CreateObject(scene, hg.TransformationMat4(hg.Vec3(-2.7, 0.5, 2.3), hg.Vec3(0, 0, 0)), cube_ref, {mat_blue_cube})

-- main loop
angle = 0

while not hg.ReadKeyboard():Key(hg.K_Escape) and hg.IsWindowOpen(win) do
	dt = hg.TickClock()
	dts = hg.time_to_sec_f(dt)
	angle = angle + dts

	shadow_near = shadow_near_min + (shadow_near_max - shadow_near_min) * (math.sin(angle * 0.75) * 0.5 + 0.5)
	spot_light:SetShadowNear(shadow_near)

	rot = rotating_cube:GetTransform():GetRot()
	rot.y = rot.y + dts
	rotating_cube:GetTransform():SetRot(rot)

	scene:Update(dt)

	view_id, pass_id = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res)

	hg.SetView2D(view_id, 0, 0, res_x, res_y, -1, 1, hg.CF_Depth, hg.Color.Black, 1, 0)
	hg.DrawText(view_id, font, string.format("Spot shadow near clip: %.2f / far: %.1f", spot_light:GetShadowNear(), spot_light:GetShadowFar()), font_program, "u_tex", 0, hg.Mat4.Identity, hg.Vec3(24, res_y - 40, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, {}, text_render_state)
	hg.DrawText(view_id, font, "Near clip animates: close casters leave the shadow map.", font_program, "u_tex", 0, hg.Mat4.Identity, hg.Vec3(24, res_y - 76, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, {}, text_render_state)

	hg.Frame()
	hg.UpdateWindow(win)
end

hg.DestroyForwardPipeline(pipeline)
hg.RenderShutdown()
hg.DestroyWindow(win)
