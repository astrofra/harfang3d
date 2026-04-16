# Lua HARFANG Minimal Scene Editor Design

## Purpose

This document defines a minimal Lua-based 3D scene editor built on HARFANG. It uses the C++ `Studio/` editor as an architectural reference, but keeps the first version intentionally small:

- Open a project file and use its parent folder as the project root.
- List the project folder content as the editor's asset browser.
- Load one or more scenes.
- Switch the active scene with a drop-down menu.
- Display the active scene graph as a tree of node names.
- Select one node from the scene graph.
- Transform the selected node with a 2D/3D projected gizmo for translation, rotation, and scale.
- Save the active scene.

The goal is not to recreate HARFANG Studio. The goal is to build a clean Lua foundation that can grow into an editor without inheriting all of Studio's plugin, docking, thumbnail, licensing, and import/export complexity.

## Studio Architecture Takeaways

The C++ Studio codebase is organized around a few useful concepts that should be retained in simplified form.

### Application Shell

`Studio/studio.cpp` owns the application lifecycle:

- Prepare filesystem and data paths.
- Initialize input, windowing, audio, rendering, and ImGui.
- Create global render objects and pipeline resources.
- Register plugins and document openers.
- Run a frame loop that separates scene/content rendering from ImGui UI drawing.
- Shutdown systems in reverse order.

For Lua, keep the same shape:

- One `app` table owns the window, pipeline, resources, input state, and document manager.
- The main loop updates only the active document.
- UI and viewport rendering are explicit phases in each frame.

### Document Model

`Studio/core/document.h` defines a generic `Document` interface. `Studio/core/document_manager.cpp` keeps a list of documents and a current document. The minimal Lua editor only needs one document type, `SceneDocument`, but should still keep a document manager because multi-scene support is part of the initial feature set.

Useful Studio behaviors to keep:

- A document has a path and a dirty state.
- The document manager owns all opened documents.
- Only the current document is active and rendered.
- Save acts on the current document by default.

Behaviors to defer:

- Undo/redo snapshots.
- Close prompts.
- Multi-window document docking.
- Inspector source switching.

### Project Model

`Studio/core/project.*` makes the project file mandatory for editor work. The project file is the entry point to the project folder:

- Opening a `.prj` file sets the project root to the parent folder of that file.
- Project content is listed from that root folder.
- Document paths are stored and displayed relative to the project root.
- Helper functions such as `GetProjectPath` and `GetProjectRelativePath` keep filename/path behavior consistent across the editor.
- Studio hides implementation folders and sidecars such as `_meta`, `_prod`, `_pkgs`, `.git`, `.svn`, `.idea`, `.meta`, and `.editor`.

The Lua editor should keep this rule in the MVP. A project is a folder-based namespace for assets. The `.prj` file can start as an empty file or a small JSON file, but the editor must open it before loading scenes from the project tree.

Minimal Lua project state:

```lua
{
	project_file = "",
	root = "",
	compiled_root = "",
	assetc_path = "",
	db = {},
	selection = {},
	filter = "",
	blacklist = { "_meta", "_prod", "_pkgs", ".git", ".gitignore", ".svn", ".idea" },
	blacklist_suffix = { ".meta", ".editor" },
}
```

Opening a project must run `assetc` on the source project root before scene loading. The compiled output must be written to an editor-owned cache outside the source tree, for example under `hg.GetUserFolder()/HARFANG_Mini_Studio/asset-cache/<project-name>-<hash>`. The hash is based on the absolute `.prj` path so two projects with the same file name do not share a compiled output folder. Source and scene paths remain project-relative so the same document identity maps to both the source path and the compiled asset name.

### SceneDocument

`Studio/core_plugins/scene_document.h` and `.cpp` show that a scene document should own:

- The `hg::Scene`.
- Scene clocks and optional Lua VM.
- The viewport state and editor camera state.
- The selection.
- The scene graph explorer state.
- Save/load logic.
- Editor-only settings stored next to the scene.

