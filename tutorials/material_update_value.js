// Create textured material with pipeline shader

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Modify material pipeline shader uniforms', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// create models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
		const cube_ref = res.AddModel('cube', cube_mdl);
		const ground_mdl = hg.CreateCubeModel(vtx_layout, 100, 0.01, 100);
		const ground_ref = res.AddModel('ground', ground_mdl);

		// create materials
		const shader = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', res, hg.GetForwardPipelineInfo());

		const mat_cube = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(1, 1, 1), 'uSpecularColor', new hg.Vec4(1, 1, 1));
		const mat_ground = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(1, 1, 1), 'uSpecularColor', new hg.Vec4(0.1, 0.1, 0.1));

		// setup scene
		const scene = new hg.Scene();

		const cam = hg.CreateCamera(scene, hg.Mat4LookAt(new hg.Vec3(-1.30, 0.27, -2.47), new hg.Vec3(0, 0.5, 0)), 0.01, 1000);
		scene.SetCurrentCamera(cam);

		hg.CreateLinearLight(scene, hg.TransformationMat4(new hg.Vec3(0, 2, 0), hg.Deg3(27.5, -97.6, 16.6)), hg.ColorI(64, 64, 64), hg.ColorI(64, 64, 64), 10);
		hg.CreateSpotLight(scene, hg.TransformationMat4(new hg.Vec3(5, 4, -5), hg.Deg3(19, -45, 0)), 0, hg.Deg(5), hg.Deg(30), hg.ColorI(255, 255, 255), hg.ColorI(255, 255, 255), 10, hg.LST_Map, 0.0001);

		const cube_node = hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(0, 0.5, 0)), cube_ref, [mat_cube]);
		hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(0, 0, 0)), ground_ref, [mat_ground]);

		// material update states
		let mat_has_texture = false;
		let mat_update_delay = 0n;

		const texture_ref = hg.LoadTextureFromAssets('textures/squares.png', 0, res);

		// main loop
		return {
			draw(dt, res_x, res_y) {
				mat_update_delay -= dt;

				if (mat_update_delay <= 0n) {
					// set or remove cube node material texture
					const mat = cube_node.GetObject().GetMaterial(0);

					if (mat_has_texture) {
						hg.SetMaterialTexture(mat, 'uDiffuseMap', hg.InvalidTextureRef, 0);
					} else {
						hg.SetMaterialTexture(mat, 'uDiffuseMap', texture_ref, 0);
					}

					// update the pipeline shader variant according to the material uniform values
					hg.UpdateMaterialPipelineProgramVariant(mat, res);

					// reset delay and flip flag
					mat_update_delay += hg.time_from_sec(1n);
					mat_has_texture = !mat_has_texture;
				}
				scene.Update(dt);

				hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
			},

			// cleanup
			dispose() {
				scene.Clear();
				res.DestroyAllTextures();
				res.DestroyAllModels();
				res.DestroyAllPrograms();
				hg.DestroyForwardPipeline(pipeline);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
