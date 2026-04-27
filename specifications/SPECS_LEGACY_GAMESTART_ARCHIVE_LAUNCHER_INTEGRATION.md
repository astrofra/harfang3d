# Legacy GameStart Archive Launcher Integration Feasibility

Date: 2026-04-19

## Executive Summary

Supporting the legacy GameStart `nArchive` container from HARFANG's Lua
launcher is feasible. The archive format itself is simple: a linear list of
entries, optional zlib-compressed payloads, and archive-local forward-slash
paths. The main work is not decoding the legacy container. The main work is
making HARFANG's asset mounting API behave the same way when the backing data
source is a local directory, a ZIP archive, or a GameStart archive.

The recommended implementation is a launcher-activated archive-backed asset
folder system:

- Keep the public `hg.AddAssetsFolder(path)` behavior unchanged for normal
  local folders.
- Add internal engine support for "asset folder backed by archive prefix"
  sources.
- Let `launcher.cpp` discover and mount `data/`, `data.zip`, or a GameStart
  archive such as `data.gsa` / `data.nac`.
- Let `launcher.cpp` install a temporary folder resolver so calls to
  `hg.AddAssetsFolder("data")` or `hg.AddAssetsFolder("data/subdir")` can map
  to the equivalent archive prefix when no local folder exists.
- Apply the same archive-backed folder mechanism to ZIP and GameStart archives,
  so ZIP launcher mode gains the same path symmetry.

A literal implementation that changes only `languages/hg_lua/launcher.cpp` and
does not touch the engine asset backend cannot fully meet the requirement.
HARFANG runtime loading goes through `hg::OpenAsset`, `hg::IsAssetFile`, and
`g_assets_reader`; those functions currently know only local folders and ZIP
packages. The practical interpretation should therefore be: the feature is
activated and configured only by the launcher for now, while the minimal asset
backend support lives in the engine.

## Feasibility Verdict

Feasible with low archive-format risk and medium integration risk.

The risk is low for reading GameStart archives because the format is compact,
already documented in `C:\works\projects\gamestart-legacy-cooker`, and uses zlib
compression that HARFANG can satisfy through its existing `miniz` dependency.

The integration risk is medium because the current asset system separates local
folders and ZIP packages, searches all folders before all packages, and exposes
`AddAssetsFolder` as a public Lua/C++ API. Preserving path compatibility without
extracting files requires a small redesign of the asset source model.

A robust MVP is estimated at 1.5 to 2.5 engineer-weeks including tests. A quick
prototype based on temporary extraction could be done faster, but it does not
meet the virtual filesystem requirement and should not be the target design.

## Goals

- Run the same launcher content from a local `data/` folder, a `data.zip` file,
  or a GameStart archive.
- Keep Lua code using `hg.AddAssetsFolder(path)` unchanged where possible.
- Allow minimal Lua branching through bootstrap-provided arguments only when a
  project genuinely needs to know the storage mode.
- Support HARFANG asset APIs from archive-backed folders:
  - `OpenAsset`
  - `AssetToData`
  - `AssetToString`
  - `IsAssetFile`
  - `Load*FromAssets`
  - the launcher's asset-backed Lua `require` searcher
- Apply the same archive-backed folder mapping to ZIP and GameStart archives.
- Avoid extracting the archive to a temporary local filesystem directory.
- Keep the first user-facing scope limited to `languages/hg_lua/launcher.cpp`.

## Non-Goals

- Do not implement GameStart archive writing in HARFANG.
- Do not reproduce GameStart asset conversion or publishing logic.
- Do not make every local filesystem API virtual. Calls such as `FileToString`,
  `Open`, `IsFile`, `Load*FromFile`, and platform-native file path consumers
  remain local filesystem APIs.
- Do not support nested archives in the first milestone.
- Do not expose a new general-purpose VFS API publicly until the launcher path
  has been validated.
- Do not require existing Lua applications to replace `AddAssetsFolder` with a
  new public function.

## Current HARFANG State

### Launcher

`languages/hg_lua/launcher.cpp` currently:

