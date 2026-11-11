.title Compiling to Assets

Compiling project resources into assets is done using the `assetc` command-line tool.

Upon invocation, it will scan the input folder and compile all resources in a supported format to the output folder. Files in an unsupported format are copied unmodified to the output folder.

*It is very important that you treat the compiled output folder as entirely disposable* and *only ever* perform modifications in the input folder. Output assets will be different for each platform you compile to.

Again, *do not ever work in the assets folder*.

If you are unclear on the resources/assets distinction see [man.Assets].

## Drag & drop

The easiest way is to drag and drop the resources folder on the assetc executable:

![assetc drag & drop](/images/docs/${HG_VERSION}/assetc.gif)

## Command-Line

Since HARFANG 3.2.5, `assetc` is now included in the packaged _Python_, _Lua_, and _Squirrel_ distributions.

### Python

In HARFANG Python 3.2.5 and above, `assetc` is packaged into the bdist wheel and can be invoked as a function of harfang.bin module.

In this example, we will compile a folder called `resources` and target the `OpenGL` API:

* From the command line:
   ```bash
   python3 -m harfang.bin assetc resources -api GL
   ```

* As a Python module:
   ```python
   import harfang.bin
   harfang.bin.assetc('resources', '-api', 'GL')
   ```

### Lua

In HARFANG Lua 3.2.5 and above, `assetc` is packaged along with the Lua extension and can be invoked as a function of harfang.bin.

In this example, we will compile a folder called `resources` and target the `OpenGL` API:

```lua
hg_bin = require "harfang.bin"
hg_bin.assetc('resources', '-api', 'GL')
```

### Squirrel

In HARFANG Squirrel 3.2.7 and above, `assetc` is packaged with the public runtime under `harfang/assetc`.

Invoke the packaged executable directly from the Squirrel package root:

```text
harfang/assetc/assetc.exe resources -api GL
```

At the moment `.nut` scripts are copied as source assets; there is no Squirrel bytecode compilation step in `assetc`.

## Usage in detail

If you need more control over the compilation options, the command line gets the following parameters:

```
assetc <input> [output PATH] [-daemon] [-platform PLATFORM] [-api API] [-defines DEFINES] [-job COUNT]
       [-toolchain PATH] [-profile NAME] [-progress] [-log_to_std_out] [-debug] [-quiet] [-verbose]
```

Option | Shortcut | Description
-------|----------|------------
`-input` | | Input project resources folder.
`-output` | | Output compiled assets folder. If unspecified, the input folder path suffixed with `_compiled` is used.
`-daemon` | `-d` | Run the compiler in daemon mode. The compiler will constantly monitor the input folder and compile its content as it is modified.
`-platform` | `-p` | Platform to target.
`-api` | | Graphics API to target. Some platforms (eg. PC) might support multiple graphics API (DX11, DX12, GL, ...).
`-defines` | `-D` | Semicolon separated defines to pass to the shader compiler (eg. FLAG;VALUE=2).
`-job` | `-j` | Maximum number of parallel job (0 - automatic).
`-toolchain` | `-t` | Path to the toolchain folder.
`-profile` | | Metadata profile to select; missing fields fall back to `default`.
`-progress` | | Output progress to the standard output.
`-log_to_std_out` | `-l` | Log errors to the standard output.
`-debug` | | Compile in debug mode (eg. output debug informations in shader).
`-quiet` | `-q` | Disable all build information but errors.
`-verbose` | `-v` | Output additional information about the compilation process.

*Note:* When run in daemon mode `assetc` will not exit after its initial run and will keep watching the input folder. When a resource is modified it will automatically be compiled to the output folder.

## Texture compilation

`assetc` uses the bundled portable `texc` compiler for BC1, BC3, BC4, BC5, BC7,
RGBA8, BGRA8 and RAW textures on Windows, Linux and macOS. RAW selects BGRA8 for
DX11 and RGBA8 for the other graphics APIs. The existing `compression`,
`generate-mips` and `max-size` metadata remain supported. The fast path uses
quality 0 and power-of-two dimensions, bounded by `max-size`.

