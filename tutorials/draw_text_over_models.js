// Draw models without a pipeline

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - Draw Text over Models', () => {
		// vertex layout and models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
		const ground_mdl = hg.CreatePlaneModel(vtx_layout, 5, 5, 1, 1);

		const shader = hg.LoadProgramFromAssets('shaders/mdl');

		// load font and shader program
		const font = hg.LoadFontFromAssets('font/default.ttf', 96);
		const font_prg = hg.LoadProgramFromAssets('core/shader/font');

		// text uniforms and render state
		const text_uniform_values = [hg.MakeUniformSetValue('u_color', new hg.Vec4(1, 1, 0))];
		const text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled);

		// Own the models' GPU buffers.
		const res = new hg.PipelineResources();
		res.AddModel('cube', cube_mdl);
		res.AddModel('ground', ground_mdl);

		// main loop
		let angle = 0;

		return {
			draw(dt, res_x, res_y) {
				angle += hg.time_to_sec_f(dt);

				// 3D view
				const viewpoint = hg.TranslationMat4(new hg.Vec3(0, 1, -3));
				hg.SetViewPerspective(0, 0, 0, res_x, res_y, viewpoint, 0.01, 5000);

				hg.DrawModel(0, cube_mdl, shader, [], [], hg.TransformationMat4(new hg.Vec3(0, 1, 0), new hg.Vec3(angle, angle, angle)));
				hg.DrawModel(0, ground_mdl, shader, [], [], hg.TranslationMat4(new hg.Vec3(0, 0, 0)));

				// 2D view, note that only the depth buffer is cleared
				hg.SetView2D(1, 0, 0, res_x, res_y, -1, 1, hg.CF_Depth, hg.ColorI(32, 32, 32), 1, 0);

				hg.DrawText(1, font, 'Hello world!', font_prg, 'u_tex', 0, hg.Mat4.Identity, new hg.Vec3(res_x / 2, res_y / 2, 0), hg.DTHA_Center, hg.DTVA_Center, text_uniform_values, [], text_render_state);
			},

			// cleanup
			dispose() {
				res.DestroyAllModels();
				hg.DestroyProgram(shader);
				hg.DestroyProgram(font_prg);
				// Font atlas textures are released by RenderShutdown (no DestroyFont API).
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
