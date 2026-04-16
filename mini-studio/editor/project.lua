local hg = require("harfang")
local AssetCompiler = require("editor.asset_compiler")

local Project = {}
Project.__index = Project

local function normalize(path)
	return hg.CleanPath(path or ""):gsub("\\", "/")
end

local function lower(path)
	return normalize(path):lower()
end

local function is_hidden_sidecar(name, suffixes)
	local lname = name:lower()
	for _, suffix in ipairs(suffixes) do
		if lname:sub(-#suffix) == suffix then
			return true
		end
	end
	return false
end

local function join_relative(folder, name)
	if folder == nil or folder == "" then
		return name
	end
	return folder .. "/" .. name
end

local function extension_priority(entry)
	if entry.is_dir then
		return 0
	end
	if entry.name:lower():sub(-4) == ".scn" then
		return 1
	end
	return 2
end

function Project.new()
	local self = setmetatable({}, Project)
	self.project_file = ""
	self.root = ""
	self.compiled_root = ""
	self.assetc_path = ""
	self.db = {}
	self.db_raw = ""
	self.selection = {}
	self.filter = ""
	self.last_error = ""
	self.last_compile_log = ""
	self.blacklist = {
		["_meta"] = true,
		["_prod"] = true,
		["_pkgs"] = true,
		[".git"] = true,
		[".gitignore"] = true,
		[".svn"] = true,
		[".idea"] = true,
		["core"] = true,
	}
	self.blacklist_suffix = { ".meta", ".editor" }
	return self
end

function Project:is_open()
	return self.project_file ~= "" and self.root ~= ""
end

function Project:absolute_path(path)
	if path == nil or path == "" then
		return ""
	end
	if hg.IsPathAbsolute(path) then
		return normalize(path)
	end
	return normalize(hg.GetCurrentWorkingDirectory() .. "/" .. path)
end

function Project:open(project_file, options)
	options = options or {}
	local path = self:absolute_path(project_file)

	if path == "" then
		self.last_error = "No project path provided."
		return false
	end
	if path:lower():sub(-4) ~= ".prj" then
		self.last_error = "Project files must use the .prj extension."
		return false
	end
	if not hg.Exists(path) or not hg.IsFile(path) then
		self.last_error = "Project file does not exist: " .. path
		return false
	end

	local root = normalize(hg.GetFilePath(path))
	local compiled_root = ""
	local compile_log = ""
	local assetc_path = normalize(options.assetc_path or self.assetc_path or "")

	if options.compile_assets ~= false then
		local ok, output_root, err, log = AssetCompiler.compile_project(path, root, {
			assetc_path = assetc_path,
			cache_root = options.compiled_cache_root,
			output_root = options.compiled_root,
		})
		compile_log = log or ""
		if not ok then
			self.last_error = err
			self.last_compile_log = compile_log
			return false
		end
		compiled_root = output_root
	end

	self.project_file = path
	self.root = root
	self.compiled_root = normalize(compiled_root)
	self.assetc_path = assetc_path
	self.db = {}
	self.db_raw = hg.FileToString(path) or ""
	self.selection = {}
	self.last_error = ""
	self.last_compile_log = compile_log

	return true
end

function Project:compile_assets(options)
	if not self:is_open() then
		self.last_error = "No project is open."
		return false
	end

	options = options or {}
	local ok, output_root, err, log = AssetCompiler.compile_project(self.project_file, self.root, {
		assetc_path = options.assetc_path or self.assetc_path,
		cache_root = options.compiled_cache_root,
		output_root = options.compiled_root or self.compiled_root,
	})
	self.last_compile_log = log or ""
	if not ok then
		self.last_error = err
		return false
	end

	self.compiled_root = normalize(output_root)
	self.last_error = ""
	return true
end

function Project:save()
	if not self:is_open() then
		self.last_error = "No project is open."
		return false
	end

	local raw = self.db_raw
	if raw == nil or raw == "" then
		raw = "{\n}\n"
	end

	local ok = hg.StringToFile(self.project_file, raw)
	if not ok then
		self.last_error = "Failed to save project: " .. self.project_file
		return false
	end

	self.last_error = ""
	return true
end

function Project:path(relative_path)
	if relative_path == nil or relative_path == "" then
		return self.root
	end
	if hg.IsPathAbsolute(relative_path) then
		return normalize(relative_path)
	end
	return normalize(self.root .. "/" .. relative_path)
end

function Project:compiled_path(relative_path)
	if self.compiled_root == nil or self.compiled_root == "" then
		return ""
	end
	if relative_path == nil or relative_path == "" then
		return self.compiled_root
	end
	return normalize(self.compiled_root .. "/" .. relative_path)
end

function Project:relative_path(path)
	local absolute = normalize(path)
	local root = normalize(self.root)
	local root_with_sep = root
	if root_with_sep:sub(-1) ~= "/" then
		root_with_sep = root_with_sep .. "/"
	end

	if lower(absolute):sub(1, #lower(root_with_sep)) == lower(root_with_sep) then
		return absolute:sub(#root_with_sep + 1)
	end
	if lower(absolute) == lower(root) then
		return ""
	end
	return absolute
end

function Project:should_hide(name)
	if self.blacklist[name] or self.blacklist[name:lower()] then
		return true
	end
	return is_hidden_sidecar(name, self.blacklist_suffix)
end

function Project:list(relative_folder, filter)
	local out = {}
	if not self:is_open() then
		return out
	end

	relative_folder = relative_folder or ""
	filter = (filter or ""):lower()

	local folder = self:path(relative_folder)
	if not hg.Exists(folder) or not hg.IsDir(folder) then
		return out
	end

	local raw_entries = hg.ListDir(folder, hg.DE_All)
	for i = 0, raw_entries:size() - 1 do
		local raw = raw_entries:at(i)
		local name = raw.name
		if name ~= "." and name ~= ".." and not self:should_hide(name) then
			local is_dir = raw.type == hg.DE_Dir
			local matches = filter == "" or name:lower():find(filter, 1, true) ~= nil
			if is_dir or matches then
				table.insert(out, {
					name = name,
					relative_path = join_relative(relative_folder, name),
					is_dir = is_dir,
				})
			end
		end
	end

	table.sort(out, function(a, b)
		local pa = extension_priority(a)
		local pb = extension_priority(b)
		if pa ~= pb then
			return pa < pb
		end
		return a.name:lower() < b.name:lower()
	end)

	return out
end

function Project:list_recursive(relative_folder, filter, out)
	out = out or {}
	for _, entry in ipairs(self:list(relative_folder, filter)) do
		table.insert(out, entry)
		if entry.is_dir then
			self:list_recursive(entry.relative_path, filter, out)
		end
	end
	return out
end

return Project
