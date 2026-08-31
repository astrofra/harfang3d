// Instantiating scenes

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Scene instances", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local scene = hg.Scene();
hg.LoadSceneFromAssets("playground/playground.scn", scene, res, hg.GetForwardPipelineInfo());

class BipedActor {
	node = null;
	delay = 0;
	state = null;
	playing_anim_ref = null;

	constructor(pos) {
		local instance_result = hg.CreateInstanceFromAssets(scene, hg.Mat4.get_Identity(), "biped/biped.scn", res, hg.GetForwardPipelineInfo());
		node = instance_result[0];
		node.GetTransform().SetPosRot(pos, hg.Deg3(0, hg.FRand(360), 0));
	}

	function start_anim(name) {
		local anim = node.GetInstanceSceneAnim(name);
		if (playing_anim_ref != null) {
			scene.StopAnim(playing_anim_ref);
		}
		playing_anim_ref = scene.PlayAnim(anim, hg.ALM_Loop);
	}

	function update(dt) {
		delay -= dt;

		if (delay <= 0) {
			local states = ["idle", "walk", "run"];
			state = states[hg.Rand(states.len())];
			delay += hg.time_from_sec_f(hg.FRRand(2, 6));
			start_anim(state);
		}

		local dt_sec_f = hg.time_to_sec_f(dt);
		local transform = node.GetTransform();
		local pos_rot = transform.GetPosRot();
		local pos = pos_rot[0];
		local rot = pos_rot[1];

		if (state == "walk") {
			pos = pos - hg.GetZ(transform.GetWorld()) * hg.Mtr(1.15) * dt_sec_f;
			rot.y += hg.Deg(50) * dt_sec_f;
		} else if (state == "run") {
			pos = pos - hg.GetZ(transform.GetWorld()) * hg.Mtr(4.5) * dt_sec_f;
			rot.y -= hg.Deg(70) * dt_sec_f;
		}

		pos = hg.Clamp(pos, hg.Vec3(-10, 0, -10), hg.Vec3(10, 0, 10));
		transform.SetPosRot(pos, rot);
	}

	function destroy() {
		scene.DestroyNode(node);
	}
}

local actors = [];
for (local i = 0; i < 20; ++i) {
	actors.append(BipedActor(hg.RandomVec3(hg.Vec3(-10, 0, -10), hg.Vec3(10, 0, 10))));
}
print(format("%d nodes in scene\n", scene.GetAllNodeCount()));

local keyboard = hg.Keyboard();
local current_res_x = res_x;
local current_res_y = res_y;

while (!keyboard.Down(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local reset = hg.RenderResetToWindow(win, current_res_x, current_res_y, hg.RF_VSync | hg.RF_MSAA4X | hg.RF_MaxAnisotropy);
	current_res_x = reset[1];
	current_res_y = reset[2];

	keyboard.Update();

	if (keyboard.Pressed(hg.K_S)) {
		actors.append(BipedActor(hg.RandomVec3(hg.Vec3(-10, 0, -10), hg.Vec3(10, 0, 10))));
	}

	if (keyboard.Pressed(hg.K_D) && actors.len() > 0) {
		local actor = actors[actors.len() - 1];
		actors.remove(actors.len() - 1);
		actor.destroy();
		scene.GarbageCollect();
	}

	local dt = hg.TickClock();

	foreach (actor in actors) {
		actor.update(dt);
	}

	scene.Update(dt);

	local view_state = hg.ComputePerspectiveViewState(hg.Mat4LookAt(hg.Vec3(0, 10, -14), hg.Vec3(0, 1, -4)), hg.Deg(45), 0.01, 1000, hg.ComputeAspectRatioX(current_res_x, current_res_y));
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, current_res_x, current_res_y), view_state, pipeline, res);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);