For Lua, the first version should own:

- `scene`: `hg.Scene`.
- `path`: the save path or asset name.
- `display_name`: file name shown in the UI.
- `dirty`: whether the scene was modified.
- `selected_node`: current `hg.Node` handle, if valid.
- `camera`: editor camera transform, FOV, z range, and orthographic flag.
- `viewport_rect`: last known ImGui viewport rectangle.
- `gizmo`: transform gizmo state.

### Scene Plugins

Studio uses `SceneDocumentPlugin` to keep scene document responsibilities modular:

- `scene_document_viewport_navigation_plugin.*` owns camera navigation.
- `scene_document_picking_plugin.*` owns picking and selection from the viewport.
- `scene_document_transform_gizmo_plugin.*` owns transform tool state, toolbar UI, viewport locking, and gizmo rendering.
- `scene_document_grid_plugin.*` owns the grid overlay.

Lua should not implement a dynamic plugin system yet, but it should keep the same separation as plain modules:

- `project.lua` owns project open/save, root path resolution, and folder listing.
- `project_view.lua` draws the project tree and opens scene files.
- `scene_graph.lua` draws the tree and updates selection.
- `viewport.lua` owns viewport rect, camera view state, and rendering.
- `gizmo.lua` owns transform tool state, hit testing, dragging, and drawing.
- `scene_io.lua` owns load and save.

This gives the same maintainability benefits without runtime plugin registration.

### Viewport Locking

Studio has a viewport lock mechanism in `SceneDocument`:

- A tool calls `LockViewport(owner_id)` when it starts a drag.
- Other tools skip interaction while the viewport is locked.
- The owner calls `UnlockViewport(owner_id)` when the drag ends.

Keep this in Lua. It prevents camera navigation and gizmo dragging from fighting over the same mouse events.

Minimal API:

```lua
document.viewport_lock = nil

function document:lock_viewport(owner)
	if self.viewport_lock == nil or self.viewport_lock == owner then
		self.viewport_lock = owner
		return true
	end
	return false
end

function document:unlock_viewport(owner)
	if self.viewport_lock == owner then
		self.viewport_lock = nil
		return true
	end
	return false
end
```

## Proposed Lua Module Layout

Suggested root for the editor:

```text
lua_editor/
  main.lua
  editor/
    asset_compiler.lua
    app.lua
    project.lua
    project_view.lua
    document_manager.lua
    scene_document.lua
    scene_io.lua
    scene_graph.lua
    viewport.lua
    camera.lua
    gizmo.lua
    math2d.lua
```

### `main.lua`

Responsibilities:

- Parse command line arguments.
- Create and run `app`.
- Forward the initial project file to `app`.
- Optionally forward initial project-relative scene paths to the document manager after the project is open.
- Shutdown HARFANG systems.

### `editor/asset_compiler.lua`

Responsibilities:

- Derive the default `assetc` executable from the configured Lua runtime path, for example `hg_lua/lua.exe` to `hg_lua/harfang/assetc/assetc.exe`.
- Compute a deterministic compiled cache folder under the user documents folder.
- Hash the absolute `.prj` path to avoid collisions between projects with the same file name.
- Run `assetc <source-root> <compiled-cache-root>` synchronously during project open.
- Return the compiled cache path and compiler log/error to `project.lua`.

### `editor/app.lua`

Responsibilities:

- Initialize `hg.InputInit`, `hg.WindowSystemInit`, `hg.RenderInit`, `hg.ImGuiInit`.
- Add the editor bootstrap asset folder for ImGui/core resources.
- Infer or receive the `assetc` executable path.
- Add the project's temporary compiled asset folder with `hg.AddAssetsFolder` after a project is open.
- Remove the previous project's compiled asset folder when switching projects.
- Create one shared `hg.ForwardPipeline`.
- Create one shared `hg.PipelineResources`.
- Clear shared pipeline resources when switching projects so cached assets from the previous compiled folder do not bleed into the new project.
- Own the open project state.
- Own `DocumentManager`.
- Run the frame loop.

