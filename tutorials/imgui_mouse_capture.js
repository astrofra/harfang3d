// Detect ImGui mouse capture

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - ImGui Mouse Capture', () => {
		//
		hg.ImGuiInit(10, hg.LoadProgramFromAssets('core/shader/imgui'), hg.LoadProgramFromAssets('core/shader/imgui_image'));
		let text_value = 'Clicking into this field will not clear the screen in red.';

		//
		const mouse = new hg.Mouse();
		const keyboard = new hg.Keyboard();

		return {
			draw(dt, res_x, res_y) {
				mouse.Update();
				keyboard.Update();

				hg.ImGuiBeginFrame(res_x, res_y, dt > 0n ? dt : 1n, mouse.GetState(), keyboard.GetState());

				let clear_color;
				if (hg.ImGuiWantCaptureMouse()) {
					clear_color = hg.Color.Black; // black background if ImGui has mouse capture
				} else {
					if (mouse.Down(hg.MB_0)) {
						clear_color = hg.Color.Red;
					} else {
						clear_color = hg.Color.Black;
					}
				}

				hg.SetView2D(0, 0, 0, res_x, res_y, -1, 0, hg.CF_Color | hg.CF_Depth, clear_color, 1, 0);

				hg.ImGuiSetNextWindowPosCenter(hg.ImGuiCond_Once);
				hg.ImGuiSetNextWindowSize(new hg.Vec2(700, 96), hg.ImGuiCond_Once);

				if (hg.ImGuiBegin('Detecting ImGui mouse capture')) {
					hg.ImGuiTextWrapped('Click outside of the GUI to clear the screen in red.');
					hg.ImGuiSeparator();
					[, text_value] = hg.ImGuiInputText('Text Input', text_value, 4096);
				}
				hg.ImGuiEnd();

				hg.ImGuiEndFrame(0);
			},

			// cleanup
			dispose() {
				hg.ImGuiShutdown();
			},
		};
	}, { resetFlags: hg.RF_VSync, resizeToWindow: true, ...options });
}