- Looks for `data/` under the current working directory.
- Looks for `data.zip` under the current working directory.
- Calls `hg::AddAssetsFolder(data_dir)` for a local folder.
- Calls `hg::AddAssetsPackage(data_zip)` for a ZIP package.
- Loads `launcher.json` through `LoadTextAsset`.
- Loads the configured entry Lua script through `AssetToData`.
- Installs an asset-backed Lua module searcher that resolves `require(...)`
  through `IsAssetFile` and `AssetToData`.
- Adds local filesystem patterns to Lua `package.path`, including `data/?.lua`
  and `data/?/init.lua`.
- Publishes launcher globals:
  - `LAUNCHER_DATA_DIR`
  - `LAUNCHER_DATA_PACKAGE`
  - `LAUNCHER_CONFIG_PATH`

This means the launcher already runs Lua source out of a ZIP archive, but only
when files are addressed as asset names inside the ZIP root. It does not make
`AddAssetsFolder("some/path")` transparently mean "mount this path inside the
archive".

### Asset System

`harfang/engine/assets.cpp` currently has two separate source collections:

- `assets_folders`: local filesystem folder paths, searched first.
- `assets_packages`: ZIP packages opened with `miniz`, searched after all
  folders.

Important current behavior:

- `AddAssetsFolder(path)` does not validate that `path` exists. It stores the
  path if it is not already mounted.
- `OpenAsset(name)` checks each mounted folder with `PathJoin(folder, name)`,
  then checks each ZIP package with `mz_zip_reader_locate_file`.
- ZIP package entries are extracted to heap memory on open.
- `IsAssetFile(name)` mirrors the same folder-then-package lookup.
- `FindAssetPath(name)` only resolves local folders and returns an empty string
  for packages.
- The public Lua binding exposes both `AddAssetsFolder` and `AddAssetsPackage`.

This is close to the needed design, but the source model needs one more concept:
a folder mount whose storage is an archive prefix instead of a local directory.

## Legacy GameStart nArchive Format

The reference material in `C:\works\projects\gamestart-legacy-cooker` describes
the archive container used by the old GameStart runtime. The relevant format
properties are:

- Two magic values are supported:
  - enhanced archives: integer `0x4E415244`, seen on disk as ASCII `DRAN`
    on little-endian systems
  - legacy archives: integer `0x4E415243`, seen on disk as ASCII `CRAN`
- Enhanced archives then store two little-endian 32-bit values:
  - `offset_padding`
  - `size_padding`
- Entries are stored linearly.
- Each entry contains:
  - aligned `uint32` alias byte length
  - aligned alias bytes, using archive-local forward-slash paths
  - aligned `uint8` method
  - aligned `uint32` original length
  - aligned `uint32` compressed length when method bit 0 means zlib
  - aligned payload bytes
- Methods:
  - `0`: raw payload
  - `1`: zlib payload
- Alias length is limited to 511 bytes by the original reader behavior.
- The writer appends `0xffffffff` as an end marker, and that marker may be
  unaligned.
- The container stores files only; there are no directory entries, timestamps,
  permissions, or platform metadata.

The HARFANG reader should implement this format directly from the documented
layout. Do not directly copy GPL-licensed implementation code from the cooker
repository into HARFANG unless licensing has been explicitly cleared for all
HARFANG distribution modes.

## Key Design Constraint

The same call should work in both modes:

```lua
hg.AddAssetsFolder("data")
```

When `data/` exists locally, this mounts the local filesystem folder. When the
launcher runs from `data.zip` or `data.gsa`, the same call must mount the
archive content that corresponds to the local `data/` folder.

This cannot be solved only by changing Lua `package.path`, because HARFANG
resources are loaded through the C++ asset reader. Scene loading, shader
loading, texture loading, audio loading, and asset-backed Lua scripts all call
into `g_assets_reader` and `g_assets_read_provider`.

Therefore, `AddAssetsFolder` itself must be able to create an archive-backed
folder mount when a launcher archive context is active.

## Recommended Architecture

### 1. Asset Container Abstraction

Introduce an internal archive container abstraction in the engine asset layer.
This can be a small private C++ interface in `assets.cpp` or a dedicated
internal header if tests need direct access.

Required operations:

- `bool IsFile(const std::string &archive_path) const`
- `bool ReadFile(const std::string &archive_path, std::string &data, std::string *error) const`
- `std::string DisplayPath(const std::string &archive_path) const`

Optional later operations:

