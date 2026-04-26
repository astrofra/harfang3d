// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "assets_bridge.h"

#include <engine/assets.h>
#include <engine/assets_internal.h>

#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <cctype>
#include <string>
#include <vector>

namespace {

bool IsDriveLetterPath(const std::string &path) { return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':'; }

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

} // namespace

HG_LUA_BRIDGE_API bool hg_lua_sync_launcher_assets(const char *folder_path, const char *package_path, const char *cwd, const char *logical_data_path,
	const char *archive_root) {
	const std::string folder = folder_path ? folder_path : "";
	const std::string package = package_path ? package_path : "";
	const std::string process_cwd = cwd ? cwd : "";
	std::string logical_root = logical_data_path ? logical_data_path : "";
	std::string archive_prefix = archive_root ? archive_root : "";

	hg::ClearAssetsFolderResolver();
	if (!folder.empty())
		hg::RemoveAssetsFolder(folder.c_str());
	if (!package.empty())
		hg::RemoveAssetsPackage(package.c_str());

	if (!logical_root.empty() && (!NormalizeRelativeAssetPath(logical_root, logical_root) || logical_root.empty()))
		return false;
	if (!archive_prefix.empty() && !NormalizeRelativeAssetPath(archive_prefix, archive_prefix))
		return false;

	if (!folder.empty() && !hg::AddAssetsFolder(folder.c_str()))
		return false;
	if (!package.empty() && !hg::AddAssetsPackage(package.c_str()))
		return false;

	if (!package.empty() && !logical_root.empty()) {
		hg::SetAssetsFolderResolver([process_cwd, package, logical_root, archive_prefix](const std::string &path, hg::AssetsFolderResolution &resolution) {
			std::string normalized_input;
			if (!NormalizeResolverInputPath(path, process_cwd, normalized_input))
				return false;

			if (normalized_input != logical_root && !hg::starts_with(normalized_input, logical_root + "/"))
				return false;

			std::string suffix;
			if (normalized_input.size() > logical_root.size())
				suffix = normalized_input.substr(logical_root.size() + 1);

			std::string resolved_prefix;
			if (!JoinArchivePath(archive_prefix, suffix, resolved_prefix))
				return false;

			resolution.logical_path = normalized_input;
			resolution.archive_path = package;
			resolution.archive_prefix = resolved_prefix;
			return true;
		});
	}

	return true;
}

HG_LUA_BRIDGE_API void hg_lua_unsync_launcher_assets(const char *folder_path, const char *package_path) {
	hg::ClearAssetsFolderResolver();

	if (folder_path && *folder_path)
		hg::RemoveAssetsFolder(folder_path);
	if (package_path && *package_path)
		hg::RemoveAssetsPackage(package_path);
}
