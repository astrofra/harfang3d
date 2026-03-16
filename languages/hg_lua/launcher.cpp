// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

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

bool LoadConfig(const std::string &path, LauncherConfig &config, std::string &error) {
	try {
		const auto content = hg::FileToString(path.c_str());
		if (content.empty()) {
			error = "configuration file is empty: " + path;
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
		error = "invalid JSON in " + path + ": " + e.what();
		return false;
	}
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

bool RunEntryPoint(lua_State *L, const std::string &entry_path, const std::vector<std::string> &args) {
	CreateArgTable(L, entry_path, args);

	auto status = luaL_loadfile(L, entry_path.c_str());
	if (status == LUA_OK) {
		const auto nargs = PushArgs(L);
		status = ProtectedCall(L, nargs, LUA_MULTRET);
	}

	if (status != LUA_OK) {
		const auto *message = lua_tostring(L, -1);
		PrintError(message != nullptr ? message : "unknown Lua error");
		lua_pop(L, 1);
		return false;
	}

	return true;
}

} // namespace

int main() {
	const auto cwd = hg::GetCurrentWorkingDirectory();
	const auto data_dir = hg::PathJoin(cwd, "data");
	const auto config_path = hg::PathJoin(data_dir, "launcher.json");

	if (!hg::IsDir(data_dir.c_str())) {
		PrintError("missing data directory in current working directory: " + data_dir);
		return 1;
	}

	if (!hg::Exists(config_path.c_str())) {
		PrintError("missing launcher configuration: " + config_path);
		return 1;
	}

	LauncherConfig config;
	std::string error;
	if (!LoadConfig(config_path, config, error)) {
		PrintError(error);
		return 1;
	}

	const auto entry_path = hg::CleanPath(hg::IsPathAbsolute(config.entry) ? config.entry : hg::PathJoin(data_dir, config.entry));
	if (!hg::Exists(entry_path.c_str())) {
		PrintError("entry script not found: " + entry_path);
		return 1;
	}

	const auto exe_path = GetExecutablePath();
	const auto exe_dir = exe_path.empty() ? cwd : hg::CutFilePath(exe_path);

	auto *L = luaL_newstate();
	if (L == nullptr) {
		PrintError("failed to create Lua state");
		return 1;
	}

	luaL_openlibs(L);
	ConfigurePackagePaths(L, exe_dir, data_dir);

	lua_pushstring(L, data_dir.c_str());
	lua_setglobal(L, "LAUNCHER_DATA_DIR");
	lua_pushstring(L, config_path.c_str());
	lua_setglobal(L, "LAUNCHER_CONFIG_PATH");

	const auto ok = RunEntryPoint(L, entry_path, config.args);
	lua_close(L);
	return ok ? 0 : 1;
}