- `bool OpenStream(...)`
- `std::vector<std::string> List(...)`
- metadata queries such as uncompressed size and compression method

Provide two implementations:

- `ZipAssetContainer`, wrapping the existing `miniz` ZIP reader.
- `GameStartAssetContainer`, indexing and reading GameStart `nArchive` files.

The first milestone can continue using heap-backed package files, matching the
current ZIP behavior. Streaming raw archive entries can be added later for large
audio/video content.

### 2. Archive-Backed Folder Source

Extend the asset source model with a source that behaves like a folder but reads
from an archive prefix.

Conceptually:

```text
logical folder path: data/audio
container path:      C:\game\data.gsa
archive prefix:      audio/
asset name:          music/theme.ogg
resolved entry:      audio/music/theme.ogg
```

This source must participate in `AddAssetsFolder` ordering, not package-root
ordering. Existing behavior searches all folders before all packages, so an
archive-backed folder created by `AddAssetsFolder` should have the same
precedence class as a local folder.

### 3. Launcher Folder Resolver

Add a small internal engine hook that `launcher.cpp` can install while the
launcher is running.

Required behavior:

- `AddAssetsFolder(path)` first keeps the current local-folder behavior when
  `path` is a real local directory.
- If `path` is not a local directory and a launcher folder resolver is active,
  `AddAssetsFolder(path)` asks the resolver whether this path maps to an archive
  prefix.
- If the resolver returns a mapping, `AddAssetsFolder(path)` mounts an
  archive-backed folder source.
- `RemoveAssetsFolder(path)` removes both local and archive-backed folder
  sources that were registered under the same logical path.

The resolver must not be bound to Lua or Python in the first milestone. The
launcher sets it before running the entry script and clears it before exit.

This keeps the public API stable for Lua and C++ callers:

```cpp
hg::AddAssetsFolder("data");
```

and:

```lua
hg.AddAssetsFolder("data")
```

both continue to call the same public function.

### 4. Launcher Source Discovery

Update `MountLauncherAssets` to discover the first usable data source from the
current working directory.

Recommended default order:

1. `data/`
2. `data.zip`
3. `data.gsa`
4. `data.nac`

The local folder should keep highest priority for development workflows. If a
folder and an archive both exist, the folder is the active launcher data source
unless a future explicit command-line option says otherwise.

Archive type detection should use magic bytes, not only file extension:

- ZIP: standard ZIP local header / end-of-central-directory handling through
  `miniz`.
- GameStart: `DRAN` / `CRAN` little-endian magic values.

### 5. Bootstrap Configuration

The current launcher reads `launcher.json`. The user-facing request references
`bootstrap.json`. To avoid breaking existing content, support both names:

1. Try `bootstrap.json`.
2. Fall back to `launcher.json`.

Both files should use the same schema in the first milestone. Existing fields
remain valid:

```json
{
  "entry": "main.lua",
  "args": []
}
```

Add optional archive mapping fields:

```json
{
  "entry": "main.lua",
  "args": [],
  "assets": {
    "logical_data_path": "data",
    "archive_root": "",
    "pass_mode_argument": false
  }
}
```

Field meanings:

- `logical_data_path`: local folder path that the archive replaces. Default:
  `data`.
- `archive_root`: prefix inside the archive that corresponds to
  `logical_data_path`. Default: empty string, meaning the archive root contains
  the contents of `data/`.
- `pass_mode_argument`: when true, the launcher appends a mode argument such as
  `--assets-source=archive` or `--assets-source=folder` to the Lua entry point.

The config cannot be used to find the initial archive path, because it lives
inside the data source. Initial source discovery must therefore remain based on
default filenames or a future command-line option.

## Path Mapping Specification

### Normalization

The launcher resolver must normalize paths before mapping them:

- Convert backslashes to forward slashes for archive lookups.
- Collapse redundant `.` path segments.
- Reject path traversal that escapes the logical data root.
- Treat an absolute path under the process current working directory as its
  relative form for mapping purposes.
- Preserve case. Archive lookups should be case-sensitive in the first
  milestone, matching the current ZIP lookup flags.

### Default Mapping

Assume this local development layout:

```text
game/
  launcher.exe
  data/
    bootstrap.json
    main.lua
    scenes/main.scn
```

