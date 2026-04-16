local hg = require("harfang")
local math2d = require("editor.math2d")

local Gizmo = {}
Gizmo.__index = Gizmo

local OWNER = "transform_gizmo"

local axis_defs = {
	{ name = "x", color = hg.Color(0.95, 0.18, 0.16, 1.0), world = function(m) return hg.Normalize(hg.GetX(m)) end },
	{ name = "y", color = hg.Color(0.18, 0.78, 0.28, 1.0), world = function(m) return hg.Normalize(hg.GetY(m)) end },
	{ name = "z", color = hg.Color(0.24, 0.45, 1.0, 1.0), world = function(m) return hg.Normalize(hg.GetZ(m)) end },
}

local active_color = hg.Color(1.0, 0.9, 0.25, 1.0)
local white = hg.Color(1.0, 1.0, 1.0, 1.0)
local panel_bg = hg.Color(0.04, 0.05, 0.06, 0.72)

local function color_u32(color)
	return hg.ImGuiGetColorU32(color)
end

local function project_point(view_state, rect, world_pos)
	local view_pos = view_state.view * world_pos
	local ok, screen = hg.ProjectToScreenSpace(view_state.proj, view_pos, hg.Vec2(rect.w, rect.h))
	if not ok then
		return nil
	end
	return hg.Vec2(rect.x + screen.x, rect.y + screen.y)
end

local function selected_transform(document)
	local node = document:get_selected_node()
	if not node:IsValid() or not node:HasTransform() then
		return nil, nil
	end
	return node, node:GetTransform()
end

local function set_world_translation(transform, position)
	local world = transform:GetWorld()
	hg.SetT(world, position)
	transform:SetWorld(world)
end

local function scale_from_camera(view_state, position)
	local _, inv_view = hg.Inverse(view_state.view)
	local camera_pos = hg.GetT(inv_view)
	return math.max(0.25, hg.Dist(camera_pos, position) * 0.22)
end

function Gizmo.new()
	local self = setmetatable({}, Gizmo)
	self.tool = "translate"
	self.space = "local"
	self.active_handle = nil
	self.dragging = false
	self.drag = nil
	return self
end

function Gizmo:cancel_drag(document)
	if self.dragging and document ~= nil then
		document:unlock_viewport(OWNER)
	end
	self.active_handle = nil
	self.dragging = false
	self.drag = nil
end

function Gizmo:handle_shortcuts(keyboard)
	if keyboard:Pressed(hg.K_W) then
		self.tool = "translate"
	elseif keyboard:Pressed(hg.K_E) then
		self.tool = "rotate"
	elseif keyboard:Pressed(hg.K_R) then
		self.tool = "scale"
	end
end

function Gizmo:draw_toolbar()
	local function tool_button(label, tool)
		if self.tool == tool then
			hg.ImGuiPushStyleColor(hg.ImGuiCol_Button, hg.Color(0.22, 0.42, 0.78, 1.0))
		end
		if hg.ImGuiButton(label) then
			self.tool = tool
		end
		if self.tool == tool then
			hg.ImGuiPopStyleColor()
		end
	end

	tool_button("W Translate", "translate")
	hg.ImGuiSameLine()
	tool_button("E Rotate", "rotate")
	hg.ImGuiSameLine()
	tool_button("R Scale", "scale")
	hg.ImGuiSameLine()
	if hg.ImGuiButton(self.space == "local" and "Local" or "World") then
		self.space = self.space == "local" and "world" or "local"
	end
end

function Gizmo:build_handles(document, view_state)
	local node, transform = selected_transform(document)
	if node == nil then
		return nil
	end

	local rect = document.viewport_rect
	local world = transform:GetWorld()
	local origin = hg.GetT(world)
	local origin_screen = project_point(view_state, rect, origin)
	if origin_screen == nil then
		return nil
	end

	local axis_world = {}
	if self.space == "world" then
		axis_world.x = hg.Vec3.Right
		axis_world.y = hg.Vec3.Up
		axis_world.z = hg.Vec3.Front
	else
		axis_world.x = axis_defs[1].world(world)
		axis_world.y = axis_defs[2].world(world)
		axis_world.z = axis_defs[3].world(world)
	end

	local scale = scale_from_camera(view_state, origin)
	local axes = {}
	for _, def in ipairs(axis_defs) do
		local axis = axis_world[def.name]
		local endpoint = origin + axis * scale
		local endpoint_screen = project_point(view_state, rect, endpoint)
		if endpoint_screen ~= nil then
			local dx, dy, length = math2d.normalize(endpoint_screen.x - origin_screen.x, endpoint_screen.y - origin_screen.y)
			table.insert(axes, {
				name = def.name,
				color = def.color,
				axis = axis,
				start = origin_screen,
				finish = endpoint_screen,
				screen_dir = hg.Vec2(dx, dy),
				world_per_pixel = scale / math.max(length, 1),
			})
		end
	end

	return {
		node = node,
		transform = transform,
		world = world,
		origin = origin,
		origin_screen = origin_screen,
		axes = axes,
		scale = scale,
	}
end

function Gizmo:pick_translate(mouse_pos, handles)
	local best = nil
	local best_dist = 12
	for _, axis in ipairs(handles.axes) do
		local dist = math2d.distance_to_segment(mouse_pos, axis.start, axis.finish)
		if dist < best_dist then
			best_dist = dist
			best = axis
		end
	end
	return best
end

