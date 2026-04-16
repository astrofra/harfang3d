local hg = require("harfang")
local SceneDocument = require("editor.scene_document")

local SceneIO = {}

local function scene_flags()
	local flags = hg.LSSF_All
	if hg.LSSF_QueueResourceLoads ~= nil then
		flags = flags | hg.LSSF_QueueResourceLoads
	end
	return flags
end

function SceneIO.load(project, relative_path, resources)
	if not project:is_open() then
		return nil, "Open a project before loading scenes."
	end

	if relative_path == nil or relative_path == "" then
		return nil, "No scene path provided."
	end

	local absolute_path = project:path(relative_path)
	if not hg.Exists(absolute_path) or not hg.IsFile(absolute_path) then
		return nil, "Scene file does not exist: " .. absolute_path
	end

	if project.compiled_root == nil or project.compiled_root == "" then
		return nil, "Project assets are not compiled. Reopen the project to compile them."
	end

	local compiled_path = project:compiled_path(relative_path)
	if not hg.Exists(compiled_path) or not hg.IsFile(compiled_path) then
		return nil, "Compiled scene file does not exist: " .. compiled_path
	end

	local scene = hg.Scene()
	local ok = hg.LoadSceneFromAssets(relative_path, scene, resources, hg.GetForwardPipelineInfo(), scene_flags())
	if not ok then
		return nil, "Failed to load scene: " .. relative_path
	end

	scene:ReadyWorldMatrices()
	local doc = SceneDocument.new(relative_path, scene)
	doc:clear_selection()
	return doc, nil
end

function SceneIO.save(project, document, resources)
	if not project:is_open() then
		return false, "Open a project before saving scenes."
	end
	if document == nil then
		return false, "No scene document is active."
	end
	if document.path == nil or document.path == "" then
		return false, "The scene document has no project-relative path."
	end

	local absolute_path = project:path(document.path)
	local ok = hg.SaveSceneJsonToFile(absolute_path, document.scene, resources, hg.LSSF_All)
	if not ok then
		return false, "Failed to save scene: " .. absolute_path
	end

	document.dirty = false
	document.dirty_reason = ""
	return true, nil
end

return SceneIO
