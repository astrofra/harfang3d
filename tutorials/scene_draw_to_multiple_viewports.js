// Draw to multiple viewports

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - Scene Draw to Multiple Viewports', () => {
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

		const mat_yellow_cube = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(255, 220, 64), 'uSpecularColor', hg.Vec4I(255, 220, 64));
		const mat_red_cube = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(255, 0, 0), 'uSpecularColor', hg.Vec4I(255, 0, 0));
		const mat_ground = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(128, 128, 128), 'uSpecularColor', hg.Vec4I(128, 128, 128));

		// setup scene (note that we do not create any camera)
		const scene = new hg.Scene();

		hg.CreateSpotLight(scene, hg.TransformationMat4(new hg.Vec3(-8, 4, -5), hg.Deg3(19, 59, 0)), 0, hg.Deg(5), hg.Deg(30), hg.Color.White, hg.Color.White, 10, hg.LST_Map, 0.00005);
		hg.CreatePointLight(scene, hg.TranslationMat4(new hg.Vec3(3, 1, 2.5)), 5, hg.ColorI(128, 192, 255), hg.Color.Black, 0);

		const yellow_cube = hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(1, 0.5, 0)), cube_ref, [mat_yellow_cube]);
		hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(-1, 0.5, 0)), cube_ref, [mat_red_cube]);
		hg.CreateObject(scene, hg.Mat4.Identity, ground_ref, [mat_ground]);

		// define viewports
		const viewports = [
			{ cam_pos: new hg.Vec3(-4.015, 2.368, -3.484), cam_rot: new hg.Vec3(0.35, 0.87, 0.0) },
			{ cam_pos: new hg.Vec3(-4.143, 2.976, 4.127), cam_rot: new hg.Vec3(0.423, 2.365, 0.0) },
			{ cam_pos: new hg.Vec3(4.020, 2.374, 3.469), cam_rot: new hg.Vec3(0.353, 4.016, 0.0) },
			{ cam_pos: new hg.Vec3(3.469, 2.374, -4.020), cam_rot: new hg.Vec3(0.353, -0.695, 0.0) },
		];

		// main loop
		return {
			draw(dt, res_x, res_y) {
				// animate yellow cube & update scene once for all viewports
				const rot = yellow_cube.GetTransform().GetRot();
				rot.y += hg.time_to_sec_f(dt);
				yellow_cube.GetTransform().SetRot(rot);

				scene.Update(dt);

				const half_width = Math.floor(res_x / 2), half_height = Math.floor(res_y / 2);
				const rects = [
					new hg.IntRect(0, 0, half_width, half_height),
					new hg.IntRect(half_width, 0, res_x, half_height),
					new hg.IntRect(0, half_height, half_width, res_y),
					new hg.IntRect(half_width, half_height, res_x, res_y),
				];
				for (let i = 0; i < viewports.length; i++) {
					viewports[i].rect = rects[i];
				}

				// prepare view-independent render data (eg. spot shadow maps)
				const render_data = new hg.SceneForwardPipelineRenderData();

				let views = new hg.SceneForwardPipelinePassViewId();
				// Native in/out parameters are returned as a JavaScript array.
				let vid = 0;
				[vid, views] = hg.PrepareSceneForwardPipelineCommonRenderData(vid, scene, render_data, pipeline, res, views);

				for (const viewport of viewports) {
					// compute viewport specific view state
					const rect = viewport.rect;
					const view_state = hg.ComputePerspectiveViewState(hg.TransformationMat4(viewport.cam_pos, viewport.cam_rot), hg.Deg(45), 0.01, 1000, hg.ComputeAspectRatioX(rect.ex - rect.sx, rect.ey - rect.sy));
					// prepare view-dependent render data & submit draw
					[vid, views] = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, view_state, scene, render_data, pipeline, res, views);
					[vid] = hg.SubmitSceneToForwardPipeline(vid, scene, viewport.rect, view_state, pipeline, render_data, res);
				}
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
