// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "assets_bridge.h"

#include <engine/assets.h>
#include <engine/assets_internal.h>

#include <foundation/data.h>
#include <foundation/dir.h>
#include <foundation/file.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <json/json.hpp>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cctype>
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

constexpr uint32_t kZipLocalHeaderMagic = 0x04034b50u;
constexpr uint32_t kZipEmptyArchiveMagic = 0x06054b50u;
constexpr uint32_t kZipSpannedArchiveMagic = 0x08074b50u;
constexpr uint32_t kLegacyEnhancedMagic = 0x4E415244u;
constexpr uint32_t kLegacyLegacyMagic = 0x4E415243u;

enum class LauncherAssetsSource { None, Folder, Zip, Legacy };

struct LauncherAssetsConfig {
	std::string logical_data_path = "data";
	std::string archive_root;
	bool pass_mode_argument = false;
};

struct LauncherConfig {
	std::string entry;
	std::vector<std::string> args;
	LauncherAssetsConfig assets;
};

struct MountedAssets {
	LauncherAssetsSource source = LauncherAssetsSource::None;
	std::string cwd;
	std::string folder_path;
	std::string package_path;
};

static std::string g_launcher_archive_root;

std::vector<std::string> MakeAssetPathCandidates(const std::string &name);
bool ResolveAssetName(const std::string &name, std::string &resolved_name);
bool NormalizeRelativeAssetPath(const std::string &path, std::string &normalized);
bool JoinArchivePath(const std::string &prefix, const std::string &suffix, std::string &joined);

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

void PrintError(const std::string &message) { std::cerr << "launcher: " << message << std::endl; }

const char *GetAssetsSourceName(LauncherAssetsSource source) {
	switch (source) {
		case LauncherAssetsSource::Folder:
			return "folder";
		case LauncherAssetsSource::Zip:
			return "zip";
		case LauncherAssetsSource::Legacy:
			return "legacy";
		default:
			return "none";
	}
}

bool IsDriveLetterPath(const std::string &path) { return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':'; }

bool IsZipMagic(uint32_t magic) { return magic == kZipLocalHeaderMagic || magic == kZipEmptyArchiveMagic || magic == kZipSpannedArchiveMagic; }

LauncherAssetsSource DetectArchiveSourceType(const std::string &path) {
	hg::ScopedFile file(hg::Open(path.c_str(), true));
	if (!file)
		return LauncherAssetsSource::None;

	uint8_t bytes[4] = {};
	if (hg::Read(file.f, bytes, sizeof(bytes)) != sizeof(bytes))
		return LauncherAssetsSource::None;

	const uint32_t magic = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) | (static_cast<uint32_t>(bytes[2]) << 16) |
						   (static_cast<uint32_t>(bytes[3]) << 24);

	if (IsZipMagic(magic))
		return LauncherAssetsSource::Zip;
	if (magic == kLegacyEnhancedMagic || magic == kLegacyLegacyMagic)
		return LauncherAssetsSource::Legacy;
	return LauncherAssetsSource::None;
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

		if (js.contains("assets")) {
			if (!js["assets"].is_object()) {
				error = "'assets' must be a JSON object";
				return false;
			}

			const auto &assets = js["assets"];
			if (assets.contains("logical_data_path")) {
				if (!assets["logical_data_path"].is_string()) {
					error = "'assets.logical_data_path' must be a string";
					return false;
				}
				config.assets.logical_data_path = assets["logical_data_path"].get<std::string>();
			}

			if (assets.contains("archive_root")) {
				if (!assets["archive_root"].is_string()) {
					error = "'assets.archive_root' must be a string";
					return false;
				}
				config.assets.archive_root = assets["archive_root"].get<std::string>();
			}

			if (assets.contains("pass_mode_argument")) {
				if (!assets["pass_mode_argument"].is_boolean()) {
					error = "'assets.pass_mode_argument' must be a boolean";
					return false;
				}
				config.assets.pass_mode_argument = assets["pass_mode_argument"].get<bool>();
			}
		}

		std::string normalized_logical_path;
		if (!NormalizeRelativeAssetPath(config.assets.logical_data_path, normalized_logical_path) || normalized_logical_path.empty()) {
			error = "'assets.logical_data_path' must be a relative path inside the asset root";
			return false;
		}
		config.assets.logical_data_path = normalized_logical_path;

		std::string normalized_archive_root;
		if (!NormalizeRelativeAssetPath(config.assets.archive_root, normalized_archive_root)) {
			error = "'assets.archive_root' must be a relative archive path";
			return false;
		}
		config.assets.archive_root = normalized_archive_root;

		return true;
	} catch (const json::exception &e) {
		error = "invalid JSON in " + source + ": " + e.what();
		return false;
	}
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

