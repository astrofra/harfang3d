// Manually setup node physics

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	if (typeof hg.SceneBullet3Physics !== 'function') {
		throw Error('physics_manual_setup.js requires an hgjs build with Bullet scene physics. Use rebuild_hg_js_bullet.bat.');
	}
	return runWindow('Harfang - Node Physics Setup', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// create models
		const vtx_mdl = hg.VertexLayoutPosFloatNormUInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_mdl, 1, 1, 1);
		const cube_ref = res.AddModel('cube', cube_mdl);

		const ground_mdl = hg.CreateCubeModel(vtx_mdl, 50, 0.01, 50);
		const ground_ref = res.AddModel('ground', ground_mdl);

		// create materials
		const prg_ref = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', res, hg.GetForwardPipelineInfo());
		const mat = hg.CreateMaterial(prg_ref, 'uDiffuseColor', new hg.Vec4(0.5, 0.5, 0.5, 1), 'uSpecularColor', new hg.Vec4(0.0, 0.0, 0.0, 0.1));

		// setup scene
		const scene = new hg.Scene();

		const cam = hg.CreateCamera(scene, hg.TransformationMat4(new hg.Vec3(0, 1, -5), hg.Deg3(5, 0, 0)), 0.01, 1000);
		scene.SetCurrentCamera(cam);

		hg.CreatePointLight(scene, hg.TranslationMat4(new hg.Vec3(6, 4, -6)), 0);
		hg.CreatePhysicCube(scene, new hg.Vec3(100, 0.02, 100), hg.TranslationMat4(new hg.Vec3(0, -0.005, 0)), ground_ref, [mat], 0);

		const clocks = new hg.SceneClocks();

		// setup physic cube
		const cube_node = hg.CreateObject(scene, hg.TransformationMat4(new hg.Vec3(0, 2.5, 0), new hg.Vec3(0, 0, 0)), cube_ref, [mat]);

		const rb = scene.CreateRigidBody();
		rb.SetType(hg.RBT_Dynamic);

		const collision = scene.CreateCollision();
		collision.SetType(hg.CT_Cube);
		collision.SetSize(new hg.Vec3(1, 1, 1));
		collision.SetMass(1);

		cube_node.SetRigidBody(rb);
		cube_node.SetCollision(0, collision);

		// scene physics
		const physics = new hg.SceneBullet3Physics();
		scene.Update(0n);
		physics.SceneCreatePhysicsFromAssets(scene);

		// main loop
		return {
			draw(dt, res_x, res_y) {
				hg.SceneUpdateSystems(scene, clocks, dt, physics, hg.time_from_sec_f(1 / 60), 1);
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