The first version should use shared pipeline resources, matching Studio's global `GetPipelineResources()` pattern. Per-document resources can be introduced later if unloading individual scenes becomes important.

### `editor/project.lua`

Responsibilities:

- Open a `.prj` file.
- Set `project.root` to the parent folder of the `.prj` file.
- Compile `project.root` to `project.compiled_root` using `assetc`.
- Load and save a small project JSON database when the file is non-empty.
- Resolve project-relative paths to absolute file paths.
- Resolve project-relative paths to compiled cache paths.
- Convert absolute file paths back to project-relative paths when they are inside the project.
- List project files recursively or one folder at a time.
- Filter hidden implementation folders and sidecar files.

Minimal project API:

```lua
Project.open(project_file)
Project.compile_assets()
Project.save()
Project.is_open()
Project.path(relative_path)
Project.compiled_path(relative_path)
Project.relative_path(path)
Project.list(relative_folder, filter)
Project.list_recursive(relative_folder, filter)
```

Project path policy:

- Scene documents store `path` as a project-relative path such as `scenes/level.scn`.
- `Project.path(document.path)` is used for file I/O.
- UI labels can use either the base file name or the relative path.
- Assets are listed by scanning the source project root, not by reading a compiled asset database.
- Runtime loads resolve through the temporary compiled asset root.

Default listing policy:

- Directories are shown before files.
- `.scn` files are sorted before other files inside the file group.
- Names are sorted alphabetically after those priorities.
- Blacklisted names and suffixes are hidden.

### `editor/project_view.lua`

Responsibilities:

- Draw the project tree.
- Draw a filter field.
- Select one project file or folder.
- Open `.scn` files on double click.
- Call `DocumentManager.open_scene(relative_path)` for scene files.

The MVP should draw a tree view only. Icon view, thumbnails, drag/drop, tooltips, and source-to-compiled-asset package info should stay deferred.

### `editor/document_manager.lua`

Responsibilities:

- Store `documents = {}`.
- Store `current_index`.
- Open scenes through `scene_io` using project-relative paths.
- Return `current_document()`.
- Draw the scene selector drop-down.
- Save the current document.

Minimal document manager API:

```lua
DocumentManager.open_scene(project_relative_path)
DocumentManager.open_scenes(project_relative_paths)
DocumentManager.current()
DocumentManager.draw_scene_selector()
DocumentManager.save_current()
```

### `editor/scene_document.lua`

Responsibilities:

- Build the Lua `SceneDocument` table.
- Own the editor-only state for one scene.
- Expose methods used by UI and tools:

```lua
document:get_selected_node()
document:set_selected_node(node)
document:clear_selection()
document:mark_dirty(reason)
document:lock_viewport(owner)
document:unlock_viewport(owner)
document:is_viewport_locked()
```

Initial fields:

```lua
{
	path = "",
	display_name = "",
	scene = hg.Scene(),
	dirty = false,
	selected_node = hg.NullNode,
	viewport_rect = { x = 0, y = 0, w = 1, h = 1 },
	editor_camera = {
		world = hg.TransformationMat4(hg.Vec3(-2.18, 1.16, -3.2), hg.Deg3(17, 34, 0)),
		fov = hg.Deg(45),
		znear = 0.01,
		zfar = 1000.0,
		orthographic = false,
		orthographic_size = 1.0,
	},
	gizmo = Gizmo.new(),
	viewport_lock = nil,
}
```

### `editor/scene_io.lua`

Responsibilities:

- Load scenes from the open project.
- Save scenes to disk.
- Normalize scene display names.
- Reset stale editor state after reload.

Relevant Lua bindings observed in `binding/bind_harfang.py`:

- `hg.LoadSceneFromFile(path, scene, resources, hg.GetForwardPipelineInfo(), flags)`
- `hg.LoadSceneFromAssets(name, scene, resources, hg.GetForwardPipelineInfo(), flags)`
- `hg.SaveSceneJsonToFile(path, scene, resources, flags)`
- `hg.SaveSceneBinaryToFile(path, scene, resources, flags)`
- `hg.LSSF_All`
- `hg.LSSF_QueueResourceLoads`

