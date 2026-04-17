// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "audio_stream.h"

#include <string.h>

#include <map>

#include "foundation/format.h"
#include "foundation/log.h"

namespace hg {

enum AudioStreamBaseFunction {
	AS_BF_Startup = 0,
	AS_BF_Shutdown,
	AS_BF_Open,
	AS_BF_Close,
	AS_BF_Seek,
	AS_BF_GetDuration,
	AS_BF_GetTimeStamp,
	AS_BF_IsEnded,
	AS_BF_GetFrame,
	AS_BF_Count
};

IAudioStreamer MakeAudioStreamer(const SharedLib &h) {
	static const char *base_function_names[AS_BF_Count] = {
		"Startup",
		"Shutdown",
		"Open",
		"Close",
		"Seek",
		"GetDuration",
		"GetTimeStamp",
		"IsEnded",
		"GetFrame",
	};
	void *base_function_handles[AS_BF_Count];

	IAudioStreamer streamer;
	memset(&streamer, 0, sizeof(IAudioStreamer));

	for (int i = 0; i < AS_BF_Count; ++i) {
		base_function_handles[i] = GetFunctionPointer(h, base_function_names[i]);
		if (!base_function_handles[i]) {
			warn(format("failed to load %1 audio stream function").arg(base_function_names[i]));
			return streamer;
		}
	}

	streamer.Startup = (int (*)())base_function_handles[AS_BF_Startup];
	streamer.Shutdown = (void (*)())base_function_handles[AS_BF_Shutdown];
	streamer.Open = (AudioStreamRef(*)(const char *))base_function_handles[AS_BF_Open];
	streamer.Close = (int (*)(AudioStreamRef))base_function_handles[AS_BF_Close];
	streamer.Seek = (int (*)(AudioStreamRef, AudioTimestamp))base_function_handles[AS_BF_Seek];
	streamer.GetDuration = (AudioTimestamp(*)(AudioStreamRef))base_function_handles[AS_BF_GetDuration];
	streamer.GetTimeStamp = (AudioTimestamp(*)(AudioStreamRef))base_function_handles[AS_BF_GetTimeStamp];
	streamer.IsEnded = (int (*)(AudioStreamRef))base_function_handles[AS_BF_IsEnded];
	streamer.GetFrame = (int (*)(AudioStreamRef, uintptr_t *, int *, AudioFrameFormat *))base_function_handles[AS_BF_GetFrame];

	return streamer;
}

static std::map<std::string, SharedLib> g_audio_module_cache;

IAudioStreamer MakeAudioStreamer(const char *module_path) {
	auto it = g_audio_module_cache.find(module_path);
	SharedLib module;
	if (it == g_audio_module_cache.end()) {
		module = LoadSharedLibrary(module_path);
		g_audio_module_cache[module_path] = module;
	} else {
		module = it->second;
	}

	if (!module) {
		IAudioStreamer empty;
		memset(&empty, 0, sizeof(IAudioStreamer));
		return empty;
	}

	return MakeAudioStreamer(module);
}

bool IsValid(IAudioStreamer &streamer) {
	return (streamer.Startup && streamer.Shutdown && streamer.Open && streamer.Close && streamer.Seek && streamer.GetDuration && streamer.GetTimeStamp &&
			streamer.IsEnded && streamer.GetFrame);
}

} // namespace hg
