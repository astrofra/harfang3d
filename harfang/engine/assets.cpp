// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/assets.h"
#include "engine/assets_internal.h"
#include "foundation/dir.h"
#include "foundation/file.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"
#include "foundation/string.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <miniz/miniz.h>

namespace hg {

namespace {

constexpr uint32_t kZipLocalHeaderMagic = 0x04034b50u;
constexpr uint32_t kZipEmptyArchiveMagic = 0x06054b50u;
constexpr uint32_t kZipSpannedArchiveMagic = 0x08074b50u;
constexpr uint32_t kGameStartEnhancedMagic = 0x4E415244u;
constexpr uint32_t kGameStartLegacyMagic = 0x4E415243u;
constexpr uint64_t kMaxArchiveEntrySize = 512ull * 1024ull * 1024ull;

enum class ArchiveKind { Unknown, Zip, GameStart };
enum class FolderSourceType { Local, Archive };

struct AssetContainer {
	AssetContainer(ArchiveKind kind_, std::string filename_) : kind(kind_), filename(std::move(filename_)) {}
	virtual ~AssetContainer() = default;

	virtual bool IsFile(const std::string &archive_path) const = 0;
	virtual bool ReadFile(const std::string &archive_path, std::string &data, std::string *error) const = 0;
	virtual std::string DisplayPath(const std::string &archive_path) const = 0;

	ArchiveKind kind = ArchiveKind::Unknown;
	std::string filename;
};

struct ZipAssetContainer final : AssetContainer {
	mz_zip_archive archive;

	ZipAssetContainer(const std::string &filename) : AssetContainer(ArchiveKind::Zip, filename) { mz_zip_zero_struct(&archive); }
	~ZipAssetContainer() override { mz_zip_reader_end(&archive); }

	bool Initialize(std::string &error) {
		if (mz_zip_reader_init_file(&archive, filename.c_str(), MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY) == MZ_FALSE) {
			error = mz_zip_get_error_string(mz_zip_get_last_error(&archive));
			return false;
		}
		return true;
	}

