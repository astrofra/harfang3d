# FFmpeg Video Stream Plugin

This plugin implements Harfang's `IVideoStreamer` ABI using FFmpeg and builds a
dynamic module named `hg_ffmpeg`.

## Build

The plugin is optional because it depends on FFmpeg development headers and
libraries.

```sh
cmake -S . -B build -DHG_BUILD_FFMPEG_PLUGIN=ON
cmake --build build --target hg_ffmpeg
```

On Windows with MSVC, set `FFMPEG_ROOT` to the FFmpeg SDK directory:

```sh
cmake -S . -B build -DHG_BUILD_FFMPEG_PLUGIN=ON -DFFMPEG_ROOT=C:/path/to/ffmpeg
```

The directory is expected to contain `include`, `lib`, and/or `bin` entries
matching the FFmpeg package layout.

## Runtime

Load the module through the existing video stream API:

```cpp
IVideoStreamer streamer = hg::MakeVideoStreamer("hg_ffmpeg.dll");
```

Use `hg_ffmpeg.so` on Linux and `hg_ffmpeg.dylib` on macOS.
