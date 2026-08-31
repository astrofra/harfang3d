// Mouse flight

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Mouse Flight", res_x, res_y, hg.RF_VSync | hg.RF_MSAA8X);

local res = hg.PipelineResources();
local pipeline = hg.CreateForwardPipeline();

local keyboard = hg.Keyboard();
local mouse = hg.Mouse();

// Access to compiled resources.
hg.AddAssetsFolder("resources_compiled");

// 2D drawing helpers.
local vtx_layout = hg.VertexLayoutPosFloatColorFloat();
local draw2D_program = hg.LoadProgramFromAssets("shaders/pos_rgb");
local draw2D_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Less, hg.FC_Disabled);

function draw_circle(view_id, center, radius, color) {
	local segment_count = 32;
	local step = 2.0 * 3.141592653589793 / segment_count;
	local p0 = hg.Vec3(center.x + radius, center.y, 0);
	local p1 = hg.Vec3(0, 0, 0);

	local vtx = hg.Vertices(vtx_layout, segment_count * 2 + 2);

	for (local i = 0; i <= segment_count; ++i) {
		p1.x = radius * cos(i * step) + center.x;
		p1.y = radius * sin(i * step) + center.y;
		vtx.Begin(2 * i).SetPos(p0).SetColor0(color).End();
		vtx.Begin(2 * i + 1).SetPos(p1).SetColor0(color).End();
		p0.x = p1.x;
		p0.y = p1.y;
	}

	hg.DrawLines(view_id, vtx, draw2D_program, draw2D_render_state);
}

// Gameplay settings.
local setting_camera_chase_offset = hg.Vec3(0, 0.2, 0);
local setting_camera_chase_distance = 1.0;

local setting_plane_speed = 0.05;
local setting_plane_mouse_sensitivity = 0.5;

// Setup game world.
local scene = hg.Scene();
hg.LoadSceneFromAssets("playground/playground.scn", scene, res, hg.GetForwardPipelineInfo());

local plane_instance = hg.CreateInstanceFromAssets(scene, hg.TranslationMat4(hg.Vec3(0, 4, 0)), "paper_plane/paper_plane.scn", res, hg.GetForwardPipelineInfo());
local plane_node = plane_instance[0];
local camera_node = hg.CreateCamera(scene, hg.TranslationMat4(hg.Vec3(0, 4, -5)), 0.01, 1000);

scene.SetCurrentCamera(camera_node);

function update_plane(mouse_x_normd, mouse_y_normd) {
	local plane_transform = plane_node.GetTransform();

	local plane_pos = plane_transform.GetPos();
	plane_pos = plane_pos + hg.Normalize(hg.GetZ(plane_transform.GetWorld())) * setting_plane_speed;
	plane_pos.y = hg.Clamp(plane_pos.y, 0.1, 50.0); // floor/ceiling

	local plane_rot = plane_transform.GetRot();

	local next_plane_rot = hg.Vec3(plane_rot.x, plane_rot.y, plane_rot.z);
	next_plane_rot.x = hg.Clamp(next_plane_rot.x + mouse_y_normd * -0.03, -0.75, 0.75);
	next_plane_rot.y = next_plane_rot.y + mouse_x_normd * 0.03;
	next_plane_rot.z = hg.Clamp(mouse_x_normd * -0.75, -1.2, 1.2);

	plane_rot = plane_rot + (next_plane_rot - plane_rot) * setting_plane_mouse_sensitivity;

	plane_transform.SetPos(plane_pos);
	plane_transform.SetRot(plane_rot);
}

function update_chase_camera(target_pos) {
	local camera_transform = camera_node.GetTransform();
	local camera_to_target = hg.Normalize(target_pos - camera_transform.GetPos());

	camera_transform.SetPos(target_pos - camera_to_target * setting_camera_chase_distance); // Camera is "distance" away from its target.
	camera_transform.SetRot(hg.ToEuler(hg.Mat3LookAt(camera_to_target)));
}

while (!keyboard.Down(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock(); // Tick clock, retrieve elapsed time since last call.

	keyboard.Update();
	mouse.Update();

	local mouse_x = mouse.X();
	local mouse_y = mouse.Y();

	local aspect_ratio = hg.ComputeAspectRatioX(res_x, res_y);
	local mouse_x_normd = (mouse_x.tofloat() / res_x.tofloat() - 0.5) * aspect_ratio.x;
	local mouse_y_normd = (mouse_y.tofloat() / res_y.tofloat() - 0.5) * aspect_ratio.y;

	update_plane(mouse_x_normd, mouse_y_normd);
	update_chase_camera(plane_node.GetTransform().GetWorld() * setting_camera_chase_offset);

	scene.Update(dt);

	local view_id = 0;
	local submit_result = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	view_id = submit_result[0];
	local passes_id = submit_result[1];

	hg.SetView2D(view_id, 0, 0, res_x, res_y, -1, 1, hg.CF_Depth, hg.Color.get_Black(), 1, 0, true);
	draw_circle(view_id, hg.Vec3(mouse_x, mouse_y, 0), 20, hg.Color.get_White());

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);

hg.WindowSystemShutdown();
hg.InputShutdown();
