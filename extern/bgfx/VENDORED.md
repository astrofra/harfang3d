# Vendored BGFX Stack

This directory vendors upstream `bgfx`, `bimg`, and `bx` directly in the
Harfang repository. The on-disk layout intentionally matches the previous
submodule layout so the existing CMake glue, include paths, and toolchain
targets keep working without path changes.

Pinned upstream commits:

- `bgfx`: `3d4bd88c0635b614b453aac1003cc56a11d1ccf0`
- `bimg`: `663f724186e26caf46494e389ed82409106205fb`
- `bx`: `ad018d47c6c107e2fe2f3ba0716f9e728ed59a39`

Current Harfang-specific delta:

- `bgfx/src/renderer_mtl.mm`: bump Metal `UNIFORM_BUFFER_SIZE` from 8 MB to
  32 MB to avoid per-frame uniform-ring overflow on heavy macOS/Metal scenes.

Maintenance rules:

- Keep Harfang-specific fixes as normal commits in this repository.
- Record new upstream SHAs here whenever refreshing the vendor drop.
- Treat BGFX upgrades as separate work from local bug fixes.
- Other third-party dependencies in Harfang still use git submodules; only the
  BGFX stack is vendored here.
