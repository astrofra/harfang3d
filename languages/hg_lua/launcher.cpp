// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <engine/assets.h>

#include <foundation/data.h>
#include <foundation/dir.h>
#include <foundation/file.h>
#include <foundation/path_tools.h>

#include <json/json.hpp>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <iostream>
#include <set>
#include <string>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace {

using json = nlohmann::json;

struct LauncherConfig {
	std::string entry;
	std::vector<std::string> args;
};

struct MountedAssets {
	std::string folder_path;
	std::string package_path;
};

std::vector<std::string> MakeAssetPathCandidates(const std::string &name);
bool ResolveAssetName(const std::string &name, std::string &resolved_name);

std::string GetExecutablePath() {
#if defined(_WIN32)
	char buffer[MAX_PATH] = {};
	const auto len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	return len > 0 ? std::string(buffer, len) : std::string();
#elif defined(__linux__)
	char buffer[PATH_MAX] = {};
	const auto len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
	return len > 0 ? std::string(buffer, len) : std::string();
#else
	return {};
#endif
}

void PrintError(const std::string &message) {
	std::cerr << "launcher: " << message << std::endl;
}

std::string JsonValueToArg(const json &value) {
	if (value.is_string())
		return value.get<std::string>();
	if (value.is_boolean())
		return value.get<bool>() ? "true" : "false";
	if (value.is_number_integer())
		return std::to_string(value.get<long long>());
	if (value.is_number_unsigned())
		return std::to_string(value.get<unsigned long long>());
	if (value.is_number_float())
		return value.dump();
	return value.dump();
}

bool ParseConfig(const std::string &content, const std::string &source, LauncherConfig &config, std::string &error) {
	try {
		if (content.empty()) {
			error = "configuration file is empty: " + source;
			return false;
		}

		const auto js = json::parse(content);
		if (!js.is_object()) {
			error = "configuration root must be a JSON object";
			return false;
		}

		if (js.contains("entry") && js["entry"].is_string())
			config.entry = js["entry"].get<std::string>();
		else if (js.contains("script") && js["script"].is_string())
			config.entry = js["script"].get<std::string>();

		if (config.entry.empty()) {
			error = "configuration must define a string field named 'entry' or 'script'";
			return false;
		}

		if (js.contains("args")) {
			if (!js["args"].is_array()) {
				error = "'args' must be an array";
				return false;
			}

			for (const auto &arg : js["args"])
				config.args.push_back(JsonValueToArg(arg));
		}

		return true;
	} catch (const json::exception &e) {
		error = "invalid JSON in " + source + ": " + e.what();
		return false;
	}
}

bool LoadTextFile(const std::string &path, std::string &content, std::string &error) {
	content = hg::FileToString(path.c_str());
	if (content.empty() && !hg::Exists(path.c_str())) {
		error = "file not found: " + path;
		return false;
	}
	return true;
}

bool LoadTextAsset(const std::string &name, std::string &content, std::string &error) {
	std::string resolved_name;
	if (!ResolveAssetName(name, resolved_name)) {
		error = "asset not found: " + name;
		return false;
	}

	content = hg::AssetToString(resolved_name.c_str());
	return true;
}

std::vector<std::string> MakeAssetPathCandidates(const std::string &name) {
	std::set<std::string> unique;
	unique.insert(name);

	auto slash = name;
	for (auto &ch : slash)
		if (ch == '\\')
			ch = '/';
	unique.insert(slash);

	auto backslash = name;
	for (auto &ch : backslash)
		if (ch == '/')
			ch = '\\';
	unique.insert(backslash);

	return {unique.begin(), unique.end()};
}

bool ResolveAssetName(const std::string &name, std::string &resolved_name) {
	for (const auto &candidate : MakeAssetPathCandidates(name)) {
		if (!hg::IsAssetFile(candidate.c_str()))
			continue;

		resolved_name = candidate;
		return true;
	}

	return false;
}

int MessageHandler(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg == nullptr)
		msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
	luaL_traceback(L, L, msg, 1);
	return 1;
}

int ProtectedCall(lua_State *L, int nargs, int nresults) {
	const int base = lua_gettop(L) - nargs;
	lua_pushcfunction(L, MessageHandler);
	lua_insert(L, base);
	const auto status = lua_pcall(L, nargs, nresults, base);
	lua_remove(L, base);
	return status;
}

