// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/timer.h"

#include "engine/audio.h"

#include <chrono>
#include <thread>
#include <vector>

using namespace hg;

struct Audio {
	bool initialized;

	Audio() {
		start_timer();
		initialized = AudioInit();
		TEST_CHECK(initialized == true);
	}

	~Audio() {
		initialized = false;
		AudioShutdown();
		stop_timer();
	}
};

static void test_InitShutdown() {
	Audio audio;
}

static std::vector<int16_t> make_square_pcm(size_t frame_count, size_t channel_count) {
	std::vector<int16_t> samples(frame_count * channel_count);
	for (size_t frame = 0; frame < frame_count; ++frame) {
		const auto sample = ((frame / 32) % 2) ? int16_t(12000) : int16_t(-12000);
		for (size_t channel = 0; channel < channel_count; ++channel)
			samples[frame * channel_count + channel] = sample;
	}
	return samples;
}

static void test_LoadLPCM() {
	Audio audio;

	const auto mono = make_square_pcm(22050, 1);
	const auto mono_snd = LoadLPCMSound(mono.data(), mono.size() * sizeof(mono[0]), AFF_LPCM_44KHZ_S16_Mono);
	TEST_CHECK(mono_snd != InvalidSoundRef);

	auto src = PlayStereo(mono_snd, {20.f, SR_Once, 0.f});
	TEST_CHECK(src != InvalidSourceRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	StopSource(src);
	UnloadSound(mono_snd);

	const auto stereo = make_square_pcm(12000, 2);
	const auto stereo_snd = LoadLPCMSound(stereo.data(), stereo.size() * sizeof(stereo[0]), AFF_LPCM_48KHZ_S16_Stereo);
	TEST_CHECK(stereo_snd != InvalidSoundRef);
	UnloadSound(stereo_snd);
}

static void test_LoadLPCMRejectInvalid() {
	Audio audio;

	const auto mono = make_square_pcm(64, 1);
	const auto mono_size = mono.size() * sizeof(mono[0]);

	TEST_CHECK(LoadLPCMSound(nullptr, mono_size, AFF_LPCM_44KHZ_S16_Mono) == InvalidSoundRef);
	TEST_CHECK(LoadLPCMSound(mono.data(), 0, AFF_LPCM_44KHZ_S16_Mono) == InvalidSoundRef);
	TEST_CHECK(LoadLPCMSound(mono.data(), mono_size - 1, AFF_LPCM_44KHZ_S16_Mono) == InvalidSoundRef);
	TEST_CHECK(LoadLPCMSound(mono.data(), mono_size, AFF_Unsupported) == InvalidSoundRef);
}

static void test_PlayWAV() {
	Audio audio;

	const auto snd = LoadWAVSoundFile("./data/audio/sine_48S16Stereo.wav");
	TEST_CHECK(snd != InvalidSourceRef);

	auto src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	// replay it
	src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	StopSource(src);
	UnloadSound(snd);
}


static void test_StreamWAV() {
	Audio audio;

	const auto src = StreamWAVFileStereo("./data/audio/sine_48S16Stereo.wav", {20.f, SR_Once, 0.5f});
	TEST_CHECK(src != InvalidSourceRef);
	while (GetSourceState(src) == SS_Initial)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Playing);
	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Stopped);
}

static void test_Timestamps() {
	Audio audio;

	const auto src = StreamWAVFileStereo("./data/audio/sine_48S16Stereo.wav", {20.f, SR_Once, 0.5f});
	TEST_CHECK(src != InvalidSourceRef);

	time_ns duration = GetSourceDuration(src);
	TEST_CHECK(duration == hg::time_from_sec(2)); // WARNING! Don't forget to change this test if you modify the input wav file.

	hg::SourceState state;

	while ((state = GetSourceState(src)) == SS_Initial) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	TEST_CHECK(state == SS_Playing);

	hg::time_ns constexpr t_break = hg::time_from_ms(1200);
	hg::time_ns constexpr t_rewind = hg::time_from_ms(200);
	hg::time_ns t_elapsed = 0;

	while ((state == SS_Playing) && (t_elapsed < t_break)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		state = GetSourceState(src);
		t_elapsed = GetSourceTimecode(src);
	}

	// Go back to t = 200ms.
	TEST_CHECK(SetSourceTimecode(src, t_rewind));

	// Remember! This is asynchronous. It means that if we call GetSourceTimecode just after, it may not return 200ms.
	// This loop should run until the timestamp is set or the call was ignored and the stream ended. The latter being an error.
	int t_wait_ms = 50;
	while ((state == SS_Playing) && (t_elapsed > t_break)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(t_wait_ms));
		state = GetSourceState(src);
		t_elapsed = GetSourceTimecode(src);
	}

	TEST_CHECK(state == SS_Playing);
	TEST_CHECK(t_elapsed < t_break);
	// We must be closer to t_rewind than t_break.
	TEST_CHECK(Abs(t_elapsed - t_rewind) < Abs(t_elapsed - t_break));

	// Play the remaining of the audio stream.
	while (GetSourceState(src) == SS_Playing) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	TEST_CHECK(GetSourceState(src) == SS_Stopped);
}

static void test_StreamOGG() {
	Audio audio;

	const auto src = StreamOGGFileStereo("./data/audio/Dance_of_the_Sugar_Plum_Fairies_(ISRC_USUAN1100270).ogg", {20.f, SR_Once, 0.5f});
	TEST_CHECK(src != InvalidSourceRef);
	while (GetSourceState(src) == SS_Initial)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Playing);
	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Stopped);
}

static void test_PlayOGG() {
	Audio audio;
	
	const auto snd = LoadOGGSoundFile("./data/audio/Dance_of_the_Sugar_Plum_Fairies_(ISRC_USUAN1100270).ogg");
	TEST_CHECK(snd != InvalidSourceRef);

	auto src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	// replay it
	src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	StopAllSources();
	UnloadSound(snd);
}

void test_audio() { 
	test_InitShutdown();
	test_LoadLPCM();
	test_LoadLPCMRejectInvalid();
	test_PlayWAV();
	test_StreamWAV();
	test_Timestamps();
	test_StreamOGG();
	test_PlayOGG();
}
