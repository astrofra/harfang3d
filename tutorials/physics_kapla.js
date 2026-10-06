// Physics kapla towers

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	if (typeof hg.SceneBullet3Physics !== 'function') {
		throw Error('physics_kapla.js requires an hgjs build with Bullet scene physics. Use rebuild_hg_js_bullet.bat.');
	}
	return runWindow('Harfang - Kapla - Press SPACEBAR', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// create models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const sphere_mdl = hg.CreateSphereModel(vtx_layout, 0.5, 12, 24);
		const sphere_ref = res.AddModel('sphere', sphere_mdl);

		// create materials
		const prg_ref = hg.LoadPipelineProgramRefFromAssets('core/shader/pbr.hps', res, hg.GetForwardPipelineInfo());

		const mat_cube = hg.CreateMaterial(prg_ref, 'uBaseOpacityColor', hg.Vec4I(255, 255, 56), 'uOcclusionRoughnessMetalnessColor', new hg.Vec4(1, 0.658, 1));
		const mat_ground = hg.CreateMaterial(prg_ref, 'uBaseOpacityColor', hg.Vec4I(171, 255, 175), 'uOcclusionRoughnessMetalnessColor', new hg.Vec4(1, 1, 1));
		const mat_spheres = hg.CreateMaterial(prg_ref, 'uBaseOpacityColor', hg.Vec4I(255, 71, 75), 'uOcclusionRoughnessMetalnessColor', new hg.Vec4(1, 0.5, 0.1));

		// setup scene
		const scene = new hg.Scene();
		scene.canvas.color = hg.ColorI(200, 210, 208);
		scene.environment.ambient = hg.Color.Black;

		const cam = hg.CreateCamera(scene, hg.Mat4.Identity, 0.01, 1000);
		scene.SetCurrentCamera(cam);

		const lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(new hg.Vec3(0, 0, 0), hg.Deg3(19, 59, 0)), new hg.Color(1.5, 0.9, 1.2, 1), new hg.Color(1.5, 0.9, 1.2, 1), 10, hg.LST_Map, 0.002, new hg.Vec4(8, 20, 40, 120));
		const back_lgt = hg.CreatePointLight(scene, hg.TranslationMat4(new hg.Vec3(30, 20, 25)), 100, new hg.Color(0.8, 0.5, 0.4, 1), new hg.Color(0.8, 0.5, 0.4, 1), 0);

		const mdl_ref = res.AddModel('ground', hg.CreateCubeModel(vtx_layout, 200, 0.1, 200));
		hg.CreatePhysicCube(scene, new hg.Vec3(200, 0.1, 200), hg.TranslationMat4(new hg.Vec3(0, -0.5, 0)), mdl_ref, [mat_ground], 0);


		function add_kapla_tower(scn, resources, width, height, length, radius, material, level_count, x, y, z) {
			// Create a Kapla tower, return a list of created nodes
			let level_y = y + height / 2;

			const kapla_mdl = hg.CreateCubeModel(vtx_layout, width, height, length);
			const kapla_ref = resources.AddModel(`kapla_${x}_${y}_${z}`, kapla_mdl); // keep both towers' model buffers owned

			const nodes = [];

			for (let i = 0; i < Math.floor(level_count / 2); i++) {
				function fill_ring(r, ring_y, size, r_adjust, y_off) {
					let step = Math.asin((size * 1.01) / 2 / (r - r_adjust)) * 2;
					const cube_count = Math.floor((2 * Math.PI) / step);
					const error = 2 * Math.PI - step * cube_count;
					step += error / cube_count; // distribute error

					let a = 0;
					while (a < (2 * Math.PI - error)) {
						const world = hg.TransformationMat4(new hg.Vec3(Math.cos(a) * r + x, ring_y, Math.sin(a) * r + z), new hg.Vec3(0, -a + y_off, 0));
						const node = hg.CreatePhysicCube(scn, new hg.Vec3(width, height, length), world, kapla_ref, [material], 0.1);
						nodes.push(node);
						a += step;
					}
				}

				fill_ring(radius - length / 2, level_y, width, length / 2, Math.PI / 2);
				level_y += height;
				fill_ring(radius - length + width / 2, level_y, length, width / 2, 0);
				fill_ring(radius - width / 2, level_y, length, width / 2, 0);
				level_y += height;
			}

			return nodes;
		}

		add_kapla_tower(scene, res, 0.5, 2, 2, 6, mat_cube, 12, -12, 0, 0);
		add_kapla_tower(scene, res, 0.5, 2, 2, 6, mat_cube, 12, 12, 0, 0);

		const clocks = new hg.SceneClocks();

		// input devices and fps controller states
		const keyboard = new hg.Keyboard();
		const mouse = new hg.Mouse();

		const cam_pos = new hg.Vec3(28.3, 31.8, 26.9);
		const cam_rot = new hg.Vec3(0.6, -2.38, 0);
		cam.GetTransform().SetPosRot(cam_pos, cam_rot);

		// setup physics
		const physics = new hg.SceneBullet3Physics();
		scene.Update(0n);
		physics.SceneCreatePhysicsFromAssets(scene);
		const physics_step = hg.time_from_sec_f(1 / 60);

		// main loop
		return {
			draw(dt, res_x, res_y) {
				keyboard.Update();
				mouse.Update();

				let s;
				if (keyboard.Down(hg.K_LShift)) {
					s = 20;
				} else {
					s = 8;
				}

				hg.FpsController(keyboard, mouse, cam_pos, cam_rot, s, dt);

				cam.GetTransform().SetPos(cam_pos);
				cam.GetTransform().SetRot(cam_rot);

				if (keyboard.Pressed(hg.K_Space)) {
					const node = hg.CreatePhysicSphere(scene, 0.5, hg.TranslationMat4(cam_pos), sphere_ref, [mat_spheres], 0.5);
					physics.NodeCreatePhysicsFromAssets(node);
					physics.NodeAddImpulse(node, hg.GetZ(cam.GetTransform().GetWorld()).mul(25.0), cam_pos);
				}

				hg.SceneUpdateSystems(scene, clocks, dt, physics, physics_step, 1);
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
