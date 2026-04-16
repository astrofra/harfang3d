local hg = require("harfang")
local Camera = require("editor.camera")
local Gizmo = require("editor.gizmo")

local Viewport = {}

local function int_rect_from_document(document)
	local r = document.viewport_rect
	return hg.IntRect(math.floor(r.x), math.floor(r.y), math.floor(r.x + r.w), math.floor(r.y + r.h))
end

function Viewport.draw_panel(document, keyboard)
	if document ~= nil then
		document.gizmo:draw_toolbar()
	end

	local available = hg.ImGuiGetContentRegionAvail()
	available.x = math.max(available.x, 64)
	available.y = math.max(available.y, 64)

	hg.ImGuiInvisibleButton("viewport_region", available)
	local rect_min = hg.ImGuiGetItemRectMin()
	local rect_size = hg.ImGuiGetItemRectSize()
	local rect = {
		x = math.floor(rect_min.x),
		y = math.floor(rect_min.y),
		w = math.max(1, math.floor(rect_size.x)),
		h = math.max(1, math.floor(rect_size.y)),
	}

	local draw_list = hg.ImGuiGetWindowDrawList()
	draw_list:AddRect(hg.Vec2(rect.x, rect.y), hg.Vec2(rect.x + rect.w, rect.y + rect.h), hg.ImGuiGetColorU32(hg.Color(0.25, 0.27, 0.3, 1)), 0, 0, 1.0)

	if document == nil then
		Gizmo.draw_empty_viewport(draw_list, rect, "Open a scene from the project tree.")
		return
	end

	document.viewport_rect = rect

	local view_state = Camera.compute_view_state(document, rect)
	local selected = document:get_selected_node()
	if selected:IsValid() and selected:HasTransform() then
		document.gizmo:update_and_draw(document, view_state, draw_list, keyboard, hg.ImGuiIsItemHovered())
	end
end

function Viewport.render_document(app, document, dt)
	if document == nil then
		return
	end

	local rect = document.viewport_rect
	if rect.w <= 1 or rect.h <= 1 then
		return
	end

	document.scene:Update(dt)

	local view_state = Camera.compute_view_state(document, rect)
	hg.ProcessLoadQueues(app.resources, hg.time_from_ms(1))

	local view_id = 1
	hg.SubmitSceneToPipeline(view_id, document.scene, int_rect_from_document(document), view_state, app.pipeline, app.resources)
end

return Viewport
