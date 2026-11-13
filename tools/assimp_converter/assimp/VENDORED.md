# Vendored Assimp

This directory is tracked directly in the HARFANG repository. It is no longer a
Git submodule, and building it does not require fetching Assimp separately.

- Source repository: https://github.com/assimp/assimp
- Source commit: `c1120a4c04e842822e219e722c2393e9c3611713` (2021-06-14)
- Declared library version: 5.0.1
- Scope: all 2,728 files tracked by the former submodule, including bundled
  dependencies, tests, documentation and license notices.
- License: see `LICENSE` and the notices in the bundled dependencies.

The source commit already contains HARFANG's removal of the duplicate miniz
implementation from Assimp's CMake sources to avoid link conflicts.

Additional local change: `contrib/poly2tri/poly2tri/sweep/sweep.cc` throws the
`std::runtime_error` in the collinear-point error branch instead of constructing
and discarding it. This fixes MSVC warning C4834 with warnings treated as errors.

The parent `CMakeLists.txt` pins the reported Git revision to `c1120a4c` and the
branch label to `vendored`, so CMake does not report HARFANG's Git revision as
Assimp's. Keep this metadata and this file in sync when updating the snapshot.

On Apple Clang, the parent also appends `-Wno-error` to the `assimp` target.
The snapshot enables `-Werror` internally, but newer Apple SDKs and compilers
diagnose its deprecated CRT calls, callback-pointer comparisons, unused
variables, and unmatched diagnostic pragmas. Warnings remain visible; this
exception applies only to the vendored library, not the converter or other
HARFANG targets. Engine and Studio builds share this policy through the same
converter target.
