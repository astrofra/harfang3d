// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "launcher_app_common.h"

#include <engine/assets.h>
#include <engine/assets_internal.h>

#include <foundation/data.h>
#include <foundation/dir.h>
#include <foundation/file.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <json/json.hpp>

#include <cctype>
#include <cstdint>
#include <set>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace hg {
namespace launcher_app {
namespace {

using json = nlohmann::json;

constexpr uint32_t kZipLocalHeaderMagic = 0x04034b50u;
constexpr uint32_t kZipEmptyArchiveMagic = 0x06054b50u;
constexpr uint32_t kZipSpannedArchiveMagic = 0x08074b50u;
constexpr uint32_t kLegacyEnhancedMagic = 0x4E415244u;
constexpr uint32_t kLegacyLegacyMagic = 0x4E415243u;

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

} // namespace

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

		if (js.contains("runtime")) {
			if (!js["runtime"].is_object()) {
				error = "'runtime' must be a JSON object";
				return false;
			}

			const auto &runtime = js["runtime"];
			if (runtime.contains("hidpi")) {
				if (!runtime["hidpi"].is_boolean()) {
					error = "'runtime.hidpi' must be a boolean";
					return false;
				}
				config.runtime.hidpi = runtime["hidpi"].get<bool>();
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

bool FinalizeConfigForMountedAssets(const MountedAssets &mounted_assets, const std::string &config_asset_name, const std::string &config_source, LauncherConfig &config,
	std::string &error) {
	if (mounted_assets.source == LauncherAssetsSource::Folder || !config.assets.archive_root.empty())
		return true;

	std::string inferred_archive_root;
	if (!NormalizeRelativeAssetPath(hg::CutFileName(config_asset_name), inferred_archive_root)) {
		error = "invalid inferred archive root from configuration path: " + config_source;
		return false;
	}

	config.assets.archive_root = inferred_archive_root;
	return true;
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

bool ResolveAssetName(const std::string &name, std::string &resolved_name) {
	for (const auto &candidate : MakeAssetPathCandidates(name)) {
		if (!hg::IsAssetFile(candidate.c_str()))
			continue;

		resolved_name = candidate;
		return true;
	}

	return false;
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

} // namespace launcher_app
} // namespace hg
