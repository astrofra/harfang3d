# HARFANG Mini Studio

First Lua iteration of `specifications/LUA_HARFANG_EDITOR_DESIGN.md`.

Run from the repository root:

```bat
mini-studio\run.bat mini-studio\assets\project.prj
```

The first argument should be a `.prj` file. Extra arguments are project-relative scene paths. If no project is passed, the launcher tries `mini-studio/assets/project.prj`.

`mini-studio/assets/core` is mounted as the local HARFANG runtime asset root for ImGui and pipeline shaders.

Implemented in this first pass:

- HARFANG window, rendering, input, and ImGui app shell.
- Project opening from `.prj` files, with project-root path helpers.
- Filtered project tree, hiding Studio implementation folders and sidecars.
- Scene document manager with multi-scene loading and switching.
- Scene graph tree with single-node selection.
- Direct viewport rendering for the active scene.
- Projected 2D gizmo overlay for translation, rotation, and uniform scale.
- Save Project and Save Scene commands.

Deferred items remain the larger Studio features from the specification: asset compiler integration, docking, undo/redo, inspectors, scene editing commands, and thumbnail/icon views.