bool NormalizeRelativeAssetPath(const std::string &path, std::string &normalized) {
	std::string value = path;
	hg::replace_all(value, "\\", "/");

	if (!value.empty() && (value[0] == '/' || IsDriveLetterPath(value)))
		return false;

	std::vector<std::string> segments;
	for (const auto &segment : hg::split(value, "/")) {
		if (segment.empty() || segment == ".")
			continue;

		if (segment == "..") {
			if (segments.empty())
				return false;
			segments.pop_back();
			continue;
		}

		if (IsDriveLetterPath(segment))
			return false;

		segments.push_back(segment);
	}

	normalized = hg::join(segments.begin(), segments.end(), "/");
	return true;
}

bool JoinArchivePath(const std::string &prefix, const std::string &suffix, std::string &joined) {
	std::vector<std::string> segments = prefix.empty() ? std::vector<std::string>() : hg::split(prefix, "/");
	const auto min_depth = segments.size();

	std::string value = suffix;
	hg::replace_all(value, "\\", "/");

	if (!value.empty() && (value[0] == '/' || IsDriveLetterPath(value)))
		return false;

	for (const auto &segment : hg::split(value, "/")) {
		if (segment.empty() || segment == ".")
			continue;

		if (segment == "..") {
			if (segments.size() <= min_depth)
				return false;
			segments.pop_back();
			continue;
		}

		if (IsDriveLetterPath(segment))
			return false;

		segments.push_back(segment);
	}

	joined = hg::join(segments.begin(), segments.end(), "/");
	return true;
}

std::vector<std::string> MakeLauncherAssetCandidates(const std::string &name, const std::string &archive_root) {
	std::set<std::string> unique;

	for (const auto &candidate : MakeAssetPathCandidates(name))
		unique.insert(candidate);

	if (!archive_root.empty()) {
		std::string prefixed_name;
		if (JoinArchivePath(archive_root, name, prefixed_name)) {
			for (const auto &candidate : MakeAssetPathCandidates(prefixed_name))
				unique.insert(candidate);
		}
	}

	return {unique.begin(), unique.end()};
}

bool ResolveLauncherAssetName(const std::string &name, const std::string &archive_root, std::string &resolved_name) {
	for (const auto &candidate : MakeLauncherAssetCandidates(name, archive_root)) {
		if (!hg::IsAssetFile(candidate.c_str()))
			continue;

		resolved_name = candidate;
		return true;
	}

	return false;
}

bool NormalizeResolverInputPath(const std::string &path, const std::string &cwd, std::string &normalized) {
	normalized = hg::CleanPath(path);
	if (normalized.empty())
		return false;

	const auto clean_cwd = hg::CleanPath(cwd);
	if (hg::IsPathAbsolute(normalized)) {
		if (!hg::PathStartsWith(normalized, clean_cwd))
			return false;
		normalized = hg::PathStripPrefix(normalized, clean_cwd);
	}

	return NormalizeRelativeAssetPath(normalized, normalized);
}

