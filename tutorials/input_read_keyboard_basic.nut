// Reading basic keyboard state

local hg = require("harfang");

hg.InputInit();

while (true) {
	local state = hg.ReadKeyboard("raw"); // Note: the "raw" device can be queried without an open window, use "default" otherwise.

	for (local key = 0; key < hg.K_Last; ++key) {
		if (state.Key(key)) {
			print("Key down: " + key.tostring() + "\n");
		}
	}

	if (state.Key(hg.K_Escape)) {
		break;
	}
}
