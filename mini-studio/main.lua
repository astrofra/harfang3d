local script_path = arg and arg[0] or debug.getinfo(1, "S").source
if script_path:sub(1, 1) == "@" then
	script_path = script_path:sub(2)
end

local script_dir = script_path:gsub("\\", "/"):match("^(.*)/[^/]*$") or "."
package.path = script_dir .. "/?.lua;" .. script_dir .. "/?/init.lua;" .. package.path

local App = require("editor.app")

local function has_extension(path, ext)
	return path:lower():sub(-#ext) == ext
end

local project_file = nil
local scenes = {}

for i = 1, #arg do
	local value = arg[i]
	if has_extension(value, ".prj") and project_file == nil then
		project_file = value
	else
		table.insert(scenes, value)
	end
end

if project_file == nil then
	local default_project = script_dir .. "/assets/project.prj"
	local f = io.open(default_project, "rb")
	if f ~= nil then
		f:close()
		project_file = default_project
	end
end

local app = App.new({
	script_dir = script_dir,
	initial_project = project_file,
	initial_scenes = scenes,
	lua_binary = "C:/works/dev/harfang/install/lua/hg_lua/lua.exe",
})

app:run()