The equivalent ZIP or GameStart archive should preferably contain:

```text
bootstrap.json
main.lua
scenes/main.scn
```

With default `logical_data_path = "data"` and `archive_root = ""`, mapping is:

| `AddAssetsFolder` argument | Local mode | Archive mode |
| --- | --- | --- |
| `data` | mounts `game/data/` | mounts archive prefix `` |
| `./data` | mounts `game/data/` | mounts archive prefix `` |
| `game/data` | mounts `game/data/` | mounts archive prefix `` when under cwd |
| `data/scenes` | mounts `game/data/scenes/` | mounts archive prefix `scenes/` |

If an existing legacy archive stores the `data` folder name inside the archive:

```text
data/bootstrap.json
data/main.lua
data/scenes/main.scn
```

then `archive_root` should be set to `data`.

### Asset Name Mapping

Once a folder source is mounted, asset names remain relative to that mounted
folder:

```lua
hg.AddAssetsFolder("data")
hg.LoadSceneFromAssets("scenes/main.scn", scene, resources, pipeline)
```

The scene load should resolve to:

- local mode: `game/data/scenes/main.scn`
- archive-root mode: `scenes/main.scn`
- archive-with-data-prefix mode: `data/scenes/main.scn`

## Launcher Behavior Requirements

### Mounting

The launcher must:

- Mount exactly one primary launcher data source by default.
- Register the primary source with the engine asset system so the launcher can
  load `bootstrap.json` / `launcher.json` and the entry script.
- Install the archive-backed folder resolver when the primary source is an
  archive.
- Clear the resolver during shutdown, before destroying archive container
  objects.

### Lua Module Loading

The existing asset searcher should continue to work:

- `require("foo")` checks `foo.lua`.
- `require("foo")` checks `foo/init.lua`.
- Search should operate through `IsAssetFile` and `AssetToData`, so it inherits
  archive-backed folder support automatically.

The local `package.path` entries can remain for development mode. In archive
mode, the asset searcher is the reliable path for Lua modules stored inside the
archive.

### Launcher Globals

Keep existing globals for compatibility, but add one explicit mode global:

- `LAUNCHER_ASSETS_SOURCE`: `folder`, `zip`, or `gamestart`

Recommended values:

- `LAUNCHER_DATA_DIR`: local folder path in folder mode, empty in archive mode
  unless a real local folder is active.
- `LAUNCHER_DATA_PACKAGE`: archive path in ZIP or GameStart mode.
- `LAUNCHER_CONFIG_PATH`: display path, for example
  `C:\game\data.gsa:bootstrap.json`.

Do not require user code to inspect these globals for ordinary asset loading.

## Asset API Behavior Requirements

### `AddAssetsFolder`

Required behavior:

- If the argument identifies an existing local directory, mount it as before.
- Otherwise, if a launcher resolver maps it to an archive prefix, mount an
  archive-backed folder source.
- Return `true` only when a new source was mounted.
- Return `false` for duplicates, invalid paths, and unmapped virtual paths.
- Preserve current duplicate handling by logical path.

### `RemoveAssetsFolder`

Required behavior:

- Remove local folder sources matching the path.
- Remove archive-backed folder sources registered under the same logical path.
- Be safe when called after the launcher archive source has already failed to
  mount partially.

### `AddAssetsPackage`

The public function can keep its current meaning: mount a package root as an
asset source. Internally it should be allowed to auto-detect ZIP and GameStart
archives once `GameStartAssetContainer` exists.

For the launcher requirement, archive-backed folder mapping is more important
than public `AddAssetsPackage("data.gsa")` support. Public GameStart package
mounting can remain undocumented until enough tests exist.

### `OpenAsset` and `IsAssetFile`

Both functions must search:

1. Local folder sources.
2. Archive-backed folder sources.
3. Package-root sources.

This keeps `AddAssetsFolder` mounts ahead of package-root mounts. The exact
internal data structure may be unified, but observable precedence must remain
compatible.

### `FindAssetPath`

`FindAssetPath` currently returns a local filesystem path. It should not pretend
that an archive entry is a real file path.

Recommended first-milestone behavior:

