// Reading advanced mouse state

local hg = require("harfang");

hg.InputInit();

local mouse = hg.Mouse("raw");

while (!hg.ReadKeyboard("raw").Key(hg.K_Escape)) { // Note: the "raw" device can be queried without an open window, use "default" otherwise.
	mouse.Update();

	local dt_x = mouse.DtX();
	local dt_y = mouse.DtY();

	if (dt_x != 0 || dt_y != 0) {
		print("Mouse delta X=" + dt_x.tostring() + " delta Y=" + dt_y.tostring() + "\n");
	}

	for (local i = 0; i < 3; ++i) {
		if (mouse.Pressed(hg.MB_0 + i)) {
			print("Mouse button " + i.tostring() + " pressed\n");
		}
		if (mouse.Released(hg.MB_0 + i)) {
			print("Mouse button " + i.tostring() + " released\n");
		}
	}
}