bool LoadLauncherConfigAsset(const MountedAssets &mounted_assets, std::string &resolved_name, std::string &content, std::string &error) {
	std::vector<std::string> candidates = {"bootstrap.json", "launcher.json"};
	if (mounted_assets.source != LauncherAssetsSource::Folder) {
		candidates.push_back("data/bootstrap.json");
		candidates.push_back("data/launcher.json");
	}

	for (const auto &candidate : candidates) {
		if (!ResolveAssetName(candidate, resolved_name))
			continue;

		content = hg::AssetToString(resolved_name.c_str());
		return true;
	}

	error = mounted_assets.source == LauncherAssetsSource::Folder ? "asset not found: bootstrap.json or launcher.json"
																 : "asset not found: bootstrap.json, launcher.json, data/bootstrap.json, or data/launcher.json";
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
		if (!ResolveLauncherAssetName(candidate, g_launcher_archive_root, resolved_name))
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

bool RunEntryPoint(lua_State *L, const MountedAssets &mounted_assets, const LauncherConfig &config) {
	if (hg::IsPathAbsolute(config.entry)) {
		CreateArgTable(L, config.entry, config.args);

		const auto status = luaL_loadfile(L, config.entry.c_str());
		if (status != LUA_OK) {
			const auto *message = lua_tostring(L, -1);
			PrintError(message != nullptr ? message : "unknown Lua error");
			lua_pop(L, 1);
			return false;
		}

		return RunLoadedChunk(L, config.entry, config.args);
	}

	std::string resolved_name;
	if (!ResolveLauncherAssetName(config.entry, config.assets.archive_root, resolved_name)) {
		PrintError("entry script not found in mounted assets: " + config.entry);
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

	return RunLoadedChunk(L, display_path, config.args);
}

bool InstallArchiveFolderResolver(const MountedAssets &mounted_assets, const LauncherConfig &config) {
	if (mounted_assets.source == LauncherAssetsSource::Folder || mounted_assets.package_path.empty()) {
		hg::ClearAssetsFolderResolver();
		return true;
	}

	const auto cwd = mounted_assets.cwd;
	const auto archive_path = mounted_assets.package_path;
	const auto logical_data_path = config.assets.logical_data_path;
	const auto archive_root = config.assets.archive_root;

	hg::SetAssetsFolderResolver([cwd, archive_path, logical_data_path, archive_root](const std::string &path, hg::AssetsFolderResolution &resolution) {
		std::string normalized_input;
		if (!NormalizeResolverInputPath(path, cwd, normalized_input))
			return false;

		if (normalized_input != logical_data_path && !hg::starts_with(normalized_input, logical_data_path + "/"))
			return false;

		std::string suffix;
		if (normalized_input.size() > logical_data_path.size())
			suffix = normalized_input.substr(logical_data_path.size() + 1);

		std::string archive_prefix;
		if (!JoinArchivePath(archive_root, suffix, archive_prefix))
			return false;

		resolution.logical_path = normalized_input;
		resolution.archive_path = archive_path;
		resolution.archive_prefix = archive_prefix;
		return true;
	});

	return true;
}

bool SyncLuaAssetsState(const MountedAssets &mounted_assets, const LauncherConfig &config) {
	return hg_lua_sync_launcher_assets(mounted_assets.folder_path.c_str(), mounted_assets.package_path.c_str(), mounted_assets.cwd.c_str(),
		config.assets.logical_data_path.c_str(), config.assets.archive_root.c_str());
}

void UnsyncLuaAssetsState(const MountedAssets &mounted_assets) { hg_lua_unsync_launcher_assets(mounted_assets.folder_path.c_str(), mounted_assets.package_path.c_str()); }

bool MountLauncherAssets(const std::string &cwd, MountedAssets &mounted_assets) {
	mounted_assets.cwd = cwd;

	const auto data_dir = hg::PathJoin(cwd, "data");
	if (hg::IsDir(data_dir.c_str()) && hg::AddAssetsFolder(data_dir.c_str())) {
		mounted_assets.source = LauncherAssetsSource::Folder;
		mounted_assets.folder_path = data_dir;
		return true;
	}

	for (const auto &filename : {"data.zip", "data.gsa", "data.nac"}) {
		const auto archive_path = hg::PathJoin(cwd, filename);
		if (!hg::Exists(archive_path.c_str()))
			continue;
		if (!hg::AddAssetsPackage(archive_path.c_str()))
			continue;

		mounted_assets.source = DetectArchiveSourceType(archive_path);
		mounted_assets.package_path = archive_path;
		return true;
	}

	return false;
}

void UnmountLauncherAssets(const MountedAssets &mounted_assets) {
	hg::ClearAssetsFolderResolver();

	if (!mounted_assets.folder_path.empty())
		hg::RemoveAssetsFolder(mounted_assets.folder_path.c_str());
	if (!mounted_assets.package_path.empty())
		hg::RemoveAssetsPackage(mounted_assets.package_path.c_str());
}

} // namespace

int LauncherMain() {
	const auto cwd = hg::GetCurrentWorkingDirectory();
	MountedAssets mounted_assets;
	if (!MountLauncherAssets(cwd, mounted_assets)) {
		PrintError("missing data source in current working directory: expected data/, data.zip, data.gsa, or data.nac");
		return 1;
	}

	std::string config_content, config_asset_name, error;
	if (!LoadLauncherConfigAsset(mounted_assets, config_asset_name, config_content, error)) {
		PrintError(error);
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	const auto config_source = ResolveAssetDisplayPath(mounted_assets, config_asset_name);
	LauncherConfig config;
	if (!ParseConfig(config_content, config_source, config, error)) {
		PrintError(error);
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	if (mounted_assets.source != LauncherAssetsSource::Folder && config.assets.archive_root.empty()) {
		std::string inferred_archive_root;
		if (!NormalizeRelativeAssetPath(hg::CutFileName(config_asset_name), inferred_archive_root)) {
			PrintError("invalid inferred archive root from configuration path: " + config_source);
			UnmountLauncherAssets(mounted_assets);
			return 1;
		}
		config.assets.archive_root = inferred_archive_root;
	}

	if (!InstallArchiveFolderResolver(mounted_assets, config)) {
		PrintError("failed to install archive assets resolver");
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	if (!SyncLuaAssetsState(mounted_assets, config)) {
		PrintError("failed to synchronize launcher assets with the Lua module");
		UnsyncLuaAssetsState(mounted_assets);
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	if (config.assets.pass_mode_argument)
		config.args.push_back(std::string("--assets-source=") + GetAssetsSourceName(mounted_assets.source));

	const auto exe_path = GetExecutablePath();
	const auto exe_dir = exe_path.empty() ? cwd : hg::CutFilePath(exe_path);
	const auto data_dir = hg::PathJoin(cwd, config.assets.logical_data_path);

	auto *L = luaL_newstate();
	if (L == nullptr) {
		PrintError("failed to create Lua state");
		UnsyncLuaAssetsState(mounted_assets);
		UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	luaL_openlibs(L);
	ConfigurePackagePaths(L, exe_dir, data_dir);
	InstallAssetSearcher(L);
	g_launcher_archive_root = config.assets.archive_root;

	lua_pushstring(L, mounted_assets.folder_path.c_str());
	lua_setglobal(L, "LAUNCHER_DATA_DIR");
	lua_pushstring(L, mounted_assets.package_path.c_str());
	lua_setglobal(L, "LAUNCHER_DATA_PACKAGE");
	lua_pushstring(L, config_source.c_str());
	lua_setglobal(L, "LAUNCHER_CONFIG_PATH");
	lua_pushstring(L, GetAssetsSourceName(mounted_assets.source));
	lua_setglobal(L, "LAUNCHER_ASSETS_SOURCE");

	const auto ok = RunEntryPoint(L, mounted_assets, config);
	g_launcher_archive_root.clear();
	lua_close(L);
	UnsyncLuaAssetsState(mounted_assets);
	UnmountLauncherAssets(mounted_assets);
	return ok ? 0 : 1;
}

int main() { return LauncherMain(); }

#if defined(_WIN32) && defined(HG_LUA_LAUNCHER_NO_CONSOLE)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return LauncherMain(); }
#endif
