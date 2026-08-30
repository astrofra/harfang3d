// Display a scene in XR

local hg = require("harfang");

function create_material(prg_ref, ubc, orm) {
	local mat = hg.Material();
	hg.SetMaterialProgram(mat, prg_ref);
	hg.SetMaterialValue(mat, "uBaseOpacityColor", ubc);
	hg.SetMaterialValue(mat, "uOcclusionRoughnessMetalnessColor", orm);
	return mat;
}

function main() {
	hg.InputInit();
	hg.WindowSystemInit();

	local res_x = 1280;
	local res_y = 720;
	local win = hg.RenderInit("Harfang - OpenXR Scene", res_x, res_y, hg.RF_None);

	hg.AddAssetsFolder("resources_compiled");

	local pipeline = hg.CreateForwardPipeline();
	local res = hg.PipelineResources();
	local render_data = hg.SceneForwardPipelineRenderData();

	if (!hg.OpenXRInit()) {
		print("OpenXRInit failed.\n");
		hg.DestroyForwardPipeline(pipeline);
		hg.RenderShutdown();
		hg.DestroyWindow(win);
		return;
	}

	local eye_framebuffers = hg.OpenXRCreateEyeFrameBuffer(hg.OXRAA_MSAA4x);

	local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

	local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
	local cube_ref = res.AddModel("cube", cube_mdl);
	local ground_mdl = hg.CreateCubeModel(vtx_layout, 50, 0.01, 50);
	local ground_ref = res.AddModel("ground", ground_mdl);

	local prg_ref = hg.LoadPipelineProgramRefFromAssets("core/shader/pbr.hps", res, hg.GetForwardPipelineInfo());

	local scene = hg.Scene();
	scene.canvas.color = hg.Color(1, 1, 217 / 255.0, 1);
	scene.environment.ambient = hg.Color(15 / 255.0, 12 / 255.0, 9 / 255.0, 1);

	hg.CreateSpotLight(scene, hg.TransformationMat4(hg.Vec3(-8, 4, -5), hg.Vec3(hg.Deg(19), hg.Deg(59), 0)), 0, hg.Deg(5), hg.Deg(30), hg.Color.get_White(), hg.Color.get_White(), 10, hg.LST_Map, 0.0001);
	hg.CreatePointLight(scene, hg.TranslationMat4(hg.Vec3(2.4, 1, 0.5)), 10, hg.Color(94 / 255.0, 1, 228 / 255.0, 1), hg.Color(94 / 255.0, 1, 228 / 255.0, 1), 0);

	local mat_cube = create_material(prg_ref, hg.Vec4(1, 230 / 255.0, 20 / 255.0, 1), hg.Vec4(1, 0.658, 0, 1));
	hg.CreateObject(scene, hg.TransformationMat4(hg.Vec3(0, 0.5, 0), hg.Vec3(0, hg.Deg(70), 0)), cube_ref, [mat_cube]);

	local mat_ground = create_material(prg_ref, hg.Vec4(1, 120 / 255.0, 147 / 255.0, 1), hg.Vec4(1, 1, 0.1, 1));
	hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 0, 0)), ground_ref, [mat_ground]);

	local quad_layout = hg.VertexLayout();
	quad_layout.Begin().Add(hg.A_Position, 3, hg.AT_Float).Add(hg.A_TexCoord0, 3, hg.AT_Float).End();

	local quad_model = hg.CreatePlaneModel(quad_layout, 1, 1, 1, 1);
	local quad_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Disabled, hg.FC_Disabled);

	local eye_t_size = res_x / 2.5;
	local eye_t_x = (res_x - 2 * eye_t_size) / 6 + eye_t_size / 2;
	local quad_matrix = hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(hg.Deg(90), 0, 0), hg.Vec3(eye_t_size, 1, eye_t_size));

	local tex0_program = hg.LoadProgramFromAssets("shaders/sprite");

	local quad_uniform_set_value_list = hg.UniformSetValueList();
	quad_uniform_set_value_list.clear();
	quad_uniform_set_value_list.push_back(hg.MakeUniformSetValue("color", hg.Vec4(1, 1, 1, 1)));

	local quad_uniform_set_texture_list = hg.UniformSetTextureList();

	local update_controllers = function(head) {
	};

	local draw_scene = function(rect, view_state, view_id, fb) {
		local views = hg.SceneForwardPipelinePassViewId();
		local view_dep_result = hg.PrepareSceneForwardPipelineViewDependentRenderData(view_id, view_state, scene, render_data, pipeline, res, views);
		view_id = view_dep_result[0];
		views = view_dep_result[1];

		local submit_result = hg.SubmitSceneToForwardPipeline(view_id, scene, rect, view_state, pipeline, render_data, res, fb);
		return submit_result[0];
	};

	while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
		local dt = hg.TickClock();

		scene.Update(dt);

		local vid = 0;
		local views = hg.SceneForwardPipelinePassViewId();
		local common_result = hg.PrepareSceneForwardPipelineCommonRenderData(vid, scene, render_data, pipeline, res, views);
		vid = common_result[0];

		local openxr_result = hg.OpenXRSubmitSceneToForwardPipeline(hg.TranslationMat4(hg.Vec3(0, 0, 0)), update_controllers, draw_scene, vid, 0.1, 100);
		local openxr_frame_info = openxr_result[0];
		vid = openxr_result[1];

		hg.SetViewRect(vid, 0, 0, res_x, res_y);
		local vs = hg.ComputeOrthographicViewState(hg.TranslationMat4(hg.Vec3(0, 0, 0)), res_y, 0.1, 100, hg.ComputeAspectRatioX(res_x, res_y));
		hg.SetViewTransform(vid, vs.view, vs.proj);

		if (openxr_frame_info.id_fbs.size() >= 2) {
			quad_uniform_set_texture_list.clear();
			quad_uniform_set_texture_list.push_back(hg.MakeUniformSetTexture("s_tex", hg.OpenXRGetColorTextureFromId(eye_framebuffers, openxr_frame_info, 0), 0));
			hg.SetT(quad_matrix, hg.Vec3(eye_t_x, 0, 1));
			hg.DrawModel(vid, quad_model, tex0_program, quad_uniform_set_value_list, quad_uniform_set_texture_list, quad_matrix, quad_render_state);

			quad_uniform_set_texture_list.clear();
			quad_uniform_set_texture_list.push_back(hg.MakeUniformSetTexture("s_tex", hg.OpenXRGetColorTextureFromId(eye_framebuffers, openxr_frame_info, 1), 0));
			hg.SetT(quad_matrix, hg.Vec3(-eye_t_x, 0, 1));
			hg.DrawModel(vid, quad_model, tex0_program, quad_uniform_set_value_list, quad_uniform_set_texture_list, quad_matrix, quad_render_state);
		}

		hg.Frame();
		hg.OpenXRFinishSubmitFrameBuffer(openxr_frame_info);
		hg.UpdateWindow(win);
	}

	hg.OpenXRShutdown();
	hg.DestroyForwardPipeline(pipeline);
	hg.RenderShutdown();
	hg.DestroyWindow(win);
}

main();
