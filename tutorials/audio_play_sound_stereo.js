// Play a mono sound with stereo panning

import * as hg from 'harfang';
import { nextFrame } from 'harfang-host';

export async function main({ frameLimit = Infinity, volume = 1 } = {}) {
	hg.InputInit();
	let audio_initialized = false;
	let snd_ref = hg.SND_Invalid, src_ref = hg.SRC_Invalid;

	try {
		audio_initialized = hg.AudioInit();
		if (!audio_initialized) throw Error('Failed to initialize audio');

		snd_ref = hg.LoadWAVSoundFile('resources_compiled/sounds/metro_announce.wav'); // WAV 44.1kHz 16bit mono
		if (snd_ref === hg.SND_Invalid) throw Error('Failed to load sounds/metro_announce.wav');
		src_ref = hg.PlayStereo(snd_ref, new hg.StereoSourceState(volume, hg.SR_Loop));
		if (src_ref === hg.SRC_Invalid) throw Error('Failed to play stereo sound');

		let angle = 0;

		for (let frame = 0; frame < frameLimit; frame++) {
			const { dtNs } = await nextFrame();
			if (hg.ReadKeyboard('raw').Key(hg.K_Escape)) break;

			angle += hg.time_to_sec_f(dtNs) * 0.5;
			hg.SetSourcePanning(src_ref, Math.sin(angle)); // panning left = -1, panning right = 1
			if (hg.GetSourceState(src_ref) !== hg.SS_Playing) throw Error('Stereo source stopped unexpectedly');
		}
	} finally {
		if (src_ref !== hg.SRC_Invalid) hg.StopSource(src_ref);
		if (snd_ref !== hg.SND_Invalid) hg.UnloadSound(snd_ref);
		if (audio_initialized) hg.AudioShutdown();
		hg.InputShutdown();
	}
}
