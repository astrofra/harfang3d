#include <algorithm>
#include <cstdarg>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
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

const char *kRequireCacheKey = "__hg_require_cache";

struct NativeLibraryHandle {
#ifdef _WIN32
	HMODULE handle{nullptr};
#else
	void *handle{nullptr};
#endif
};

std::vector<std::string> g_script_search_paths;
std::vector<std::string> g_native_search_paths;
std::vector<std::string> g_script_directory_stack;
std::unordered_map<std::string, NativeLibraryHandle> g_native_modules;

std::string NormalizePath(std::string path) {
	std::replace(path.begin(), path.end(), '\\', '/');
	return path;
}

std::string DirName(const std::string &path) {
	const auto normalized = NormalizePath(path);
	const auto pos = normalized.find_last_of('/');
	return pos == std::string::npos ? std::string{} : normalized.substr(0, pos);
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

bool HasFileExtension(const std::string &path) { return path.find_last_of('.') != std::string::npos; }

bool IsAbsolutePath(const std::string &path) {
	if (path.empty())
		return false;
#ifdef _WIN32
	if (path.size() > 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':')
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

std::string GetExecutablePath() {
#ifdef _WIN32
	std::vector<char> buffer(MAX_PATH, '\0');
	for (;;) {
		const DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0)
			return {};
		if (length < buffer.size() - 1)
			return NormalizePath(std::string(buffer.data(), length));
		buffer.resize(buffer.size() * 2);
	}
#else
	char buffer[PATH_MAX] = {};
	const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
	if (length <= 0)
		return {};
	buffer[length] = '\0';
	return NormalizePath(std::string(buffer));
#endif
}

std::string GetCurrentWorkingDirectory() {
#ifdef _WIN32
	std::vector<char> buffer(MAX_PATH, '\0');
	if (_getcwd(buffer.data(), static_cast<int>(buffer.size())) == nullptr)
		return {};
	return NormalizePath(buffer.data());
#else
	char buffer[PATH_MAX] = {};
	if (getcwd(buffer, sizeof(buffer)) == nullptr)
		return {};
	return NormalizePath(buffer);
#endif
}

void AddSearchPath(std::vector<std::string> &paths, const std::string &path) {
	if (path.empty())
		return;
	const auto normalized = NormalizePath(path);
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
		if (FileExists(candidate)) {
			resolved_path = candidate;
			return true;
		}
	}
	return false;
}

std::vector<std::string> BuildScriptRoots() {
	std::vector<std::string> roots;
	if (!g_script_directory_stack.empty())
		AddSearchPath(roots, g_script_directory_stack.back());
	for (const auto &path : g_script_search_paths)
		AddSearchPath(roots, path);
	return roots;
}

std::vector<std::string> BuildNativeRoots() {
	std::vector<std::string> roots;
	if (!g_script_directory_stack.empty())
		AddSearchPath(roots, g_script_directory_stack.back());
	for (const auto &path : g_native_search_paths)
		AddSearchPath(roots, path);
	return roots;
}

struct ScopedScriptDirectory {
	explicit ScopedScriptDirectory(const std::string &script_path) {
		const auto dir = DirName(script_path);
		if (!dir.empty()) {
			g_script_directory_stack.push_back(dir);
			active = true;
		}
	}

	~ScopedScriptDirectory() {
		if (active)
			g_script_directory_stack.pop_back();
	}

	bool active{false};
};

bool RunScriptFile(HSQUIRRELVM v, const std::string &path, int argc = 0, char **argv = nullptr, SQInteger *retval = nullptr) {
	if (SQ_FAILED(sqstd_loadfile(v, path.c_str(), SQTrue)))
		return false;

	ScopedScriptDirectory script_scope(path);

	int call_args = 1;
	sq_pushroottable(v);
	for (int i = 0; i < argc; ++i) {
		sq_pushstring(v, argv[i], -1);
		++call_args;
	}

	if (SQ_FAILED(sq_call(v, call_args, SQTrue, SQTrue)))
		return false;

	if (retval && sq_gettype(v, -1) == OT_INTEGER)
		sq_getinteger(v, -1, retval);
	return true;
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

bool ResolveScriptPath(const std::string &requested_path, std::string &resolved_path) {
	return ResolveFileFromRoots(requested_path, BuildScriptRoots(), ".nut", resolved_path);
}

bool ResolveNativeModulePath(const std::string &module_name, std::string &resolved_path) {
	return ResolveFileFromRoots(module_name, BuildNativeRoots(), NativeModuleExtension(), resolved_path);
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
	if (!ResolveNativeModulePath(module_name, library_path)) {
		return sq_throwerror(v, _SC("native module not found"));
	}

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

	std::string script_path;
	if (!ResolveScriptPath(path_cstr, script_path))
		return sq_throwerror(v, _SC("script not found"));

	const SQInteger top = sq_gettop(v);
	if (!RunScriptFile(v, script_path)) {
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

void PrintVersionInfos() { std::fprintf(stdout, "%s %s (%d bits)\n", SQUIRREL_VERSION, SQUIRREL_COPYRIGHT, int(sizeof(SQInteger) * 8)); }

void PrintUsage() {
	std::fprintf(stderr,
		"usage: hg_squirrel <script.nut> [args]\n"
		"available options:\n"
		"  -v   display version information\n"
		"  -h   print help\n");
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

int main(int argc, char **argv) {
	if (argc < 2) {
		PrintUsage();
		return 1;
	}

	if (std::strcmp(argv[1], "-v") == 0) {
		PrintVersionInfos();
		return 0;
	}

	if (std::strcmp(argv[1], "-h") == 0) {
		PrintVersionInfos();
		PrintUsage();
		return 0;
	}

	const auto executable_path = GetExecutablePath();
	const auto executable_dir = DirName(executable_path);
	const auto current_working_dir = GetCurrentWorkingDirectory();

	AddSearchPath(g_script_search_paths, current_working_dir);
	AddSearchPath(g_script_search_paths, executable_dir);
	AddSearchPath(g_script_search_paths, JoinPath(executable_dir, "harfang"));
	AddSearchPath(g_native_search_paths, executable_dir);
	AddSearchPath(g_native_search_paths, current_working_dir);

	std::string entry_script_path;
	if (!ResolveScriptPath(argv[1], entry_script_path)) {
		std::fprintf(stderr, "hg_squirrel: script not found: %s\n", argv[1]);
		return 2;
	}

	HSQUIRRELVM vm = sq_open(1024);
	if (!vm)
		return 3;

	sq_setprintfunc(vm, PrintFunc, ErrorFunc);
	sqstd_seterrorhandlers(vm);

	if (!RegisterStdLibs(vm) || SQ_FAILED(RegisterLauncherFunctions(vm))) {
		sq_close(vm);
		return 4;
	}

	SQInteger retval = 0;
	const bool ok = RunScriptFile(vm, entry_script_path, argc - 2, argc > 2 ? &argv[2] : nullptr, &retval);

	sq_close(vm);
	ReleaseNativeModules();
	return ok ? int(retval) : 5;
}
