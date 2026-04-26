// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#if defined(_WIN32)
#if defined(HG_LUA_BUILD_DLL)
#define HG_LUA_BRIDGE_API extern "C" __declspec(dllexport)
#else
#define HG_LUA_BRIDGE_API extern "C" __declspec(dllimport)
#endif
#else
#define HG_LUA_BRIDGE_API extern "C"
#endif

HG_LUA_BRIDGE_API bool hg_lua_sync_launcher_assets(const char *folder_path, const char *package_path, const char *cwd, const char *logical_data_path,
	const char *archive_root);
HG_LUA_BRIDGE_API void hg_lua_unsync_launcher_assets(const char *folder_path, const char *package_path);
