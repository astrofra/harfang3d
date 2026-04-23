# Raw PCM To SoundRef Bridge Spec

Date: 2026-04-23

This document replaces the earlier recommendation to add a CPU-side `hg::Sound`
class with `SetData()` / `GetData()` as the first step.

For the `lib-say` integration use case, a lower-impact solution exists and is
preferred for the MVP.

## Executive Summary

HARFANG does not need a new `Sound` class to consume the raw PCM buffer
returned by `lib-say`.

The least invasive solution is to add one public C++ function to the existing
audio API:

```cpp
namespace hg {

SoundRef LoadLPCMSound(const void *data, size_t size, AudioFrameFormat format);

} // namespace hg
```

That function would:

- accept a foreign raw PCM pointer
- validate it against an existing `AudioFrameFormat`
- upload the PCM data into the current OpenAL-backed `SoundRef` model
- return a regular `SoundRef`

This keeps the change set small:

- no new `hg::Sound` type
- no new ownership model similar to `Picture`
- no change to `SoundRef`
- no change to playback functions such as `PlayStereo()` or `PlaySpatialized()`

For the current `lib-say` output, this bridge is sufficient.

## Why This Is Lower Impact

The current static audio path already knows how to turn decoded PCM into
OpenAL buffers. In `harfang/engine/audio.cpp`, file-based sound loading:

- decodes PCM through `IAudioStreamer`
- creates OpenAL buffers
- stores them behind a `SoundRef`

The requested `lib-say` use case already provides decoded PCM directly, so the
missing piece is only a public upload function that skips the file and decoder
stages.

Compared to introducing a new `hg::Sound` class, this bridge avoids:

- adding new engine data structures
- adding copy/move/ownership semantics for audio buffers
- adding a second audio object model to document and bind
- expanding the C++ and FabGen surface beyond one focused function

## Current State

### Existing Audio API

`harfang/engine/audio.h` currently exposes:

- `SoundRef`
- `LoadWAVSoundFile`, `LoadWAVSoundAsset`
- `LoadOGGSoundFile`, `LoadOGGSoundAsset`
- `PlayStereo`, `PlaySpatialized`
- `UnloadSound`

Internally, `SoundRef` is a runtime resource handle backed by OpenAL buffers.
It is not a CPU-side PCM container.

### Existing Script Interop

HARFANG already exposes pointer-based interop patterns elsewhere:

- `Picture.SetData(...)`
- `Picture.GetData()`
- `int_to_VoidPointer(ptr)`

So the binding layer already has the primitives required to pass a foreign
memory address into HARFANG.

### lib-say Contract

The Lua extension in `C:\works\projects\lib-say` returns:

- `blob:GetData()` -> raw pointer as integer
- `blob:GetSize()` -> byte size
- `info.sample_rate`
- `info.channels`
- `info.bits_per_sample`
- `info.pcm_encoding`

For `format = "raw"`, `lib-say` currently produces:

- 44100 Hz
- mono
- signed 16-bit PCM
- little-endian (`s16le`)

This maps directly to:

- `hg.AFF_LPCM_44KHZ_S16_Mono`

That means HARFANG does not need to understand `lib-say` specifically. It only
needs a generic raw PCM upload entry point.

## Recommended Public API

### C++

```cpp
namespace hg {

SoundRef LoadLPCMSound(const void *data, size_t size, AudioFrameFormat format);

} // namespace hg
```

### Semantics

- `data` points to decoded LPCM bytes
- `size` is the exact byte size of the PCM block
- `format` describes sample rate, channel count, and sample resolution
- the function copies the PCM data into OpenAL before it returns
- the function does not retain the caller-owned pointer after return

This last rule is the key property that makes the function safe for foreign
Lua/Python memory owners such as `lib-say` blobs.

## Script Binding Specification

### Preferred Scripting Surface

To minimize friction with `lib-say`, the Lua/Python binding should expose the
function as taking an integer pointer directly, even if the underlying C++
function takes `const void *`.

Recommended binding signatures:

### Lua

```lua
SoundRef LoadLPCMSound(integer ptr, integer size, AudioFrameFormat format)
```

### Python

```python
LoadLPCMSound(ptr: int, size: int, format: AudioFrameFormat) -> SoundRef
```

The binding implementation can route the integer through:

```cpp
reinterpret_cast<const void *>(ptr)
```

This is preferable to forcing scripts to call `int_to_VoidPointer()` first,
because `lib-say` already returns the pointer as an integer.

### Acceptable Fallback Binding

If binding directly from integer pointer is inconvenient in FabGen, the
acceptable fallback is:

- C++: `LoadLPCMSound(const void *data, size_t size, AudioFrameFormat format)`
- Lua/Python: same shape through `VoidPointer`
- script code uses `hg.int_to_VoidPointer(blob:GetData())`

That is still a low-impact solution, only slightly less ergonomic.

## lib-say Usage Model

### Recommended Lua Example

```lua
local hg = require("harfang")
local say = require("say")

local blob, info = say.synthesize("Bonjour depuis Lua", {
	lang = "fr",
	format = "raw"
})

assert(info.sample_rate == 44100)
assert(info.channels == 1)
assert(info.bits_per_sample == 16)
assert(info.pcm_encoding == "s16le")

local snd_ref = hg.LoadLPCMSound(
	blob:GetData(),
	blob:GetSize(),
	hg.AFF_LPCM_44KHZ_S16_Mono
)

local src_ref = hg.PlayStereo(snd_ref, hg.StereoSourceState(1, hg.SR_Once))
```

This is the direct bridge requested by the use case:

- `lib-say` raw buffer pointer
- HARFANG upload function
- regular `SoundRef`

### Fallback Lua Example With `VoidPointer`

