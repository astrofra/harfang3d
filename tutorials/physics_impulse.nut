// Physics impulse

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Physics Force/Impulse (Press space to alternate)", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local cube_ref = res.AddModel("cube", cube_mdl);

local ground_mdl = hg.CreateCubeModel(vtx_layout, 50, 0.01, 50);
local ground_ref = res.AddModel("ground", ground_mdl);

local prg_ref = hg.LoadPipelineProgramRefFromFile("resources_compiled/core/shader/default.hps", res, hg.GetForwardPipelineInfo());
local mat = hg.CreateMaterial(prg_ref, "uDiffuseColor", hg.Vec4(1, 1, 1), "uSpecularColor", hg.Vec4(1, 1, 1));

local scene = hg.Scene();

local cam = hg.CreateCamera(scene, hg.TransformationMat4(hg.Vec3(0, 1.5, -5), hg.Vec3(hg.Deg(10), 0, 0)), 0.01, 1000);
scene.SetCurrentCamera(cam);

local lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(hg.Deg(30), hg.Deg(59), 0)), hg.Color(1, 1, 1), hg.Color(1, 1, 1), 10, hg.LST_Map, 0.002, hg.Vec4(2, 4, 10, 16));

local cube_node = hg.CreatePhysicCube(scene, hg.Vec3(1, 1, 1), hg.TranslationMat4(hg.Vec3(0, 1.5, 0)), cube_ref, [mat], 2);
local ground_node = hg.CreatePhysicCube(scene, hg.Vec3(100, 0.02, 100), hg.TranslationMat4(hg.Vec3(0, -0.005, 0)), ground_ref, [mat], 0);

local clocks = hg.SceneClocks();

local physics = hg.SceneBullet3Physics();
physics.SceneCreatePhysicsFromAssets(scene);
local physics_step = hg.time_from_sec_f(1 / 60.0);

local keyboard = hg.Keyboard();
local use_force = true;

while (!keyboard.Down(hg.K_Escape) && hg.IsWindowOpen(win)) {
	keyboard.Update();

	local dt = hg.TickClock();

	if (keyboard.Pressed(hg.K_Space)) {
		use_force = !use_force;
	}

	local world_pos = hg.GetT(cube_node.GetTransform().GetWorld());
	local dist_to_ground = world_pos.y - 0.5;

	if (dist_to_ground < 1.0) {
		local k = -(dist_to_ground - 1.0);

		if (use_force) {
			local force = hg.Vec3(0, 1, 0) * k * 80.0;
			physics.NodeAddForce(cube_node, force, world_pos);
		} else {
			local stiffness = 10.0;
			local cur_velocity = physics.NodeGetLinearVelocity(cube_node);
			local tgt_velocity = hg.Vec3(0, 1, 0) * k * stiffness;
			local impulse = tgt_velocity - cur_velocity;
			physics.NodeAddImpulse(cube_node, impulse, world_pos);
		}
	}

	physics.NodeWake(cube_node);

	hg.SceneUpdateSystems(scene, clocks, dt, physics, physics_step, 3);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	local view_id = submit_result[0];

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);

hg.WindowSystemShutdown();
hg.InputShutdown();
