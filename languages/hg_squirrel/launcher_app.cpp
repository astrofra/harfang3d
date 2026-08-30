// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "../launcher_app_common.h"

#include <engine/assets.h>

#include <foundation/dir.h>
#include <foundation/file.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <platform/window_system.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

extern "C" {
#include "squirrel.h"
#include "sqstdaux.h"
#include "sqstdblob.h"
#include "sqstdio.h"
#include "sqstdmath.h"
#include "sqstdstring.h"
#include "sqstdsystem.h"
}

namespace {

using hg::launcher_app::LauncherConfig;
using hg::launcher_app::MountedAssets;

const char *kRequireCacheKey = "__hg_require_cache";

enum class ScriptSource { FileSystem, Asset };

struct ScriptContext {
	ScriptSource source = ScriptSource::FileSystem;
	std::string path;
	std::string display_path;
};

struct NativeLibraryHandle {
#ifdef _WIN32
	HMODULE handle{nullptr};
#else
	void *handle{nullptr};
#endif
};

MountedAssets g_mounted_assets;
std::string g_launcher_archive_root;
std::vector<std::string> g_script_search_paths;
std::vector<std::string> g_native_search_paths;
std::vector<ScriptContext> g_script_context_stack;
std::unordered_map<std::string, NativeLibraryHandle> g_native_modules;

void PrintError(const std::string &message) { std::fprintf(stderr, "launcher_squirrel: %s\n", message.c_str()); }

std::string NormalizePath(std::string path) {
	std::replace(path.begin(), path.end(), '\\', '/');
	return path;
}

std::string DirName(const std::string &path) {
	const auto normalized = NormalizePath(path);
	const auto pos = normalized.find_last_of('/');
	return pos == std::string::npos ? std::string() : normalized.substr(0, pos);
}

bool HasFileExtension(const std::string &path) {
	return path.find_last_of('.') != std::string::npos;
}

bool IsAbsolutePath(const std::string &path) {
	if (path.empty())
		return false;
#ifdef _WIN32
	if (path.size() > 2 && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':')
		return true;
	return path.size() > 1 && (path[0] == '\\' || path[0] == '/');
#else
	return path[0] == '/';
#endif
}

std::string JoinPath(const std::string &lhs, const std::string &rhs) {
	if (lhs.empty())
		return NormalizePath(rhs);
	if (rhs.empty())
		return NormalizePath(lhs);
	if (IsAbsolutePath(rhs))
		return NormalizePath(rhs);
	if (lhs.back() == '/' || lhs.back() == '\\')
		return NormalizePath(lhs + rhs);
	return NormalizePath(lhs + "/" + rhs);
}

bool FileExists(const std::string &path) {
	if (path.empty())
		return false;

	FILE *file = std::fopen(path.c_str(), "rb");
	if (!file)
		return false;

	std::fclose(file);
	return true;
}

void AddSearchPath(std::vector<std::string> &paths, const std::string &path) {
	if (path.empty())
		return;

	const auto normalized = NormalizePath(path);
	if (normalized.empty())
		return;

	if (std::find(paths.begin(), paths.end(), normalized) == paths.end())
		paths.push_back(normalized);
}

std::string NativeModuleExtension() {
#ifdef _WIN32
	return ".dll";
#elif __APPLE__
	return ".dylib";
#else
	return ".so";
#endif
}

std::string BaseNameWithoutExtension(const std::string &path) {
	auto normalized = NormalizePath(path);
	const auto slash = normalized.find_last_of('/');
	if (slash != std::string::npos)
		normalized = normalized.substr(slash + 1);

	const auto dot = normalized.find_last_of('.');
	if (dot != std::string::npos)
		normalized = normalized.substr(0, dot);

	return normalized;
}

std::string ModuleEntryPointName(const std::string &module_name) {
	std::string symbol = "sqmodule_" + BaseNameWithoutExtension(module_name);
	for (auto &ch : symbol) {
		if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_')
			ch = '_';
	}
	return symbol;
}

std::vector<std::string> BuildSearchCandidates(const std::string &name, const std::vector<std::string> &roots, const std::string &default_extension) {
	std::vector<std::string> candidates;

	if (IsAbsolutePath(name)) {
		candidates.push_back(NormalizePath(name));
		if (!HasFileExtension(name))
			candidates.push_back(NormalizePath(name + default_extension));
		return candidates;
	}

	for (const auto &root : roots) {
		candidates.push_back(JoinPath(root, name));
		if (!HasFileExtension(name))
			candidates.push_back(JoinPath(root, name + default_extension));
	}

	return candidates;
}

bool ResolveFileFromRoots(const std::string &name, const std::vector<std::string> &roots, const std::string &default_extension, std::string &resolved_path) {
	for (const auto &candidate : BuildSearchCandidates(name, roots, default_extension)) {
		if (!FileExists(candidate))
			continue;

		resolved_path = candidate;
		return true;
	}

	return false;
}

const ScriptContext *GetCurrentScriptContext() { return g_script_context_stack.empty() ? nullptr : &g_script_context_stack.back(); }

std::vector<std::string> BuildFileSystemScriptRoots() {
	std::vector<std::string> roots;

	if (const auto *context = GetCurrentScriptContext()) {
		if (context->source == ScriptSource::FileSystem)
			AddSearchPath(roots, DirName(context->path));
	}

	for (const auto &path : g_script_search_paths)
		AddSearchPath(roots, path);

	return roots;
}

std::vector<std::string> BuildNativeRoots() {
	std::vector<std::string> roots;

	if (const auto *context = GetCurrentScriptContext()) {
		if (context->source == ScriptSource::FileSystem)
			AddSearchPath(roots, DirName(context->path));
	}

	for (const auto &path : g_native_search_paths)
		AddSearchPath(roots, path);

	return roots;
}

std::vector<std::string> BuildScriptNameCandidates(const std::string &name) {
	std::vector<std::string> candidates;
	candidates.push_back(name);
	if (!HasFileExtension(name))
		candidates.push_back(name + ".nut");
	return candidates;
}

std::string AssetDirName(const std::string &path) {
	const auto pos = path.find_last_of('/');
	return pos == std::string::npos ? std::string() : path.substr(0, pos);
}

bool ResolveFileSystemScriptPath(const std::string &requested_path, std::string &resolved_path) {
	return ResolveFileFromRoots(requested_path, BuildFileSystemScriptRoots(), ".nut", resolved_path);
}

bool ResolveAssetScriptPath(const std::string &requested_path, std::string &resolved_path) {
	if (IsAbsolutePath(requested_path))
		return false;

	for (const auto &candidate : BuildScriptNameCandidates(requested_path)) {
		std::string normalized_candidate;
		if (!hg::launcher_app::NormalizeRelativeAssetPath(candidate, normalized_candidate))
			continue;

		if (const auto *context = GetCurrentScriptContext()) {
			if (context->source == ScriptSource::Asset) {
				const auto asset_dir = AssetDirName(context->path);
				if (!asset_dir.empty()) {
					std::string relative_candidate;
					if (hg::launcher_app::JoinArchivePath(asset_dir, normalized_candidate, relative_candidate) &&
						hg::launcher_app::ResolveLauncherAssetName(relative_candidate, g_launcher_archive_root, resolved_path))
						return true;
				}
			}
		}

		if (hg::launcher_app::ResolveLauncherAssetName(normalized_candidate, g_launcher_archive_root, resolved_path))
			return true;
	}

	return false;
}

bool ResolveAnyScriptPath(const std::string &requested_path, ScriptContext &context) {
	if (const auto *current = GetCurrentScriptContext()) {
		if (current->source == ScriptSource::Asset && !IsAbsolutePath(requested_path)) {
			if (ResolveAssetScriptPath(requested_path, context.path)) {
				context.source = ScriptSource::Asset;
				context.display_path = hg::launcher_app::ResolveAssetDisplayPath(g_mounted_assets, context.path);
				return true;
			}
		}
	}

	if (ResolveFileSystemScriptPath(requested_path, context.path)) {
		context.source = ScriptSource::FileSystem;
		context.display_path = context.path;
		return true;
	}

	if (ResolveAssetScriptPath(requested_path, context.path)) {
		context.source = ScriptSource::Asset;
		context.display_path = hg::launcher_app::ResolveAssetDisplayPath(g_mounted_assets, context.path);
		return true;
	}

	return false;
}

bool ResolveNativeModulePath(const std::string &module_name, std::string &resolved_path) {
	return ResolveFileFromRoots(module_name, BuildNativeRoots(), NativeModuleExtension(), resolved_path);
}

struct ScopedScriptContext {
	explicit ScopedScriptContext(const ScriptContext &context) { g_script_context_stack.push_back(context); }
	~ScopedScriptContext() { g_script_context_stack.pop_back(); }
};

bool CallLoadedScript(HSQUIRRELVM v, const std::vector<std::string> &args, SQInteger *retval) {
	SQInteger call_args = 1;
	sq_pushroottable(v);
	for (const auto &arg : args) {
		sq_pushstring(v, arg.c_str(), -1);
		++call_args;
	}

	if (SQ_FAILED(sq_call(v, call_args, retval != nullptr ? SQTrue : SQFalse, SQTrue)))
		return false;

	if (retval != nullptr && sq_gettype(v, -1) == OT_INTEGER)
		sq_getinteger(v, -1, retval);

	return true;
}

bool RunScriptFile(HSQUIRRELVM v, const ScriptContext &context, const std::vector<std::string> &args, SQInteger *retval = nullptr) {
	if (SQ_FAILED(sqstd_loadfile(v, context.path.c_str(), SQTrue)))
		return false;

	ScopedScriptContext script_scope(context);
	return CallLoadedScript(v, args, retval);
}

bool RunScriptBuffer(HSQUIRRELVM v, const ScriptContext &context, const hg::Data &source, const std::vector<std::string> &args, SQInteger *retval = nullptr) {
	if (SQ_FAILED(sq_compilebuffer(v, reinterpret_cast<const SQChar *>(source.GetData()), static_cast<SQInteger>(source.GetSize()), context.display_path.c_str(), SQTrue)))
		return false;

	ScopedScriptContext script_scope(context);
	return CallLoadedScript(v, args, retval);
}

bool RunAssetScript(HSQUIRRELVM v, const ScriptContext &context, const std::vector<std::string> &args, SQInteger *retval = nullptr) {
	const auto source = hg::AssetToData(context.path.c_str());
	return RunScriptBuffer(v, context, source, args, retval);
}

SQRESULT PushOrCreateRegistryTable(HSQUIRRELVM v, const char *name) {
	const SQInteger top = sq_gettop(v);

	sq_pushregistrytable(v);
	sq_pushstring(v, name, -1);
	if (SQ_SUCCEEDED(sq_get(v, -2))) {
		sq_remove(v, -2);
		return SQ_OK;
	}

	sq_settop(v, top);
	sq_pushregistrytable(v);
	sq_newtable(v);
	sq_pushstring(v, name, -1);
	sq_push(v, -2);
	if (SQ_FAILED(sq_newslot(v, -4, SQFalse))) {
		sq_settop(v, top);
		return SQ_ERROR;
	}

	sq_remove(v, -2);
	return SQ_OK;
}

bool PushCachedModule(HSQUIRRELVM v, const std::string &name) {
	const SQInteger top = sq_gettop(v);
	if (SQ_FAILED(PushOrCreateRegistryTable(v, kRequireCacheKey))) {
		sq_settop(v, top);
		return false;
	}

	sq_pushstring(v, name.c_str(), -1);
	if (SQ_FAILED(sq_get(v, -2))) {
		sq_settop(v, top);
		return false;
	}

	sq_remove(v, -2);
	return true;
}

SQRESULT CacheModule(HSQUIRRELVM v, const std::string &name, SQInteger module_idx) {
	HSQOBJECT module_object;
	sq_resetobject(&module_object);
	sq_getstackobj(v, module_idx, &module_object);

	if (SQ_FAILED(PushOrCreateRegistryTable(v, kRequireCacheKey)))
		return SQ_ERROR;

	sq_pushstring(v, name.c_str(), -1);
	sq_pushobject(v, module_object);
	if (SQ_FAILED(sq_newslot(v, -3, SQFalse))) {
		sq_poptop(v);
		return SQ_ERROR;
	}

	sq_poptop(v);
	return SQ_OK;
}

SQRESULT BindModuleInRootTable(HSQUIRRELVM v, const std::string &name, SQInteger module_idx) {
	HSQOBJECT module_object;
	sq_resetobject(&module_object);
	sq_getstackobj(v, module_idx, &module_object);

	sq_pushroottable(v);
	sq_pushstring(v, name.c_str(), -1);
	sq_pushobject(v, module_object);
	if (SQ_FAILED(sq_newslot(v, -3, SQFalse))) {
		sq_poptop(v);
		return SQ_ERROR;
	}

	sq_poptop(v);
	return SQ_OK;
}

bool RegisterStdLibs(HSQUIRRELVM v) {
	sq_pushroottable(v);
	const bool ok = SQ_SUCCEEDED(sqstd_register_iolib(v)) &&
		SQ_SUCCEEDED(sqstd_register_bloblib(v)) &&
		SQ_SUCCEEDED(sqstd_register_mathlib(v)) &&
		SQ_SUCCEEDED(sqstd_register_stringlib(v)) &&
		SQ_SUCCEEDED(sqstd_register_systemlib(v));
	sq_poptop(v);
	return ok;
}

NativeLibraryHandle LoadNativeLibrary(const std::string &path) {
	NativeLibraryHandle library;
#ifdef _WIN32
	library.handle = LoadLibraryA(path.c_str());
#else
	library.handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
	return library;
}

void *LookupNativeSymbol(const NativeLibraryHandle &library, const std::string &symbol) {
#ifdef _WIN32
	return library.handle ? reinterpret_cast<void *>(GetProcAddress(library.handle, symbol.c_str())) : nullptr;
#else
	return library.handle ? dlsym(library.handle, symbol.c_str()) : nullptr;
#endif
}

void ReleaseNativeLibrary(NativeLibraryHandle &library) {
#ifdef _WIN32
	if (library.handle)
		FreeLibrary(library.handle);
#else
	if (library.handle)
		dlclose(library.handle);
#endif
	library.handle = nullptr;
}

void ReleaseNativeModules() {
	for (auto &module : g_native_modules)
		ReleaseNativeLibrary(module.second);
	g_native_modules.clear();
}

using ModuleEntryPoint = SQRESULT (*)(HSQUIRRELVM);

SQInteger RequireModule(HSQUIRRELVM v) {
	const SQInteger top = sq_gettop(v);
	const SQChar *name_cstr = nullptr;
	if (SQ_FAILED(sq_getstring(v, 2, &name_cstr)))
		return sq_throwerror(v, _SC("require(module) expects a string module name"));

	const std::string module_name = name_cstr;
	if (PushCachedModule(v, module_name))
		return 1;

	std::string library_path;
	if (!ResolveNativeModulePath(module_name, library_path))
		return sq_throwerror(v, _SC("native module not found"));

	NativeLibraryHandle library = LoadNativeLibrary(library_path);
	if (!library.handle)
		return sq_throwerror(v, _SC("failed to load native module"));

	const auto symbol_name = ModuleEntryPointName(module_name);
	const auto entry_point = reinterpret_cast<ModuleEntryPoint>(LookupNativeSymbol(library, symbol_name));
	if (!entry_point) {
		ReleaseNativeLibrary(library);
		return sq_throwerror(v, _SC("native module entry point not found"));
	}

	if (SQ_FAILED(entry_point(v))) {
		ReleaseNativeLibrary(library);
		sq_settop(v, top);
		return SQ_ERROR;
	}

	const SQInteger module_idx = sq_gettop(v);
	if (SQ_FAILED(CacheModule(v, module_name, module_idx)) || SQ_FAILED(BindModuleInRootTable(v, module_name, module_idx))) {
		ReleaseNativeLibrary(library);
		sq_settop(v, top);
		return SQ_ERROR;
	}

	g_native_modules[module_name] = library;
	return 1;
}

SQInteger IncludeScript(HSQUIRRELVM v) {
	const SQChar *path_cstr = nullptr;
	if (SQ_FAILED(sq_getstring(v, 2, &path_cstr)))
		return sq_throwerror(v, _SC("include(path) expects a string path"));

	ScriptContext context;
	if (!ResolveAnyScriptPath(path_cstr, context))
		return sq_throwerror(v, _SC("script not found"));

	const SQInteger top = sq_gettop(v);
	const bool ok = context.source == ScriptSource::Asset ? RunAssetScript(v, context, {}) : RunScriptFile(v, context, {});
	if (!ok) {
		sq_settop(v, top);
		return SQ_ERROR;
	}

	sq_settop(v, top);
	return 0;
}

SQRESULT RegisterLauncherFunctions(HSQUIRRELVM v) {
	sq_pushroottable(v);

	sq_pushstring(v, _SC("require"), -1);
	sq_newclosure(v, RequireModule, 0);
	sq_setnativeclosurename(v, -1, _SC("require"));
	sq_setparamscheck(v, 2, _SC(".s"));
	if (SQ_FAILED(sq_newslot(v, -3, SQFalse))) {
		sq_poptop(v);
		return SQ_ERROR;
	}

	sq_pushstring(v, _SC("include"), -1);
	sq_newclosure(v, IncludeScript, 0);
	sq_setnativeclosurename(v, -1, _SC("include"));
	sq_setparamscheck(v, 2, _SC(".s"));
	if (SQ_FAILED(sq_newslot(v, -3, SQFalse))) {
		sq_poptop(v);
		return SQ_ERROR;
	}

	sq_pushstring(v, _SC("Include"), -1);
	sq_pushstring(v, _SC("include"), -1);
	if (SQ_FAILED(sq_get(v, -3)) || SQ_FAILED(sq_newslot(v, -3, SQFalse))) {
		sq_poptop(v);
		return SQ_ERROR;
	}

	sq_pushstring(v, _SC("Import"), -1);
	sq_pushstring(v, _SC("include"), -1);
	if (SQ_FAILED(sq_get(v, -3)) || SQ_FAILED(sq_newslot(v, -3, SQFalse))) {
		sq_poptop(v);
		return SQ_ERROR;
	}

	sq_poptop(v);
	return SQ_OK;
}

bool SetRootString(HSQUIRRELVM v, const SQChar *key, const std::string &value) {
	sq_pushroottable(v);
	sq_pushstring(v, key, -1);
	sq_pushstring(v, value.c_str(), -1);
	if (SQ_FAILED(sq_newslot(v, -3, SQFalse))) {
		sq_poptop(v);
		return false;
	}

	sq_poptop(v);
	return true;
}

bool RunEntryPoint(HSQUIRRELVM v, const LauncherConfig &config, SQInteger &retval) {
	if (IsAbsolutePath(config.entry)) {
		const ScriptContext context{ScriptSource::FileSystem, NormalizePath(config.entry), NormalizePath(config.entry)};
		return RunScriptFile(v, context, config.args, &retval);
	}

	ScriptContext context;
	context.source = ScriptSource::Asset;
	if (!hg::launcher_app::ResolveLauncherAssetName(config.entry, config.assets.archive_root, context.path)) {
		PrintError("entry script not found in mounted assets: " + config.entry);
		return false;
	}

	context.display_path = hg::launcher_app::ResolveAssetDisplayPath(g_mounted_assets, context.path);
	return RunAssetScript(v, context, config.args, &retval);
}

void PrintFunc(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
#ifdef SQUNICODE
	vfwprintf(stdout, s, vl);
#else
	vfprintf(stdout, s, vl);
#endif
	va_end(vl);
}

void ErrorFunc(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
#ifdef SQUNICODE
	vfwprintf(stderr, s, vl);
#else
	vfprintf(stderr, s, vl);
#endif
	va_end(vl);
}

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

