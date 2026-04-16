local hg = require("harfang")

local SceneGraph = {}

local function safe_node_name(node)
	local name = node:GetName()
	if name == nil or name == "" then
		name = "(unnamed)"
	end
	return name
end

local function draw_node(document, node)
	local selected_node = document:get_selected_node()
	local selected = selected_node:IsValid() and node == selected_node
	local children = document.scene:GetNodeChildren(node)
	local has_children = children:size() > 0

	local flags = hg.ImGuiTreeNodeFlags_OpenOnArrow
	if selected then
		flags = flags | hg.ImGuiTreeNodeFlags_Selected
	end
	if not has_children then
		flags = flags | hg.ImGuiTreeNodeFlags_Leaf | hg.ImGuiTreeNodeFlags_NoTreePushOnOpen
	end

	local label = string.format("%s##node:%d", safe_node_name(node), node:GetUid())
	local opened = hg.ImGuiTreeNodeEx(label, flags)

	if hg.ImGuiIsItemClicked(hg.ImGuiMouseButton_Left) then
		document:set_selected_node(node)
	end

	if has_children and opened then
		for i = 0, children:size() - 1 do
			draw_node(document, children:at(i))
		end
		hg.ImGuiTreePop()
	end
end

function SceneGraph.draw(document)
	if document == nil then
		hg.ImGuiTextWrapped("Open a scene to inspect its graph.")
		return
	end

	local root_flags = hg.ImGuiTreeNodeFlags_DefaultOpen | hg.ImGuiTreeNodeFlags_OpenOnArrow
	if not document:get_selected_node():IsValid() then
		root_flags = root_flags | hg.ImGuiTreeNodeFlags_Selected
	end

	if hg.ImGuiTreeNodeEx("Scene##scene-root", root_flags) then
		if hg.ImGuiIsItemClicked(hg.ImGuiMouseButton_Left) then
			document:clear_selection()
		end

		local nodes = document.scene:GetNodes()
		for i = 0, nodes:size() - 1 do
			local node = nodes:at(i)
			if document.scene:IsRoot(node) then
				draw_node(document, node)
			end
		end
		hg.ImGuiTreePop()
	end
end

return SceneGraph
