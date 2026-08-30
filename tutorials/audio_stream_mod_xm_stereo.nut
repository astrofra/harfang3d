// Stream a MOD/XM/S3M/IT tracker module with the XMP audio plugin.

local hg = require("harfang");

function main() {
	local module_path = "resources/sounds/4-mat's madness.mod";

	if (!hg.Exists(module_path)) {
		print("Missing module file: " + module_path + "\n");
		return;
	}

	hg.InputInit();
	hg.AudioInit();

	local src_ref = hg.StreamModuleFileStereo(module_path, hg.StereoSourceState(1, hg.SR_Loop));
	if (src_ref == hg.SRC_Invalid) {
		print("Failed to stream " + module_path + "\n");
		hg.AudioShutdown();
		hg.InputShutdown();
		return;
	}

	local angle = 0.0;
	while (!hg.ReadKeyboard("raw").Key(hg.K_Escape)) {
		angle += hg.time_to_sec_f(hg.TickClock()) * 0.5;
		hg.SetSourcePanning(src_ref, sin(angle));
	}

	hg.StopSource(src_ref);
	hg.AudioShutdown();
	hg.InputShutdown();
}

main();
