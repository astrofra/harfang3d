// Draw text

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - Draw Text', () => {
		// load font and shader program
		const font = hg.LoadFontFromAssets('font/default.ttf', 96);
		const font_prg = hg.LoadProgramFromAssets('core/shader/font');

		// text uniforms and render state
		const text_uniform_values = [hg.MakeUniformSetValue('u_color', new hg.Vec4(1, 1, 0))];
		const text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled);

		// main loop
		return {
			draw(dt, res_x, res_y) {
				hg.SetView2D(0, 0, 0, res_x, res_y, -1, 1, hg.CF_Color | hg.CF_Depth, hg.ColorI(32, 32, 32), 0, 1);

				hg.DrawText(0, font, 'Hello world!', font_prg, 'u_tex', 0, hg.Mat4.Identity, new hg.Vec3(res_x / 2, res_y / 2, 0), hg.DTHA_Center, hg.DTVA_Center, text_uniform_values, [], text_render_state);
			},

			// cleanup
			dispose() {
				hg.DestroyProgram(font_prg);
				// Font atlas textures are released by RenderShutdown (no DestroyFont API).
			},
		};
	}, { resetFlags: hg.RF_VSync, ...options });
}