function Gizmo:start_drag(document, handles, mouse_pos)
	if not document:lock_viewport(OWNER) then
		return
	end

	self.dragging = true

	if self.tool == "translate" then
		local axis = self:pick_translate(mouse_pos, handles)
		if axis == nil then
			self:cancel_drag(document)
			return
		end
		self.active_handle = axis.name
		self.drag = {
			tool = "translate",
			start_mouse = hg.Vec2(mouse_pos),
			start_position = hg.GetT(handles.transform:GetWorld()),
			axis = axis.axis,
			screen_dir = axis.screen_dir,
			world_per_pixel = axis.world_per_pixel,
			last_amount = 0,
		}
	elseif self.tool == "rotate" then
		self.active_handle = "screen"
		self.drag = {
			tool = "rotate",
			start_mouse = hg.Vec2(mouse_pos),
			center = hg.Vec2(handles.origin_screen),
			start_rot = handles.transform:GetRot(),
			start_angle = math2d.angle_from(handles.origin_screen, mouse_pos),
			last_delta = 0,
		}
	elseif self.tool == "scale" then
		self.active_handle = "uniform"
		self.drag = {
			tool = "scale",
			start_mouse = hg.Vec2(mouse_pos),
			start_scale = handles.transform:GetScale(),
			last_factor = 1,
		}
	end
end

function Gizmo:update_drag(document, handles, mouse_pos)
	if self.drag == nil then
		return
	end

	local _, transform = selected_transform(document)
	if transform == nil then
		self:cancel_drag(document)
		return
	end

	if self.drag.tool == "translate" then
		local delta = mouse_pos - self.drag.start_mouse
		local pixels = hg.Dot(delta, self.drag.screen_dir)
		local amount = pixels * self.drag.world_per_pixel
		local position = self.drag.start_position + self.drag.axis * amount
		set_world_translation(transform, position)
		if math.abs(amount - self.drag.last_amount) > 0.0001 then
			self.drag.last_amount = amount
			document:mark_dirty("translate")
		end
	elseif self.drag.tool == "rotate" then
		local angle = math2d.angle_from(self.drag.center, mouse_pos)
		local delta = angle - self.drag.start_angle
		local rot = hg.Vec3(self.drag.start_rot)
		rot.y = rot.y + delta
		transform:SetRot(rot)
		if math.abs(delta - self.drag.last_delta) > 0.0001 then
			self.drag.last_delta = delta
			document:mark_dirty("rotate")
		end
	elseif self.drag.tool == "scale" then
		local dy = self.drag.start_mouse.y - mouse_pos.y
		local factor = math.max(0.05, 1 + dy * 0.01)
		transform:SetScale(self.drag.start_scale * factor)
		if math.abs(factor - self.drag.last_factor) > 0.0001 then
			self.drag.last_factor = factor
			document:mark_dirty("scale")
		end
	end
end

function Gizmo:draw_overlay(draw_list, handles, mouse_pos)
	if handles == nil then
		return
	end

	if self.tool == "translate" then
		for _, axis in ipairs(handles.axes) do
			local color = axis.color
			if self.active_handle == axis.name then
				color = active_color
			end
			draw_list:AddLine(axis.start, axis.finish, color_u32(color), 4.0)
			draw_list:AddCircleFilled(axis.finish, 5.0, color_u32(color), 12)
			draw_list:AddText(axis.finish + hg.Vec2(6, -6), color_u32(white), axis.name:upper())
		end
	elseif self.tool == "rotate" then
		local radius = 46
		local color = self.active_handle == "screen" and active_color or hg.Color(0.95, 0.75, 0.25, 1)
		draw_list:AddCircle(handles.origin_screen, radius, color_u32(color), 48, 3.0)
		draw_list:AddLine(handles.origin_screen, mouse_pos, color_u32(hg.Color(1, 1, 1, 0.35)), 1.0)
	elseif self.tool == "scale" then
		local size = 8
		local min = handles.origin_screen - hg.Vec2(size, size)
		local max = handles.origin_screen + hg.Vec2(size, size)
		local color = self.active_handle == "uniform" and active_color or white
		draw_list:AddRectFilled(min, max, color_u32(color), 2)
		draw_list:AddText(max + hg.Vec2(6, -6), color_u32(white), "S")
	end
end

function Gizmo:update_and_draw(document, view_state, draw_list, keyboard, hovered)
	self:handle_shortcuts(keyboard)

	local handles = self:build_handles(document, view_state)
	if handles == nil then
		self:cancel_drag(document)
		return
	end

	local mouse_pos = hg.ImGuiGetMousePos()
	local can_interact = hovered or self.dragging

	if self.dragging then
		if hg.ImGuiIsMouseDown(hg.ImGuiMouseButton_Left) then
			self:update_drag(document, handles, mouse_pos)
		else
			self:cancel_drag(document)
		end
	elseif can_interact and not document:is_viewport_locked(OWNER) then
		self.active_handle = nil
		if self.tool == "translate" then
			local hit = self:pick_translate(mouse_pos, handles)
			if hit ~= nil then
				self.active_handle = hit.name
			end
		elseif self.tool == "rotate" then
			local distance = math2d.distance(mouse_pos, handles.origin_screen)
			if math.abs(distance - 46) <= 8 then
				self.active_handle = "screen"
			end
		elseif self.tool == "scale" then
			if math2d.distance(mouse_pos, handles.origin_screen) <= 14 then
				self.active_handle = "uniform"
			end
		end

		if self.active_handle ~= nil and hg.ImGuiIsMouseClicked(hg.ImGuiMouseButton_Left) then
			self:start_drag(document, handles, mouse_pos)
		end
	end

	self:draw_overlay(draw_list, handles, mouse_pos)
end

function Gizmo.draw_empty_viewport(draw_list, rect, message)
	draw_list:AddRectFilled(hg.Vec2(rect.x, rect.y), hg.Vec2(rect.x + rect.w, rect.y + rect.h), color_u32(panel_bg), 0)
	draw_list:AddText(hg.Vec2(rect.x + 12, rect.y + 12), color_u32(white), message)
end

return Gizmo
