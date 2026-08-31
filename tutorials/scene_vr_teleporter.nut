// Display a scene in VR with a teleporter probe

local hg = require("harfang");

function create_material(prg_ref, ubc, orm) {
	local mat = hg.Material();
	hg.SetMaterialProgram(mat, prg_ref);
	hg.SetMaterialValue(mat, "uBaseOpacityColor", ubc);
	hg.SetMaterialValue(mat, "uOcclusionRoughnessMetalnessColor", orm);
	return mat;
}

function get_head_pos(vr_state, actor_pos) {
	local head_t = hg.GetT(vr_state.head);
	return hg.Vec3(head_t.x, actor_pos.y, head_t.z);
}

function init_vr_controllers(vr_controller = null, vr_controller_idx = null) {
	if (vr_controller == null || vr_controller_idx == null) {
		vr_controller = [hg.VRController(), hg.VRController()];
		vr_controller_idx = [0, 0];
	}

	local name_template = "openvr_controller_";
	local left_hand_connected = vr_controller[0].IsConnected();
	local right_hand_connected = vr_controller[1].IsConnected();

	if (!left_hand_connected) {
		for (local i = 1; i <= 16; ++i) {
			if (!right_hand_connected || vr_controller_idx[1] != i) {
				local controller_state = hg.ReadVRController(name_template + i);
				if (controller_state.IsConnected()) {
					vr_controller[0] = hg.VRController(name_template + i);
					vr_controller_idx[0] = i;
					left_hand_connected = true;
					print(format("Using controller %d for left hand\n", i));
					break;
				}
			}
		}
	}

	if (!right_hand_connected) {
		for (local i = 1; i <= 16; ++i) {
			if (!left_hand_connected || vr_controller_idx[0] != i) {
				local controller_state = hg.ReadVRController(name_template + i);
				if (controller_state.IsConnected()) {
					vr_controller[1] = hg.VRController(name_template + i);
					vr_controller_idx[1] = i;
					right_hand_connected = true;
					print(format("Using controller %d for right hand\n", i));
					break;
				}
			}
		}
	}

	return [vr_controller, vr_controller_idx];
}

function update_vr_controllers(vr_controller, vr_controller_idx) {
	local init_result = init_vr_controllers(vr_controller, vr_controller_idx);
	vr_controller = init_result[0];
	vr_controller_idx = init_result[1];

	for (local i = 0; i < 2; ++i) {
		vr_controller[i].Update();
	}

	return [vr_controller, vr_controller_idx];
}

function update_teleporter_node(controller, actor_pos, head_pos, teleporter_node, playground) {
	if (!controller.IsConnected()) {
		teleporter_node.Disable();
		return actor_pos;
	}

	local world = controller.World();
	local t = hg.GetT(world);
	local z = hg.GetZ(world);
	local min_vy = -0.1;

	if (z.y >= min_vy) {
		teleporter_node.Disable();
		return actor_pos;
	}

	local teleport_i = null;
	if (playground != null) {
		teleport_i = t + z * ((playground[0].y - t.y) / z.y);
		teleport_i = hg.Clamp(teleport_i, playground[0], playground[1]);
	} else {
		teleport_i = t + z * ((0 - t.y) / z.y);
	}

	local teleporter_pos = teleport_i;
	if (controller.Pressed(hg.VRCB_Axis1)) {
		local new_actor_pos = teleport_i + (actor_pos - head_pos);
		local actor_diff = new_actor_pos - actor_pos;
		teleporter_pos = teleport_i + actor_diff;
		actor_pos = new_actor_pos;
	}

	teleporter_node.GetTransform().SetPos(teleporter_pos);
	teleporter_node.Enable();
	return actor_pos;
}

function draw_spline(p1, p2, p3, p4, vid, vtx_layout_spline, line_shader) {
	local step = 10;
	local segment_count = step + 2;
	local prev_value = p1;
	local vtx = hg.Vertices(vtx_layout_spline, segment_count * 2);

	for (local i = 0; i < segment_count; ++i) {
		local step_range = (1.0 / step) * i;
		local val = hg.Vec3(
			hg.CubicInterpolate(p2.x, p1.x, p4.x, p3.x, step_range),
			hg.CubicInterpolate(min(p2.y, 0), p1.y, p4.y, p3.y, step_range),
			hg.CubicInterpolate(p2.z, p1.z, p4.z, p3.z, step_range)
		);

		vtx.Begin(2 * i).SetPos(prev_value).SetColor0(hg.Color.get_Blue()).End();
		vtx.Begin(2 * i + 1).SetPos(val).SetColor0(hg.Color.get_Blue()).End();
		prev_value = val;
	}

	hg.DrawLines(vid, vtx, line_shader);
}

