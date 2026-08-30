// Display a scene in VR

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
	local win = hg.RenderInit("Harfang - OpenVR Scene", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

	hg.AddAssetsFolder("resources_compiled");

	local pipeline = hg.CreateForwardPipeline();
	local res = hg.PipelineResources();
	local render_data = hg.SceneForwardPipelineRenderData();

	if (!hg.OpenVRInit()) {
		print("OpenVRInit failed.\n");
		hg.DestroyForwardPipeline(pipeline);
		hg.RenderShutdown();
		hg.DestroyWindow(win);
		return;
	}

	local vr_left_fb = hg.OpenVRCreateEyeFrameBuffer(hg.OVRAA_MSAA4x);
	local vr_right_fb = hg.OpenVRCreateEyeFrameBuffer(hg.OVRAA_MSAA4x);

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

	while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
		local dt = hg.TickClock();

		scene.Update(dt);

		local actor_body_mtx = hg.TransformationMat4(hg.Vec3(-1.3, 0.45, -2), hg.Vec3(0, 0, 0));
		local vr_state = hg.OpenVRGetState(actor_body_mtx, 0.01, 1000);
		local view_states = hg.OpenVRStateToViewState(vr_state);
		local left = view_states[0];
		local right = view_states[1];

		local vid = 0;
		local views = hg.SceneForwardPipelinePassViewId();

		local common_result = hg.PrepareSceneForwardPipelineCommonRenderData(vid, scene, render_data, pipeline, res, views);
		vid = common_result[0];
		views = common_result[1];

		local vr_eye_rect = hg.IntRect(0, 0, vr_state.width, vr_state.height);

		local left_result = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, left, scene, render_data, pipeline, res, views);
		vid = left_result[0];
		views = left_result[1];

		local submit_left_result = hg.SubmitSceneToForwardPipeline(vid, scene, vr_eye_rect, left, pipeline, render_data, res, vr_left_fb.GetHandle());
		vid = submit_left_result[0];
		views = submit_left_result[1];

		local right_result = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, right, scene, render_data, pipeline, res, views);
		vid = right_result[0];
		views = right_result[1];

		local submit_right_result = hg.SubmitSceneToForwardPipeline(vid, scene, vr_eye_rect, right, pipeline, render_data, res, vr_right_fb.GetHandle());
		vid = submit_right_result[0];

		hg.SetViewRect(vid, 0, 0, res_x, res_y);
		local vs = hg.ComputeOrthographicViewState(hg.TranslationMat4(hg.Vec3(0, 0, 0)), res_y, 0.1, 100, hg.ComputeAspectRatioX(res_x, res_y));
		hg.SetViewTransform(vid, vs.view, vs.proj);

		quad_uniform_set_texture_list.clear();
		quad_uniform_set_texture_list.push_back(hg.MakeUniformSetTexture("s_tex", hg.OpenVRGetColorTexture(vr_left_fb), 0));
		hg.SetT(quad_matrix, hg.Vec3(eye_t_x, 0, 1));
		hg.DrawModel(vid, quad_model, tex0_program, quad_uniform_set_value_list, quad_uniform_set_texture_list, quad_matrix, quad_render_state);

		quad_uniform_set_texture_list.clear();
		quad_uniform_set_texture_list.push_back(hg.MakeUniformSetTexture("s_tex", hg.OpenVRGetColorTexture(vr_right_fb), 0));
		hg.SetT(quad_matrix, hg.Vec3(-eye_t_x, 0, 1));
		hg.DrawModel(vid, quad_model, tex0_program, quad_uniform_set_value_list, quad_uniform_set_texture_list, quad_matrix, quad_render_state);

		hg.Frame();
		hg.OpenVRSubmitFrame(vr_left_fb, vr_right_fb);
		hg.UpdateWindow(win);
	}

	hg.OpenVRShutdown();
	hg.DestroyForwardPipeline(pipeline);
	hg.RenderShutdown();
	hg.DestroyWindow(win);
}

main();
