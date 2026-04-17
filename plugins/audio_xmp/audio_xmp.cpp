// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/audio_stream_interface.h"

#include <xmp.h>

#include <string.h>

#include <vector>

namespace {

static const int XMPAudioSampleRate = 48000;

struct XMPAudioStream {
	xmp_context context{};
	AudioTimestamp timestamp{};
	AudioTimestamp duration{};
	bool ended{false};
};

static std::vector<XMPAudioStream> g_streams;
static bool g_started{false};

static AudioTimestamp MillisecondsToTimestamp(int ms) { return AudioTimestamp(ms) * 1000000LL; }

static bool IsValidStream(AudioStreamRef ref) { return ref < g_streams.size() && g_streams[ref].context; }

static AudioStreamRef AllocateStream() {
	for (AudioStreamRef ref = 0; ref < g_streams.size(); ++ref) {
		if (!g_streams[ref].context)
			return ref;
	}

	g_streams.resize(g_streams.size() + 8);
	return AudioStreamRef(g_streams.size() - 8);
}

static void ReleaseStream(AudioStreamRef ref) {
	if (!IsValidStream(ref))
		return;

	auto &stream = g_streams[ref];
	xmp_end_player(stream.context);
	xmp_release_module(stream.context);
	xmp_free_context(stream.context);
	stream = {};
}

} // namespace

AUDIO_STREAM_API int Startup() {
	g_started = true;
	return 1;
}

AUDIO_STREAM_API void Shutdown() {
	for (AudioStreamRef ref = 0; ref < g_streams.size(); ++ref)
		ReleaseStream(ref);
	g_streams.clear();
	g_started = false;
}

AUDIO_STREAM_API AudioStreamRef Open(const char *name) {
	if (!g_started || !name)
		return InvalidAudioStreamRef;

	xmp_context context = xmp_create_context();
	if (!context)
		return InvalidAudioStreamRef;

	if (xmp_load_module(context, name) != 0) {
		xmp_free_context(context);
		return InvalidAudioStreamRef;
	}

	xmp_scan_module(context);

	if (xmp_start_player(context, XMPAudioSampleRate, 0) != 0) {
		xmp_release_module(context);
		xmp_free_context(context);
		return InvalidAudioStreamRef;
	}

	struct xmp_frame_info info;
	memset(&info, 0, sizeof(info));
	xmp_get_frame_info(context, &info);

	const auto ref = AllocateStream();
	auto &stream = g_streams[ref];
	stream.context = context;
	stream.timestamp = MillisecondsToTimestamp(info.time);
	stream.duration = MillisecondsToTimestamp(info.total_time);
	stream.ended = false;

	return ref;
}

AUDIO_STREAM_API int Close(AudioStreamRef ref) {
	if (!IsValidStream(ref))
		return 0;

	ReleaseStream(ref);
	return 1;
}

AUDIO_STREAM_API int Seek(AudioStreamRef ref, AudioTimestamp t) {
	if (!IsValidStream(ref))
		return 0;

	const int ms = int(t / 1000000LL);
	if (xmp_seek_time(g_streams[ref].context, ms) != 0)
		return 0;

	struct xmp_frame_info info;
	memset(&info, 0, sizeof(info));
	xmp_get_frame_info(g_streams[ref].context, &info);

	g_streams[ref].timestamp = MillisecondsToTimestamp(info.time);
	g_streams[ref].ended = false;
	return 1;
}

AUDIO_STREAM_API AudioTimestamp GetDuration(AudioStreamRef ref) {
	if (!IsValidStream(ref))
		return 0;

	return g_streams[ref].duration;
}

AUDIO_STREAM_API AudioTimestamp GetTimeStamp(AudioStreamRef ref) {
	if (!IsValidStream(ref))
		return 0;

	return g_streams[ref].timestamp;
}

AUDIO_STREAM_API int IsEnded(AudioStreamRef ref) {
	if (!IsValidStream(ref))
		return 1;

	return g_streams[ref].ended ? 1 : 0;
}

AUDIO_STREAM_API int GetFrame(AudioStreamRef ref, uintptr_t *data, int *size, AudioFrameFormat *format) {
	if (!IsValidStream(ref) || !data || !size || !format)
		return 0;

	auto &stream = g_streams[ref];
	if (stream.ended)
		return 0;

	const int result = xmp_play_frame(stream.context);
	if (result != 0) {
		stream.ended = true;
		return 0;
	}

	struct xmp_frame_info info;
	memset(&info, 0, sizeof(info));
	xmp_get_frame_info(stream.context, &info);

	if (info.loop_count > 0) {
		stream.ended = true;
		return 0;
	}

	if (!info.buffer || info.buffer_size <= 0)
		return 0;

	stream.timestamp = MillisecondsToTimestamp(info.time);
	if (info.total_time > 0)
		stream.duration = MillisecondsToTimestamp(info.total_time);

	*data = reinterpret_cast<uintptr_t>(info.buffer);
	*size = info.buffer_size;
	*format = AFF_LPCM_48KHZ_S16_Stereo;
	return 1;
}