void CreateArgTable(lua_State *L, const std::string &script_path, const std::vector<std::string> &args) {
	lua_createtable(L, static_cast<int>(args.size()), 1);

	lua_pushstring(L, script_path.c_str());
	lua_rawseti(L, -2, 0);

	for (size_t i = 0; i < args.size(); ++i) {
		lua_pushstring(L, args[i].c_str());
		lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
	}

	lua_setglobal(L, "arg");
}

int PushArgs(lua_State *L) {
	lua_getglobal(L, "arg");
	if (!lua_istable(L, -1))
		return 0;

	const auto n = static_cast<int>(luaL_len(L, -1));
	luaL_checkstack(L, n + 3, "too many launcher arguments");
	for (int i = 1; i <= n; ++i)
		lua_rawgeti(L, -i, i);
	lua_remove(L, -n - 1);
	return n;
}

void PrependPackageField(lua_State *L, const char *field, const std::vector<std::string> &patterns) {
	lua_getglobal(L, "package");
	lua_getfield(L, -1, field);

	std::string value;
	for (const auto &pattern : patterns) {
		value += pattern;
		value += ';';
	}

	if (const auto *existing = lua_tostring(L, -1))
		value += existing;

	lua_pop(L, 1);
	lua_pushlstring(L, value.data(), value.size());
	lua_setfield(L, -2, field);
	lua_pop(L, 1);
}

void ConfigurePackagePaths(lua_State *L, const std::string &exe_dir, const std::string &data_dir) {
	PrependPackageField(L, "path", {
		hg::PathJoin(exe_dir, "?.lua"),
		hg::PathJoin(exe_dir, "?", "init.lua"),
		hg::PathJoin(exe_dir, "harfang", "?.lua"),
		hg::PathJoin({exe_dir, "harfang", "?", "init.lua"}),
		hg::PathJoin(data_dir, "?.lua"),
		hg::PathJoin(data_dir, "?", "init.lua"),
	});

	PrependPackageField(L, "cpath", {
		hg::PathJoin(exe_dir, "?.dll"),
		hg::PathJoin(exe_dir, "loadall.dll"),
	});
}

std::string ResolveAssetDisplayPath(const MountedAssets &mounted_assets, const std::string &name) {
	if (!mounted_assets.folder_path.empty()) {
		const auto folder_asset_path = hg::PathJoin(mounted_assets.folder_path, name);
		if (hg::Exists(folder_asset_path.c_str()))
			return folder_asset_path;
	}

	if (!mounted_assets.package_path.empty())
		return mounted_assets.package_path + ":" + name;

	return name;
}

std::string ModuleNameToAssetPath(const std::string &module_name) {
	std::string path = module_name;
	for (auto &ch : path)
		if (ch == '.')
			ch = '/';
	return path;
}

int AssetSearcher(lua_State *L) {
	const auto *module_name = luaL_checkstring(L, 1);
	const auto base_name = ModuleNameToAssetPath(module_name);

	for (const auto &candidate : {base_name + ".lua", base_name + "/init.lua"}) {
		std::string resolved_name;
		if (!ResolveAssetName(candidate, resolved_name))
			continue;

		const auto source = hg::AssetToData(resolved_name.c_str());
		const auto chunk_name = "@" + resolved_name;
		const auto status = luaL_loadbuffer(L, reinterpret_cast<const char *>(source.GetData()), source.GetSize(), chunk_name.c_str());
		if (status == LUA_OK) {
			lua_pushstring(L, resolved_name.c_str());
			return 2;
		}

		return 1;
	}

	lua_pushfstring(L, "\n\tno asset module '%s'", module_name);
	return 1;
}

void InstallAssetSearcher(lua_State *L) {
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "searchers");

	const auto count = static_cast<int>(luaL_len(L, -1));
	for (int i = count + 1; i > 2; --i) {
		lua_rawgeti(L, -1, i - 1);
		lua_rawseti(L, -2, i);
	}

	lua_pushcfunction(L, AssetSearcher);
	lua_rawseti(L, -2, 2);

	lua_pop(L, 2);
}

bool RunLoadedChunk(lua_State *L, const std::string &script_name, const std::vector<std::string> &args) {
	CreateArgTable(L, script_name, args);

	const auto nargs = PushArgs(L);
	const auto status = ProtectedCall(L, nargs, LUA_MULTRET);
	if (status != LUA_OK) {
		const auto *message = lua_tostring(L, -1);
		PrintError(message != nullptr ? message : "unknown Lua error");
		lua_pop(L, 1);
		return false;
	}

	return true;
}

