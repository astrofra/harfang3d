// Physics Impulse

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main({ useForce: use_force = true, ...options } = {}) {
	if (typeof hg.SceneBullet3Physics !== 'function') {
		throw Error('physics_impulse.js requires an hgjs build with Bullet scene physics. Use rebuild_hg_js_bullet.bat.');
	}
	return runWindow('Harfang - Physics Force/Impulse (Press space to alternate)', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// create models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
		const cube_ref = res.AddModel('cube', cube_mdl);

		const ground_mdl = hg.CreateCubeModel(vtx_layout, 50, 0.01, 50);
		const ground_ref = res.AddModel('ground', ground_mdl);

		// create material
		const prg_ref = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', res, hg.GetForwardPipelineInfo());
		const mat = hg.CreateMaterial(prg_ref, 'uDiffuseColor', new hg.Vec4(1, 1, 1), 'uSpecularColor', new hg.Vec4(1, 1, 1));

		// setup scene
		const scene = new hg.Scene();

		const cam = hg.CreateCamera(scene, hg.TransformationMat4(new hg.Vec3(0, 1.5, -5), hg.Deg3(10, 0, 0)), 0.01, 1000);
		scene.SetCurrentCamera(cam);

		const lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(new hg.Vec3(), hg.Deg3(30, 59, 0)), hg.Color.White, hg.Color.White, 10, hg.LST_Map, 0.002, new hg.Vec4(2, 4, 10, 16));

		const cube_node = hg.CreatePhysicCube(scene, new hg.Vec3(1, 1, 1), hg.TranslationMat4(new hg.Vec3(0, 2.5, 0)), cube_ref, [mat], 2);
		const ground_node = hg.CreatePhysicCube(scene, new hg.Vec3(100, 0.02, 100), hg.TranslationMat4(new hg.Vec3(0, -0.005, 0)), ground_ref, [mat], 0);

		const clocks = new hg.SceneClocks();

		// scene physics
		const physics = new hg.SceneBullet3Physics();
		scene.Update(0n);
		physics.SceneCreatePhysicsFromAssets(scene);
		const physics_step = hg.time_from_sec_f(1 / 60);

		// main loop
		const keyboard = new hg.Keyboard();
		console.log(`Mode: ${use_force ? 'force' : 'impulse'} (Space to switch)`);

		return {
			draw(dt, res_x, res_y) {
				keyboard.Update();

				if (keyboard.Pressed(hg.K_Space)) {
					use_force = !use_force;
					console.log(`Mode: ${use_force ? 'force' : 'impulse'}`);
				}

				const world_pos = hg.GetT(cube_node.GetTransform().GetWorld());
				const dist_to_ground = world_pos.y - 0.5;

				if (dist_to_ground < 1) {
					const k = 1 - dist_to_ground;

					if (use_force) {
						const F = new hg.Vec3(0, k * 80, 0); // apply a force inversely proportional to the distance to the ground
						physics.NodeAddForce(cube_node, F, world_pos);
					} else {
						const stiffness = 10;

						const cur_velocity = physics.NodeGetLinearVelocity(cube_node);
						const tgt_velocity = new hg.Vec3(0, k * stiffness, 0); // compute a velocity that brings us to 1 meter above the ground

						const I = tgt_velocity.sub(cur_velocity); // an impulse is an instantaneous change in velocity
						physics.NodeAddImpulse(cube_node, I, world_pos);
					}
				}

				physics.NodeWake(cube_node);

				// Bound long pauses while keeping the native fixed physics step (BigInt ns).
				hg.SceneUpdateSystems(scene, clocks, dt > 50000000n ? 50000000n : dt, physics, physics_step, 3);
				hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
			},

			// cleanup
			dispose() {
				hg.SceneClearSystems(scene, physics);
				res.DestroyAllTextures();
				res.DestroyAllModels();
				res.DestroyAllPrograms();
				hg.DestroyForwardPipeline(pipeline);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