	bool IsFile(const std::string &archive_path) const override {
		const auto index = mz_zip_reader_locate_file(const_cast<mz_zip_archive *>(&archive), archive_path.c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
		if (index < 0)
			return false;
		return mz_zip_reader_is_file_a_directory(const_cast<mz_zip_archive *>(&archive), static_cast<mz_uint>(index)) == MZ_FALSE;
	}

	bool ReadFile(const std::string &archive_path, std::string &data, std::string *error) const override {
		const auto index = mz_zip_reader_locate_file(const_cast<mz_zip_archive *>(&archive), archive_path.c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
		if (index < 0)
			return false;

		if (mz_zip_reader_is_file_a_directory(const_cast<mz_zip_archive *>(&archive), static_cast<mz_uint>(index)) != MZ_FALSE) {
			if (error)
				*error = "entry is a directory";
			return false;
		}

		size_t size = 0;
		void *buffer = mz_zip_reader_extract_to_heap(const_cast<mz_zip_archive *>(&archive), static_cast<mz_uint>(index), &size, 0);
		if (!buffer) {
			if (error)
				*error = mz_zip_get_error_string(mz_zip_get_last_error(const_cast<mz_zip_archive *>(&archive)));
			return false;
		}

		data.assign(static_cast<const char *>(buffer), size);
		mz_free(buffer);
		return true;
	}

	std::string DisplayPath(const std::string &archive_path) const override { return filename + ":" + archive_path; }
};

struct RawBinaryFile {
	RawBinaryFile() = default;
	~RawBinaryFile() { Close(); }

	bool Open(const std::string &path, std::string &error) {
#if _WIN32
		errno_t err = _wfopen_s(&handle, utf8_to_wchar(path).c_str(), L"rb");
		if (err != 0 || handle == nullptr) {
			char msg[256] = {};
			strerror_s(msg, sizeof(msg), err);
			error = format("failed to open '%1': %2").arg(path).arg(msg).str();
			return false;
		}
#else
		handle = fopen(path.c_str(), "rb");
		if (!handle) {
			error = format("failed to open '%1': %2").arg(path).arg(strerror(errno)).str();
			return false;
		}
#endif
		return true;
	}

	void Close() {
		if (handle) {
			fclose(handle);
			handle = nullptr;
		}
	}

	bool ReadExact(void *data, size_t size) const { return size == 0 || (handle && fread(data, 1, size, handle) == size); }

	bool Seek(uint64_t offset) const {
		if (!handle)
			return false;
#if _WIN32
		return _fseeki64(handle, static_cast<__int64>(offset), SEEK_SET) == 0;
#else
		return fseeko(handle, static_cast<off_t>(offset), SEEK_SET) == 0;
#endif
	}

	uint64_t Tell() const {
		if (!handle)
			return 0;
#if _WIN32
		return static_cast<uint64_t>(_ftelli64(handle));
#else
		return static_cast<uint64_t>(ftello(handle));
#endif
	}

	FILE *handle = nullptr;
};

struct GameStartEntry {
	uint8_t method = 0;
	uint32_t length = 0;
	uint32_t compressed_length = 0;
	uint64_t data_offset = 0;
};

struct GameStartMetadata {
	uint32_t revision = 0;
	uint32_t offset_padding = 0;
	uint32_t size_padding = 0;
	uint64_t file_size = 0;
	uint32_t entry_count = 0;
};

bool ReadU8(const RawBinaryFile &file, uint8_t &value) { return file.ReadExact(&value, sizeof(value)); }

bool ReadU32LE(const RawBinaryFile &file, uint32_t &value) {
	uint8_t bytes[4];
	if (!file.ReadExact(bytes, sizeof(bytes)))
		return false;
	value = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) | (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
	return true;
}

bool PeekU32LE(const RawBinaryFile &file, uint32_t &value) {
	const auto cursor = file.Tell();
	if (!ReadU32LE(file, value))
		return false;
	return file.Seek(cursor);
}

bool IsDriveLetterPath(const std::string &path) { return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':'; }

bool NormalizeArchiveSegments(const std::string &path, std::vector<std::string> &segments, size_t min_depth) {
	std::string normalized = path;
	replace_all(normalized, "\\", "/");

	if (!normalized.empty() && (normalized[0] == '/' || IsDriveLetterPath(normalized)))
		return false;

	for (const auto &segment : split(normalized, "/")) {
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

	return true;
}

bool NormalizeArchivePrefix(const std::string &path, std::string &normalized) {
	std::vector<std::string> segments;
	if (!NormalizeArchiveSegments(path, segments, 0))
		return false;
	normalized = join(segments.begin(), segments.end(), "/");
	return true;
}

bool ResolveArchiveAssetPath(const std::string &prefix, const std::string &name, std::string &resolved) {
	std::vector<std::string> segments = prefix.empty() ? std::vector<std::string>() : split(prefix, "/");
	const auto min_depth = segments.size();

	if (!NormalizeArchiveSegments(name, segments, min_depth))
		return false;

	resolved = join(segments.begin(), segments.end(), "/");
	return !resolved.empty();
}

bool AlignRead(const RawBinaryFile &file, uint32_t alignment, uint64_t file_size, const std::string &field_name, std::string &error) {
	if (alignment <= 1)
		return true;

	const auto cursor = file.Tell();
	const auto aligned = ((cursor + alignment - 1) / alignment) * alignment;
	if (aligned > file_size) {
		error = "truncated " + field_name;
		return false;
	}
	if (!file.Seek(aligned)) {
		error = "failed to seek to " + field_name;
		return false;
	}
	return true;
}

struct GameStartAssetContainer final : AssetContainer {
	GameStartAssetContainer(const std::string &filename) : AssetContainer(ArchiveKind::GameStart, filename) {}

	bool Initialize(std::string &error) {
		const auto info = GetFileInfo(filename.c_str());
		if (!info.is_file) {
			error = "file not found";
			return false;
		}

		metadata.file_size = static_cast<uint64_t>(info.size);

		RawBinaryFile file;
		if (!file.Open(filename, error))
			return false;

		uint32_t magic = 0;
		if (!ReadU32LE(file, magic)) {
			error = "truncated archive header";
			return false;
		}

		if (magic == kGameStartEnhancedMagic) {
			metadata.revision = 1;
			if (!ReadU32LE(file, metadata.offset_padding) || !ReadU32LE(file, metadata.size_padding)) {
				error = "truncated enhanced archive header";
				return false;
			}
		} else if (magic == kGameStartLegacyMagic) {
			metadata.revision = 0;
			metadata.offset_padding = 0;
			metadata.size_padding = 0;
		} else {
			error = format("invalid GameStart archive magic 0x%1").arg(static_cast<int>(magic)).str();
			return false;
		}

		while (true) {
			const auto cursor = file.Tell();
			if (cursor == metadata.file_size)
				break;

			if (metadata.file_size - cursor < sizeof(uint32_t)) {
				error = "truncated archive end marker";
				return false;
			}

			uint32_t marker = 0;
			if (!PeekU32LE(file, marker)) {
				error = "truncated archive marker";
				return false;
			}
			if (marker == 0xffffffffu)
				break;

			if (!AlignRead(file, metadata.offset_padding, metadata.file_size, "entry alias length", error))
				return false;

			uint32_t alias_length = 0;
			if (!ReadU32LE(file, alias_length)) {
				error = "truncated entry alias length";
				return false;
			}
			if (alias_length == 0xffffffffu)
				break;
			if (alias_length == 0 || alias_length > 511) {
				error = format("invalid alias length %1").arg(static_cast<int>(alias_length)).str();
				return false;
			}

			if (!AlignRead(file, metadata.offset_padding, metadata.file_size, "entry alias", error))
				return false;

			std::string alias(alias_length, '\0');
			if (!file.ReadExact(&alias[0], alias.size())) {
				error = "truncated entry alias";
				return false;
			}

			std::string normalized_alias;
			if (!NormalizeArchivePrefix(alias, normalized_alias) || normalized_alias.empty()) {
				error = format("invalid entry '%1'").arg(alias).str();
				return false;
			}
			if (entries.find(normalized_alias) != entries.end()) {
				error = format("duplicate entry '%1'").arg(normalized_alias).str();
				return false;
			}

			if (!AlignRead(file, metadata.offset_padding, metadata.file_size, "entry method", error))
				return false;

			uint8_t method = 0;
			if (!ReadU8(file, method)) {
				error = format("truncated entry method for '%1'").arg(normalized_alias).str();
				return false;
			}
			if ((method & ~0x1u) != 0) {
				error = format("unsupported method %1 for entry '%2'").arg(static_cast<int>(method)).arg(normalized_alias).str();
				return false;
			}

			if (!AlignRead(file, metadata.offset_padding, metadata.file_size, "entry size", error))
				return false;

			uint32_t length = 0;
			if (!ReadU32LE(file, length)) {
				error = format("truncated entry size for '%1'").arg(normalized_alias).str();
				return false;
			}

			uint32_t compressed_length = length;
			if ((method & 0x1u) != 0) {
				if (!AlignRead(file, metadata.offset_padding, metadata.file_size, "entry compressed size", error))
					return false;
				if (!ReadU32LE(file, compressed_length)) {
					error = format("truncated compressed size for '%1'").arg(normalized_alias).str();
					return false;
				}
			}

			if (static_cast<uint64_t>(length) > kMaxArchiveEntrySize) {
				error = format("entry '%1' exceeds the maximum supported size").arg(normalized_alias).str();
				return false;
			}

			if (!AlignRead(file, metadata.offset_padding, metadata.file_size, "entry payload", error))
				return false;

			const auto data_offset = file.Tell();
			const auto stored_length = static_cast<uint64_t>((method & 0x1u) != 0 ? compressed_length : length);
			if (data_offset > metadata.file_size || stored_length > metadata.file_size - data_offset) {
				error = format("entry '%1' payload exceeds archive bounds").arg(normalized_alias).str();
				return false;
			}

			entries[normalized_alias] = {method, length, compressed_length, data_offset};
			metadata.entry_count++;

			if (!file.Seek(data_offset + stored_length)) {
				error = format("failed to skip payload for entry '%1'").arg(normalized_alias).str();
				return false;
			}
		}

		return true;
	}

	bool IsFile(const std::string &archive_path) const override { return entries.find(archive_path) != entries.end(); }

	bool ReadFile(const std::string &archive_path, std::string &data, std::string *error) const override {
		const auto it = entries.find(archive_path);
		if (it == entries.end())
			return false;

		const auto &entry = it->second;
		const auto stored_length = static_cast<size_t>((entry.method & 0x1u) != 0 ? entry.compressed_length : entry.length);

		RawBinaryFile file;
		std::string local_error;
		if (!file.Open(filename, local_error)) {
			if (error)
				*error = local_error;
			return false;
		}
		if (!file.Seek(entry.data_offset)) {
			if (error)
				*error = "failed to seek to entry payload";
			return false;
		}

		std::string payload(stored_length, '\0');
		if (stored_length > 0 && !file.ReadExact(&payload[0], stored_length)) {
			if (error)
				*error = "failed to read entry payload";
			return false;
		}

		if ((entry.method & 0x1u) == 0) {
			data = std::move(payload);
			return true;
		}

		data.resize(entry.length);
		mz_ulong decoded_length = static_cast<mz_ulong>(data.size());
		const auto result = mz_uncompress(reinterpret_cast<unsigned char *>(data.empty() ? nullptr : &data[0]), &decoded_length,
			reinterpret_cast<const unsigned char *>(payload.empty() ? nullptr : payload.data()), static_cast<mz_ulong>(payload.size()));

		if (result != MZ_OK || decoded_length != data.size()) {
			data.clear();
			if (error)
				*error = result == MZ_OK ? "decompression size mismatch" : "zlib decompression failed";
			return false;
		}

		return true;
	}

	std::string DisplayPath(const std::string &archive_path) const override { return filename + ":" + archive_path; }

	GameStartMetadata metadata;
	std::map<std::string, GameStartEntry> entries;
};

const char *GetArchiveKindName(ArchiveKind kind) {
	switch (kind) {
		case ArchiveKind::Zip:
			return "ZIP";
		case ArchiveKind::GameStart:
			return "GameStart";
		default:
			return "archive";
	}
}

bool IsZipMagic(uint32_t magic) { return magic == kZipLocalHeaderMagic || magic == kZipEmptyArchiveMagic || magic == kZipSpannedArchiveMagic; }

bool DetectArchiveKind(const std::string &path, ArchiveKind &kind, std::string &error) {
	kind = ArchiveKind::Unknown;

	RawBinaryFile file;
	if (!file.Open(path, error))
		return false;

	uint32_t magic = 0;
	if (!ReadU32LE(file, magic)) {
		error = "truncated archive header";
		return false;
	}

	if (IsZipMagic(magic)) {
		kind = ArchiveKind::Zip;
		return true;
	}

	if (magic == kGameStartEnhancedMagic || magic == kGameStartLegacyMagic) {
		kind = ArchiveKind::GameStart;
		return true;
	}

	error = "unsupported archive format";
	return false;
}

struct FolderSource {
	FolderSourceType type = FolderSourceType::Local;
	std::string logical_path;
	std::string local_path;
	std::string archive_prefix;
	std::shared_ptr<AssetContainer> container;
};

struct PackageSource {
	std::string path;
	std::shared_ptr<AssetContainer> container;
};

static std::mutex assets_mutex;
static std::deque<FolderSource> assets_folders;
static std::deque<PackageSource> assets_packages;
static std::map<std::string, std::weak_ptr<AssetContainer>> archive_containers;
static AssetsFolderResolver g_assets_folder_resolver;

bool GetOrCreateArchiveContainer(const std::string &path, std::shared_ptr<AssetContainer> &container, std::string &error, ArchiveKind *kind = nullptr) {
	auto it = archive_containers.find(path);
	if (it != archive_containers.end()) {
		if (auto cached = it->second.lock()) {
			container = std::move(cached);
			if (kind)
				*kind = container->kind;
			return true;
		}
		archive_containers.erase(it);
	}

	ArchiveKind detected_kind = ArchiveKind::Unknown;
	if (!DetectArchiveKind(path, detected_kind, error))
		return false;

	switch (detected_kind) {
		case ArchiveKind::Zip: {
			auto zip = std::make_shared<ZipAssetContainer>(path);
			if (!zip->Initialize(error))
				return false;
			container = std::move(zip);
			break;
		}
		case ArchiveKind::GameStart: {
			auto gamestart = std::make_shared<GameStartAssetContainer>(path);
			if (!gamestart->Initialize(error))
				return false;
			container = std::move(gamestart);
			break;
		}
		default:
			error = "unsupported archive format";
			return false;
	}

	archive_containers[path] = container;
	if (kind)
		*kind = detected_kind;
	return true;
}

struct MemoryFile {
	std::string data;
	size_t cursor = 0;
};

struct Asset_ {
	File file;
	MemoryFile mem_file;

	size_t (*get_size)(Asset_ &asset);
	size_t (*read)(Asset_ &asset, void *data, size_t size);
	bool (*seek)(Asset_ &asset, ptrdiff_t offset, SeekMode mode);
	size_t (*tell)(Asset_ &asset);
	void (*close)(Asset_ &asset);
	bool (*is_eof)(Asset_ &asset);
};

size_t AssetFileGetSize(Asset_ &asset) { return GetSize(asset.file); }
size_t AssetFileRead(Asset_ &asset, void *data, size_t size) { return Read(asset.file, data, size); }
bool AssetFileSeek(Asset_ &asset, ptrdiff_t offset, SeekMode mode) { return Seek(asset.file, offset, mode); }
size_t AssetFileTell(Asset_ &asset) { return Tell(asset.file); }
void AssetFileClose(Asset_ &asset) { Close(asset.file); }
bool AssetFileIsEOF(Asset_ &asset) { return IsEOF(asset.file); }

size_t MemoryFileGetSize(Asset_ &asset) { return asset.mem_file.data.size(); }

size_t MemoryFileRead(Asset_ &asset, void *data, size_t size) {
	if (asset.mem_file.cursor >= asset.mem_file.data.size())
		return 0;

	const auto remaining = asset.mem_file.data.size() - asset.mem_file.cursor;
	const auto read_size = std::min(size, remaining);
	memcpy(data, asset.mem_file.data.data() + asset.mem_file.cursor, read_size);
	asset.mem_file.cursor += read_size;
	return read_size;
}

bool MemoryFileSeek(Asset_ &asset, ptrdiff_t offset, SeekMode mode) {
	int64_t base = 0;
	if (mode == SM_Current)
		base = static_cast<int64_t>(asset.mem_file.cursor);
	else if (mode == SM_End)
		base = static_cast<int64_t>(asset.mem_file.data.size());

	const auto cursor = base + static_cast<int64_t>(offset);
	if (cursor < 0 || cursor > static_cast<int64_t>(asset.mem_file.data.size()))
		return false;

	asset.mem_file.cursor = static_cast<size_t>(cursor);
	return true;
}

size_t MemoryFileTell(Asset_ &asset) { return asset.mem_file.cursor; }
void MemoryFileClose(Asset_ &) {}
bool MemoryFileIsEOF(Asset_ &asset) { return asset.mem_file.cursor >= asset.mem_file.data.size(); }

static generational_vector_list<Asset_> assets;

Asset MakeMemoryAsset(std::string data) {
	return {assets.add_ref({{}, {std::move(data), 0}, MemoryFileGetSize, MemoryFileRead, MemoryFileSeek, MemoryFileTell, MemoryFileClose, MemoryFileIsEOF})};
}

} // namespace

void SetAssetsFolderResolver(AssetsFolderResolver resolver) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	g_assets_folder_resolver = std::move(resolver);
}

void ClearAssetsFolderResolver() {
	std::lock_guard<std::mutex> lock(assets_mutex);
	g_assets_folder_resolver = {};
}

bool AddAssetsFolder(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	if (std::find_if(assets_folders.begin(), assets_folders.end(), [path](const FolderSource &source) { return source.logical_path == path; }) != assets_folders.end())
		return false;

	if (IsDir(path)) {
		assets_folders.push_front({FolderSourceType::Local, path, path, {}, {}});
		return true;
	}

	if (g_assets_folder_resolver) {
		AssetsFolderResolution resolution;
		if (g_assets_folder_resolver(path, resolution) && !resolution.logical_path.empty() && !resolution.archive_path.empty()) {
			if (std::find_if(assets_folders.begin(), assets_folders.end(),
					[&resolution](const FolderSource &source) { return source.logical_path == resolution.logical_path; }) != assets_folders.end())
				return false;

			std::string normalized_prefix;
			if (!NormalizeArchivePrefix(resolution.archive_prefix, normalized_prefix)) {
				warn(format("Failed to mount archive-backed assets folder '%1': invalid archive prefix '%2'").arg(path).arg(resolution.archive_prefix));
				return false;
			}

			std::shared_ptr<AssetContainer> container;
			ArchiveKind kind = ArchiveKind::Unknown;
			std::string error;
			if (!GetOrCreateArchiveContainer(resolution.archive_path, container, error, &kind)) {
				warn(format("Failed to mount %1 archive '%2' for assets folder '%3': %4")
						 .arg(GetArchiveKindName(kind))
						 .arg(resolution.archive_path)
						 .arg(path)
						 .arg(error));
				return false;
			}

			assets_folders.push_front({FolderSourceType::Archive, resolution.logical_path, {}, normalized_prefix, std::move(container)});
			return true;
		}
	}

	// Legacy fallback: accept paths that don't exist yet (relative paths resolved later by OpenAsset against the
	// running cwd). Existing demos rely on this — e.g. hg.AddAssetsFolder("resources_compiled") inside a Lua entry
	// script whose effective cwd makes the path resolvable.
	assets_folders.push_front({FolderSourceType::Local, path, path, {}, {}});
	return true;
}

void RemoveAssetsFolder(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	std::string logical_path = CleanPath(path);
	if (g_assets_folder_resolver) {
		AssetsFolderResolution resolution;
		if (g_assets_folder_resolver(path, resolution) && !resolution.logical_path.empty())
			logical_path = resolution.logical_path;
	}

	assets_folders.erase(std::remove_if(assets_folders.begin(), assets_folders.end(),
						  [&](const FolderSource &source) { return source.type == FolderSourceType::Local ? source.local_path == path : source.logical_path == logical_path; }),
		assets_folders.end());
}

bool AddAssetsPackage(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	if (std::find_if(assets_packages.begin(), assets_packages.end(), [path](const PackageSource &source) { return source.path == path; }) != assets_packages.end())
		return false;

	std::shared_ptr<AssetContainer> container;
	ArchiveKind kind = ArchiveKind::Unknown;
	std::string error;
	if (!GetOrCreateArchiveContainer(path, container, error, &kind)) {
		warn(format("Failed to mount %1 archive '%2': %3").arg(GetArchiveKindName(kind)).arg(path).arg(error));
		return false;
	}

	assets_packages.push_front({path, std::move(container)});
	return true;
}

void RemoveAssetsPackage(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	assets_packages.erase(std::remove_if(assets_packages.begin(), assets_packages.end(), [path](const PackageSource &source) { return source.path == path; }),
		assets_packages.end());
}

std::string FindAssetPath(const char *name) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	for (const auto &source : assets_folders) {
		if (source.type != FolderSourceType::Local)
			continue;

		const auto asset_path = PathJoin({source.local_path, name});
		if (IsFile(asset_path.c_str()))
			return asset_path;
	}
	return {};
}

Asset OpenAsset(const char *name, bool silent) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	for (const auto &source : assets_folders) {
		if (source.type == FolderSourceType::Local) {
			const auto asset_path = PathJoin({source.local_path, name});
			const auto file = Open(asset_path.c_str(), true);
			if (IsValid(file))
				return {assets.add_ref({file, {}, AssetFileGetSize, AssetFileRead, AssetFileSeek, AssetFileTell, AssetFileClose, AssetFileIsEOF})};
			continue;
		}

		std::string archive_path;
		if (!ResolveArchiveAssetPath(source.archive_prefix, name, archive_path))
			continue;
		if (!source.container->IsFile(archive_path))
			continue;

		std::string data, error;
		if (source.container->ReadFile(archive_path, data, &error))
			return MakeMemoryAsset(std::move(data));

		if (!silent)
			warn(format("Failed to read asset '%1' from '%2': %3").arg(name).arg(source.container->DisplayPath(archive_path)).arg(error));
		return {};
	}

