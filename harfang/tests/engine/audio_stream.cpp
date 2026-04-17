// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/audio_stream.h"
#include "foundation/file.h"

using namespace hg;

#if WIN32
static const char *module_name = "DummyAudioStream.dll";
#elif __APPLE__
static const char *module_name = "DummyAudioStream.dylib";
#else
static const char *module_name = "DummyAudioStream.so";
#endif

static std::string GetModulePath() {
	const std::string candidates[] = {
		std::string("./data/") + module_name,
		std::string("./data/RelWithDebInfo/") + module_name,
		std::string("./data/Debug/") + module_name,
		std::string("./data/Release/") + module_name,
	};

	for (const auto &candidate : candidates)
		if (IsFile(candidate.c_str()))
			return candidate;

	return candidates[0];
}

static void test_LoadModuleAndMakeAudioStreamer() {
	const auto module_path = GetModulePath();
	SharedLib module = LoadSharedLibrary(module_path.c_str());
	IAudioStreamer streamer = MakeAudioStreamer(module);
	TEST_CHECK(IsValid(streamer) == true);

	TEST_CHECK(streamer.Startup() == 1);
	streamer.Shutdown();
}

static void test_MakeAudioStreamer() {
	const auto module_path = GetModulePath();
	IAudioStreamer streamer = MakeAudioStreamer(module_path.c_str());
	TEST_CHECK(IsValid(streamer) == true);

	TEST_CHECK(streamer.Startup() == 1);

	AudioStreamRef ref = streamer.Open("dummy.mod");
	TEST_CHECK(ref != InvalidAudioStreamRef);

	TEST_CHECK(streamer.GetDuration(ref) == 1000000000LL);
	TEST_CHECK(streamer.Seek(ref, 500000000LL) == 1);
	TEST_CHECK(streamer.GetTimeStamp(ref) == 500000000LL);

	uintptr_t data;
	int size;
	AudioFrameFormat format;
	TEST_CHECK(streamer.GetFrame(ref, &data, &size, &format) == 1);
	TEST_CHECK(data != 0);
	TEST_CHECK(size == 1920);
	TEST_CHECK(format == AFF_LPCM_48KHZ_S16_Stereo);
	TEST_CHECK(streamer.IsEnded(ref) == 1);

	TEST_CHECK(streamer.Close(ref) == 1);
	streamer.Shutdown();
}

void test_audio_stream() {
	test_LoadModuleAndMakeAudioStreamer();
	test_MakeAudioStreamer();
}
