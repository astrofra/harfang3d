#include "engine/audio_stream_interface.h"

#include <stdint.h>

static int16_t g_buffer[960];
static AudioTimestamp g_timestamp = 0;
static bool g_open = false;
static bool g_frame_sent = false;

AUDIO_STREAM_API int Startup() {
	for (int i = 0; i < 960; ++i)
		g_buffer[i] = int16_t(i);
	return 1;
}

AUDIO_STREAM_API void Shutdown() { g_open = false; }

AUDIO_STREAM_API AudioStreamRef Open(const char *name) {
	g_open = name != nullptr;
	g_frame_sent = false;
	g_timestamp = 0;
	return g_open ? 1 : InvalidAudioStreamRef;
}

AUDIO_STREAM_API int Close(AudioStreamRef ref) {
	g_open = false;
	return ref == 1 ? 1 : 0;
}

AUDIO_STREAM_API int Seek(AudioStreamRef ref, AudioTimestamp t) {
	if (!g_open || ref != 1)
		return 0;
	g_timestamp = t;
	g_frame_sent = false;
	return 1;
}

AUDIO_STREAM_API AudioTimestamp GetDuration(AudioStreamRef ref) { return ref == 1 ? 1000000000LL : 0; }

AUDIO_STREAM_API AudioTimestamp GetTimeStamp(AudioStreamRef ref) { return ref == 1 ? g_timestamp : 0; }

AUDIO_STREAM_API int IsEnded(AudioStreamRef ref) { return ref != 1 || g_frame_sent ? 1 : 0; }

AUDIO_STREAM_API int GetFrame(AudioStreamRef ref, uintptr_t *data, int *size, AudioFrameFormat *format) {
	if (!g_open || ref != 1 || !data || !size || !format || g_frame_sent)
		return 0;

	*data = uintptr_t(g_buffer);
	*size = sizeof(g_buffer);
	*format = AFF_LPCM_48KHZ_S16_Stereo;
	g_timestamp = ByteToTimestamp(*format, *size);
	g_frame_sent = true;
	return 1;
}
