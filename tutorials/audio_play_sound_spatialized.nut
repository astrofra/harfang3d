// Spatialized sound

local hg = require("harfang");

hg.InputInit();
hg.AudioInit();
hg.AddAssetsFolder("resources_compiled");

local snd_ref = hg.LoadWAVSoundAsset("sounds/metro_announce.wav");
local src_ref = hg.PlaySpatialized(snd_ref, hg.SpatializedSourceState(hg.Mat4.get_Identity(), 1, hg.SR_Loop));

local angle = 0.0;

while (!hg.ReadKeyboard("raw").Key(hg.K_Escape)) {
	local dt = hg.TickClock();
	local dt_sec_f = hg.time_to_sec_f(dt);

	local src_old_pos = hg.Vec3(sin(angle), 0, cos(angle)) * 5;
	angle += dt_sec_f * 45;
	local src_new_pos = hg.Vec3(sin(angle), 0, cos(angle)) * 5;

	local src_vel = hg.Vec3(0, 0, 0);
	if (dt_sec_f > 0) {
		src_vel = (src_new_pos - src_old_pos) / dt_sec_f;
	}

	hg.SetSourceTransform(src_ref, hg.TranslationMat4(src_new_pos), src_vel);
}

hg.StopSource(src_ref);
hg.AudioShutdown();
hg.InputShutdown();
