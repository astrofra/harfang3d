local hg = require("harfang")
local Gizmo = require("editor.gizmo")

local SceneDocument = {}
SceneDocument.__index = SceneDocument

local function display_name(path)
	local name = hg.GetFileName(path)
	if name == nil or name == "" then
		return path
	end
	return name
end

function SceneDocument.new(path, scene)
	local self = setmetatable({}, SceneDocument)
	self.path = path or ""
	self.display_name = display_name(self.path)
	self.scene = scene or hg.Scene()
	self.dirty = false
	self.dirty_reason = ""
	self.selected_node = hg.NullNode
	self.viewport_rect = { x = 0, y = 0, w = 1, h = 1 }
	self.editor_camera = {
		world = hg.TransformationMat4(hg.Vec3(-2.18, 1.16, -3.2), hg.Deg3(17, 34, 0)),
		fov = hg.Deg(45),
		znear = 0.01,
		zfar = 1000.0,
		orthographic = false,
		orthographic_size = 1.0,
	}
	self.gizmo = Gizmo.new()
	self.viewport_lock = nil
	return self
end

function SceneDocument:get_selected_node()
	if self.selected_node ~= nil and self.selected_node:IsValid() then
		return self.selected_node
	end
	self.selected_node = hg.NullNode
	return self.selected_node
end

function SceneDocument:set_selected_node(node)
	if node ~= nil and node:IsValid() then
		self.selected_node = node
	else
		self:clear_selection()
	end
end

function SceneDocument:clear_selection()
	self.selected_node = hg.NullNode
	self.gizmo:cancel_drag(self)
end

function SceneDocument:mark_dirty(reason)
	self.dirty = true
	self.dirty_reason = reason or "modified"
end

function SceneDocument:lock_viewport(owner)
	if self.viewport_lock == nil or self.viewport_lock == owner then
		self.viewport_lock = owner
		return true
	end
	return false
end

function SceneDocument:unlock_viewport(owner)
	if self.viewport_lock == owner then
		self.viewport_lock = nil
		return true
	end
	return false
end

function SceneDocument:is_viewport_locked(owner)
	if owner ~= nil then
		return self.viewport_lock ~= nil and self.viewport_lock ~= owner
	end
	return self.viewport_lock ~= nil
end

return SceneDocument
