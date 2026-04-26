// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <functional>
#include <string>

namespace hg {

struct AssetsFolderResolution {
	std::string logical_path;
	std::string archive_path;
	std::string archive_prefix;
};

using AssetsFolderResolver = std::function<bool(const std::string &path, AssetsFolderResolution &resolution)>;

// Internal launcher hook used to map AddAssetsFolder("data/...") to archive prefixes.
void SetAssetsFolderResolver(AssetsFolderResolver resolver);
void ClearAssetsFolderResolver();

} // namespace hg