```lua
local snd_ref = hg.LoadLPCMSound(
	hg.int_to_VoidPointer(blob:GetData()),
	blob:GetSize(),
	hg.AFF_LPCM_44KHZ_S16_Mono
)
```

### Important Limitation

This bridge only applies to decoded raw PCM.

It does not accept:

- AIFF file bytes
- WAV file bytes
- OGG file bytes
- arbitrary encoded container payloads

For `lib-say`, that means the integration path is:

- `format = "raw"`: supported
- `format = "aiff"`: out of scope for this API

## Validation Rules

`LoadLPCMSound()` should reject invalid or incoherent buffers and return
`InvalidSoundRef`.

Recommended checks:

- `format != AFF_Unsupported`
- `data != nullptr`
- `size > 0`
- `size % bytes_per_frame == 0`
- `AFF_ALFormat(format) != 0`

Where:

```cpp
bytes_per_frame = (AFF_ChannelCount[format] * AFF_Resolution[format]) / 8
```

For the current `AudioFrameFormat` enum, valid inputs are limited to:

- 44.1 kHz / 16-bit / mono
- 48 kHz / 16-bit / mono
- 44.1 kHz / 16-bit / stereo
- 48 kHz / 16-bit / stereo

## Engine Implementation Notes

### Public Header Change

Add to `harfang/engine/audio.h`:

```cpp
SoundRef LoadLPCMSound(const void *data, size_t size, AudioFrameFormat format);
```

### `audio.cpp` Implementation

Implementation should stay inside the existing audio system:

1. validate the input
2. allocate a free `SoundRef` with the existing helper
3. create one OpenAL buffer
4. upload the PCM with `alBufferData(...)`
5. store the buffer in `sounds[snd_ref].buffers`
6. return the new `SoundRef`

Pseudo-shape:

```cpp
SoundRef LoadLPCMSound(const void *data, size_t size, AudioFrameFormat format) {
	if (!data || size == 0 || format == AFF_Unsupported)
		return InvalidSoundRef;

	const size_t bytes_per_frame = (AFF_ChannelCount[format] * AFF_Resolution[format]) / 8;
	if (bytes_per_frame == 0 || (size % bytes_per_frame) != 0)
		return InvalidSoundRef;

	const auto al_format = AFF_ALFormat(format);
	if (al_format == 0)
		return InvalidSoundRef;

	const auto snd_ref = GetFreeSoundRef();
	auto &sound = sounds[snd_ref];

	sound.buffers.push_back(AL_INVALID_VALUE);
	alGenBuffers(1, &sound.buffers.back());
	alBufferData(
		sound.buffers.back(),
		al_format,
		data,
		numeric_cast<ALsizei>(size),
		AFF_Frequency[format]
	);

	return snd_ref;
}
```

### Buffer Strategy

The MVP should upload the whole PCM block as one OpenAL buffer.

That is sufficient for the `lib-say` use case and avoids unnecessary
complexity. If very large static sounds later need chunking, that can be added
independently.

## FabGen / Binding Notes

Update `binding/bind_harfang.py` with one new binding.

Recommended approach:

- add a tiny route helper that converts `intptr_t` to `const void *`
- bind `LoadLPCMSound` for Lua and Python

Example helper shape:

```cpp
static hg::SoundRef _LoadLPCMSoundFromPtr(intptr_t ptr, size_t size, AudioFrameFormat format) {
	return hg::LoadLPCMSound(reinterpret_cast<const void *>(ptr), size, format);
}
```

Then bind the scripting function against that helper so the script-facing API
matches what `lib-say` already returns.

## Documentation Requirements

The API documentation must state clearly:

- the input is decoded PCM, not a file format
- the pointer is borrowed only for the duration of the call
- the function copies the PCM data into the audio backend before returning
- the returned value is a normal `SoundRef`
- the caller is responsible for mapping its own metadata to
  `AudioFrameFormat`

## Optional Future Extension

If HARFANG later needs a persistent CPU-side audio object, a separate `hg::Sound`
class can still be added later.

That would only be justified if HARFANG needs features such as:

- keeping decoded PCM resident on the CPU
- inspecting or editing samples after import
- sharing one CPU-side sound object across several upload operations
- mirroring the `Picture` API shape for consistency

That is not required to support `lib-say`.

## Non-Goals

This specification does not propose:

- adding a lib-say-specific dependency to HARFANG
- parsing `lib-say` metadata tables inside the engine
- adding a new `hg::Sound` class in the MVP
- supporting AIFF/WAV/OGG container bytes through this API
- changing `SoundRef`, `PlayStereo`, `PlaySpatialized`, or `UnloadSound`

## Testing

### C++

Add tests for:

- valid mono 44.1 kHz PCM upload
- valid stereo 48 kHz PCM upload
- null pointer rejection
- zero size rejection
- bad alignment rejection
- unsupported format rejection

### Lua

Add a smoke test that:

- synthesizes a short `raw` buffer with `lib-say` when available, or uses a
  small synthetic PCM buffer otherwise
- calls `LoadLPCMSound(...)`
- starts playback with `PlayStereo(...)`

### Python

Add a pointer-based smoke test for:

- integer pointer input
- byte size
- `AudioFrameFormat`
- `SoundRef` creation

## Recommendation

Adopt the direct raw PCM bridge as the MVP.

Recommended API:

```cpp
SoundRef LoadLPCMSound(const void *data, size_t size, AudioFrameFormat format);
```

Recommended scripting ergonomics:

- bind it as `LoadLPCMSound(ptr, size, format)`
- let scripts pass `lib-say`'s `blob:GetData()` directly

This solves the concrete integration problem with much less impact on HARFANG
than adding a full `Sound` object with `SetData()` / `GetData()`.
