// How to resize the render window.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options = {}) {
	let res_x = options.width ?? 512, res_y = options.height ?? 512;
	return runWindow('Harfang - Render Resize to Window', win => {
		// create model
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
		const cube_prg = hg.LoadProgramFromAssets('shaders/mdl');

		// Own the model's GPU buffers.
		const res = new hg.PipelineResources();
		res.AddModel('cube', cube_mdl);

		// main loop
		return {
			draw() {
				let render_was_reset;
				[render_was_reset, res_x, res_y] = hg.RenderResetToWindow(win, res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X | hg.RF_MaxAnisotropy);
				if (render_was_reset) {
					console.log(`Render reset to ${res_x}x${res_y}`);
				}

				const viewpoint = hg.TransformationMat4(new hg.Vec3(1, 1, -2), hg.Deg3(24, -27, 0));
				hg.SetViewPerspective(0, 0, 0, res_x, res_y, viewpoint, 0.01, 100, 1.8, hg.CF_Color | hg.CF_Depth, hg.ColorI(64, 64, 64), 1, 0);

				hg.DrawModel(0, cube_mdl, cube_prg, [], [], hg.TranslationMat4(new hg.Vec3(0, 0, 0)));
			},

			// cleanup
			dispose() {
				res.DestroyAllModels();
				hg.DestroyProgram(cube_prg);
			},
		};
	}, { width: 512, height: 512, resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