Default policy:

- Require an open project before loading scenes.
- Treat scene document paths as project-relative paths.
- Validate that the source scene exists at `Project.path(relative_path)` so saves and project tree operations still target the original asset.
- Require the compiled scene to exist at `Project.compiled_path(relative_path)`.
- Load with `LoadSceneFromAssets(relative_path, ...)` after `project.compiled_root` has been added with `hg.AddAssetsFolder`.
- Do not fall back to `LoadSceneFromFile` for project scenes; HARFANG scene runtime loads must use compiled assets.
- Save with `SaveSceneJsonToFile` for inspectable output.
- Save to `Project.path(relative_path)` in the original source project folder.
- Mark the document clean only after a successful save.

### Project Flow

UI:

- `File > Open Project...`
- `File > Save Project`
- Project tree panel with a filter field.
- Double-clicking a `.scn` file opens it as a scene document.

Open operation:

1. Validate that the selected file exists and has a `.prj` extension.
2. Resolve the absolute `.prj` path and set the source project root to its parent directory.
3. Compute the compiled cache directory under the user documents folder using a hash of the absolute `.prj` path.
4. Run `assetc <source-root> <compiled-cache-root>`.
5. If compilation fails, keep the previous project open and show the asset compiler error.
6. Load JSON if the file is non-empty; otherwise accept it as a legacy empty project file.
7. Set `project.project_file`, `project.root`, and `project.compiled_root`.
8. Remove the previous project's compiled asset folder from HARFANG's asset lookup.
9. Add the new `project.compiled_root` as an asset folder.
10. Clear the project listing cache, open documents, and shared pipeline resources.

Save operation:

1. Serialize `project.db`.
2. Save it back to `project.project_file`.
3. Do not save open scene documents implicitly unless the command is `Save All`.
4. Do not write into the compiled cache. It is derived data and is refreshed by reopening or recompiling the project.

Project listing:

1. List entries from `Project.path(relative_folder)`.
2. Drop blacklisted folders and sidecars.
3. Apply the text filter to files.
4. Sort directories first, then `.scn` files, then other files.
5. Keep paths project-relative when emitting UI selection or open events.

### `editor/scene_graph.lua`

Responsibilities:

- Draw a tree for the active document.
- Display node names.
- Select exactly one node.
- Clear selection when the root scene item is clicked.

Lua bindings expose:

- `scene:GetNodes()`
- `scene:GetNodeChildren(node)`
- `scene:IsRoot(node)`
- `node:GetName()`
- `node:GetUid()`
- `node:IsValid()`
- `node:HasTransform()`

Tree algorithm:

1. Draw a root item named `Scene`.
2. Iterate `scene:GetNodes()`.
3. For each node where `scene:IsRoot(node)` is true, recursively draw it.
4. Use `scene:GetNodeChildren(node)` for recursion.
5. Use `node:GetName()` as the user-facing label.
6. Use `node:GetUid()` as the ImGui ID suffix to avoid duplicate-name collisions.
7. If a node is clicked, call `document:set_selected_node(node)`.

Selection storage:

- Store the selected `hg.Node` handle.
- Clear it when a load operation replaces the scene.
- Each frame, validate `selected_node:IsValid()` before using it.

### `editor/viewport.lua`

Responsibilities:

- Reserve the viewport area in ImGui.
- Store the active viewport rectangle in the document.
- Compute `hg.ViewState`.
- Update the active scene.
- Submit the active scene to the forward pipeline.
- Call `gizmo.update` and `gizmo.draw`.

Studio renders a scene document to an off-screen framebuffer and displays the color texture in ImGui. The first Lua version can be simpler:

- Draw the viewport as an ImGui child/invisible region.
- Render the scene directly to the backbuffer using the computed viewport rectangle.
- Draw ImGui after the scene so panels appear above the viewport.

