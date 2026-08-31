// Mouse scene projection

local hg = require("harfang");

function draw_line(pos_a, pos_b, line_color, vid, vtx_line_layout, line_shader) {
	local vtx = hg.Vertices(vtx_line_layout, 2);
	vtx.Begin(0).SetPos(pos_a).SetColor0(line_color).End();
	vtx.Begin(1).SetPos(pos_b).SetColor0(line_color).End();
	hg.DrawLines(vid, vtx, line_shader);
}

function add_debug_cross(lines, pos, world, size) {
	lines.append([pos + hg.GetX(world) * size, pos - hg.GetX(world) * size, hg.Color(1, 0, 0, 1)]);
	lines.append([pos + hg.GetY(world) * size, pos - hg.GetY(world) * size, hg.Color(0, 1, 0, 1)]);
	lines.append([pos + hg.GetZ(world) * size, pos - hg.GetZ(world) * size, hg.Color(0, 0, 1, 1)]);
	return lines;
}

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Mouse scene projection", res_x, res_y, hg.RF_VSync);

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

hg.AddAssetsFolder("resources_compiled");

local scene = hg.Scene();
hg.LoadSceneFromAssets("mouse_scene_projection/mouse_scene_projection.scn", scene, res, hg.GetForwardPipelineInfo());

local camera = scene.GetNode("Camera");
scene.SetCurrentCamera(camera);

local rectangle_node = scene.GetNode("rectangle");

local vtx_line_layout = hg.VertexLayoutPosFloatColorUInt8();
local shader_for_line = hg.LoadProgramFromAssets("shaders/pos_rgb");

local keyboard = hg.Keyboard();
local mouse = hg.Mouse();

local screen_pos_middle = hg.Vec3(res_x / 2.0, res_y / 2.0, 1.0);
local screen_pos_up_left = hg.Vec3(0, res_y, 1.0);
local screen_pos_down_right = hg.Vec3(res_x, 0, 1.0);

while (!keyboard.Pressed(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();

	local reset = hg.RenderResetToWindow(win, res_x, res_y, hg.RF_VSync);
	local render_was_reset = reset[0];
	res_x = reset[1];
	res_y = reset[2];

	if (render_was_reset) {
		screen_pos_middle = hg.Vec3(res_x / 2.0, res_y / 2.0, 1.0);
		screen_pos_up_left = hg.Vec3(0, res_y, 1.0);
		screen_pos_down_right = hg.Vec3(res_x, 0, 1.0);
	}

	local lines = [];

	keyboard.Update();
	mouse.Update();

	local mouse_x = mouse.X();
	local mouse_y = mouse.Y();

	local cursor_screen_pos = hg.Vec3(mouse_x, mouse_y, 1);
	local resolution = hg.Vec2(res_x, res_y);

	local view_state = hg.ComputePerspectiveViewState(
		camera.GetTransform().GetWorld(),
		camera.GetCamera().GetFov(),
		camera.GetCamera().GetZNear(),
		camera.GetCamera().GetZFar(),
		hg.ComputeAspectRatioX(res_x, res_y)
	);

	local inv_proj_result = hg.Inverse(view_state.proj);
	local inv_proj = inv_proj_result[0];
	local inv_proj_ok = inv_proj_result[1];

	local inv_view_result = hg.Inverse(view_state.view);
	local inv_view_ok = inv_view_result[0];
	local inv_view = inv_view_result[1];

	if (!inv_proj_ok || !inv_view_ok) {
		hg.Frame();
		hg.UpdateWindow(win);
		continue;
	}

	local ray_o = hg.GetT(inv_view);

	local view_pos_result = hg.UnprojectFromScreenSpace(inv_proj, cursor_screen_pos, resolution);
	local view_pos_ok = view_pos_result[0];
	local view_pos = view_pos_result[1];

	local view_pos_middle_result = hg.UnprojectFromScreenSpace(inv_proj, screen_pos_middle, resolution);
	local view_pos_middle_ok = view_pos_middle_result[0];
	local view_pos_middle = view_pos_middle_result[1];

	local view_pos_up_left_result = hg.UnprojectFromScreenSpace(inv_proj, screen_pos_up_left, resolution);
	local view_pos_up_left_ok = view_pos_up_left_result[0];
	local view_pos_up_left = view_pos_up_left_result[1];

	local view_pos_down_right_result = hg.UnprojectFromScreenSpace(inv_proj, screen_pos_down_right, resolution);
	local view_pos_down_right_ok = view_pos_down_right_result[0];
	local view_pos_down_right = view_pos_down_right_result[1];

	if (!view_pos_ok || !view_pos_middle_ok || !view_pos_up_left_ok || !view_pos_down_right_ok) {
		hg.Frame();
		hg.UpdateWindow(win);
		continue;
	}

	local view_pos_normalize = hg.Normalize(view_pos) + ray_o;
	local view_pos_middle_normalize = hg.Normalize(view_pos_middle) + ray_o;
	local view_pos_up_left_normalize = hg.Normalize(view_pos_up_left) + ray_o;
	local view_pos_down_right_normalize = hg.Normalize(view_pos_down_right) + ray_o;

	lines = add_debug_cross(lines, view_pos_normalize, hg.TransformationMat4(view_pos_normalize, hg.Vec3(0, 0, 0)), 0.01);
	lines = add_debug_cross(lines, view_pos_middle_normalize, hg.TransformationMat4(view_pos_middle_normalize, hg.Vec3(0, 0, 0)), 0.1);
	lines = add_debug_cross(lines, view_pos_up_left_normalize, hg.TransformationMat4(view_pos_up_left_normalize, hg.Vec3(0, 0, 0)), 0.1);
	lines = add_debug_cross(lines, view_pos_down_right_normalize, hg.TransformationMat4(view_pos_down_right_normalize, hg.Vec3(0, 0, 0)), 0.1);

	lines.append([hg.Vec3(0, 1.5, -5), view_pos, hg.Color(0, 0, 1, 1)]);

	local mat_look_at = hg.Mat4LookAt(rectangle_node.GetTransform().GetPos(), view_pos);
	rectangle_node.GetTransform().SetWorld(mat_look_at);

	scene.Update(dt);

	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	local view_id = submit_result[0];
	local pass_ids = submit_result[1];

	local opaque_view_id = hg.GetSceneForwardPipelinePassViewId(pass_ids, hg.SFPP_Opaque);
	for (local i = 0; i < lines.len(); ++i) {
		local line = lines[i];
		draw_line(line[0], line[1], line[2], opaque_view_id, vtx_line_layout, shader_for_line);
	}

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
