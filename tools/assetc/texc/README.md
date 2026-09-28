# texc

Portable image-to-DDS compiler backported from `harfang-evolution`,
`tools/assetc/texc` (source revision `f9e5ea1443331d68042f21f7a26ebb06c5556311`).
The bundled stb, fmt and bc7enc sources retain their original license notices.

`texc` builds with assetc and is installed in its host/target toolchain directory.
It can also be built on its own with CMake 3.16.2+ and a C++14 compiler:

```sh
cmake -S tools/assetc/texc -B build/texc -DCMAKE_BUILD_TYPE=Release
cmake --build build/texc --config Release
```

Use an optimized Release build for texture compilation performance. The texc
targets also retain optimization in Harfang's default RelWithDebInfo configuration.
No ISPC or OpenMP installation is required; assetc schedules textures in parallel.

```sh
texc -f BC7 -q 0 -r -s 2048 -m input.png output.dds
texc -h
```

Supported outputs: BC1, BC3, BC4, BC5, BC7, RGBA8 and BGRA8. Quality ranges from
0 (fastest, the assetc default) to 10. `-m` generates the complete mip chain;
`-r` rounds dimensions up to powers of two. When combined with `-s`, the power
of two dimensions stay within the size limit. Omit `-r` to preserve NPOT sizes.

The backport fixes rectangular mip chains, uncompressed DDS mip sizes/pitch,
write error reporting, and resizing of narrow images and non-power-of-two limits.

Run the binary regression checks with Python 3 (no third-party packages):

```sh
python tools/assetc/tests/test_texc.py --texc /path/to/texc \
  --assetc /path/to/assetc --texturec /path/to/texturec -v
```

`--assetc` and `--texturec` are optional. Use an installed assetc package so its
runtime libraries are available. Tests generate their inputs and temporary
toolchains, check DDS sizes and channels, exercise bimg decoding, and verify
compiler selection, metadata, paths containing spaces and incremental rebuilds.
