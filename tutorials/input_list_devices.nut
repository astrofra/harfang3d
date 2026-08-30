// List input devices

local hg = require("harfang");

function join_names(names) {
	local out = "";
	local first = true;
	foreach (name in names) {
		if (!first) {
			out += ",";
		}
		out += name;
		first = false;
	}
	return out;
}

hg.InputInit();

local mouse_names = hg.GetMouseNames();
print("Mouse device names: " + join_names(mouse_names) + "\n");

local keyboard_names = hg.GetKeyboardNames();
print("Keyboard device names: " + join_names(keyboard_names) + "\n");

local gamepad_names = hg.GetGamepadNames();
print("Gamepad device names: " + join_names(gamepad_names) + "\n");