If Lua bindings for framebuffer creation and `hg.ImGuiImage` are sufficient in the target runtime, a later pass can switch to Studio's off-screen framebuffer pattern.

Frame sequence:

1. Poll keyboard and mouse.
2. Compute `dt`.
3. Start ImGui frame.
4. Draw the main menu, current project label, and scene selector.
5. Draw a project tree panel.
6. Draw the active scene graph panel.
7. Draw the viewport panel and capture its rectangle.
8. Update and render the current scene into that rectangle.
9. Draw gizmo overlay.
10. End ImGui frame.
11. Call `bgfx.frame()` and `hg.UpdateWindow(window)`.

Rendering sequence:

```lua
local scene = document.scene
scene:Update(dt)

local aspect = hg.ComputeAspectRatioX(document.viewport_rect.w, document.viewport_rect.h)
local view_state = camera.compute_view_state(document.editor_camera, aspect)

local view_id = 0
local views = hg.SceneForwardPipelinePassViewId()
hg.SubmitSceneToPipeline(
	view_id,
	scene,
	hg.iRect(document.viewport_rect.x, document.viewport_rect.y, document.viewport_rect.x + document.viewport_rect.w, document.viewport_rect.y + document.viewport_rect.h),
	view_state,
	app.pipeline,
	app.resources,
	views
)
```

Adjust the exact Lua call signature to the generated HARFANG Lua runtime in use, since `bgfx::ViewId` in/out bindings may return updated values depending on the binding generator.

### `editor/camera.lua`

Responsibilities:

- Store editor camera state.
- Compute `hg.ViewState`.
- Provide orbit/pan/dolly later.

MVP camera behavior:

- Use a fixed perspective camera if the scene has no current camera.
- Optionally use `scene:GetCurrentCamera():ComputeCameraViewState(aspect)` when a scene camera is valid.
- Defer full Studio-like viewport navigation until after the transform gizmo works.

### `editor/gizmo.lua`

Responsibilities:

- Store the active tool: `translate`, `rotate`, or `scale`.
- Store the transform space: `local` or `world`.
- Hit-test visible handles.
- Start a drag only if the viewport lock is available.
- Apply transform deltas to the selected node.
- Mark the document dirty when a transform changes.
- Draw the gizmo as a 2D/3D overlay.

Studio's transform gizmo is split across:

- `Studio/core/gizmo_transform.*`: math and drawing helpers.
- `Studio/core_plugins/scene_document_transform_gizmo_plugin.*`: tool state, shortcuts, snapping, viewport lock, transform application.

Lua should follow the same separation inside a single module at first:

```lua
local Gizmo = {
	tool = "translate",
	space = "local",
	active_handle = nil,
	dragging = false,
	start_mouse = nil,
	start_world = nil,
	start_trs = nil,
}
```

#### Gizmo View Matrix

For one selected node:

- `gizmo_world = strip_scale(selected_node:GetTransform():GetWorld())`
- In local mode, use the node rotation.
- In world mode, use identity rotation at the node world position.

For MVP, support one selected node only. Multi-selection can be added later by using Studio's centroid-matrix approach.

#### Gizmo Scale

Studio keeps gizmo size stable on screen:

- `ComputeGizmoScale(proj)` returns a projection-dependent scale.
- The plugin then applies camera distance and viewport height.

Lua MVP:

- Use a constant screen-space gizmo length first, based on projected 2D points.
- Later port the Studio scale formula:

```text
scale = (is_ortho and 1.0 or distance(camera_pos, gizmo_pos)) * constant / viewport_height
```

#### Projection Helpers

Lua bindings expose:

- `hg.ProjectToScreenSpace`
- `hg.ProjectOrthoToScreenSpace`
- `hg.UnprojectFromScreenSpace`
- `hg.UnprojectOrthoFromScreenSpace`
- `hg.Compute2DProjectionMatrix`