Formats outside this set, and HDR/EXR inputs, use the existing compilers
(`texconv` when available and compatible, otherwise BGFX `texturec`). Custom
toolchains without `texc` also retain this fallback. Changing the selected
compiler invalidates the texture's compilation cache automatically.

See [man.GLTF] and [man.FBX] to convert common 3d formats to Harfang resources.

## Gaussian PLY, splat and SPZ compilation

`assetc` delegates Gaussian PLY, `.splat` and `.spz` conversion to `gsplatc`, installed in the same
toolchain directory as `texturec` and `shaderc`. For a resource named
`island.ply`, create `island.ply.meta`:

```json
{
  "profiles": {
    "default": {
      "type": "gaussian-splat",
      "sh-degree": 3,
      "normalize": false
    },
    "preview": {
      "max-count": 50000,
      "sh-degree": 1
    }
  }
}
```

Compiling the resources produces `island.hgs` in the corresponding output
directory. Use `-profile preview` to select the example override. PLY, `.splat` and `.spz` resources
without `type: "gaussian-splat"` retain pass-through behavior; this converter
does not import polygon meshes.

The importer accepts Graphdeco/Postshot binary little-endian PLY with named
scalar float properties. It validates the entire source, optionally selects
evenly spaced splats, and writes the versioned HGS2 runtime format. The runtime
loads this compiled asset through the native Gaussian model API.

Parameter | Default | Meaning
----------|---------|--------
`max-count` | All source splats | Optional explicit reduction cap, integer 1–2097152. Omit it to retain every source splat. Explicit zero is invalid. Naive decimation can reduce opacity coverage.
`sh-degree` | 3 | Maximum stored spherical-harmonics degree, integer 0–3. Missing source bands are not invented.
`storage` | `quantized` | `quantized` for adaptive quantization and block compression; `float32` for lossless canonical attributes with block compression.
`normalize` | false | Boolean. Center the full source position bounds and uniformly scale their largest extent to two units; otherwise preserve source coordinates.

Selected profiles inherit each missing field from `default`. An explicit cap in
`default` therefore also applies to `reference` or another profile unless that
profile supplies its own cap. With no effective `max-count`, `assetc` does not
pass `--max-count` to `gsplatc`, whose default also retains every splat.

The native implementation accepts up to 2,097,152 retained splats and 512 MiB of
expanded canonical data per model. Files exceeding these limits fail with an
error; they are never reduced silently. Request a smaller `max-count` or SH
degree explicitly if needed. Compression reduces disk size, not the expanded
CPU/GPU memory required to load the model.

Metadata changes participate in incremental compilation. Invalid Gaussian
settings, missing `gsplatc`, unsupported source schemas and converter failures
fail the build. One-shot `assetc` returns a nonzero status when compilation
fails. A separate source `island.hgs` alongside the opted-in `island.ply` is
rejected as an output-name collision.

Headerless 32-byte `.splat` sources use the same metadata schema, for example
`plant.splat.meta` produces `plant.hgs`. The layout is little-endian float32
XYZ position and linear XYZ scale, byte RGBA, and packed byte WXYZ quaternion.
RGB becomes SH0 coefficients; alpha is linear, including zero. No higher SH
bands are invented. Every source record is validated before output is written.
Two opted-in `.ply`/`.splat`/`.spz` sources with the same basename are rejected because
they would produce the same HGS file.

Gzip SPZ versions 2 and 3 use the same metadata: `island.spz.meta` produces
`island.hgs`. Import preserves SH0-3, antialiasing and stored coordinate axes,
with the same decimation and normalization options. Gzip integrity and exact
attribute lengths are checked; decompressed input is capped at 512 MiB.
SPZ v1/v4, extension flags and SH4 are unsupported and fail explicitly.
SPZ quantization is retained; conversion cannot recover lost source precision.

HGS2 stores blocks of up to 256 splats compressed with zlib/DEFLATE. Quantized
storage is the default for every input format, including PLY. Adaptive block
ranges avoid intentional color/SH clipping. `gsplatc --storage float32` (or
`"storage": "float32"` in a metadata profile) provides a reference representation.
The tool reports measured quantization errors and actual file size. Native
loading reconstructs float buffers; GPU memory use is unchanged. HGS0/HGS1
are not accepted: recompile existing Gaussian resources after updating.
