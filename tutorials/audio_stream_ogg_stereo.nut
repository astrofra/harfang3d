// Stream a mono OGG file with stereo panning

local hg = require("harfang");

hg.InputInit();
hg.AudioInit();

local src_ref = hg.StreamOGGFileStereo("resources_compiled/sounds/metro_announce.ogg", hg.StereoSourceState(1, hg.SR_Loop));
local angle = 0.0;

while (!hg.ReadKeyboard("raw").Key(hg.K_Escape)) {
	angle += hg.time_to_sec_f(hg.TickClock()) * 0.5;
	hg.SetSourcePanning(src_ref, sin(angle));
}

hg.StopSource(src_ref);
hg.AudioShutdown();
hg.InputShutdown();
