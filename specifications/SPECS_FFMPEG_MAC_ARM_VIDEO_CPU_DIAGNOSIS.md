---
name: ffmpeg-mac-arm-video-cpu-diagnosis
description: "Source-level diagnosis of why HARFANG's FFmpeg video plugin is CPU-heavy on Apple Silicon: likely non-selected VideoToolbox decode, mandatory CPU colorspace conversion to RGBA, and per-frame CPU-to-GPU texture uploads"
metadata:
  node_type: memory
  type: project
---

This note reviews `harfang3d/plugins/ffmpeg` and the engine upload path used by
HARFANG video playback on macOS/Apple Silicon.

The short version is:

- The current plugin is structurally CPU-heavy on Mac ARM.
- Even when FFmpeg has VideoToolbox support available, the HARFANG plugin still
  forces a CPU-side frame path.
- The dominant costs are not H.264 bitstream parsing alone, but
  `decode -> CPU frame conversion -> full RGBA upload each frame`.

I did not run an Instruments profile here. This is a source-level diagnosis
validated with local `ffprobe` / `ffmpeg` checks against the bundled macOS ARM
FFmpeg build and the sample videos shipped in `interactive-book/app/assets/videos/`.

## Executive Conclusion

The plugin is a reasonable portable prototype, but it is not a low-overhead
Apple Silicon playback path.

Three facts matter most:

1. The plugin always exports frames as CPU `RGBA32`
   (`plugins/ffmpeg/ffmpeg_video_stream.cpp:771-790`).
2. Every decoded frame is converted through `sws_scale(..., AV_PIX_FMT_RGBA, ...)`
   on the CPU (`plugins/ffmpeg/ffmpeg_video_stream.cpp:565-572`).
3. HARFANG then uploads that whole CPU buffer into a BGFX texture with
   `bgfx::updateTexture2D(...)` (`harfang/engine/video_stream.cpp:146-190`).

On Apple Silicon, that means the pipeline is still CPU-centric even if hardware
decode is theoretically available. With 1080p60 content, especially 10-bit or
4:2:2 variants, that is enough to heat the SoC and make the fan audible.

## What The Code Does Today

### 1. The plugin tries hardware decode, but likely does not actually select it

In `VideoStream::Open`, the plugin enumerates FFmpeg hardware configs and, if it
finds one, creates a hardware device context and stores `hw_pix_fmt`
(`plugins/ffmpeg/ffmpeg_video_stream.cpp:377-413`).

However, the implementation never installs a `codec_ctx->get_format` callback
and never explicitly asks the decoder to choose `hw_pix_fmt`.

That matters because FFmpeg hardware decode selection normally depends on the
decoder choosing the hardware pixel format. This implementation prepares a
hardware device, but the decode loop only uses the hardware path if
`frame->format == hw_pix_fmt` (`plugins/ffmpeg/ffmpeg_video_stream.cpp:553-558`).

So the current code strongly suggests:

- the plugin may create a VideoToolbox-capable device context,
- but the decoder is very likely to stay on software pixel formats in practice,
- meaning Apple Silicon hardware decode may not be engaged at all.

This is the single most important implementation-level risk in the current
plugin.

### 2. Even a working hardware path still falls back to CPU memory

If a frame does arrive in hardware format, the plugin immediately pulls it back
to a software frame with `av_hwframe_transfer_data(sw_frame, frame, 0)`
(`plugins/ffmpeg/ffmpeg_video_stream.cpp:553-556`).

After that, it still runs `sws_scale` into a CPU RGBA buffer
(`plugins/ffmpeg/ffmpeg_video_stream.cpp:565-572`).

So even the "hardware" path is not zero-copy and not GPU-native. It still does:

- hardware decode,
- hardware-to-system-memory transfer,
- CPU colorspace conversion,
- CPU-to-GPU upload.

That is better than pure software decode, but it is still expensive.

### 3. The ABI forces inefficient output formats for modern video

`harfang/engine/video_stream_interface.h` only exposes:

- `VFF_YUV422`
- `VFF_RGB24`
- `VFF_RGBA32`

There is no support for:

- `NV12`
- `P010`
- planar `YUV420`
- multi-plane textures
- Metal / CVPixelBuffer / IOSurface handles

As a result, the plugin cannot expose the native formats that VideoToolbox and
modern H.264 decoders actually like to produce on Apple hardware.

This is not just a plugin bug. It is also an interface limitation.

### 4. The engine uploads the full texture from CPU memory every update