	if (config.assets.pass_mode_argument)
		config.args.push_back(std::string("--assets-source=") + hg::launcher_app::GetAssetsSourceName(mounted_assets.source));

	const auto executable_path = hg::launcher_app::GetExecutablePath();
	const auto executable_dir = executable_path.empty() ? cwd : DirName(executable_path);

	g_mounted_assets = mounted_assets;
	g_launcher_archive_root = config.assets.archive_root;
	g_script_search_paths.clear();
	g_native_search_paths.clear();
	AddSearchPath(g_script_search_paths, cwd);
	AddSearchPath(g_script_search_paths, executable_dir);
	AddSearchPath(g_script_search_paths, JoinPath(executable_dir, "harfang"));
	AddSearchPath(g_native_search_paths, executable_dir);
	AddSearchPath(g_native_search_paths, cwd);

	HSQUIRRELVM vm = sq_open(1024);
	if (!vm) {
		PrintError("failed to create Squirrel VM");
		g_launcher_archive_root.clear();
		g_mounted_assets = MountedAssets{};
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	sq_setprintfunc(vm, PrintFunc, ErrorFunc);
	sqstd_seterrorhandlers(vm);

	if (!RegisterStdLibs(vm) || SQ_FAILED(RegisterLauncherFunctions(vm)) ||
		!SetRootString(vm, _SC("LAUNCHER_DATA_DIR"), mounted_assets.folder_path) ||
		!SetRootString(vm, _SC("LAUNCHER_DATA_PACKAGE"), mounted_assets.package_path) ||
		!SetRootString(vm, _SC("LAUNCHER_CONFIG_PATH"), config_source) ||
		!SetRootString(vm, _SC("LAUNCHER_ASSETS_SOURCE"), hg::launcher_app::GetAssetsSourceName(mounted_assets.source))) {
		sq_close(vm);
		ReleaseNativeModules();
		g_launcher_archive_root.clear();
		g_mounted_assets = MountedAssets{};
		hg::launcher_app::UnmountLauncherAssets(mounted_assets);
		return 1;
	}

	SQInteger retval = 0;
	const bool ok = RunEntryPoint(vm, config, retval);

	sq_close(vm);
	ReleaseNativeModules();
	g_script_context_stack.clear();
	g_script_search_paths.clear();
	g_native_search_paths.clear();
	g_launcher_archive_root.clear();
	g_mounted_assets = MountedAssets{};
	hg::launcher_app::UnmountLauncherAssets(mounted_assets);
	return ok ? static_cast<int>(retval) : 1;
}

int main() { return LauncherMain(); }

#if defined(_WIN32) && defined(HG_SQUIRREL_LAUNCHER_NO_CONSOLE)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return LauncherMain(); }
#endif
