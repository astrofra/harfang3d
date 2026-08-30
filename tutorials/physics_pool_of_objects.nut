// Physics cubes & spheres in a box.

local hg = require("harfang");

function create_material(prg_ref, diffuse, specular, self_color) {
	local mat = hg.CreateMaterial(prg_ref, "uDiffuseColor", diffuse, "uSpecularColor", specular);
	hg.SetMaterialValue(mat, "uSelfColor", self_color);
	return mat;
}

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Physics Pool", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local sphere_mdl = hg.CreateSphereModel(vtx_layout, 0.5, 12, 24);
local sphere_ref = res.AddModel("sphere", sphere_mdl);
local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local cube_ref = res.AddModel("cube", cube_mdl);

local prg_ref = hg.LoadPipelineProgramRefFromAssets("core/shader/default.hps", res, hg.GetForwardPipelineInfo());

local mat_ground = create_material(prg_ref, hg.Vec4(0.5, 0.5, 0.5), hg.Vec4(0.1, 0.1, 0.1), hg.Vec4(0, 0, 0));
local mat_walls = create_material(prg_ref, hg.Vec4(0.5, 0.5, 0.5), hg.Vec4(0.1, 0.1, 0.1), hg.Vec4(0, 0, 0));
local mat_objects = create_material(prg_ref, hg.Vec4(0.5, 0.5, 0.5), hg.Vec4(1, 1, 1), hg.Vec4(0, 0, 0));

local scene = hg.Scene();
scene.canvas.color = hg.ColorI(22, 56, 76);
scene.environment.fog_color = scene.canvas.color;
scene.environment.fog_near = 20;
scene.environment.fog_far = 80;

local cam_mtx = hg.TransformationMat4(hg.Vec3(0, 20, -30), hg.Deg3(30, 0, 0));
local cam = hg.CreateCamera(scene, cam_mtx, 0.01, 5000);
scene.SetCurrentCamera(cam);

local lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Deg3(30, 59, 0)), hg.Color(1, 0.8, 0.7), hg.Color(1, 0.8, 0.7), 10, hg.LST_Map, 0.002, hg.Vec4(50, 100, 200, 400));
local back_lgt = hg.CreatePointLight(scene, hg.TranslationMat4(hg.Vec3(0, 10, 10)), 100, hg.ColorI(94, 155, 228), hg.ColorI(94, 255, 228));

local mdl_ref = res.AddModel("ground", hg.CreateCubeModel(vtx_layout, 100, 1, 100));
hg.CreatePhysicCube(scene, hg.Vec3(30, 1, 30), hg.TranslationMat4(hg.Vec3(0, -0.5, 0)), mdl_ref, [mat_ground], 0);
mdl_ref = res.AddModel("wall", hg.CreateCubeModel(vtx_layout, 1, 11, 32));
hg.CreatePhysicCube(scene, hg.Vec3(1, 11, 32), hg.TranslationMat4(hg.Vec3(-15.5, -0.5, 0)), mdl_ref, [mat_walls], 0);
hg.CreatePhysicCube(scene, hg.Vec3(1, 11, 32), hg.TranslationMat4(hg.Vec3(15.5, -0.5, 0)), mdl_ref, [mat_walls], 0);
mdl_ref = res.AddModel("wall2", hg.CreateCubeModel(vtx_layout, 32, 11, 1));
hg.CreatePhysicCube(scene, hg.Vec3(32, 11, 1), hg.TranslationMat4(hg.Vec3(0, -0.5, -15.5)), mdl_ref, [mat_walls], 0);
hg.CreatePhysicCube(scene, hg.Vec3(32, 11, 1), hg.TranslationMat4(hg.Vec3(0, -0.5, 15.5)), mdl_ref, [mat_walls], 0);

local clocks = hg.SceneClocks();

local physics = hg.SceneBullet3Physics();
physics.SceneCreatePhysicsFromAssets(scene);

local physic_nodes = [];

local font = hg.LoadFontFromAssets("font/default.ttf", 32);
local font_program = hg.LoadProgramFromAssets("core/shader/font.vsb", "core/shader/font.fsb");

local text_uniform_values = [hg.MakeUniformSetValue("u_color", hg.Vec4(1, 1, 0.5))];
local text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled);

while (hg.IsWindowOpen(win)) {
	local state = hg.ReadKeyboard();

	if (state.Key(hg.K_S)) {
		for (local i = 0; i < 7; ++i) {
			hg.SetMaterialValue(mat_objects, "uDiffuseColor", hg.RandomVec4(0, 1));

			local spawn_pos = hg.RandomVec3(hg.Vec3(-10, 18, -10), hg.Vec3(10, 18, 10));
			local node = null;
			if (hg.FRand() > 0.5) {
				node = hg.CreatePhysicCube(scene, hg.Vec3(1, 1, 1), hg.TranslationMat4(spawn_pos), cube_ref, [mat_objects], 1);
			} else {
				node = hg.CreatePhysicSphere(scene, 0.5, hg.TranslationMat4(spawn_pos), sphere_ref, [mat_objects], 1);
			}

			physics.NodeCreatePhysicsFromAssets(node);
			physic_nodes.append(node);
		}
	} else if (state.Key(hg.K_D)) {
		if (physic_nodes.len() > 0) {
			for (local i = 0; i < 7; ++i) {
				if (physic_nodes.len() == 0) {
					break;
				}

				local node = physic_nodes[0];
				scene.DestroyNode(node);
				physic_nodes.remove(0);
			}
		}

		hg.SceneGarbageCollectSystems(scene, physics);
	} else if (state.Key(hg.K_Escape)) {
		break;
	}

	local dt = hg.TickClock();

	hg.SceneUpdateSystems(scene, clocks, dt, physics, hg.time_from_sec_f(1 / 60.0), 4);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	local view_id = submit_result[0];

	hg.SetView2D(view_id, 0, 0, res_x, res_y, -1, 1, hg.CF_Depth, hg.Color.get_Black(), 1, 0);
	hg.DrawText(view_id, font, "S: Add object - D: Destruct object", font_program, "u_tex", 0, hg.Mat4.get_Identity(), hg.Vec3(460, res_y - 60, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, [], text_render_state);
	hg.DrawText(view_id, font, physic_nodes.len().tostring() + " Object", font_program, "u_tex", 0, hg.Mat4.get_Identity(), hg.Vec3(res_x - 200, res_y - 60, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, [], text_render_state);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);

hg.WindowSystemShutdown();
hg.InputShutdown();
