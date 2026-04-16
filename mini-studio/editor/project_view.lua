local hg = require("harfang")

local ProjectView = {}

local function is_scene(path)
	return path:lower():sub(-4) == ".scn"
end

local function draw_entry(project, manager, entry)
	local label = entry.name .. "##project:" .. entry.relative_path

	if entry.is_dir then
		local flags = hg.ImGuiTreeNodeFlags_OpenOnArrow
		if hg.ImGuiTreeNodeEx(label, flags) then
			local children = project:list(entry.relative_path, project.filter)
			for _, child in ipairs(children) do
				draw_entry(project, manager, child)
			end
			hg.ImGuiTreePop()
		end
		return
	end

	local selected = project.selection.path == entry.relative_path
	if hg.ImGuiSelectable(label, selected, hg.ImGuiSelectableFlags_AllowDoubleClick) then
		project.selection.path = entry.relative_path
	end

	if is_scene(entry.relative_path) and hg.ImGuiIsItemHovered() and hg.ImGuiIsMouseDoubleClicked(hg.ImGuiMouseButton_Left) then
		manager:open_scene(entry.relative_path)
	end
end

function ProjectView.draw(project, manager)
	local changed, new_filter = hg.ImGuiInputText("Filter", project.filter or "", 256)
	if changed then
		project.filter = new_filter
	end

	hg.ImGuiSeparator()

	if not project:is_open() then
		hg.ImGuiTextWrapped("Open a .prj file to list project assets.")
		return
	end

	local entries = project:list("", project.filter)
	for _, entry in ipairs(entries) do
		draw_entry(project, manager, entry)
	end
end

return ProjectView
