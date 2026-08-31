// Reading basic mouse state

local hg = require("harfang");

hg.InputInit();

while (!hg.ReadKeyboard("raw").Key(hg.K_Escape)) { // Note: the "raw" device can be queried without an open window, use "default" otherwise.
	local state = hg.ReadMouse("raw");

	local x = state.X();
	local y = state.Y();

	local button_0 = state.Button(hg.MB_0);
	local button_1 = state.Button(hg.MB_1);
	local button_2 = state.Button(hg.MB_2);

	local wheel = state.Wheel();

	print("Mouse state: X=" + x.tostring() +
		" Y=" + y.tostring() +
		" B0=" + button_0.tostring() +
		" B1=" + button_1.tostring() +
		" B2=" + button_2.tostring() +
		" Wheel=" + wheel.tostring() + "\n");
}
