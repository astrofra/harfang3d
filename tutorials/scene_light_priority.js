// Dynamically assign lights to the fixed pipeline slots by adjusting their priority

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Light priority relative to a specific world position', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// create models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const light_mdl = hg.CreateSphereModel(vtx_layout, 0.05, 8, 16);
		const light_ref = res.AddModel('light', light_mdl);
		const orb_mdl = hg.CreateSphereModel(vtx_layout, 1, 16, 32);
		const orb_ref = res.AddModel('orb', orb_mdl);
		const ground_mdl = hg.CreateCubeModel(vtx_layout, 100, 0.01, 100);
		const ground_ref = res.AddModel('ground', ground_mdl);

		// create materials
		const shader = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', res, hg.GetForwardPipelineInfo());

		const mat_light = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(0, 0, 0), 'uSpecularColor', new hg.Vec4(0, 0, 0));
		hg.SetMaterialValue(mat_light, 'uSelfColor', new hg.Vec4(1, 0.9, 0.75));
		const mat_orb = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(1, 1, 1), 'uSpecularColor', new hg.Vec4(1, 1, 1));
		hg.SetMaterialValue(mat_orb, 'uSelfColor', new hg.Vec4(0, 0, 0));
		const mat_ground = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(1, 1, 1), 'uSpecularColor', new hg.Vec4(1, 1, 1));
		hg.SetMaterialValue(mat_ground, 'uSelfColor', new hg.Vec4(0, 0, 0));

		// setup scene
		const scene = new hg.Scene();

		const cam = hg.CreateCamera(scene, hg.Mat4LookAt(new hg.Vec3(5, 4, -7), new hg.Vec3(0, 1.5, 0)), 0.01, 1000);
		scene.SetCurrentCamera(cam);

		const orb_node = hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(0, 1, 0)), orb_ref, [mat_orb]);
		hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(0, 0, 0)), ground_ref, [mat_ground]);

		// create an array of dynamic lights
		const light_obj = scene.CreateObject(light_ref, [mat_light]); // sphere model to visualize lights

		const light_nodes = [];
		for (let i = 0; i < 16; i++) {
			const node = hg.CreatePointLight(scene, hg.Mat4.Identity, 1.5, new hg.Color(1, 0.85, 0.25, 1), new hg.Color(1, 0.9, 0.5, 1));
			node.SetObject(light_obj);
			light_nodes.push(node);
		}

		// main loop
		let angle = 0;

		return {
			draw(dt, res_x, res_y) {
				// animate lights
				angle += hg.time_to_sec_f(dt);

				for (let i = 0; i < light_nodes.length; i++) {
					const node = light_nodes[i];
					const a = angle + (i + 1) * hg.Deg(15); // preserve Lua's one-based animation phase
					node.GetTransform().SetPos(new hg.Vec3(Math.cos(a * -0.6) * Math.sin(a) * 5, Math.cos(a * 1.25) * 2 + 2.15, Math.sin(a * 0.5) * Math.cos(-a * 0.8) * 5));
				}
				// update light priorities according to their distance to the orb
				for (const node of light_nodes) {
					const priority = hg.Dist(orb_node.GetTransform().GetPos(), node.GetTransform().GetPos());
					// const priority = node.GetTransform().GetPos().y; // uncomment to prioritize lights near the ground
					node.GetLight().SetPriority(-priority);
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
