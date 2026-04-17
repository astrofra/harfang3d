// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/audio_stream_interface.h"

#include "platform/shared_library.h"

namespace hg {

IAudioStreamer MakeAudioStreamer(const SharedLib &h);
IAudioStreamer MakeAudioStreamer(const char *module_path);

bool IsValid(IAudioStreamer &streamer);

} // namespace hg
