// ImGui basics

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - ImGui Edit', () => {
		// initialize ImGui
		const imgui_prg = hg.LoadProgramFromAssets('core/shader/imgui');
		const imgui_img_prg = hg.LoadProgramFromAssets('core/shader/imgui_image');

		hg.ImGuiInit(10, imgui_prg, imgui_img_prg);

		let imgui_output_view = 255;
		let imgui_view_clear_color = new hg.Color(0, 0, 0);
		let imgui_clear_color_preset = 0;

		// main loop
		return {
			draw(dt, res_x, res_y) {
				// ImGui frame
				hg.ImGuiBeginFrame(res_x, res_y, dt > 0n ? dt : 1n, hg.ReadMouse(), hg.ReadKeyboard());

				hg.ImGuiSetNextWindowPosCenter(hg.ImGuiCond_Once);

				// The overload with an open flag returns [visible, open].
				const [visible] = hg.ImGuiBegin('ImGui Controls', true, hg.ImGuiWindowFlags_AlwaysAutoResize);
				if (visible) {
					let val_modified;
					[val_modified, imgui_clear_color_preset] = hg.ImGuiCombo('Set Clear Color', imgui_clear_color_preset, ['Red', 'Green', 'Blue']);

					// apply preset if a combo entry was selected
					if (val_modified) {
						if (imgui_clear_color_preset === 0) {
							imgui_view_clear_color = new hg.Color(1, 0, 0);
						} else if (imgui_clear_color_preset === 1) {
							imgui_view_clear_color = new hg.Color(0, 1, 0);
						} else {
							imgui_view_clear_color = new hg.Color(0, 0, 1);
						}
					}

					// reset clear color to black on button click
					if (hg.ImGuiButton('Reset Clear Color')) {
						imgui_view_clear_color = hg.Color.Black;
					}
					// custom clear color edit
					[val_modified, imgui_view_clear_color] = hg.ImGuiColorEdit('Edit Clear Color', imgui_view_clear_color);

					// edit the ImGui output view
					[val_modified, imgui_output_view] = hg.ImGuiInputInt('ImGui Output View', imgui_output_view);
					if (val_modified) {
						imgui_output_view = Math.max(0, Math.min(imgui_output_view, 255)); // keep output view in [0;255] range
					}
				}
				hg.ImGuiEnd();

				hg.SetView2D(imgui_output_view, 0, 0, res_x, res_y, -1, 0, hg.CF_Color | hg.CF_Depth, imgui_view_clear_color, 1, 0);
				hg.ImGuiEndFrame(imgui_output_view);
			},

			// cleanup
			dispose() {
				hg.ImGuiShutdown(); // ImGui owns both programs and its font texture
			},
		};
	}, { resetFlags: hg.RF_VSync, resizeToWindow: true, ...options });
}