`hg::UpdateTexture(...)` calls `streamer.GetFrame(...)`, then performs
`bgfx::updateTexture2D(...)` on the returned frame
(`harfang/engine/video_stream.cpp:137-190`).

The plugin-side `GetFrame` API always returns a CPU buffer, and the engine does
not perform any GPU-side YUV conversion. That means every displayed frame is
uploaded as a full bitmap.

In `interactive-book`, this upload path is exercised on every app update:
`CreateSceneVideoPlayer:Update()` calls `hg.UpdateTexture(...)` every frame
(`interactive-book/app/video_player.lua:107-121`). On the plugin side,
`CommitFrame()` can intentionally return the last frame again when no new video
frame is ready yet (`plugins/ffmpeg/ffmpeg_video_stream.cpp:608-640`).

That means 25 fps or 30 fps videos rendered in a 60 Hz loop can trigger
duplicate full-texture uploads of unchanged image data.

For `RGBA32`, the raw upload size is:

| Frame size | Bytes per frame | Upload at 30 fps | Upload at 60 fps |
| --- | ---: | ---: | ---: |
| 1280x720 | 3.52 MiB | 105 MiB/s | 211 MiB/s |
| 1920x1080 | 7.91 MiB | 237 MiB/s | 475 MiB/s |
| 3840x2160 | 31.64 MiB | 949 MiB/s | 1.85 GiB/s |

That is only the upload. It does not include:

- decode cost,
- `av_hwframe_transfer_data`,
- `sws_scale`,
- thread scheduling overhead.

### 5. The playback loop uses polling and `yield()`

Both the demuxer thread and decode loop contain hot-loop `std::this_thread::yield()`
calls (`plugins/ffmpeg/ffmpeg_video_stream.cpp:264`, `515`).

More importantly, `Demuxer::Get()` does not block when the packet queue is empty;
it returns `NULL` immediately (`plugins/ffmpeg/ffmpeg_video_stream.cpp:216-225`).
`VideoStream::Loop()` then spins back around (`plugins/ffmpeg/ffmpeg_video_stream.cpp:534-537`).

That design wastes CPU compared with a blocking consumer wait on a condition
variable. It is probably not the main cost, but it adds unnecessary core usage.

### 6. Software decode is allowed to use many CPU threads

If `codec_ctx->thread_count == 1`, the plugin bumps the decoder thread count up
to hardware concurrency, capped at 8, and enables slice threading
(`plugins/ffmpeg/ffmpeg_video_stream.cpp:394-399`).

If the stream is decoded in software, that means this plugin is explicitly
allowed to consume a large chunk of the CPU package. On an Apple Silicon laptop,
that is exactly the kind of behavior that causes heat and fan noise.

## Local Validation On This Machine

### Bundled FFmpeg does have VideoToolbox support

The macOS ARM `libavcodec.61.dylib` bundled under
`interactive-book/app/bin/hg_lua-macos-arm64/`:

- links `VideoToolbox.framework`,
- and was built with `--enable-videotoolbox`.

So the problem is not "FFmpeg on this machine has no Apple hardware decode
support".

### The same FFmpeg build can use VideoToolbox outside HARFANG

Local CLI checks with the same FFmpeg build show VideoToolbox decode being
selected successfully:

- `brute-concrete.mp4` reinitializes to `pix_fmt: videotoolbox_vld` and outputs
  `nv12`.
- `marine-melodies.mp4` reinitializes to `pix_fmt: videotoolbox_vld` and outputs
  `p210le`.

That means Apple hardware decode is locally available in principle. The HARFANG
plugin is simply not using a GPU-native end-to-end path.

## The Shipped Video Assets Make The Problem Worse

The videos in `interactive-book/app/assets/videos/` are not lightweight test
clips. Several are demanding playback targets:

| File | Codec / profile | Size | FPS | Pixel format |
| --- | --- | --- | ---: | --- |
| `marine-melodies.mp4` | H.264 High 4:2:2 | 1920x1080 | 60 | `yuv422p10le` |
| `offscreen-colonies.mp4` | H.264 High 10 | 1920x1080 | 60 | `yuv420p10le` |
| `remnants.mp4` | H.264 High 10 | 1920x1080 | 60 | `yuv420p10le` |
| `brute-concrete.mp4` | H.264 High | 1920x1080 | 60 | `yuv420p` |
| `lowpoly.mp4` | H.264 High | 1920x1080 | 30 | `yuv420p` |
| `bones-of-civilisation.mp4` | H.264 High | 1280x720 | 60 | `yuv420p` |