Use these to bridge between 3D node transforms and 2D mouse interaction.

#### Translation Tool

MVP handles:

- X axis.
- Y axis.
- Z axis.

Hit testing:

1. Project the gizmo origin and each axis endpoint to screen space.
2. Compute mouse distance to each projected segment.
3. Select the closest segment below a pixel threshold.

Dragging:

1. At drag start, store selected node world matrix and mouse position.
2. Convert the current mouse position to a world ray with `UnprojectFromScreenSpace`.
3. Use a drag plane that contains the active axis and faces the camera as much as possible.
4. Intersect the start and current rays with the drag plane.
5. Project the delta onto the active axis.
6. Apply `transform:SetWorld(new_world)`.

This is simpler than Studio's full axis/plane lock mask, but keeps the same conceptual model.

#### Rotation Tool

MVP handles:

- One screen-space rotation ring around the selected node.
- Optional X/Y/Z rings later.

Hit testing:

1. Project gizmo center.
2. Measure mouse distance to the ring radius.
3. Select the ring when the distance is within a threshold.

Dragging:

1. Store the vector from projected center to start mouse.
2. Compute the signed angle to the current vector.
3. Rotate around the camera forward axis for the screen ring.
4. Later add local X/Y/Z rings by using the node's local axes.

#### Scale Tool

MVP handles:

- Uniform center handle.
- Optional X/Y/Z axis scale handles later.

Dragging:

1. Store the selected node local scale at drag start.
2. Use vertical mouse delta or distance from center to compute a scale factor.
3. Clamp to a small positive minimum.
4. Apply `transform:SetScale(new_scale)`.

#### Drawing

Prefer a late overlay view:

- Use one bgfx view after the scene view.
- Use `hg.Compute2DProjectionMatrix` for 2D overlay drawing, or draw 3D line primitives in the scene view.
- X/Y/Z colors should match Studio convention: X red, Y green, Z blue, active handle yellow/white.

If ImGui draw-list bindings are available in the runtime, drawing projected 2D lines and circles through ImGui is the fastest MVP path. If they are not available, draw the gizmo as HARFANG line primitives.

### Save Flow

UI:

- `File > Open Project...`
- `File > Save Project`
- `File > Save Scene`
- `File > Save As...` can be deferred.
- Scene selector drop-down near the top of the window.

Save operation:

1. Get the current document.
2. Validate that a project is open and that the document has a project-relative `path`.
3. Resolve the absolute path with `Project.path(document.path)`.
4. Call `hg.SaveSceneJsonToFile(absolute_path, document.scene, app.resources, hg.LSSF_All)`.
5. On success, set `document.dirty = false`.
6. On failure, show a modal or log line.

Dirty tracking:

- Set dirty when the gizmo applies a transform.
- Set dirty when a future tree rename, delete, create, or inspector edit changes the scene.
- Do not set dirty on camera navigation, scene selector changes, or viewport state changes.

## Initial UI Layout

Keep the UI fixed and simple:

```text
+--------------------------------------------------+
| File menu | Project: my_project | Scene: [scene]  |
+----------------------+---------------------------+
| Project              | Viewport                  |
| - scenes/            |                           |
|   - level.scn        |        3D scene           |
| Scene Graph          |        + gizmo            |
| - Scene              |                           |
|   - Node A           |                           |
|   - Node B           |                           |
+----------------------+---------------------------+
| Status line: selected node, dirty/save state      |
+--------------------------------------------------+
```

Do not implement docking in the first version. Studio's docking is useful but not necessary for the minimal tool.

## Minimal Implementation Milestones

### Milestone 1: App Shell

- Initialize HARFANG window, input, rendering, and ImGui.
- Create shared `ForwardPipeline` and `PipelineResources`.
- Draw a menu bar and empty three-area layout: project tree, scene graph, viewport.

Exit criteria:

- A blank editor window opens.
- ImGui receives mouse and keyboard input.
- The app closes cleanly.

### Milestone 2: Project Opening and Project Tree