- Return local filesystem paths for local folder sources only.
- Return an empty string for archive-backed sources.
- Add a separate internal display-path helper if launcher diagnostics need
  strings such as `data.gsa:scenes/main.scn`.

Document that APIs requiring real paths must use local files or be converted to
`*FromAssets` equivalents.

## GameStart Reader Requirements

The GameStart archive reader must:

- Read little-endian 32-bit integer fields explicitly.
- Accept both enhanced and legacy magic values.
- Store archive metadata:
  - revision
  - offset padding
  - size padding
  - file size
  - entry count
- Build an index from normalized alias to entry metadata when mounted.
- Reject unsafe aliases:
  - empty names
  - absolute paths
  - drive-letter paths
  - `..` path segments
  - backslash-only traversal variants
- Reject malformed entries:
  - alias length greater than 511 bytes
  - payload ranges outside the archive file
  - compressed length greater than remaining file bytes
  - unsupported method bits beyond the known raw/zlib bit unless explicitly
    masked for compatibility
- Recognize the unaligned `0xffffffff` end marker.
- Use `miniz` zlib APIs, such as `mz_uncompress`, for method `1`.
- Verify that decompression produces exactly the declared original length.
- Surface useful mount and read errors through HARFANG logging.

The reader may open the archive file per read in the first implementation, or
keep a platform file handle owned by the container. If a shared handle is kept,
reads must be protected by the existing asset mutex or by an internal container
mutex.

## ZIP Reader Requirements

ZIP support already exists through `miniz`. The ZIP side must be adapted to the
same container abstraction used by GameStart archives.

Required changes:

- A ZIP package must be usable both as:
  - a package-root source, equivalent to today's `AddAssetsPackage`
  - an archive-backed folder source, equivalent to `AddAssetsFolder("data")`
    resolving to a ZIP prefix
- ZIP path lookup should remain case-sensitive for compatibility.
- ZIP extraction can remain heap-backed in the first milestone.
- Existing tests for `AddAssetsPackage` must continue to pass.

## Memory and Performance

The first milestone can follow the current ZIP strategy: read or decompress an
entire asset into memory when `OpenAsset` succeeds.

This is acceptable for:

- Lua scripts
- JSON files
- scene files
- shaders
- compiled textures and models of typical size

Known limitation:

- Large raw audio/video assets may consume unnecessary memory if opened through
  asset APIs that expect streaming.

Future improvement:

- Add stream-backed archive handles for raw entries.
- Add chunked zlib inflate for compressed entries if needed.
- Add an LRU cache only after profiling real content.

## Security and Robustness

Archive handling must be defensive even if the first users are trusted legacy
projects.

Required safeguards:

- Do not extract archive entries to disk by default.
- Validate all entry offsets and sizes before indexing.
- Use 64-bit arithmetic for file offsets and size computations even though the
  archive format stores 32-bit fields.
- Enforce a configurable maximum uncompressed asset size. A conservative first
  default such as 512 MiB per entry is acceptable, with a launcher error if an
  entry exceeds it.
- Reject path traversal aliases during archive indexing.
- Ensure malformed archives fail mount cleanly and leave no half-mounted asset
  source behind.
- Avoid direct source-code reuse from the GPL cooker unless license
  compatibility is approved for HARFANG's distribution model.

## Error Reporting

Mount failures should include:

- archive path
- detected archive type, when any
- reason for failure
- entry name when parsing fails on a specific entry

Asset read failures should include:

- requested asset name
- mounted logical folder or package path
- archive entry path
- decompression error string when applicable

Launcher-level errors should keep the existing prefix:

```text
launcher: ...
```

Examples:

```text
launcher: failed to mount GameStart archive 'data.gsa': invalid entry 'a/../b'
launcher: entry script not found in mounted assets: main.lua
launcher: failed to read asset 'scenes/main.scn' from 'data.gsa:scenes/main.scn': zlib decompression failed
```

## Testing Requirements

### Engine Tests

Extend `harfang/tests/engine/assets.cpp` or add a nearby test file covering:

- Existing ZIP package behavior remains unchanged.
- GameStart archive root package can read raw entries.
- GameStart archive root package can read zlib entries.
- `IsAssetFile` works for GameStart entries.
- Missing entries fail without crashing.
- Malformed archive fixtures fail cleanly.
- Archive aliases with `..` or absolute paths are rejected.