function draw_teleporter_spline(controller_mtx, ground_mtx, vid, vtx_layout_spline, line_shader) {
	local dir_teleporter = hg.GetZ(controller_mtx);
	local pos_start = hg.GetT(controller_mtx);
	local flat_dir = hg.Normalize(hg.Vec3(dir_teleporter.x, 0, dir_teleporter.z));
	local cos_angle = hg.Dot(dir_teleporter, flat_dir);

	if (cos_angle < -1.0) {
		cos_angle = -1.0;
	} else if (cos_angle > 1.0) {
		cos_angle = 1.0;
	}

	local angle = acos(cos_angle);
	local strength_force = (sin(angle) + 1) * 0.5;
	strength_force = strength_force * strength_force * 2.0;

	draw_spline(pos_start, pos_start + dir_teleporter * strength_force, ground_mtx + hg.Vec3(0, -strength_force, 0), ground_mtx, vid, vtx_layout_spline, line_shader);
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

	local cube_mdl = hg.CreateCubeModel(vtx_layout, 0.1, 0.1, 0.1);
	local cube_ref = res.AddModel("cube", cube_mdl);
	local ground_mdl = hg.CreateCubeModel(vtx_layout, 50, 0.01, 50);
	local ground_ref = res.AddModel("ground", ground_mdl);

	local line_shader = hg.LoadProgramFromFile("resources_compiled/shaders/pos_rgb");
	local vtx_layout_spline = hg.VertexLayout();
	vtx_layout_spline.Begin();
	vtx_layout_spline.Add(hg.A_Position, 3, hg.AT_Float);
	vtx_layout_spline.Add(hg.A_Color0, 3, hg.AT_Float);
	vtx_layout_spline.End();

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

	local teleporter_result = hg.CreateInstanceFromAssets(scene, hg.Mat4.get_Identity(), "teleporter/teleporter.scn", res, hg.GetForwardPipelineInfo());
	local teleporter_node = teleporter_result[0];
	local teleporter_loaded = teleporter_result[1];

	local hand_left_result = hg.CreateInstanceFromAssets(scene, hg.Mat4.get_Identity(), "vr_controller/vr_controller.scn", res, hg.GetForwardPipelineInfo());
	local hand_left = hand_left_result[0];
	local hand_left_loaded = hand_left_result[1];
	hand_left.SetName("hand_left");

	local hand_right_result = hg.CreateInstanceFromAssets(scene, hg.Mat4.get_Identity(), "vr_controller/vr_controller.scn", res, hg.GetForwardPipelineInfo());
	local hand_right = hand_right_result[0];
	local hand_right_loaded = hand_right_result[1];
	hand_right.SetName("hand_right");

	if (!teleporter_loaded || !hand_left_loaded || !hand_right_loaded) {
		print("Missing VR teleporter assets.\n");
		hg.OpenVRShutdown();
		hg.DestroyForwardPipeline(pipeline);
		hg.RenderShutdown();
		hg.DestroyWindow(win);
		return;
	}

	teleporter_node.Disable();

	local actor_pos = hg.Vec3(-1.3, 0, -2);
	local head_pos = hg.Vec3(0, 0, 0);
	local playground = [hg.Vec3(-3, 0, -3), hg.Vec3(3, 0, 3)];

	local init_result = init_vr_controllers();
	local vr_controller = init_result[0];
	local vr_controller_idx = init_result[1];

	while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
		local dt = hg.TickClock();

		local controllers_result = update_vr_controllers(vr_controller, vr_controller_idx);
		vr_controller = controllers_result[0];
		vr_controller_idx = controllers_result[1];

		hand_left.GetTransform().SetWorld(vr_controller[0].World());
		hand_right.GetTransform().SetWorld(vr_controller[1].World());

		local controller = vr_controller[0];
		actor_pos = update_teleporter_node(controller, actor_pos, head_pos, teleporter_node, playground);

		scene.Update(dt);

		local vr_state = hg.OpenVRGetState(hg.TranslationMat4(actor_pos), 0.1, 200);
		head_pos = get_head_pos(vr_state, actor_pos);

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

		if (controller.IsConnected() && teleporter_node.IsEnabled()) {
			hg.SetViewFrameBuffer(vid, vr_left_fb.GetHandle());
			hg.SetViewRect(vid, 0, 0, vr_state.width, vr_state.height);
			hg.SetViewClear(vid, 0, 0, 1.0, 0);
			hg.SetViewTransform(vid, left.view, left.proj);
			draw_teleporter_spline(controller.World(), teleporter_node.GetTransform().GetPos(), vid, vtx_layout_spline, line_shader);
			vid += 1;

			hg.SetViewFrameBuffer(vid, vr_right_fb.GetHandle());
			hg.SetViewRect(vid, 0, 0, vr_state.width, vr_state.height);
			hg.SetViewClear(vid, 0, 0, 1.0, 0);
			hg.SetViewTransform(vid, right.view, right.proj);
			draw_teleporter_spline(controller.World(), teleporter_node.GetTransform().GetPos(), vid, vtx_layout_spline, line_shader);
			vid += 1;
		}

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
