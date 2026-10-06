// Instantiating scenes

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Scene instances', () => {
		// rendering pipeline
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// load host scene
		const scene = new hg.Scene();
		if (!hg.LoadSceneFromAssets('playground/playground.scn', scene, res, hg.GetForwardPipelineInfo())) {
			throw Error('Failed to load playground/playground.scn');
		}


		// declare the biped actor class
		class BipedActor {
			constructor(pos) {
				const [node, success] = hg.CreateInstanceFromAssets(scene, hg.Mat4.Identity, 'biped/biped.scn', res, hg.GetForwardPipelineInfo());
				if (!success) throw Error('Failed to instantiate biped/biped.scn');
				this.__node = node;
				this.__node.GetTransform().SetPosRot(pos, hg.Deg3(0, hg.FRand(360), 0));
				this.__delay = 0n;
				this.__state = null;
				this.__playing_anim_ref = null;
			}

			__start_anim(name) {
				const anim = this.__node.GetInstanceSceneAnim(name); // get instance specific animation
				if (anim.equals(hg.InvalidSceneAnimRef)) throw Error('Missing biped animation: ' + name);
				if (this.__playing_anim_ref !== null) {
					scene.StopAnim(this.__playing_anim_ref);
				}
				this.__playing_anim_ref = scene.PlayAnim(anim, hg.ALM_Loop);
			}

			update(dt) {
				// check for state change
				this.__delay -= dt;

				if (this.__delay <= 0n) {
					const states = ['idle', 'walk', 'run'];
					this.__state = states[hg.Rand(states.length)];
					this.__delay += hg.time_from_sec_f(hg.FRRand(2, 6)); // 2 to 6 seconds before next state change
					this.__start_anim(this.__state);
				}

				// apply motion
				const dt_sec_f = hg.time_to_sec_f(dt);

				const transform = this.__node.GetTransform();
				let [pos, rot] = transform.GetPosRot();

				if (this.__state === 'walk') {
					pos = pos.sub(hg.GetZ(transform.GetWorld()).mul(hg.Mtr(1.15) * dt_sec_f)); // 1.15 m/sec
					rot.y += hg.Deg(50) * dt_sec_f;
				} else if (this.__state === 'run') {
					pos = pos.sub(hg.GetZ(transform.GetWorld()).mul(hg.Mtr(4.5) * dt_sec_f)); // 4.5 m/sec
					rot.y -= hg.Deg(70) * dt_sec_f;
				}

				// confine actor to playground
				pos = hg.Clamp(pos, new hg.Vec3(-10, 0, -10), new hg.Vec3(10, 0, 10));

				transform.SetPosRot(pos, rot);
			}

			destroy() {
				if (this.__playing_anim_ref !== null) scene.StopAnim(this.__playing_anim_ref);
				this.__node.DestroyInstance();
				scene.DestroyNode(this.__node);
			}
		}


		// spawn initial actors
		const actors = [];
		for (let i = 0; i < 20; i++) {
			const actor = new BipedActor(hg.RandomVec3(new hg.Vec3(-10, 0, -10), new hg.Vec3(10, 0, 10)));
			actors.push(actor);
		}
		console.log(`${scene.GetAllNodeCount()} nodes in scene`);

		// main loop
		const keyboard = new hg.Keyboard();

		return {
			draw(dt, res_x, res_y) {
				keyboard.Update();

				if (keyboard.Pressed(hg.K_S)) {
					actors.push(new BipedActor(hg.RandomVec3(new hg.Vec3(-10, 0, -10), new hg.Vec3(10, 0, 10))));
				}

				if (keyboard.Pressed(hg.K_D)) {
					if (actors.length > 0) {
						actors.shift().destroy();
						scene.GarbageCollect();
					}
				}

				for (const actor of actors) {
					actor.update(dt);
				}

				scene.Update(dt);

				const view_state = hg.ComputePerspectiveViewState(hg.Mat4LookAt(new hg.Vec3(0, 10, -14), new hg.Vec3(0, 1, -4)), hg.Deg(45), 0.01, 1000, hg.ComputeAspectRatioX(res_x, res_y));
				hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, res_x, res_y), view_state, pipeline, res);
			},

			// cleanup
			dispose() {
				for (const actor of actors) actor.destroy();
				scene.Clear();
				res.DestroyAllTextures();
				res.DestroyAllModels();
				res.DestroyAllPrograms();
				hg.DestroyForwardPipeline(pipeline);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X | hg.RF_MaxAnisotropy, resizeToWindow: true, ...options });
}
