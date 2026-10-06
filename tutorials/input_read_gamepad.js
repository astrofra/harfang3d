// Reading advanced gamepad state

import * as hg from 'harfang';
import { nextFrame } from 'harfang-host';

export async function main({ hidden = false, frameLimit = Infinity } = {}) {
	hg.InputInit();
	hg.WindowSystemInit();
	let win;

	try {
		win = hg.NewWindow('Harfang - Read Gamepad', 320, 200, 32, hidden ? hg.WV_Hidden : hg.WV_Windowed);

		const gamepad = new hg.Gamepad();

		for (let frame = 0; frame < frameLimit; frame++) {
			const { closed } = await nextFrame(win);
			if (closed || hg.ReadKeyboard().Key(hg.K_Escape)) break;
			gamepad.Update();

			if (gamepad.Connected()) {
				console.log('Gamepad slot 0 was just connected');
			}
			if (gamepad.Disconnected()) {
				console.log('Gamepad slot 0 was just disconnected');
			}
			if (gamepad.Pressed(hg.GB_ButtonA)) {
				console.log('Gamepad button A pressed');
			}
			if (gamepad.Pressed(hg.GB_ButtonB)) {
				console.log('Gamepad button B pressed');
			}
			if (gamepad.Pressed(hg.GB_ButtonX)) {
				console.log('Gamepad button X pressed');
			}
			if (gamepad.Pressed(hg.GB_ButtonY)) {
				console.log('Gamepad button Y pressed');
			}

			const axis_left_x = gamepad.Axes(hg.GA_LeftX);
			if (Math.abs(axis_left_x) > 0.1) {
				console.log(`Gamepad axis left X: ${axis_left_x.toFixed(6)}`);
			}
			const axis_left_y = gamepad.Axes(hg.GA_LeftY);
			if (Math.abs(axis_left_y) > 0.1) {
				console.log(`Gamepad axis left Y: ${axis_left_y.toFixed(6)}`);
			}
		}
	} finally {
		if (win) hg.DestroyWindow(win);
		hg.WindowSystemShutdown();
		hg.InputShutdown();
	}
}
