// Speak a sentence with lib-say and HARFANG's raw LPCM bridge.

local hg = require("harfang");

function main() {
	local say = null;

	try {
		say = require("say");
	} catch (e) {
		print("Missing Squirrel native module say.dll exporting sqmodule_say(HSQUIRRELVM).\n");
		print("audio_play_tts_say.nut depends on the planned Squirrel port of tutorials/hg_lua/say.dll.\n");
		return;
	}

	hg.InputInit();
	hg.AudioInit();

	local phrase = "This is Harfang speaking!";
	local synth_result = say.synthesize(phrase, {
		lang = "en",
		format = "raw"
	});

	if (synth_result == null || synth_result.len() == 0 || synth_result[0] == null) {
		print("say.synthesize() did not return a PCM blob.\n");
		hg.AudioShutdown();
		hg.InputShutdown();
		return;
	}

	local blob = synth_result[0];
	local snd_ref = hg.LoadLPCMSound(blob.GetData(), blob.GetSize(), hg.AFF_LPCM_44KHZ_S16_Mono);
	local src_ref = hg.PlayStereo(snd_ref, hg.StereoSourceState(1, hg.SR_Once));

	print("Speaking: \"" + phrase + "\"\n");
	print("Press Escape to stop playback and exit.\n");

	while (hg.GetSourceState(src_ref) != hg.SS_Stopped && !hg.ReadKeyboard("raw").Key(hg.K_Escape)) {
		hg.TickClock();
		hg.Sleep(hg.time_from_ms(10));
	}

	hg.StopSource(src_ref);
	hg.UnloadSound(snd_ref);

	hg.AudioShutdown();
	hg.InputShutdown();
}

main();
