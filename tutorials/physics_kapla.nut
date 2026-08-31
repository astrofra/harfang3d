// Physics kapla towers

local hg = require("harfang");

local PI = 3.141592653589793;

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Kapla - Press SPACEBAR", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local sphere_mdl = hg.CreateSphereModel(vtx_layout, 0.5, 12, 24);
local sphere_ref = res.AddModel("sphere", sphere_mdl);

local prg_ref = hg.LoadPipelineProgramRefFromAssets("core/shader/pbr.hps", res, hg.GetForwardPipelineInfo());

local mat_cube = hg.CreateMaterial(prg_ref, "uBaseOpacityColor", hg.Vec4I(255, 255, 56), "uOcclusionRoughnessMetalnessColor", hg.Vec4(1, 0.658, 1));
local mat_ground = hg.CreateMaterial(prg_ref, "uBaseOpacityColor", hg.Vec4I(171, 255, 175), "uOcclusionRoughnessMetalnessColor", hg.Vec4(1, 1, 1));
local mat_spheres = hg.CreateMaterial(prg_ref, "uBaseOpacityColor", hg.Vec4I(255, 71, 75), "uOcclusionRoughnessMetalnessColor", hg.Vec4(1, 0.5, 0.1));

local scene = hg.Scene();
scene.canvas.color = hg.ColorI(200, 210, 208);
scene.environment.ambient = hg.Color(0, 0, 0, 1);

local cam = hg.CreateCamera(scene, hg.Mat4.get_Identity(), 0.01, 1000);
scene.SetCurrentCamera(cam);

local lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Deg3(19, 59, 0)), hg.Color(1.5, 0.9, 1.2, 1), hg.Color(1.5, 0.9, 1.2, 1), 10, hg.LST_Map, 0.002, hg.Vec4(8, 20, 40, 120));
local back_lgt = hg.CreatePointLight(scene, hg.TranslationMat4(hg.Vec3(30, 20, 25)), 100, hg.Color(0.8, 0.5, 0.4, 1), hg.Color(0.8, 0.5, 0.4, 1), 0);

local mdl_ref = res.AddModel("ground", hg.CreateCubeModel(vtx_layout, 200, 0.1, 200));
hg.CreatePhysicCube(scene, hg.Vec3(200, 0.1, 200), hg.TranslationMat4(hg.Vec3(0, -0.5, 0)), mdl_ref, [mat_ground], 0);

function add_kapla_tower(scn, resources, width, height, length, radius, material, level_count, x, y, z) {
	local level_y = y + height / 2.0;

	local kapla_mdl = hg.CreateCubeModel(vtx_layout, width, height, length);
	local kapla_ref = resources.AddModel("kapla", kapla_mdl);

	local nodes = [];
	local half_levels = level_count / 2;

	function fill_ring(r, ring_y, size, r_adjust, y_off) {
		local step = asin((size * 1.01) / 2.0 / (r - r_adjust)) * 2.0;
		local cube_count = floor((2.0 * PI) / step);
		local error = 2.0 * PI - step * cube_count;
		step += error / cube_count;

		local a = 0.0;
		while (a < (2.0 * PI - error)) {
			local world = hg.TransformationMat4(hg.Vec3(cos(a) * r + x, ring_y, sin(a) * r + z), hg.Vec3(0, -a + y_off, 0));
			local node = hg.CreatePhysicCube(scn, hg.Vec3(width, height, length), world, kapla_ref, [material], 0.1);
			nodes.append(node);
			a += step;
		}
	}

	for (local i = 0; i < half_levels; ++i) {
		fill_ring(radius - length / 2.0, level_y, width, length / 2.0, PI / 2.0);
		level_y += height;
		fill_ring(radius - length + width / 2.0, level_y, length, width / 2.0, 0);
		fill_ring(radius - width / 2.0, level_y, length, width / 2.0, 0);
		level_y += height;
	}

	return nodes;
}

add_kapla_tower(scene, res, 0.5, 2, 2, 6, mat_cube, 12, -12, 0, 0);
add_kapla_tower(scene, res, 0.5, 2, 2, 6, mat_cube, 12, 12, 0, 0);

local clocks = hg.SceneClocks();

local keyboard = hg.Keyboard();
local mouse = hg.Mouse();

local cam_pos = hg.Vec3(28.3, 31.8, 26.9);
local cam_rot = hg.Vec3(0.6, -2.38, 0);

local physics = hg.SceneBullet3Physics();
physics.SceneCreatePhysicsFromAssets(scene);
local physics_step = hg.time_from_sec_f(1 / 60.0);

while (!keyboard.Down(hg.K_Escape) && hg.IsWindowOpen(win)) {
	keyboard.Update();
	mouse.Update();

	local dt = hg.TickClock();

	local speed = 8.0;
	if (keyboard.Down(hg.K_LShift)) {
		speed = 20.0;
	}

	hg.FpsController(keyboard, mouse, cam_pos, cam_rot, speed, dt);

	cam.GetTransform().SetPos(cam_pos);
	cam.GetTransform().SetRot(cam_rot);

	if (keyboard.Pressed(hg.K_Space)) {
		local node = hg.CreatePhysicSphere(scene, 0.5, hg.TranslationMat4(cam_pos), sphere_ref, [mat_spheres], 0.5);
		physics.NodeCreatePhysicsFromAssets(node);
		physics.NodeAddImpulse(node, hg.GetZ(cam.GetTransform().GetWorld()) * 25.0, cam_pos);
	}

	hg.SceneUpdateSystems(scene, clocks, dt, physics, physics_step, 1);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	local view_id = submit_result[0];

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.DestroyForwardPipeline(pipeline);

hg.RenderShutdown();
hg.DestroyWindow(win);