	std::string package_path;
	if (ResolveArchiveAssetPath({}, name, package_path)) {
		for (const auto &source : assets_packages) {
			if (!source.container->IsFile(package_path))
				continue;

			std::string data, error;
			if (source.container->ReadFile(package_path, data, &error))
				return MakeMemoryAsset(std::move(data));

			if (!silent)
				warn(format("Failed to read asset '%1' from '%2': %3").arg(name).arg(source.container->DisplayPath(package_path)).arg(error));
			return {};
		}
	}

	if (!silent)
		warn(format("Failed to open asset '%1' (file not found)").arg(name));

	return {};
}

void Close(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		asset_.close(asset_);
		assets.remove_ref(asset.ref);
	}
}

bool IsAssetFile(const char *name) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	for (const auto &source : assets_folders) {
		if (source.type == FolderSourceType::Local) {
			if (IsFile(PathJoin({source.local_path, name}).c_str()))
				return true;
			continue;
		}

		std::string archive_path;
		if (ResolveArchiveAssetPath(source.archive_prefix, name, archive_path) && source.container->IsFile(archive_path))
			return true;
	}

	std::string package_path;
	if (!ResolveArchiveAssetPath({}, name, package_path))
		return false;

	for (const auto &source : assets_packages)
		if (source.container->IsFile(package_path))
			return true;

	return false;
}

size_t GetSize(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.get_size(asset_);
	}
	return 0;
}

size_t Read(Asset asset, void *data, size_t size) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.read(asset_, data, size);
	}
	return 0;
}

bool Seek(Asset asset, ptrdiff_t offset, SeekMode mode) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.seek(asset_, offset, mode);
	}
	return false;
}

size_t Tell(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.tell(asset_);
	}
	return 0;
}

bool IsEOF(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.is_eof(asset_);
	}
	return false;
}

std::string AssetToString(const char *name) {
	const auto h = OpenAsset(name);
	if (h.ref == invalid_gen_ref)
		return {};
	const auto size = GetSize(h);
	std::string str;
	str.resize(size + 1);
	Read(h, &str[0], size);
	Close(h);
	return str;
}

Data AssetToData(const char *name) {
	Data data;
	Asset asset = OpenAsset(name);

	if (IsValid(asset)) {
		data.Resize(GetSize(asset));
		Read(asset, data.GetData(), data.GetSize());
		Close(asset);
	}

	return data;
}

} // namespace hg
