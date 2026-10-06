// Spot light shadow clip

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - Spot Shadow Clip', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// create models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
		const cube_ref = res.AddModel('cube', cube_mdl);
		const blocker_mdl = hg.CreateCubeModel(vtx_layout, 0.7, 1.6, 0.7);
		const blocker_ref = res.AddModel('blocker', blocker_mdl);
		const ground_mdl = hg.CreateCubeModel(vtx_layout, 12, 0.02, 14);
		const ground_ref = res.AddModel('ground', ground_mdl);
		const wall_mdl = hg.CreateCubeModel(vtx_layout, 8, 3, 0.08);
		const wall_ref = res.AddModel('wall', wall_mdl);

		// create materials
		const shader = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', res, hg.GetForwardPipelineInfo());

		const mat_yellow_cube = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(255, 220, 64), 'uSpecularColor', hg.Vec4I(255, 220, 64));
		const mat_red_cube = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(255, 72, 56), 'uSpecularColor', hg.Vec4I(255, 72, 56));
		const mat_blue_cube = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(64, 128, 255), 'uSpecularColor', hg.Vec4I(64, 128, 255));
		const mat_ground = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(160, 160, 160), 'uSpecularColor', hg.Vec4I(80, 80, 80));
		const mat_wall = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(190, 190, 190), 'uSpecularColor', hg.Vec4I(80, 80, 80));

		const font = hg.LoadFontFromAssets('font/default.ttf', 32);
		const font_program = hg.LoadProgramFromAssets('core/shader/font');
		const text_uniform_values = [hg.MakeUniformSetValue('u_color', new hg.Vec4(1, 1, 1))];
		const text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled);

		// setup scene
		const scene = new hg.Scene();
		scene.canvas.color = hg.ColorI(20, 24, 30);
		scene.environment.ambient = new hg.Color(0.025, 0.025, 0.025);

		const cam = hg.CreateCamera(scene, hg.Mat4LookAt(new hg.Vec3(5.5, 4.0, -8.0), new hg.Vec3(0.1, 1.0, 0.6)), 0.01, 100);
		scene.SetCurrentCamera(cam);

		const spot_pos = new hg.Vec3(0, 4.2, -5.0);
		const spot_target = new hg.Vec3(0, 0.9, 2.8);
		const shadow_near_min = 0.1;
		const shadow_near_max = 7.5;
		const shadow_far = 18.0;

		const spot_node = hg.CreateSpotLight(scene, hg.Mat4LookAt(spot_pos, spot_target), 0, hg.Deg(4), hg.Deg(34), hg.Color.White, 1, hg.Color.White, 1, 10, hg.LST_Map, 0.00005, shadow_near_min, shadow_far);
		const spot_light = spot_node.GetLight();

		hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(0, -0.01, 0.8)), ground_ref, [mat_ground]);
		hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(0, 1.5, 4.6)), wall_ref, [mat_wall]);

		const blockers = [
			{ pos: new hg.Vec3(-1.4, 0.8, -2.0), mat: mat_yellow_cube },
			{ pos: new hg.Vec3(-0.65, 0.8, -0.8), mat: mat_red_cube },
			{ pos: new hg.Vec3(0.0, 0.8, 0.4), mat: mat_blue_cube },
			{ pos: new hg.Vec3(0.65, 0.8, 1.6), mat: mat_red_cube },
			{ pos: new hg.Vec3(1.4, 0.8, 2.8), mat: mat_yellow_cube },
		];

		for (const blocker of blockers) {
			hg.CreateObject(scene, hg.TranslationMat4(blocker.pos), blocker_ref, [blocker.mat]);
		}

		const rotating_cube = hg.CreateObject(scene, hg.TransformationMat4(new hg.Vec3(-2.7, 0.5, 2.3), new hg.Vec3(0, 0, 0)), cube_ref, [mat_blue_cube]);

		// main loop
		let angle = 0;

		return {
			draw(dt, res_x, res_y) {
				const dts = hg.time_to_sec_f(dt);
				angle += dts;

				const shadow_near = shadow_near_min + (shadow_near_max - shadow_near_min) * (Math.sin(angle * 0.75) * 0.5 + 0.5);
				spot_light.SetShadowNear(shadow_near);

				const rot = rotating_cube.GetTransform().GetRot();
				rot.y += dts;
				rotating_cube.GetTransform().SetRot(rot);

				scene.Update(dt);

				const [view_id] = hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);

				hg.SetView2D(view_id, 0, 0, res_x, res_y, -1, 1, hg.CF_Depth, hg.Color.Black, 1, 0);
				hg.DrawText(view_id, font, `Spot shadow near clip: ${spot_light.GetShadowNear().toFixed(2)} / far: ${spot_light.GetShadowFar().toFixed(1)}`, font_program, 'u_tex', 0, hg.Mat4.Identity, new hg.Vec3(24, res_y - 40, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, [], text_render_state);
				hg.DrawText(view_id, font, 'Near clip animates: close casters leave the shadow map.', font_program, 'u_tex', 0, hg.Mat4.Identity, new hg.Vec3(24, res_y - 76, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, [], text_render_state);
			},

			// cleanup
			dispose() {
				scene.Clear();
				hg.DestroyProgram(font_program);
				res.DestroyAllTextures();
				res.DestroyAllModels();
				res.DestroyAllPrograms();
				hg.DestroyForwardPipeline(pipeline);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