This matters because:

- 1080p60 is already costly if every frame is converted to CPU RGBA and uploaded.
- 10-bit and 4:2:2 content adds more conversion pressure than ordinary 8-bit
  4:2:0 distribution video.
- the fan issue can appear even when the compressed bitrate is modest, because
  the expensive part is the decoded pixel path, not just file I/O.

## Most Likely Root Causes, Ranked

### Root cause 1: CPU RGBA conversion plus CPU-to-GPU upload is the dominant cost

This is the clearest conclusion from the source.

Regardless of whether decode itself is software or hardware-assisted, the plugin
ends up producing a CPU `RGBA32` frame and the engine uploads that full frame
into a texture every update.

That is fundamentally expensive on Apple Silicon laptops.

### Root cause 2: the intended FFmpeg hardware decode path is probably not fully activated

The hardware device setup exists, but the usual format-selection hook is absent.
This means the plugin may quietly fall back to software surfaces and only use
CPU decoding, especially in cases where FFmpeg needs explicit hardware format
selection.

Given that the local FFmpeg build can use VideoToolbox from the CLI, this is a
strong missed opportunity.

### Root cause 3: busy polling adds avoidable CPU overhead

The plugin spins in both its demux and decode loops instead of blocking cleanly
when no work is available.

This is not the largest problem, but it is unnecessary overhead in a playback
path that is already expensive.

### Root cause 4: the video stream ABI is too narrow for an efficient Apple path

Without `NV12` / `P010` / multi-plane / native-surface support, the engine has
no way to do:

- hardware-decoded YUV surfaces,
- shader-based YUV-to-RGB conversion,
- or zero-copy Metal texture interop.

So even a fixed plugin remains boxed into a CPU-centric architecture.

## What Is Probably Not The Real Issue

- Not a missing FFmpeg hardware backend. VideoToolbox is present locally.
- Not mainly an H.264 bitrate problem. Several assets are only a few Mb/s, yet
  still expensive because the decoded pixel pipeline is heavy.
- Not specifically a Metal renderer bug. The pressure comes earlier, in the
  plugin and video-stream ABI design.

## Recommended Actions

### Short-term, low-risk

1. Transcode user-facing video assets toward Apple-friendly playback targets:
   H.264 8-bit `yuv420p`, or HEVC if acceptable for the project.
2. Reduce frame rate where 60 fps is not visually necessary. 30 fps cuts decode,
   conversion, and upload pressure roughly in half.
3. Avoid 10-bit and 4:2:2 masters for in-engine playback unless there is a very
   strong visual reason.

These steps do not fix the architecture, but they will reduce heat quickly.

### Medium-term, contained plugin work

1. Implement proper FFmpeg hardware-format selection for this plugin and log the
   actual decode path at runtime.
2. Confirm, per stream, whether frames arrive as software `yuv420p` / `yuv422p10le`
   or hardware `videotoolbox_vld`.
3. If hardware decode is active, keep that path working across seek / pause /
   resume.

This should reduce CPU spent on decode itself, but it will not eliminate the
CPU-side conversion and upload costs.

### High-impact architectural fix

1. Extend the HARFANG video stream ABI beyond `RGBA32` / `RGB24` / `YUV422`.
2. Add support for `NV12` and `P010`-style outputs, ideally as multi-plane video
   frames.
3. Perform YUV-to-RGB conversion in a shader instead of `sws_scale`.
4. On macOS specifically, consider a native VideoToolbox-to-Metal path
   (`CVPixelBuffer` / `CVMetalTextureCache`) to avoid the software bounce
   entirely.

This is the change that would materially modernize video playback on Apple
Silicon.

### Cleanup work worth doing anyway

1. Replace the decode-loop polling with blocking waits on condition variables.
2. Expose whether a frame is actually new, so the engine can skip redundant
   texture uploads when the render rate is above the video frame rate.

These are smaller wins, but they are straightforward and technically justified.

## Final Assessment

If H.264 playback in HARFANG on Mac ARM is making the fan loud, the code gives a
coherent explanation:

- the plugin is very likely not fully enabling VideoToolbox decode,
- even its best case still converts frames on the CPU,
- and HARFANG uploads a full CPU RGBA frame into a texture every update.

So the current behavior is expected, not surprising.

If the goal is "quiet playback on Apple Silicon laptops", the real fix is not a
tiny FFmpeg tweak. The durable fix is to stop treating decoded video as a CPU
RGBA bitmap and move toward native YUV / GPU-backed playback.
