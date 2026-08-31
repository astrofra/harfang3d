// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "assets_bridge.h"
#include "../launcher_app_common.h"

#include <engine/assets.h>

#include <foundation/dir.h>
#include <foundation/path_tools.h>

#include <platform/window_system.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

using hg::launcher_app::LauncherConfig;
using hg::launcher_app::MountedAssets;

std::string g_launcher_archive_root;

void PrintError(const std::string &message) { std::cerr << "launcher: " << message << std::endl; }

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
		if (!hg::launcher_app::ResolveLauncherAssetName(candidate, g_launcher_archive_root, resolved_name))
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
	if (!hg::launcher_app::ResolveLauncherAssetName(config.entry, config.assets.archive_root, resolved_name)) {
		PrintError("entry script not found in mounted assets: " + config.entry);
		return false;
	}

	const auto source = hg::AssetToData(resolved_name.c_str());
	const auto display_path = hg::launcher_app::ResolveAssetDisplayPath(mounted_assets, resolved_name);
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

bool SyncLuaAssetsState(const MountedAssets &mounted_assets, const LauncherConfig &config) {
	return hg_lua_sync_launcher_assets(mounted_assets.folder_path.c_str(), mounted_assets.package_path.c_str(), mounted_assets.cwd.c_str(),
		config.assets.logical_data_path.c_str(), config.assets.archive_root.c_str());
}

void UnsyncLuaAssetsState(const MountedAssets &mounted_assets) { hg_lua_unsync_launcher_assets(mounted_assets.folder_path.c_str(), mounted_assets.package_path.c_str()); }

} // namespace

int LauncherMain() {
	const auto cwd = hg::GetCurrentWorkingDirectory();
	MountedAssets mounted_assets;
	if (!hg::launcher_app::MountLauncherAssets(cwd, mounted_assets)) {
		PrintError("missing data source in current working directory: expected data/, data.zip, data.gsa, or data.nac");
		return 1;
	}

	std::string config_content, config_asset_name, error;
	if (!hg::launcher_app::LoadLauncherConfigAsset(mounted_assets, config_asset_name, config_content, error)) {
		PrintError(error);
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	const auto config_source = hg::launcher_app::ResolveAssetDisplayPath(mounted_assets, config_asset_name);
	LauncherConfig config;
	if (!hg::launcher_app::ParseConfig(config_content, config_source, config, error)) {
		PrintError(error);
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	if (!hg::launcher_app::FinalizeConfigForMountedAssets(mounted_assets, config_asset_name, config_source, config, error)) {
		PrintError(error);
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	hg::SetHiDPIMode(config.runtime.hidpi ? hg::HDPIM_Enabled : hg::HDPIM_Disabled);

	if (!hg::launcher_app::InstallArchiveFolderResolver(mounted_assets, config)) {
		PrintError("failed to install archive assets resolver");
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	if (!SyncLuaAssetsState(mounted_assets, config)) {
		PrintError("failed to synchronize launcher assets with the Lua module");
		UnsyncLuaAssetsState(mounted_assets);
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	if (config.assets.pass_mode_argument)
		config.args.push_back(std::string("--assets-source=") + hg::launcher_app::GetAssetsSourceName(mounted_assets.source));

	const auto exe_path = hg::launcher_app::GetExecutablePath();
	const auto exe_dir = exe_path.empty() ? cwd : hg::CutFilePath(exe_path);
	const auto data_dir = hg::PathJoin(cwd, config.assets.logical_data_path);

	auto *L = luaL_newstate();
	if (L == nullptr) {
		PrintError("failed to create Lua state");
		UnsyncLuaAssetsState(mounted_assets);
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
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
	lua_pushstring(L, hg::launcher_app::GetAssetsSourceName(mounted_assets.source));
	lua_setglobal(L, "LAUNCHER_ASSETS_SOURCE");

	const auto ok = RunEntryPoint(L, mounted_assets, config);
	g_launcher_archive_root.clear();
	lua_close(L);
	UnsyncLuaAssetsState(mounted_assets);
	hg::launcher_app::UnmountLauncherAssets(mounted_assets);
	return ok ? 0 : 1;
}

int main() { return LauncherMain(); }

#if defined(_WIN32) && defined(HG_LUA_LAUNCHER_NO_CONSOLE)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return LauncherMain(); }
#endif
