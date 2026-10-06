// Toyota 2JZ-GTE Engine model by Serhii Denysenko (CGTrader: serhiidenysenko8256)
// URL : https://www.cgtrader.com/3d-models/vehicle/part/toyota-2jz-gte-engine-2932b715-2f42-4ecd-93ce-df9507c67ce8

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('AAA Scene', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// load scene
		const scene = new hg.Scene();
		if (!hg.LoadSceneFromAssets('car_engine/engine.scn', scene, res, hg.GetForwardPipelineInfo())) {
			throw Error('Failed to load car_engine/engine.scn');
		}
		const trs = scene.GetNode('engine_master').GetTransform();
		if (!trs.IsValid()) throw Error('Missing engine_master transform');

		// AAA pipeline
		const pipeline_aaa_config = new hg.ForwardPipelineAAAConfig();
		const pipeline_aaa = hg.CreateForwardPipelineAAAFromAssets('core', pipeline_aaa_config, hg.BR_Equal, hg.BR_Equal);
		pipeline_aaa_config.sample_count = 1;
		let target_dof_focus_point = 3.5;
		let target_dof_focus_length = 2.0;
		pipeline_aaa_config.dof_focus_point = target_dof_focus_point; // Distance to the focus point (in meters)
		pipeline_aaa_config.dof_focus_length = target_dof_focus_length; // Depth of field (in meters); smaller values result in a narrower focused area.

		// main loop
		let frame = 0;

		return {
			draw(dt, res_x, res_y) {
				trs.SetRot(trs.GetRot().add(new hg.Vec3(0, hg.Deg(15) * hg.time_to_sec_f(dt), 0)));

				// change DOF randomly
				if (frame % 250 === 0) {
					target_dof_focus_point = Math.random() + 2.5; // random value between 2.5 and 3.5
					target_dof_focus_length = Math.random() * 1.5 + 0.5; // random value between 0.5 and 2.0
				}

				pipeline_aaa_config.dof_focus_point = hg.Lerp(pipeline_aaa_config.dof_focus_point, target_dof_focus_point, 0.1);
				pipeline_aaa_config.dof_focus_length = hg.Lerp(pipeline_aaa_config.dof_focus_length, target_dof_focus_length, 0.1);

				scene.Update(dt);
				hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, res_x, res_y), true, pipeline, res, pipeline_aaa, pipeline_aaa_config, frame);

				frame++; // runWindow owns the Frame() call
			},

			// cleanup
			dispose() {
				scene.Clear();
				hg.DestroyForwardPipelineAAA(pipeline_aaa);
				res.DestroyAllTextures();
				res.DestroyAllModels();
				res.DestroyAllPrograms();
				hg.DestroyForwardPipeline(pipeline);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
