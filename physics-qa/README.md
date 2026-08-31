# HARFANG Physics QA

Vendorized snapshot of `https://github.com/harfang3d/physics-qa-lua.git`,
imported on 2026-08-31 to support future physics backend benchmarking, QA, and
regression testing inside Harfang.

This folder is meant to preserve the original Lua-based test suite while making
it usable from a current local Harfang build, without keeping the historical
embedded runtime binaries from the upstream repository.

## Purpose

This suite is a visual and behavioral regression harness for scene physics.

It is useful when:

- validating a Bullet replacement or alternative backend;
- checking backend behavior against the current Bullet integration;
- reproducing edge cases around rigid bodies, raycasts, kinematic bodies,
  collision events, and mesh colliders;
- building a future benchmark and QA baseline for experimental backends such as
  Tau.

## What Is Vendorized

The vendorized snapshot keeps:

- the original top-level Lua test scripts;
- source assets under `assets/`;
- import metadata under `_meta/`;
- images and upstream license;
- the upstream README as [UPSTREAM_README.md](UPSTREAM_README.md).

It intentionally does not keep:

- the upstream `bin/` runtime snapshot;
- the upstream `.vscode/` folder;
- upstream-generated `assets_compiled/` runtime output.

Instead, this folder uses the local Harfang Lua build under
`install/lua/hg_lua`.

## Layout

- `assets/`: editable source assets for the QA scenes.
- `assets_compiled/`: generated runtime assets. Rebuild locally.
- `*.lua`: individual physics QA scenarios.
- `build-assets.bat`: rebuild the QA assets using the local Harfang `assetc`.
- `run-one.bat`: run one Lua scenario with the local `hg_lua` runtime.
- `run-all.bat`: run every top-level Lua scenario sequentially.

## Prerequisites

Expected default local runtime:

- `../../install/lua/hg_lua/lua.exe`
- `../../install/lua/hg_lua/harfang/assetc/assetc.exe`

If your local build is elsewhere, define `HG_LUA_DIR` before running the batch
files.

Example:

```bat
set HG_LUA_DIR=C:\path\to\hg_lua
```

## Usage

Build the assets:

```bat
build-assets.bat
```

Run one scenario:

```bat
run-one.bat rb_dynamic_chair_multi_colbox.lua
```

Run the whole suite:

```bat
run-all.bat
```

## Deterministic Physics Dumps

`rb_dynamic_variable_friction.lua` supports a fixed-step JSON Lines capture for
backend comparisons. The capture records each cuboid's world matrix, linear
velocity, and angular velocity after every physics update.

Capture the Bullet reference and Tau candidate:

```bat
dump-one.bat bullet rb_dynamic_variable_friction.lua
dump-one.bat tau rb_dynamic_variable_friction.lua
python compare_physics_dumps.py qa_dumps\bullet\rb_dynamic_variable_friction.jsonl qa_dumps\tau\rb_dynamic_variable_friction.jsonl
python plot_physics_trajectories.py qa_dumps\bullet\rb_dynamic_variable_friction.jsonl qa_dumps\tau\rb_dynamic_variable_friction.jsonl
```

The captures are written to `qa_dumps/<backend>/<scenario>.jsonl`. The default
run records 600 ticks at 60 Hz and exits automatically. Set
`HG_PHYSICS_QA_DUMP_SAMPLES` or `HG_PHYSICS_QA_DUMP_EVERY` to change the
capture duration or sampling interval.

`plot_physics_trajectories.py` writes `qa_dumps/trajectory_comparison.png` by
default. It renders Bullet trajectories in blue and Tau trajectories in red on
a black 3D plot. Use `--elevation` and `--azimuth` to change the view.

## Notes

- This is still an upstream Lua QA suite, not yet a fully automated pass/fail
  test harness.
- Most scenarios open an interactive window and rely on visual inspection or
  manual exit.
- Most scripts still target `SceneBullet3Physics` directly today.
- `rb_dynamic_chair_multi_colbox.lua` already uses the compile-time
  `ScenePhysics` alias so the same Lua script can run against the selected
  backend without adding runtime selection logic.
