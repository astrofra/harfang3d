// Reading advanced gamepad state

local hg = require("harfang");

function absf(value) {
	return value < 0 ? -value : value;
}

hg.InputInit();

hg.WindowSystemInit();
local win = hg.NewWindow("Harfang - Read Gamepad", 320, 200);

local gamepad = hg.Gamepad();

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	gamepad.Update();

	if (gamepad.Connected()) {
		print("Gamepad slot 0 was just connected\n");
	}
	if (gamepad.Disconnected()) {
		print("Gamepad slot 0 was just disconnected\n");
	}
	if (gamepad.Pressed(hg.GB_ButtonA)) {
		print("Gamepad button A pressed\n");
	}
	if (gamepad.Pressed(hg.GB_ButtonB)) {
		print("Gamepad button B pressed\n");
	}
	if (gamepad.Pressed(hg.GB_ButtonX)) {
		print("Gamepad button X pressed\n");
	}
	if (gamepad.Pressed(hg.GB_ButtonY)) {
		print("Gamepad button Y pressed\n");
	}

	local axis_left_x = gamepad.Axes(hg.GA_LeftX);
	if (absf(axis_left_x) > 0.1) {
		print("Gamepad axis left X: " + axis_left_x.tostring() + "\n");
	}

	local axis_left_y = gamepad.Axes(hg.GA_LeftY);
	if (absf(axis_left_y) > 0.1) {
		print("Gamepad axis left Y: " + axis_left_y.tostring() + "\n");
	}

	hg.UpdateWindow(win);
}

hg.DestroyWindow(win);
