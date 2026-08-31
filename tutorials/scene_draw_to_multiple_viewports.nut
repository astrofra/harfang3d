// Draw to multiple viewports

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Scene Draw to Multiple Viewports", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local cube_ref = res.AddModel("cube", cube_mdl);
local ground_mdl = hg.CreateCubeModel(vtx_layout, 100, 0.01, 100);
local ground_ref = res.AddModel("ground", ground_mdl);

local shader = hg.LoadPipelineProgramRefFromAssets("core/shader/default.hps", res, hg.GetForwardPipelineInfo());

local mat_yellow_cube = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(255, 220, 64), "uSpecularColor", hg.Vec4I(255, 220, 64));
local mat_red_cube = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(255, 0, 0), "uSpecularColor", hg.Vec4I(255, 0, 0));
local mat_ground = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4I(128, 128, 128), "uSpecularColor", hg.Vec4I(128, 128, 128));

local scene = hg.Scene();

hg.CreateSpotLight(scene, hg.TransformationMat4(hg.Vec3(-8, 4, -5), hg.Deg3(19, 59, 0)), 0, hg.Deg(5), hg.Deg(30), hg.Color.get_White(), hg.Color.get_White(), 10, hg.LST_Map, 0.00005);
hg.CreatePointLight(scene, hg.TranslationMat4(hg.Vec3(3, 1, 2.5)), 5, hg.ColorI(128, 192, 255), hg.Color.get_Black(), 0);

local yellow_cube = hg.CreateObject(scene, hg.TransformationMat4(hg.Vec3(1, 0.5, 0), hg.Vec3(0, hg.Deg(0), 0)), cube_ref, [mat_yellow_cube]);
hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(-1, 0.5, 0)), cube_ref, [mat_red_cube]);
hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 0, 0)), ground_ref, [mat_ground]);

local viewports = [
	{ rect = hg.IntRect(0, 0, res_x / 2, res_y / 2), cam_pos = hg.Vec3(-4.015, 2.368, -3.484), cam_rot = hg.Vec3(0.35, 0.87, 0.0) },
	{ rect = hg.IntRect(res_x / 2, 0, res_x, res_y / 2), cam_pos = hg.Vec3(-4.143, 2.976, 4.127), cam_rot = hg.Vec3(0.423, 2.365, 0.0) },
	{ rect = hg.IntRect(0, res_y / 2, res_x / 2, res_y), cam_pos = hg.Vec3(4.020, 2.374, 3.469), cam_rot = hg.Vec3(0.353, 4.016, 0.0) },
	{ rect = hg.IntRect(res_x / 2, res_y / 2, res_x, res_y), cam_pos = hg.Vec3(3.469, 2.374, -4.020), cam_rot = hg.Vec3(0.353, -0.695, 0.0) }
];

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();

	local rot = yellow_cube.GetTransform().GetRot();
	rot.y += hg.time_to_sec_f(dt);
	yellow_cube.GetTransform().SetRot(rot);

	scene.Update(dt);

	local render_data = hg.SceneForwardPipelineRenderData();
	local views = hg.SceneForwardPipelinePassViewId();
	local vid = 0;

	local common_result = hg.PrepareSceneForwardPipelineCommonRenderData(vid, scene, render_data, pipeline, res, views);
	vid = common_result[0];
	views = common_result[1];

	foreach (viewport in viewports) {
		local view_state = hg.ComputePerspectiveViewState(hg.TransformationMat4(viewport.cam_pos, viewport.cam_rot), hg.Deg(45), 0.01, 1000, hg.ComputeAspectRatioX(res_x, res_y));

		local view_dep_result = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, view_state, scene, render_data, pipeline, res, views);
		vid = view_dep_result[0];
		views = view_dep_result[1];

		local submit_result = hg.SubmitSceneToForwardPipeline(vid, scene, viewport.rect, view_state, pipeline, render_data, res);
		vid = submit_result[0];
	}

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.DestroyForwardPipeline(pipeline);
hg.RenderShutdown();
hg.DestroyWindow(win);
