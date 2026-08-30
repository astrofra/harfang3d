#pragma once

#include <string>
#include <vector>

namespace hg {
namespace launcher_app {

enum class LauncherAssetsSource { None, Folder, Zip, Legacy };

struct LauncherAssetsConfig {
	std::string logical_data_path = "data";
	std::string archive_root;
	bool pass_mode_argument = false;
};

struct LauncherRuntimeConfig {
	bool hidpi = true;
};

struct LauncherConfig {
	std::string entry;
	std::vector<std::string> args;
	LauncherAssetsConfig assets;
	LauncherRuntimeConfig runtime;
};

struct MountedAssets {
	LauncherAssetsSource source = LauncherAssetsSource::None;
	std::string cwd;
	std::string folder_path;
	std::string package_path;
};

std::string GetExecutablePath();
const char *GetAssetsSourceName(LauncherAssetsSource source);

bool ParseConfig(const std::string &content, const std::string &source, LauncherConfig &config, std::string &error);
bool FinalizeConfigForMountedAssets(const MountedAssets &mounted_assets, const std::string &config_asset_name, const std::string &config_source, LauncherConfig &config,
	std::string &error);

bool NormalizeRelativeAssetPath(const std::string &path, std::string &normalized);
bool JoinArchivePath(const std::string &prefix, const std::string &suffix, std::string &joined);
bool ResolveAssetName(const std::string &name, std::string &resolved_name);
bool ResolveLauncherAssetName(const std::string &name, const std::string &archive_root, std::string &resolved_name);
bool NormalizeResolverInputPath(const std::string &path, const std::string &cwd, std::string &normalized);

bool LoadLauncherConfigAsset(const MountedAssets &mounted_assets, std::string &resolved_name, std::string &content, std::string &error);
std::string ResolveAssetDisplayPath(const MountedAssets &mounted_assets, const std::string &name);

bool InstallArchiveFolderResolver(const MountedAssets &mounted_assets, const LauncherConfig &config);
bool MountLauncherAssets(const std::string &cwd, MountedAssets &mounted_assets);
void UnmountLauncherAssets(const MountedAssets &mounted_assets);

} // namespace launcher_app
} // namespace hg
