local hg = require("harfang")

local Project = require("editor.project")
local ProjectView = require("editor.project_view")
local DocumentManager = require("editor.document_manager")
local SceneGraph = require("editor.scene_graph")
local Viewport = require("editor.viewport")

local App = {}
App.__index = App

local function path_join(a, b)
	if a:sub(-1) == "/" or a:sub(-1) == "\\" then
		return a .. b
	end
	return a .. "/" .. b
end

local function normalize(path)
	return hg.NormalizePath(hg.CleanPath(path)):gsub("\\", "/")
end

local function add_assets_folder_if_present(path)
	path = normalize(path)
	if hg.Exists(path) and hg.IsDir(path) then
		hg.AddAssetsFolder(path)
		return true
	end
	return false
end

function App.new(config)
	config = config or {}
	local self = setmetatable({}, App)
	self.script_dir = config.script_dir or "."
	self.initial_project = config.initial_project
	self.initial_scenes = config.initial_scenes or {}
	self.lua_binary = config.lua_binary or ""
	self.res_x = 1280
	self.res_y = 800
	self.render_flags = hg.RF_VSync | hg.RF_MSAA4X
	self.window = nil
	self.pipeline = nil
	self.resources = nil
	self.project = Project.new()
	self.documents = DocumentManager.new(self.project, nil)
	self.keyboard = hg.Keyboard()
	self.mouse = hg.Mouse()
	self.status = ""
	self.open_project_popup_requested = false
	self.open_project_path = path_join(self.script_dir, "assets/project.prj")
	self.clear_color = hg.Color(0.08, 0.085, 0.095, 1)
	return self
end

function App:resource_candidates()
	local cwd = hg.GetCurrentWorkingDirectory():gsub("\\", "/")
	local script_dir = self.script_dir:gsub("\\", "/")
	return {
		path_join(script_dir, "assets"),
		"resources_compiled",
		"tutorials/resources_compiled",
		path_join(cwd, "tutorials/resources_compiled"),
		path_join(path_join(script_dir, ".."), "tutorials/resources_compiled"),
	}
end

function App:init_systems()
	hg.InputInit()
	hg.WindowSystemInit()

	self.window = hg.RenderInit("HARFANG Mini Studio", self.res_x, self.res_y, self.render_flags)

	for _, candidate in ipairs(self:resource_candidates()) do
		add_assets_folder_if_present(candidate)
	end

	self.pipeline = hg.CreateForwardPipeline()
	self.resources = hg.PipelineResources()
	self.documents:set_resources(self.resources)

	local imgui_prg = hg.LoadProgramFromAssets("core/shader/imgui")
	local imgui_img_prg = hg.LoadProgramFromAssets("core/shader/imgui_image")
	hg.ImGuiInit(10, imgui_prg, imgui_img_prg)
end

function App:shutdown_systems()
	hg.ImGuiShutdown()
	if self.pipeline ~= nil then
		hg.DestroyForwardPipeline(self.pipeline)
	end
	hg.RenderShutdown()
	if self.window ~= nil then
		hg.DestroyWindow(self.window)
	end
	hg.WindowSystemShutdown()
	hg.InputShutdown()
end

function App:open_project(path)
	if self.project:open(path) then
		if self.project.compiled_root ~= nil and self.project.compiled_root ~= "" then
			add_assets_folder_if_present(self.project.compiled_root)
		else
			add_assets_folder_if_present(self.project.root)
		end
		self.documents:clear()
		self.status = "Opened project: " .. self.project.project_file
		return true
	end

	self.status = self.project.last_error
	return false
end

function App:request_open_project_popup()
	if self.project:is_open() then
		self.open_project_path = self.project.project_file
	end
	self.open_project_popup_requested = true
end

function App:draw_open_project_popup()
	if self.open_project_popup_requested then
		hg.ImGuiOpenPopup("Open Project")
		self.open_project_popup_requested = false
	end

	if hg.ImGuiBeginPopup("Open Project") then
		hg.ImGuiTextWrapped("Enter a .prj file path.")
		hg.ImGuiPushItemWidth(560)
		local submit
		submit, self.open_project_path = hg.ImGuiInputText("Project", self.open_project_path, 4096, hg.ImGuiInputTextFlags_EnterReturnsTrue)
		hg.ImGuiPopItemWidth()

		if hg.ImGuiButton("Open") or submit then
			if self:open_project(self.open_project_path) then
				hg.ImGuiCloseCurrentPopup()
			end
		end
		hg.ImGuiSameLine()
		if hg.ImGuiButton("Default") then
			self.open_project_path = path_join(self.script_dir, "assets/project.prj")
		end
		hg.ImGuiSameLine()
		if hg.ImGuiButton("Cancel") then
			hg.ImGuiCloseCurrentPopup()
		end
		hg.ImGuiEndPopup()
	end
end

function App:draw_menu()
	if hg.ImGuiBeginMainMenuBar() then
		if hg.ImGuiBeginMenu("File") then
			if hg.ImGuiMenuItem("Open Project...", "Ctrl+O", false, true) then
				self:request_open_project_popup()
			end
			if hg.ImGuiMenuItem("Save Project", "", false, self.project:is_open()) then
				if self.project:save() then
					self.status = "Saved project."
				else
					self.status = self.project.last_error
				end
			end
			if hg.ImGuiMenuItem("Save Scene", "Ctrl+S", false, self.documents:current() ~= nil) then
				if self.documents:save_current() then
					self.status = "Saved scene."
				else
					self.status = self.documents.last_error
				end
			end
			hg.ImGuiEndMenu()
		end

		hg.ImGuiText("Project:")
		hg.ImGuiSameLine()
		if self.project:is_open() then
			hg.ImGuiText(self.project.root)
		else
			hg.ImGuiText("none")
		end
		hg.ImGuiSameLine()
		self.documents:draw_scene_selector()

		hg.ImGuiEndMainMenuBar()
	end
