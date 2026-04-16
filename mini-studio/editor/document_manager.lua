local hg = require("harfang")
local SceneIO = require("editor.scene_io")

local DocumentManager = {}
DocumentManager.__index = DocumentManager

function DocumentManager.new(project, resources)
	local self = setmetatable({}, DocumentManager)
	self.project = project
	self.resources = resources
	self.documents = {}
	self.current_index = 0
	self.last_error = ""
	return self
end

function DocumentManager:set_resources(resources)
	self.resources = resources
end

function DocumentManager:clear()
	self.documents = {}
	self.current_index = 0
	self.last_error = ""
end

function DocumentManager:find_by_path(path)
	for i, document in ipairs(self.documents) do
		if document.path == path then
			return i, document
		end
	end
	return nil, nil
end

function DocumentManager:open_scene(project_relative_path)
	local existing_index = self:find_by_path(project_relative_path)
	if existing_index ~= nil then
		self.current_index = existing_index
		return self.documents[existing_index]
	end

	local document, err = SceneIO.load(self.project, project_relative_path, self.resources)
	if document == nil then
		self.last_error = err or "Failed to open scene."
		return nil
	end

	table.insert(self.documents, document)
	self.current_index = #self.documents
	self.last_error = ""
	return document
end

function DocumentManager:open_scenes(project_relative_paths)
	for _, path in ipairs(project_relative_paths or {}) do
		self:open_scene(path)
	end
end

function DocumentManager:current()
	if self.current_index <= 0 or self.current_index > #self.documents then
		return nil
	end
	return self.documents[self.current_index]
end

function DocumentManager:labels()
	local labels = {}
	for _, document in ipairs(self.documents) do
		local label = document.display_name
		if document.dirty then
			label = label .. " *"
		end
		table.insert(labels, label)
	end
	return labels
end

function DocumentManager:draw_scene_selector()
	if #self.documents == 0 then
		hg.ImGuiText("Scene: none")
		return
	end

	local index = math.max(0, self.current_index - 1)
	local changed, new_index = hg.ImGuiCombo("Scene", index, self:labels())
	if changed then
		self.current_index = new_index + 1
	end
end

function DocumentManager:save_current()
	local document = self:current()
	local ok, err = SceneIO.save(self.project, document, self.resources)
	if not ok then
		self.last_error = err or "Failed to save scene."
		return false
	end
	self.last_error = ""
	return true
end

return DocumentManager
