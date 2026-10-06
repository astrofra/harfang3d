// List input devices

import * as hg from 'harfang';

// Native string lists use size()/at(); join the names as in Lua's table.concat.
function device_names_to_string(names) {
	const values = [];
	for (let i = 0; i < names.size(); i++) {
		values.push(names.at(i));
	}
	return values.join(',');
}

export function main() {
	hg.InputInit();

	try {
		let names = hg.GetMouseNames();
		console.log('Mouse device names: ' + device_names_to_string(names));

		names = hg.GetKeyboardNames();
		console.log('Keyboard device names: ' + device_names_to_string(names));

		names = hg.GetGamepadNames();
		console.log('Gamepad device names: ' + device_names_to_string(names));
	} finally {
		hg.InputShutdown();
	}
}
