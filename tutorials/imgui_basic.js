// ImGui basics

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - ImGui Basics', () => {
		// initialize ImGui
		const imgui_prg = hg.LoadProgramFromAssets('core/shader/imgui');
		const imgui_img_prg = hg.LoadProgramFromAssets('core/shader/imgui_image');

		hg.ImGuiInit(10, imgui_prg, imgui_img_prg);

		// main loop
		return {
			draw(dt, res_x, res_y) {
				hg.ImGuiBeginFrame(res_x, res_y, dt > 0n ? dt : 1n, hg.ReadMouse(), hg.ReadKeyboard());

				if (hg.ImGuiBegin('Window')) {
					hg.ImGuiText('Hello World!');
				}
				// End is required even when Begin returns false (a collapsed window).
				hg.ImGuiEnd();

				hg.SetView2D(0, 0, 0, res_x, res_y, -1, 1, hg.CF_Color | hg.CF_Depth, hg.Color.Black, 1, 0);
				hg.ImGuiEndFrame(0);
			},

			// cleanup
			dispose() {
				// ImGui owns both programs and destroys them with its font texture.
				hg.ImGuiShutdown();
			},
		};
	}, { resetFlags: hg.RF_VSync, resizeToWindow: true, ...options });
}