Add archive-backed folder tests:

- Local `AddAssetsFolder("data")` reads from a real folder.
- Archive-mode `AddAssetsFolder("data")` reads the same asset names from a ZIP
  prefix.
- Archive-mode `AddAssetsFolder("data")` reads the same asset names from a
  GameStart prefix.
- `RemoveAssetsFolder("data")` removes archive-backed sources.

### Launcher Smoke Tests

Add a minimal launcher fixture with equivalent content in three forms:

- `data/`
- `data.zip`
- `data.gsa`

The same Lua entry script should:

- call `hg.AddAssetsFolder("data")`
- load a text asset through `hg.AssetToString`
- `require` a Lua module stored in assets
- optionally load a tiny scene or JSON file from assets

The expected output and exit code must be identical across the three modes.

### Path Mapping Tests

Cover:

- `data`
- `./data`
- `data/subdir`
- backslash input on Windows
- absolute path under the current working directory
- archive root `""`
- archive root `"data"`

## Implementation Plan

### Phase 1: Internal Asset Source Refactor

- Introduce internal container and source concepts without changing public
  headers unless necessary.
- Wrap current ZIP package behavior in `ZipAssetContainer`.
- Preserve current `AddAssetsPackage` tests.
- Add archive-backed folder source support.

### Phase 2: GameStart Reader

- Implement `GameStartAssetContainer`.
- Add index-time validation.
- Add raw and zlib payload reads.
- Add unit fixtures for legacy and enhanced archives.

### Phase 3: Launcher Integration

- Update `MountLauncherAssets` to discover `data/`, `data.zip`, `data.gsa`, and
  `data.nac`.
- Support `bootstrap.json` with fallback to `launcher.json`.
- Install the launcher folder resolver for archive modes.
- Add `LAUNCHER_ASSETS_SOURCE`.
- Keep old launcher globals for compatibility.

### Phase 4: ZIP Folder Parity

- Ensure the same resolver path works for ZIP archives.
- Add smoke tests proving `AddAssetsFolder("data")` behaves the same for local
  folder, ZIP, and GameStart archive modes.

### Phase 5: Documentation

- Document launcher-supported data sources.
- Document the recommended archive layout.
- Document that `*FromFile` APIs remain local filesystem APIs.
- Keep public GameStart `AddAssetsPackage` support undocumented until it is
  intentionally accepted as a stable API.

## Open Questions

- Should the launcher accept an explicit command-line data source path, for
  example `launcher --data my_archive.gsa`, or is filename discovery enough for
  the first milestone?
- Should `AddAssetsPackage` publicly support GameStart archives immediately, or
  should only launcher auto-mounting use that support at first?
- Should archive path lookup stay strictly case-sensitive on Windows? The
  current ZIP lookup is case-sensitive, so this spec recommends preserving that.
- Do legacy projects store `data/` at archive root, or do they store the
  contents of `data/` at archive root? The default should match HARFANG's
  current `data.zip` behavior: archive root contains the contents of `data/`.
- Are there real legacy archives with method values where only bit 0 matters and
  other bits are set? The reader can mask bit 0 for compatibility, but should
  log unsupported extra method bits during validation.

## Acceptance Criteria

The feature is accepted when:

- Existing `AddAssetsFolder` and `AddAssetsPackage` tests still pass.
- A local `data/` launcher fixture runs successfully.
- An equivalent `data.zip` launcher fixture runs successfully without Lua code
  changes.
- An equivalent GameStart archive launcher fixture runs successfully without Lua
  code changes.
- A script can call `hg.AddAssetsFolder("data")` in all three modes and load the
  same asset names.
- Lua `require` works for modules stored in ZIP and GameStart archives.
- Malformed GameStart archives fail with clear launcher or asset-system errors.
- No archive extraction is used for the normal path.

## Final Recommendation

Proceed with the archive-backed folder design.

Do not implement this as a launcher-only extraction step except as a throwaway
debug prototype. The durable solution is to make the engine asset layer capable
of representing a folder backed by an archive prefix, then let `launcher.cpp`
be the only component that activates that behavior for now. This satisfies the
legacy GameStart requirement and fixes the same path-symmetry problem for ZIP
launcher deployments at the same time.
