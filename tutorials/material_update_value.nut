// Create textured material with pipeline shader

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Modify material pipeline shader uniforms", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

// Create models.
local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local cube_ref = res.AddModel("cube", cube_mdl);
local ground_mdl = hg.CreateCubeModel(vtx_layout, 100, 0.01, 100);
local ground_ref = res.AddModel("ground", ground_mdl);

// Create materials.
local shader = hg.LoadPipelineProgramRefFromAssets("core/shader/default.hps", res, hg.GetForwardPipelineInfo());

local mat_cube = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4(1, 1, 1), "uSpecularColor", hg.Vec4(1, 1, 1));
local mat_ground = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4(1, 1, 1), "uSpecularColor", hg.Vec4(0.1, 0.1, 0.1));

// Setup scene.
local scene = hg.Scene();

local cam = hg.CreateCamera(scene, hg.Mat4LookAt(hg.Vec3(-1.30, 0.27, -2.47), hg.Vec3(0, 0.5, 0)), 0.01, 1000);
scene.SetCurrentCamera(cam);

hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 2, 0), hg.Deg3(27.5, -97.6, 16.6)), hg.ColorI(64, 64, 64), hg.ColorI(64, 64, 64), 10);
hg.CreateSpotLight(scene, hg.TransformationMat4(hg.Vec3(5, 4, -5), hg.Deg3(19, -45, 0)), 0, hg.Deg(5), hg.Deg(30), hg.ColorI(255, 255, 255), hg.ColorI(255, 255, 255), 10, hg.LST_Map, 0.0001);

local cube_node = hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 0.5, 0)), cube_ref, [mat_cube]);
hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 0, 0)), ground_ref, [mat_ground]);

local mat_has_texture = false;
local mat_update_delay = 0;

local texture_ref = hg.LoadTextureFromAssets("textures/squares.png", 0, res);

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();

	mat_update_delay -= dt;

	if (mat_update_delay <= 0) {
		local mat = cube_node.GetObject().GetMaterial(0);

		if (mat_has_texture) {
			hg.SetMaterialTexture(mat, "uDiffuseMap", hg.InvalidTextureRef, 0);
		} else {
			hg.SetMaterialTexture(mat, "uDiffuseMap", texture_ref, 0);
		}

		hg.UpdateMaterialPipelineProgramVariant(mat, res);

		mat_update_delay += hg.time_from_sec(1);
		mat_has_texture = !mat_has_texture;
	}

	scene.Update(dt);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	local view_id = submit_result[0];

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
