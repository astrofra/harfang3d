// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/assets.h"
#include "engine/assets_internal.h"
#include "foundation/dir.h"
#include "foundation/file.h"
#include "foundation/path_tools.h"
#include "foundation/string.h"
#include "../utils.h"

#include <miniz/miniz.h>

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

using namespace hg;

namespace {

constexpr uint32_t kLegacyEnhancedMagic = 0x4E415244u;

struct TestArchiveEntry {
	std::string path;
	std::string content;
	bool compressed = false;
};

struct TempDirectory {
	TempDirectory() {
		path = test::CreateTempFilepath();
		if (Exists(path.c_str()))
			Unlink(path.c_str());
		MkTree(path.c_str());
	}

	~TempDirectory() {
		if (!path.empty())
			RmTree(path.c_str());
	}

	std::string path;
};

struct ScopedWorkingDirectory {
	explicit ScopedWorkingDirectory(const std::string &path) : previous(GetCurrentWorkingDirectory()) { SetCurrentWorkingDirectory(path); }
	~ScopedWorkingDirectory() { SetCurrentWorkingDirectory(previous); }

	std::string previous;
};

struct ScopedAssetsResolver {
	~ScopedAssetsResolver() { ClearAssetsFolderResolver(); }
};

bool IsDriveLetterPath(const std::string &path) { return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':'; }

bool NormalizeRelativePath(const std::string &path, std::string &normalized) {
	std::string value = path;
	replace_all(value, "\\", "/");

	if (!value.empty() && (value[0] == '/' || IsDriveLetterPath(value)))
		return false;

	std::vector<std::string> segments;
	for (const auto &segment : split(value, "/")) {
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

	normalized = join(segments.begin(), segments.end(), "/");
	return true;
}

bool JoinRelativePath(const std::string &prefix, const std::string &suffix, std::string &joined) {
	std::vector<std::string> segments = prefix.empty() ? std::vector<std::string>() : split(prefix, "/");
	const auto min_depth = segments.size();

	std::string value = suffix;
	replace_all(value, "\\", "/");

	if (!value.empty() && (value[0] == '/' || IsDriveLetterPath(value)))
		return false;

	for (const auto &segment : split(value, "/")) {
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

	joined = join(segments.begin(), segments.end(), "/");
	return true;
}

void WriteU32LE(File file, uint32_t value) {
	uint8_t bytes[4] = {
		static_cast<uint8_t>(value & 0xffu),
		static_cast<uint8_t>((value >> 8) & 0xffu),
		static_cast<uint8_t>((value >> 16) & 0xffu),
		static_cast<uint8_t>((value >> 24) & 0xffu),
	};
	TEST_CHECK(Write(file, bytes, sizeof(bytes)) == sizeof(bytes));
}

void AlignWrite(File file, uint32_t alignment) {
	if (alignment <= 1)
		return;

	uint8_t zero = 0;
	while ((Tell(file) % alignment) != 0)
		TEST_CHECK(Write(file, &zero, sizeof(zero)) == sizeof(zero));
}

void WriteZipArchive(const std::string &path, const std::vector<TestArchiveEntry> &entries) {
	mz_zip_archive archive;
	mz_zip_zero_struct(&archive);

	TEST_CHECK(mz_zip_writer_init_file(&archive, path.c_str(), 0) != MZ_FALSE);
	for (const auto &entry : entries) {
		const auto level = entry.compressed ? 6 : 0;
		TEST_CHECK(mz_zip_writer_add_mem(&archive, entry.path.c_str(), entry.content.data(), entry.content.size(), static_cast<mz_uint>(level)) != MZ_FALSE);
	}
	TEST_CHECK(mz_zip_writer_finalize_archive(&archive) != MZ_FALSE);
	TEST_CHECK(mz_zip_writer_end(&archive) != MZ_FALSE);
}

void WriteLegacyArchive(const std::string &path, const std::vector<TestArchiveEntry> &entries) {
	File file = OpenWrite(path.c_str());
	TEST_CHECK(IsValid(file) == true);

	WriteU32LE(file, kLegacyEnhancedMagic);
	WriteU32LE(file, 4);
	WriteU32LE(file, 0);

	for (const auto &entry : entries) {
		std::string alias = entry.path;
		replace_all(alias, "\\", "/");

		std::string payload = entry.content;
		uint8_t method = entry.compressed ? 1 : 0;

		if (entry.compressed) {
			mz_ulong compressed_size = mz_compressBound(static_cast<mz_ulong>(entry.content.size()));
			std::string compressed(compressed_size, '\0');
			TEST_CHECK(mz_compress2(reinterpret_cast<unsigned char *>(&compressed[0]), &compressed_size,
				reinterpret_cast<const unsigned char *>(entry.content.data()), static_cast<mz_ulong>(entry.content.size()), 6) == MZ_OK);
			compressed.resize(compressed_size);
			payload = std::move(compressed);
		}

		AlignWrite(file, 4);
		WriteU32LE(file, static_cast<uint32_t>(alias.size()));

		AlignWrite(file, 4);
		if (!alias.empty())
			TEST_CHECK(Write(file, alias.data(), alias.size()) == alias.size());

		AlignWrite(file, 4);
		TEST_CHECK(Write(file, &method, sizeof(method)) == sizeof(method));

		AlignWrite(file, 4);
		WriteU32LE(file, static_cast<uint32_t>(entry.content.size()));

		if (entry.compressed) {
			AlignWrite(file, 4);
			WriteU32LE(file, static_cast<uint32_t>(payload.size()));
		}

		AlignWrite(file, 4);
		if (!payload.empty())
			TEST_CHECK(Write(file, payload.data(), payload.size()) == payload.size());
	}

	WriteU32LE(file, 0xffffffffu);
	Close(file);
}

void InstallArchiveFolderResolverForTests(const std::string &archive_path, const std::string &logical_root, const std::string &archive_root) {
	SetAssetsFolderResolver([archive_path, logical_root, archive_root](const std::string &path, AssetsFolderResolution &resolution) {
		std::string normalized_path;
		if (!NormalizeRelativePath(CleanPath(path), normalized_path) || normalized_path.empty())
			return false;

		if (normalized_path != logical_root && !starts_with(normalized_path, logical_root + "/"))
			return false;

		std::string suffix;
		if (normalized_path.size() > logical_root.size())
			suffix = normalized_path.substr(logical_root.size() + 1);

		std::string archive_prefix;
		if (!JoinRelativePath(archive_root, suffix, archive_prefix))
			return false;

		resolution.logical_path = normalized_path;
		resolution.archive_path = archive_path;
		resolution.archive_prefix = archive_prefix;
		return true;
	});
}

} // namespace

void test_assets() {
	std::string pkg_path = "./data/package0000.zip";
	TEST_CHECK(AddAssetsPackage(pkg_path.c_str()) == true);

	Asset asset = OpenAsset("0000.txt");
	TEST_CHECK(IsValid(asset) == true);
	Close(asset);

	{
		std::string txt = AssetToString("0000.txt");
		TEST_CHECK(strcmp(txt.c_str(), "_TEST_ 0000") == 0);
	}

	{
		std::string txt = AssetToString("dir00/0000.txt");
		TEST_CHECK(strcmp(txt.c_str(), "test 0000.txt") == 0);
	}

	{
		std::string txt = AssetToString("dir00/dir02/0200.txt");
		TEST_CHECK(strcmp(txt.c_str(), "test 0200") == 0);
	}

	RemoveAssetsPackage(pkg_path.c_str());

	{
		TempDirectory dir;
		const auto archive_path = PathJoin(dir.path, "data.gsa");

		WriteLegacyArchive(archive_path, {
			{"raw.txt", "raw payload", false},
			{"nested/compressed.txt", "compressed payload", true},
		});

		TEST_CHECK(AddAssetsPackage(archive_path.c_str()) == true);
		TEST_CHECK(IsAssetFile("raw.txt") == true);
		TEST_CHECK(IsAssetFile("nested/compressed.txt") == true);
		TEST_CHECK(IsAssetFile("missing.txt") == false);
		TEST_CHECK(strcmp(AssetToString("raw.txt").c_str(), "raw payload") == 0);
		TEST_CHECK(strcmp(AssetToString("nested/compressed.txt").c_str(), "compressed payload") == 0);

		const auto missing = OpenAsset("missing.txt", true);
		TEST_CHECK(IsValid(missing) == false);

		RemoveAssetsPackage(archive_path.c_str());
	}

	{
		TempDirectory dir;
		const auto archive_path = PathJoin(dir.path, "bad.gsa");

		WriteLegacyArchive(archive_path, {
			{"../escape.txt", "bad", false},
		});

		TEST_CHECK(AddAssetsPackage(archive_path.c_str()) == false);
	}

	{
		TempDirectory dir;
		const auto data_dir = PathJoin(dir.path, "data");
		TEST_CHECK(MkTree(data_dir.c_str()) == true);
		TEST_CHECK(StringToFile(PathJoin(data_dir, "local.txt").c_str(), "local payload") == true);

		ScopedWorkingDirectory cwd(dir.path);
		TEST_CHECK(AddAssetsFolder("data") == true);
		TEST_CHECK(strcmp(AssetToString("local.txt").c_str(), "local payload") == 0);
		RemoveAssetsFolder("data");
		TEST_CHECK(IsAssetFile("local.txt") == false);
	}

	{
		TempDirectory dir;
		const auto archive_path = PathJoin(dir.path, "virtual.zip");
		WriteZipArchive(archive_path, {
			{"hello.txt", "zip root", false},
			{"subdir/nested.txt", "zip nested", true},
		});

		ScopedWorkingDirectory cwd(dir.path);
		ScopedAssetsResolver resolver;
		InstallArchiveFolderResolverForTests(archive_path, "data", "");

		TEST_CHECK(AddAssetsFolder("data") == true);
		TEST_CHECK(strcmp(AssetToString("hello.txt").c_str(), "zip root") == 0);
		RemoveAssetsFolder("data");
		TEST_CHECK(IsAssetFile("hello.txt") == false);

		TEST_CHECK(AddAssetsFolder("data/subdir") == true);
		TEST_CHECK(strcmp(AssetToString("nested.txt").c_str(), "zip nested") == 0);
		RemoveAssetsFolder("data/subdir");
		TEST_CHECK(IsAssetFile("nested.txt") == false);
	}

	{
		TempDirectory dir;
		const auto archive_path = PathJoin(dir.path, "virtual.gsa");
		WriteLegacyArchive(archive_path, {
			{"data/root.txt", "gsa root", false},
			{"data/scenes/scene.txt", "gsa scene", true},
		});

		ScopedWorkingDirectory cwd(dir.path);
		ScopedAssetsResolver resolver;
		InstallArchiveFolderResolverForTests(archive_path, "data", "data");

		TEST_CHECK(AddAssetsFolder("./data") == true);
		TEST_CHECK(strcmp(AssetToString("root.txt").c_str(), "gsa root") == 0);
		RemoveAssetsFolder("data");
		TEST_CHECK(IsAssetFile("root.txt") == false);

		TEST_CHECK(AddAssetsFolder("data/scenes") == true);
		TEST_CHECK(strcmp(AssetToString("scene.txt").c_str(), "gsa scene") == 0);
		RemoveAssetsFolder("data/scenes");
		TEST_CHECK(IsAssetFile("scene.txt") == false);
	}
}