- Implement `Project`.
- Open a `.prj` file.
- Set the project root from the `.prj` parent folder.
- Draw a project tree that lists files and folders.
- Hide `_meta`, `_prod`, `_pkgs`, VCS folders, `.meta`, and `.editor`.
- Filter the project tree by text.

Exit criteria:

- A project can be opened.
- The project tree lists the project's asset files with project-relative paths.
- Double-clicking a non-scene file does not crash or alter the current document.

### Milestone 3: Scene Loading and Switching

- Implement `DocumentManager`.
- Load one or more `.scn` files from the project tree.
- Store one `SceneDocument` per scene.
- Draw the scene selector drop-down.
- Render only the active scene.

Exit criteria:

- Two scenes can be loaded.
- Switching the drop-down changes the rendered scene.

### Milestone 4: Scene Graph Tree

- Draw root nodes and child nodes recursively.
- Select one node from the tree.
- Highlight the selected node.
- Show selected node name and UID in the status line.

Exit criteria:

- Clicking a node in the tree changes the selected node.
- Selection is cleared when the scene is reloaded or closed.

### Milestone 5: Translation Gizmo

- Draw a projected X/Y/Z translation gizmo at the selected node.
- Hit-test the projected axis lines.
- Drag along one axis.
- Apply the result to `node:GetTransform():SetWorld(...)`.
- Mark the document dirty.

Exit criteria:

- A selected node can be moved with the mouse.
- Camera and gizmo interaction do not conflict because of viewport locking.

### Milestone 6: Rotation and Scale Gizmos

- Add tool selection buttons or shortcuts:
  - `W`: translate.
  - `E`: rotate.
  - `R`: scale.
- Add screen-space rotation ring.
- Add uniform scale handle.

Exit criteria:

- A selected node can be translated, rotated, and scaled.
- The active tool is visible in the toolbar/status line.

### Milestone 7: Save Project and Scene

- Implement `File > Save Project`.
- Implement `File > Save Scene`.
- Call `hg.SaveSceneJsonToFile`.
- Clear the dirty flag after successful save.
- Keep the scene selector label marked with `*` while dirty.

Exit criteria:

- Project settings are written back to the opened `.prj` file.
- Transform a node, save, reopen the project so the temporary compiled cache is refreshed, reopen the scene, and verify the transform persisted.

## Deferred Work

Do not include these in the initial build:

- Undo/redo snapshots.
- Multi-node selection.
- Viewport object picking.
- Project icon view, thumbnails, file drag/drop, and file type inspectors.
- Scene graph drag/drop reparenting.
- Node create/delete/rename.
- Inspector editing.
- Material editing.
- Animation controls.
- Scene instance editing.
- Grid, camera icons, light icons, and thumbnails.
- AAA pipeline settings.
- Plugin loading.

## Risks and Notes

- HARFANG Lua binding signatures can differ slightly from C++ because some `bgfx::ViewId &` values are generated as in/out values. Validate the exact runtime call pattern with a tiny render script before implementing all modules.
- The `.prj` file is part of the MVP. It defines the project folder and should be opened before scene files so scene document paths stay project-relative.
- Keep project root resolution independent from compiled asset output. The editor lists and saves source files from the project folder, while runtime scene loading maps the same project-relative paths to the temporary compiled cache.
- If direct backbuffer rendering into an ImGui-defined viewport causes ordering or clipping issues, switch to Studio's framebuffer-to-ImGui-image model.
- Store editor-only settings in a Lua-specific sidecar file only after the MVP works, for example `<scene>.lua_editor.json`. Avoid writing Studio's `.editor` files unless compatibility with C++ Studio is explicitly required.
- Node handles should be treated as scene-local and cleared after reload. If selection must survive reload later, store the selected node UID and resolve it by scanning `scene:GetAllNodes()`.
- The first gizmo should optimize for predictable behavior, not full feature parity. Studio's `gizmo_transform` code can be ported incrementally once the Lua editor loop is stable.
