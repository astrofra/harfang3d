local hg = require("harfang")

local AssetCompiler = {}

local function normalize(path)
	return hg.CleanPath(path or ""):gsub("\\", "/")
end

local function path_join(a, b)
	if a == nil or a == "" then
		return b
	end
	if a:sub(-1) == "/" or a:sub(-1) == "\\" then
		return a .. b
	end
	return a .. "/" .. b
end

local function quote_arg(path)
	return '"' .. tostring(path):gsub('"', '\\"') .. '"'
end

local function sanitize_name(name)
	name = (name or "project"):gsub("[^%w%._%-]+", "_")
	if name == "" then
		return "project"
	end
	return name
end

local function fnv1a32(value)
	local hash = 2166136261
	value = normalize(value):lower()
	for i = 1, #value do
		hash = hash ~ value:byte(i)
		hash = (hash * 16777619) % 4294967296
	end
	return string.format("%08x", hash)
end

function AssetCompiler.assetc_from_lua_binary(lua_binary)
	if lua_binary == nil or lua_binary == "" then
		return ""
	end

	local lua_dir = normalize(hg.GetFilePath(lua_binary))
	local exe = "assetc"
	if package.config:sub(1, 1) == "\\" then
		exe = "assetc.exe"
	end

	return path_join(path_join(path_join(lua_dir, "harfang"), "assetc"), exe)
end

function AssetCompiler.default_cache_root()
	local root = hg.GetUserFolder()
	if root == nil or root == "" then
		root = hg.GetCurrentWorkingDirectory()
	end
	return path_join(path_join(normalize(root), "HARFANG_Mini_Studio"), "asset-cache")
end

function AssetCompiler.output_root_for_project(project_file, cache_root)
	local project_name = sanitize_name(hg.GetFileName(project_file))
	local project_hash = fnv1a32(project_file)
	return path_join(cache_root or AssetCompiler.default_cache_root(), project_name .. "-" .. project_hash)
end

function AssetCompiler.compile_project(project_file, source_root, options)
	options = options or {}

	local assetc_path = normalize(options.assetc_path or "")
	if assetc_path == "" then
		return false, "", "assetc path is not configured.", ""
	end
	if not hg.Exists(assetc_path) or not hg.IsFile(assetc_path) then
		return false, "", "assetc was not found: " .. assetc_path, ""
	end

	local input_root = normalize(source_root or "")
	if input_root == "" or not hg.Exists(input_root) or not hg.IsDir(input_root) then
		return false, "", "Asset source folder does not exist: " .. input_root, ""
	end

	local output_root = normalize(options.output_root or AssetCompiler.output_root_for_project(project_file, options.cache_root))
	if not hg.MkTree(output_root) and (not hg.Exists(output_root) or not hg.IsDir(output_root)) then
		return false, "", "Failed to create compiled asset cache: " .. output_root, ""
	end

	local command = table.concat({
		quote_arg(assetc_path),
		"-quiet",
		quote_arg(input_root),
		quote_arg(output_root),
	}, " ")
	if package.config:sub(1, 1) == "\\" then
		command = '"' .. command .. '"'
	end
	command = command .. " 2>&1"

	local handle = io.popen(command)
	if handle == nil then
		return false, "", "Failed to launch assetc.", ""
	end

	local log = handle:read("*a") or ""
	local ok, _, code = handle:close()
	if not ok then
		return false, output_root, string.format("assetc failed with exit code %s.", tostring(code)), log
	end

	return true, output_root, "", log
end

return AssetCompiler
