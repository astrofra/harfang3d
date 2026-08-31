// Reading advanced keyboard state

local hg = require("harfang");

hg.InputInit();

// Note: the "raw" device can be queried without an open window, use "default" otherwise.
local keyboard = hg.Keyboard("raw");

while (!keyboard.Pressed(hg.K_Escape)) {
	keyboard.Update();

	for (local key = 0; key < hg.K_Last; ++key) {
		if (keyboard.Released(key)) {
			print("Key released: " + key.tostring() + "\n");
		}
	}
}