end

function App:handle_global_shortcuts()
	local ctrl = self.keyboard:Down(hg.K_LCtrl) or self.keyboard:Down(hg.K_RCtrl)
	if ctrl and self.keyboard:Pressed(hg.K_S) then
		if self.documents:save_current() then
			self.status = "Saved scene."
		else
			self.status = self.documents.last_error
		end
	end
end

function App:update_window_size()
	local render_was_reset
	render_was_reset, self.res_x, self.res_y = hg.RenderResetToWindow(self.window, self.res_x, self.res_y, self.render_flags)
	return render_was_reset
end

function App:draw_project_window(menu_h, status_h, side_w)
	local work_h = self.res_y - menu_h - status_h
	local project_h = math.floor(work_h * 0.48)
	hg.ImGuiSetNextWindowPos(hg.Vec2(0, menu_h), hg.ImGuiCond_Always)
	hg.ImGuiSetNextWindowSize(hg.Vec2(side_w, project_h), hg.ImGuiCond_Always)
	if hg.ImGuiBegin("Project", true, hg.ImGuiWindowFlags_NoMove | hg.ImGuiWindowFlags_NoResize | hg.ImGuiWindowFlags_NoCollapse) then
		ProjectView.draw(self.project, self.documents)
	end
	hg.ImGuiEnd()
end

function App:draw_scene_graph_window(menu_h, status_h, side_w)
	local work_h = self.res_y - menu_h - status_h
	local project_h = math.floor(work_h * 0.48)
	hg.ImGuiSetNextWindowPos(hg.Vec2(0, menu_h + project_h), hg.ImGuiCond_Always)
	hg.ImGuiSetNextWindowSize(hg.Vec2(side_w, work_h - project_h), hg.ImGuiCond_Always)
	if hg.ImGuiBegin("Scene Graph", true, hg.ImGuiWindowFlags_NoMove | hg.ImGuiWindowFlags_NoResize | hg.ImGuiWindowFlags_NoCollapse) then
		SceneGraph.draw(self.documents:current())
	end
	hg.ImGuiEnd()
end

function App:draw_viewport_window(menu_h, status_h, side_w)
	hg.ImGuiSetNextWindowPos(hg.Vec2(side_w, menu_h), hg.ImGuiCond_Always)
	hg.ImGuiSetNextWindowSize(hg.Vec2(self.res_x - side_w, self.res_y - menu_h - status_h), hg.ImGuiCond_Always)
	if hg.ImGuiBegin("Viewport", true, hg.ImGuiWindowFlags_NoMove | hg.ImGuiWindowFlags_NoResize | hg.ImGuiWindowFlags_NoCollapse) then
		Viewport.draw_panel(self.documents:current(), self.keyboard)
	end
	hg.ImGuiEnd()
end

function App:draw_status_window(status_h)
	hg.ImGuiSetNextWindowPos(hg.Vec2(0, self.res_y - status_h), hg.ImGuiCond_Always)
	hg.ImGuiSetNextWindowSize(hg.Vec2(self.res_x, status_h), hg.ImGuiCond_Always)
	if hg.ImGuiBegin("Status", true, hg.ImGuiWindowFlags_NoTitleBar | hg.ImGuiWindowFlags_NoMove | hg.ImGuiWindowFlags_NoResize | hg.ImGuiWindowFlags_NoCollapse | hg.ImGuiWindowFlags_NoScrollbar) then
		local doc = self.documents:current()
		if doc ~= nil then
			local node = doc:get_selected_node()
			local selected = "Selected: none"
			if node:IsValid() then
				selected = string.format("Selected: %s (%d)", node:GetName(), node:GetUid())
			end
			local dirty = doc.dirty and "dirty" or "clean"
			hg.ImGuiText(string.format("%s | Scene: %s | %s | %s", selected, doc.path, dirty, self.status or ""))
		else
			hg.ImGuiText(self.status ~= "" and self.status or "No scene open.")
		end
	end
	hg.ImGuiEnd()
end

function App:draw_ui()
	local menu_h = 24
	local status_h = 28
	local side_w = 340

	self:draw_menu()
	self:draw_project_window(menu_h, status_h, side_w)
	self:draw_scene_graph_window(menu_h, status_h, side_w)
	self:draw_viewport_window(menu_h, status_h, side_w)
	self:draw_status_window(status_h)
	self:draw_open_project_popup()
end

function App:open_initial_documents()
	if self.initial_project ~= nil and self.initial_project ~= "" then
		if self:open_project(self.initial_project) then
			self.documents:open_scenes(self.initial_scenes)
			if self.documents.last_error ~= "" then
				self.status = self.documents.last_error
			end
		end
	end
end

function App:run()
	self:init_systems()
	self:open_initial_documents()

	while not self.keyboard:Pressed(hg.K_Escape) and hg.IsWindowOpen(self.window) do
		self:update_window_size()
		self.mouse:Update()
		self.keyboard:Update()

		local dt = hg.TickClock()
		self:handle_global_shortcuts()

		hg.ImGuiBeginFrame(self.res_x, self.res_y, dt, self.mouse:GetState(), self.keyboard:GetState())
		self:draw_ui()

		hg.SetView2D(0, 0, 0, self.res_x, self.res_y, -1, 1, hg.CF_Color | hg.CF_Depth, self.clear_color, 1, 0)
		Viewport.render_document(self, self.documents:current(), dt)
		hg.ImGuiEndFrame(255)

		hg.Frame()
		hg.UpdateWindow(self.window)
	end

	self:shutdown_systems()
end

return App