bool RunEntryPoint(lua_State *L, const MountedAssets &mounted_assets, const std::string &entry_name, const std::vector<std::string> &args) {
	if (hg::IsPathAbsolute(entry_name)) {
		CreateArgTable(L, entry_name, args);

		const auto status = luaL_loadfile(L, entry_name.c_str());
		if (status != LUA_OK) {
			const auto *message = lua_tostring(L, -1);
			PrintError(message != nullptr ? message : "unknown Lua error");
			lua_pop(L, 1);
			return false;
		}

		return RunLoadedChunk(L, entry_name, args);
	}

	std::string resolved_name;
	if (!ResolveAssetName(entry_name, resolved_name)) {
		PrintError("entry script not found in mounted assets: " + entry_name);
		return false;
	}

	const auto source = hg::AssetToData(resolved_name.c_str());
	const auto display_path = ResolveAssetDisplayPath(mounted_assets, resolved_name);
	const auto chunk_name = "@" + display_path;
	const auto status = luaL_loadbuffer(L, reinterpret_cast<const char *>(source.GetData()), source.GetSize(), chunk_name.c_str());
	if (status != LUA_OK) {
		const auto *message = lua_tostring(L, -1);
		PrintError(message != nullptr ? message : "unknown Lua error");
		lua_pop(L, 1);
		return false;
	}

	return RunLoadedChunk(L, display_path, args);
}

bool MountLauncherAssets(const std::string &cwd, MountedAssets &mounted_assets) {
	const auto data_dir = hg::PathJoin(cwd, "data");
	const auto data_zip = hg::PathJoin(cwd, "data.zip");

	if (hg::IsDir(data_dir.c_str()) && hg::AddAssetsFolder(data_dir.c_str()))
		mounted_assets.folder_path = data_dir;

	if (hg::Exists(data_zip.c_str()) && hg::AddAssetsPackage(data_zip.c_str()))
		mounted_assets.package_path = data_zip;

	return !mounted_assets.folder_path.empty() || !mounted_assets.package_path.empty();
}

void UnmountLauncherAssets(const MountedAssets &mounted_assets) {
	if (!mounted_assets.folder_path.empty())
		hg::RemoveAssetsFolder(mounted_assets.folder_path.c_str());
	if (!mounted_assets.package_path.empty())
		hg::RemoveAssetsPackage(mounted_assets.package_path.c_str());
}

} // namespace

int main() {
	const auto cwd = hg::GetCurrentWorkingDirectory();
	MountedAssets mounted_assets;
	if (!MountLauncherAssets(cwd, mounted_assets)) {
		PrintError("missing data source in current working directory: expected data/ or data.zip");
		return 1;
	}

	std::string config_content, error;
	if (!LoadTextAsset("launcher.json", config_content, error)) {
		PrintError(error);
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	LauncherConfig config;
	const auto config_source = ResolveAssetDisplayPath(mounted_assets, "launcher.json");
	if (!ParseConfig(config_content, config_source, config, error)) {
		PrintError(error);
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	const auto exe_path = GetExecutablePath();
	const auto exe_dir = exe_path.empty() ? cwd : hg::CutFilePath(exe_path);
	const auto data_dir = hg::PathJoin(cwd, "data");

	auto *L = luaL_newstate();
	if (L == nullptr) {
		PrintError("failed to create Lua state");
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	luaL_openlibs(L);
	ConfigurePackagePaths(L, exe_dir, data_dir);
	InstallAssetSearcher(L);

	lua_pushstring(L, mounted_assets.folder_path.c_str());
	lua_setglobal(L, "LAUNCHER_DATA_DIR");
	lua_pushstring(L, mounted_assets.package_path.c_str());
	lua_setglobal(L, "LAUNCHER_DATA_PACKAGE");
	lua_pushstring(L, config_source.c_str());
	lua_setglobal(L, "LAUNCHER_CONFIG_PATH");

	const auto ok = RunEntryPoint(L, mounted_assets, config.entry, config.args);
	lua_close(L);
	UnmountLauncherAssets(mounted_assets);
	return ok ? 0 : 1;
}
