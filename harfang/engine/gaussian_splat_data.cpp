// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/gaussian_splat_data.h"

#include "engine/assets_rw_interface.h"
#include "foundation/file.h"
#include "foundation/rw_interface.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <new>
#include <sstream>
#include <utility>

namespace hg {
namespace {

constexpr size_t HGSHeaderSize = 128;
constexpr uint32_t HGSEndianMarker = 0x01020304u;
constexpr uint32_t HGSAntialiased = 1u, HGSNormalized = 2u, HGSHigherSH = 4u;
constexpr uint32_t HGSKnownFlags = HGSAntialiased | HGSNormalized | HGSHigherSH;
constexpr size_t MaxPLYHeaderSize = 64 * 1024, MaxPLYLineSize = 4096;
constexpr size_t SplatRecordSize = 32;

GaussianSplatDataResult ErrorResult(const std::string &message) {
	GaussianSplatDataResult result;
	result.message = message;
	return result;
}

bool SetError(std::string *error, const std::string &message) {
	if (error)
		*error = message;
	return false;
}

uint16_t GetU16(const uint8_t *p) { return uint16_t(p[0]) | uint16_t(uint16_t(p[1]) << 8); }
uint32_t GetU32(const uint8_t *p) {
	return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
uint64_t GetU64(const uint8_t *p) { return uint64_t(GetU32(p)) | (uint64_t(GetU32(p + 4)) << 32); }
float GetF32(const uint8_t *p) {
	const uint32_t bits = GetU32(p);
	float value;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

void PutU16(Data &out, uint16_t v) {
	const uint8_t bytes[] = {uint8_t(v), uint8_t(v >> 8)};
	out.Write(bytes, sizeof(bytes));
}
void PutU32(Data &out, uint32_t v) {
	const uint8_t bytes[] = {uint8_t(v), uint8_t(v >> 8), uint8_t(v >> 16), uint8_t(v >> 24)};
	out.Write(bytes, sizeof(bytes));
}
void PutU64(Data &out, uint64_t v) {
	PutU32(out, uint32_t(v));
	PutU32(out, uint32_t(v >> 32));
}
void PutF32(Data &out, float v) {
	uint32_t bits;
	std::memcpy(&bits, &v, sizeof(bits));
	PutU32(out, bits);
}

bool CheckedMul(uint64_t a, uint64_t b, uint64_t &out) {
	if (a && b > std::numeric_limits<uint64_t>::max() / a)
		return false;
	out = a * b;
	return true;
}

bool CheckedAdd(uint64_t a, uint64_t b, uint64_t &out) {
	if (b > std::numeric_limits<uint64_t>::max() - a)
		return false;
	out = a + b;
	return true;
}

uint32_t CoefficientCount(uint32_t degree) { return (degree + 1) * (degree + 1); }

bool IsFinite(const Vec3 &v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

std::vector<std::string> Split(const std::string &line) {
	std::istringstream stream(line);
	std::vector<std::string> words;
	std::string word;
	while (stream >> word)
		words.push_back(word);
	return words;
}

bool ParseU32(const std::string &text, uint32_t &value) {
	if (text.empty() || text[0] == '-')
		return false;
	errno = 0;
	char *end = nullptr;
	const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
	if (errno || end != text.c_str() + text.size() || parsed > std::numeric_limits<uint32_t>::max())
		return false;
	value = uint32_t(parsed);
	return true;
}

std::string Trim(const std::string &value) {
	const auto first = value.find_first_not_of(" \t");
	if (first == std::string::npos)
		return {};
	const auto last = value.find_last_not_of(" \t");
	return value.substr(first, last - first + 1);
}

bool RobustScale(float source, float &scale) {
	const double value = std::exp(double(source));
	if (!std::isfinite(value) || value <= 0.0 || value > std::numeric_limits<float>::max())
		return false;
	scale = float(value);
	return std::isfinite(scale) && scale > 0.f;
}

bool RobustOpacity(float source, float &opacity) {
	double value;
	if (source >= 0.f)
		value = 1.0 / (1.0 + std::exp(-double(source)));
	else {
		const double e = std::exp(double(source));
		value = e / (1.0 + e);
	}
	if (!std::isfinite(value))
		return false;
	opacity = float(value);
	return std::isfinite(opacity) && opacity > 0.f && opacity <= 1.f;
}

bool NormalizeQuaternion(const float *source, float *rotation) {
	const double w = source[0], x = source[1], y = source[2], z = source[3];
	const double norm = std::sqrt(w * w + x * x + y * y + z * z);
	if (!std::isfinite(norm) || norm <= 0.0)
		return false;
	rotation[0] = float(w / norm);
	rotation[1] = float(x / norm);
	rotation[2] = float(y / norm);
	rotation[3] = float(z / norm);
	return std::isfinite(rotation[0]) && std::isfinite(rotation[1]) && std::isfinite(rotation[2]) && std::isfinite(rotation[3]);
}

bool ValidateCanonical(const GaussianSplatData &data, std::string &error) {
	if (!data.count || data.count > GaussianSplatMaxCount)
		return SetError(&error, "Gaussian count must be in 1..1048576");
	if (data.source_count < data.count)
		return SetError(&error, "Gaussian source count is smaller than retained count");
	if (data.sh_degree > 3)
		return SetError(&error, "Gaussian SH degree exceeds 3");
	const uint64_t non_dc = CoefficientCount(data.sh_degree) - 1;
	uint64_t base_size, sh_size;
	if (!CheckedMul(data.count, 16, base_size) || !CheckedMul(uint64_t(data.count) * non_dc, 4, sh_size) || base_size != data.base.size() ||
		sh_size != data.higher_sh.size())
		return SetError(&error, "Gaussian CPU payload sizes disagree with count/SH degree");
	if (!IsFinite(data.bounds_min) || !IsFinite(data.bounds_max) || data.bounds_min.x > data.bounds_max.x || data.bounds_min.y > data.bounds_max.y ||
		data.bounds_min.z > data.bounds_max.z)
		return SetError(&error, "Gaussian bounds are invalid");

	Vec3 bounds_min, bounds_max;
	for (uint32_t i = 0; i < data.count; ++i) {
		const float *record = data.base.data() + size_t(i) * 16;
		for (uint32_t j = 0; j < 16; ++j)
			if (!std::isfinite(record[j]))
				return SetError(&error, "Non-finite native base payload at splat " + std::to_string(i));
		if (record[3] < 0.f || record[3] > 1.f)
			return SetError(&error, "Native opacity outside [0,1] at splat " + std::to_string(i));
		if (record[4] <= 0.f || record[5] <= 0.f || record[6] <= 0.f)
			return SetError(&error, "Native scale is not positive at splat " + std::to_string(i));
		if (record[7] != 0.f || record[15] != 0.f)
			return SetError(&error, "Native base reserved lanes must be zero");
		const double norm = std::sqrt(double(record[8]) * record[8] + double(record[9]) * record[9] + double(record[10]) * record[10] +
			double(record[11]) * record[11]);
		if (!std::isfinite(norm) || std::abs(norm - 1.0) > 0.001)
			return SetError(&error, "Native quaternion is not normalized at splat " + std::to_string(i));
		const Vec3 p{record[0], record[1], record[2]};
		if (!i)
			bounds_min = bounds_max = p;
		else {
			bounds_min.x = std::min(bounds_min.x, p.x);
			bounds_min.y = std::min(bounds_min.y, p.y);
			bounds_min.z = std::min(bounds_min.z, p.z);
			bounds_max.x = std::max(bounds_max.x, p.x);
			bounds_max.y = std::max(bounds_max.y, p.y);
			bounds_max.z = std::max(bounds_max.z, p.z);
		}
	}
	for (size_t i = 0; i < data.higher_sh.size(); i += 4) {
		if (!std::isfinite(data.higher_sh[i]) || !std::isfinite(data.higher_sh[i + 1]) || !std::isfinite(data.higher_sh[i + 2]) ||
			!std::isfinite(data.higher_sh[i + 3]))
			return SetError(&error, "Non-finite native SH payload at vec4 " + std::to_string(i / 4));
		if (data.higher_sh[i + 3] != 0.f)
			return SetError(&error, "Native SH reserved lane must be zero");
	}
	auto close = [](float a, float b) { return std::abs(double(a) - b) <= 0.00001 * std::max(1.0, std::max(std::abs(double(a)), std::abs(double(b)))); };
	if (!close(bounds_min.x, data.bounds_min.x) || !close(bounds_min.y, data.bounds_min.y) || !close(bounds_min.z, data.bounds_min.z) ||
		!close(bounds_max.x, data.bounds_max.x) || !close(bounds_max.y, data.bounds_max.y) || !close(bounds_max.z, data.bounds_max.z))
		return SetError(&error, "Native bounds do not match the position payload");
	return true;
}

GaussianSplatDataResult FinalizeGaussianSplatImport(GaussianSplatData &data, const Vec3 &source_min, const Vec3 &source_max, bool normalize) {
	if (normalize) {
		const double extent = std::max(double(source_max.x) - source_min.x,
			std::max(double(source_max.y) - source_min.y, double(source_max.z) - source_min.z));
		if (!std::isfinite(extent) || extent <= 0.0)
			return ErrorResult("Cannot normalize a zero-extent Gaussian asset");
		const double factor = 2.0 / extent;
		const double center[] = {(double(source_min.x) + source_max.x) * 0.5, (double(source_min.y) + source_max.y) * 0.5,
			(double(source_min.z) + source_max.z) * 0.5};
		if (!std::isfinite(factor) || factor <= 0.0 || factor > std::numeric_limits<float>::max())
			return ErrorResult("Normalization scale is not finite/positive");
		for (uint32_t i = 0; i < data.count; ++i) {
			float *base = data.base.data() + size_t(i) * 16;
			for (uint32_t axis = 0; axis < 3; ++axis) {
				base[axis] = float((double(base[axis]) - center[axis]) * factor);
				base[4 + axis] = float(double(base[4 + axis]) * factor);
				if (!std::isfinite(base[axis]) || !std::isfinite(base[4 + axis]) || base[4 + axis] <= 0.f)
					return ErrorResult("Normalization produced an invalid position/scale");
			}
		}
		data.normalized = true;
		data.bounds_min = data.bounds_max = {data.base[0], data.base[1], data.base[2]};
		for (uint32_t i = 1; i < data.count; ++i) {
			const float *base = data.base.data() + size_t(i) * 16;
			data.bounds_min.x = std::min(data.bounds_min.x, base[0]);
			data.bounds_min.y = std::min(data.bounds_min.y, base[1]);
			data.bounds_min.z = std::min(data.bounds_min.z, base[2]);
			data.bounds_max.x = std::max(data.bounds_max.x, base[0]);
			data.bounds_max.y = std::max(data.bounds_max.y, base[1]);
			data.bounds_max.z = std::max(data.bounds_max.z, base[2]);
		}
	}
	std::string validation_error;
	if (!ValidateCanonical(data, validation_error))
		return ErrorResult(validation_error);
	GaussianSplatDataResult result;
	result.ok = true;
	result.message = "OK";
	result.data = std::move(data);
	return result;
}

GaussianSplatDataResult ReadNative(const Data &input, uint64_t byte_cap) {
	if (!input.GetData() || input.GetSize() < HGSHeaderSize)
		return ErrorResult("Missing HGS1 header");
	if (input.GetSize() > byte_cap || input.GetSize() > GaussianSplatMaxNativeBytes)
		return ErrorResult("Native Gaussian asset exceeds the configured byte budget");
	const uint8_t *bytes = input.GetData();
	if (std::memcmp(bytes, "HGS1", 4) != 0)
		return ErrorResult("Unsupported Gaussian magic (expected HGS1; HGS0 is not accepted)");
	if (GetU16(bytes + 4) != 1 || GetU16(bytes + 6) != 0)
		return ErrorResult("Unsupported HGS major/minor version");
	if (GetU32(bytes + 8) != HGSEndianMarker || GetU32(bytes + 12) != HGSHeaderSize)
		return ErrorResult("Invalid HGS1 endian marker or header size");
	const uint64_t total_size = GetU64(bytes + 16);
	if (total_size != input.GetSize() || total_size > GaussianSplatMaxNativeBytes)
		return ErrorResult("HGS1 total length is invalid or exceeds 512 MiB");
	const uint32_t flags = GetU32(bytes + 24), degree = GetU32(bytes + 28), count = GetU32(bytes + 32), source_count = GetU32(bytes + 36);
	if (flags & ~HGSKnownFlags)
		return ErrorResult("HGS1 contains unsupported required flags");
	if (degree > 3 || !count || count > GaussianSplatMaxCount || source_count < count)
		return ErrorResult("HGS1 count/source count/SH degree is invalid");
	if (!!(flags & HGSHigherSH) != (degree > 0))
		return ErrorResult("HGS1 higher-SH flag disagrees with SH degree");
	const uint64_t base_offset = GetU64(bytes + 40), base_length = GetU64(bytes + 48), sh_offset = GetU64(bytes + 56), sh_length = GetU64(bytes + 64);
	const uint32_t non_dc = CoefficientCount(degree) - 1;
	uint64_t expected_base, expected_sh, expected_sh_offset, expected_total;
	if (!CheckedMul(count, 64, expected_base) || !CheckedMul(uint64_t(count) * non_dc, 16, expected_sh) ||
		!CheckedAdd(HGSHeaderSize, expected_base, expected_sh_offset) || !CheckedAdd(expected_sh_offset, expected_sh, expected_total))
		return ErrorResult("HGS1 section length overflow");
	if (base_offset != HGSHeaderSize || base_length != expected_base || sh_offset != expected_sh_offset || sh_length != expected_sh || total_size != expected_total)
		return ErrorResult("HGS1 section offsets/lengths are not canonical or overlap");
	if (GetU32(bytes + 96) != 64 || GetU32(bytes + 100) != 16 || GetU32(bytes + 104) != non_dc || GetU32(bytes + 108) != 0)
		return ErrorResult("HGS1 stride/coefficient count/color encoding is unsupported");
	for (size_t i = 112; i < HGSHeaderSize; ++i)
		if (bytes[i] != 0)
			return ErrorResult("HGS1 reserved header bytes must be zero");

	GaussianSplatData data;
	data.count = count;
	data.source_count = source_count;
	data.sh_degree = degree;
	data.antialiased = (flags & HGSAntialiased) != 0;
	data.normalized = (flags & HGSNormalized) != 0;
	data.bounds_min = {GetF32(bytes + 72), GetF32(bytes + 76), GetF32(bytes + 80)};
	data.bounds_max = {GetF32(bytes + 84), GetF32(bytes + 88), GetF32(bytes + 92)};
	try {
		data.base.resize(size_t(expected_base / 4));
		data.higher_sh.resize(size_t(expected_sh / 4));
	} catch (const std::bad_alloc &) {
		return ErrorResult("Insufficient memory for native Gaussian payload");
	}
	for (size_t i = 0; i < data.base.size(); ++i)
		data.base[i] = GetF32(bytes + base_offset + i * 4);
	for (size_t i = 0; i < data.higher_sh.size(); ++i)
		data.higher_sh[i] = GetF32(bytes + sh_offset + i * 4);
	std::string validation_error;
	if (!ValidateCanonical(data, validation_error))
		return ErrorResult(validation_error);
	GaussianSplatDataResult result;
	result.ok = true;
	result.message = "OK";
	result.data = std::move(data);
	return result;
}

bool LoadBoundedFile(const char *path, uint64_t cap, Data &data, std::string &error) {
	if (!path || !*path)
		return SetError(&error, "Empty input path");
	ScopedFile file(Open(path, true));
	if (!IsValid(file))
		return SetError(&error, std::string("Cannot open '") + path + "'");
	const size_t size = GetSize(file);
	if (size > cap)
		return SetError(&error, "Input exceeds the byte budget");
	try {
		if (!data.Resize(size))
			return SetError(&error, "Cannot allocate the bounded input buffer");
	} catch (const std::bad_alloc &) {
		return SetError(&error, "Cannot allocate the bounded input buffer");
	}
	if (Read(file, data.GetData(), size) != size)
		return SetError(&error, "Input changed or was truncated while reading");
	return true;
}

} // namespace

GaussianSplatDataResult ImportGaussianSplatPLY(const Data &input, const GaussianSplatImportOptions &options) {
	if (!input.GetData() || input.GetSize() < 16)
		return ErrorResult("Empty or truncated Gaussian PLY");
	if (input.GetSize() > GaussianSplatMaxPLYBytes)
		return ErrorResult("Gaussian PLY exceeds the 1 GiB input budget");
	if (!options.max_count || options.max_count > GaussianSplatMaxCount || options.sh_degree > 3)
		return ErrorResult("Import max-count must be in 1..1048576 and SH degree in 0..3");

	size_t cursor = 0, payload_offset = 0;
	uint32_t source_count = 0;
	bool format_seen = false, vertex_seen = false, end_seen = false, antialias_seen = false, antialiased = false;
	std::vector<std::string> property_names;
	std::map<std::string, uint32_t> property_indices;
	uint32_t line_number = 0;
	while (cursor < input.GetSize()) {
		const size_t line_start = cursor;
		const void *newline_ptr = std::memchr(input.GetData() + cursor, '\n', input.GetSize() - cursor);
		if (!newline_ptr)
			return ErrorResult("Unterminated PLY header line");
		const size_t line_end = size_t(static_cast<const uint8_t *>(newline_ptr) - input.GetData());
		cursor = line_end + 1;
		++line_number;
		if (cursor > MaxPLYHeaderSize || line_end - line_start > MaxPLYLineSize)
			return ErrorResult("PLY header/line exceeds its bound");
		std::string line(reinterpret_cast<const char *>(input.GetData() + line_start), line_end - line_start);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.find('\0') != std::string::npos || line.find('\r') != std::string::npos)
			return ErrorResult("Invalid control character in PLY header");
		const auto words = Split(line);
		if (line_number == 1) {
			if (line != "ply")
				return ErrorResult("Not a PLY file");
			continue;
		}
		if (words.empty())
			return ErrorResult("Blank PLY header line at line " + std::to_string(line_number));
		if (words[0] == "format") {
			if (format_seen || words.size() != 3)
				return ErrorResult("Duplicate/malformed PLY format");
			if (words[1] != "binary_little_endian" || words[2] != "1.0")
				return ErrorResult("Only binary_little_endian PLY 1.0 is supported");
			format_seen = true;
		} else if (words[0] == "comment") {
			std::string comment = Trim(line.substr(7));
			std::string compact;
			for (char c : comment)
				if (c != ' ' && c != '\t')
					compact.push_back(c);
			const std::string key = "postshot.anti_aliasing=";
			if (compact.compare(0, key.size(), key) == 0) {
				const std::string value = compact.substr(key.size());
				if (antialias_seen || (value != "0" && value != "1"))
					return ErrorResult("Duplicate/invalid postshot.anti_aliasing metadata");
				antialias_seen = true;
				antialiased = value == "1";
			}
		} else if (words[0] == "element") {
			if (!format_seen || vertex_seen || words.size() != 3 || words[1] != "vertex" || !ParseU32(words[2], source_count) || !source_count)
				return ErrorResult("Only one non-empty vertex element is supported; other PLY elements are rejected");
			vertex_seen = true;
		} else if (words[0] == "property") {
			if (!vertex_seen || words.size() != 3 || words[1] != "float")
				return ErrorResult("Only scalar float vertex properties are supported at line " + std::to_string(line_number));
			if (property_names.size() >= 256 || property_indices.count(words[2]))
				return ErrorResult("Duplicate/excess PLY property '" + words[2] + "'");
			property_indices[words[2]] = uint32_t(property_names.size());
			property_names.push_back(words[2]);
		} else if (words[0] == "end_header") {
			if (words.size() != 1 || !format_seen || !vertex_seen || property_names.empty())
				return ErrorResult("Incomplete PLY header");
			payload_offset = cursor;
			end_seen = true;
			break;
		} else
			return ErrorResult("Unsupported PLY header directive '" + words[0] + "'");
	}
	if (!end_seen)
		return ErrorResult("Missing PLY end_header");

	const char *required[] = {"x", "y", "z", "f_dc_0", "f_dc_1", "f_dc_2", "opacity", "scale_0", "scale_1", "scale_2", "rot_0", "rot_1", "rot_2", "rot_3"};
	for (const char *name : required)
		if (!property_indices.count(name))
			return ErrorResult(std::string("Missing Gaussian PLY property '") + name + "'");
	std::map<uint32_t, uint32_t> rest_indices;
	for (const auto &property : property_indices) {
		if (property.first.compare(0, 7, "f_rest_") == 0) {
			uint32_t index;
			const std::string suffix = property.first.substr(7);
			if (!ParseU32(suffix, index) || std::to_string(index) != suffix || rest_indices.count(index))
				return ErrorResult("Invalid/duplicate f_rest suffix '" + suffix + "'");
			rest_indices[index] = property.second;
		}
	}
	for (uint32_t i = 0; i < rest_indices.size(); ++i)
		if (!rest_indices.count(i))
			return ErrorResult("Non-contiguous Gaussian f_rest coefficient bands");
	uint32_t source_degree = std::numeric_limits<uint32_t>::max();
	for (uint32_t degree = 0; degree <= 3; ++degree)
		if (rest_indices.size() == 3u * (CoefficientCount(degree) - 1))
			source_degree = degree;
	if (source_degree == std::numeric_limits<uint32_t>::max())
		return ErrorResult("Gaussian f_rest properties do not form complete degree 0..3 RGB bands");
	const uint32_t degree = std::min(source_degree, options.sh_degree), non_dc = CoefficientCount(degree) - 1;
	const uint64_t stride = uint64_t(property_names.size()) * 4, payload_size = stride * source_count;
	if (payload_size > input.GetSize() || payload_offset > input.GetSize() - payload_size || payload_offset + payload_size != input.GetSize())
		return ErrorResult("PLY payload length does not exactly match vertex count and property stride");

	GaussianSplatData data;
	data.count = std::min(source_count, options.max_count);
	data.source_count = source_count;
	data.sh_degree = degree;
	data.antialiased = antialiased;
	try {
		data.base.resize(size_t(data.count) * 16);
		data.higher_sh.resize(size_t(data.count) * non_dc * 4);
	} catch (const std::bad_alloc &) {
		return ErrorResult("Insufficient memory for retained Gaussian payload");
	}
	std::vector<float> values(property_names.size());
	uint32_t retained = 0;
	Vec3 bounds_min, bounds_max;
	Vec3 source_min, source_max;
	for (uint32_t source = 0; source < source_count; ++source) {
		const uint8_t *record = input.GetData() + payload_offset + size_t(source * stride);
		for (size_t j = 0; j < values.size(); ++j) {
			values[j] = GetF32(record + j * 4);
			if (!std::isfinite(values[j]))
				return ErrorResult("Non-finite PLY property '" + property_names[j] + "' at source record " + std::to_string(source));
		}
		std::array<float, 16> base{};
		const char *position_names[] = {"x", "y", "z"};
		for (uint32_t j = 0; j < 3; ++j) {
			base[j] = values[property_indices[position_names[j]]];
			base[12 + j] = values[property_indices["f_dc_" + std::to_string(j)]];
			if (!RobustScale(values[property_indices["scale_" + std::to_string(j)]], base[4 + j]))
				return ErrorResult("Invalid exponential scale at source record " + std::to_string(source));
		}
		if (!RobustOpacity(values[property_indices["opacity"]], base[3]))
			return ErrorResult("Invalid sigmoid opacity at source record " + std::to_string(source));
		float quaternion[4];
		for (uint32_t j = 0; j < 4; ++j)
			quaternion[j] = values[property_indices["rot_" + std::to_string(j)]];
		if (!NormalizeQuaternion(quaternion, &base[8]))
			return ErrorResult("Zero/invalid WXYZ quaternion at source record " + std::to_string(source));
		for (uint32_t axis = 0; axis < 3; ++axis) {
			source_min[axis] = source ? std::min(source_min[axis], base[axis]) : base[axis];
			source_max[axis] = source ? std::max(source_max[axis], base[axis]) : base[axis];
		}

		const uint64_t numerator = (uint64_t(retained) * 2 + 1) * source_count;
		const uint32_t selected_index = retained < data.count ? uint32_t(numerator / (uint64_t(data.count) * 2)) : source_count;
		if (retained < data.count && source == selected_index) {
			std::copy(base.begin(), base.end(), data.base.begin() + size_t(retained) * 16);
			for (uint32_t k = 1; k <= non_dc; ++k)
				for (uint32_t channel = 0; channel < 3; ++channel) {
					const uint32_t source_rest = channel * (CoefficientCount(source_degree) - 1) + (k - 1);
					data.higher_sh[(size_t(retained) * non_dc + (k - 1)) * 4 + channel] = values[rest_indices[source_rest]];
				}
			const Vec3 p{base[0], base[1], base[2]};
			if (!retained)
				bounds_min = bounds_max = p;
			else {
				bounds_min.x = std::min(bounds_min.x, p.x);
				bounds_min.y = std::min(bounds_min.y, p.y);
				bounds_min.z = std::min(bounds_min.z, p.z);
				bounds_max.x = std::max(bounds_max.x, p.x);
				bounds_max.y = std::max(bounds_max.y, p.y);
				bounds_max.z = std::max(bounds_max.z, p.z);
			}
			++retained;
		}
	}
	if (retained != data.count)
		return ErrorResult("Internal midpoint selection did not retain the requested count");
	data.bounds_min = bounds_min;
	data.bounds_max = bounds_max;

	return FinalizeGaussianSplatImport(data, source_min, source_max, options.normalize);
}

GaussianSplatDataResult ImportGaussianSplatPLYFromFile(const char *path, const GaussianSplatImportOptions &options) {
	Data input;
	std::string error;
	if (!LoadBoundedFile(path, GaussianSplatMaxPLYBytes, input, error))
		return ErrorResult(error);
	return ImportGaussianSplatPLY(input, options);
}

GaussianSplatDataResult ImportGaussianSplatBinary(const Data &input, const GaussianSplatImportOptions &options) {
	if (!input.GetData() || input.GetSize() < SplatRecordSize)
		return ErrorResult("Empty or truncated headerless 32-byte Gaussian .splat input");
	if (input.GetSize() > GaussianSplatMaxBinaryBytes)
		return ErrorResult("Gaussian .splat input exceeds the 1 GiB input budget");
	if (input.GetSize() % SplatRecordSize)
		return ErrorResult("Gaussian .splat input length must be an exact multiple of 32 bytes");
	if (!options.max_count || options.max_count > GaussianSplatMaxCount || options.sh_degree > 3)
		return ErrorResult("Import max-count must be in 1..1048576 and SH degree in 0..3");

	const uint32_t source_count = uint32_t(input.GetSize() / SplatRecordSize);
	GaussianSplatData data;
	data.count = std::min(source_count, options.max_count);
	data.source_count = source_count;
	data.sh_degree = 0;
	try {
		data.base.resize(size_t(data.count) * 16);
	} catch (const std::bad_alloc &) {
		return ErrorResult("Insufficient memory for retained Gaussian payload");
	}

	uint32_t retained = 0;
	Vec3 bounds_min, bounds_max;
	Vec3 source_min, source_max;
	for (uint32_t source = 0; source < source_count; ++source) {
		const uint8_t *record = input.GetData() + size_t(source) * SplatRecordSize;
		std::array<float, 16> base{};
		for (uint32_t axis = 0; axis < 3; ++axis) {
			base[axis] = GetF32(record + axis * 4);
			if (!std::isfinite(base[axis]))
				return ErrorResult("Non-finite .splat position at source record " + std::to_string(source));
			base[4 + axis] = GetF32(record + 12 + axis * 4);
			if (!std::isfinite(base[4 + axis]) || base[4 + axis] <= 0.f)
				return ErrorResult("Invalid positive linear scale at source record " + std::to_string(source));
			base[12 + axis] = float((double(record[24 + axis]) / 255.0 - 0.5) / 0.28209479177387814);
		}
		base[3] = float(record[27]) / 255.f;
		float quaternion[4];
		for (uint32_t j = 0; j < 4; ++j)
			quaternion[j] = float((int(record[28 + j]) - 128) / 128.0);
		if (!NormalizeQuaternion(quaternion, &base[8]))
			return ErrorResult("Zero/invalid packed WXYZ quaternion at source record " + std::to_string(source));
		for (uint32_t axis = 0; axis < 3; ++axis) {
			source_min[axis] = source ? std::min(source_min[axis], base[axis]) : base[axis];
			source_max[axis] = source ? std::max(source_max[axis], base[axis]) : base[axis];
		}

		const uint64_t numerator = (uint64_t(retained) * 2 + 1) * source_count;
		const uint32_t selected_index = retained < data.count ? uint32_t(numerator / (uint64_t(data.count) * 2)) : source_count;
		if (retained < data.count && source == selected_index) {
			std::copy(base.begin(), base.end(), data.base.begin() + size_t(retained) * 16);
			const Vec3 p{base[0], base[1], base[2]};
			if (!retained)
				bounds_min = bounds_max = p;
			else {
				bounds_min.x = std::min(bounds_min.x, p.x);
				bounds_min.y = std::min(bounds_min.y, p.y);
				bounds_min.z = std::min(bounds_min.z, p.z);
				bounds_max.x = std::max(bounds_max.x, p.x);
				bounds_max.y = std::max(bounds_max.y, p.y);
				bounds_max.z = std::max(bounds_max.z, p.z);
			}
			++retained;
		}
	}
	if (retained != data.count)
		return ErrorResult("Internal midpoint selection did not retain the requested count");
	data.bounds_min = bounds_min;
	data.bounds_max = bounds_max;
	return FinalizeGaussianSplatImport(data, source_min, source_max, options.normalize);
}

GaussianSplatDataResult ImportGaussianSplatBinaryFromFile(const char *path, const GaussianSplatImportOptions &options) {
	Data input;
	std::string error;
	if (!LoadBoundedFile(path, GaussianSplatMaxBinaryBytes, input, error))
		return ErrorResult(error);
	return ImportGaussianSplatBinary(input, options);
}

GaussianSplatDataResult ReadGaussianSplatData(const Data &input) { return ReadNative(input, GaussianSplatMaxNativeBytes); }

GaussianSplatDataResult ReadGaussianSplatDataFromFile(const char *path) {
	Data input;
	std::string error;
	if (!LoadBoundedFile(path, GaussianSplatMaxNativeBytes, input, error))
		return ErrorResult(error);
	return ReadNative(input, GaussianSplatMaxNativeBytes);
}

GaussianSplatDataResult ReadGaussianSplatDataFromAssets(const char *name) {
	if (!name || !*name)
		return ErrorResult("Empty Gaussian asset name");
	ScopedReadHandle handle(g_assets_read_provider, name, true);
	if (!g_assets_reader.is_valid(handle))
		return ErrorResult(std::string("Cannot open Gaussian asset '") + name + "'");
	const size_t size = g_assets_reader.size(handle);
	if (!size || size > GaussianSplatMaxNativeBytes)
		return ErrorResult("Gaussian asset is empty or exceeds 512 MiB");
	const Data input = LoadData(g_assets_reader, handle);
	if (input.GetSize() != size)
		return ErrorResult("Gaussian asset read was truncated");
	return ReadNative(input, size);
}

bool WriteGaussianSplatData(const GaussianSplatData &data, Data &output, std::string *error) {
	output.Reset();
	std::string validation_error;
	if (!ValidateCanonical(data, validation_error))
		return SetError(error, validation_error);
	const uint32_t non_dc = CoefficientCount(data.sh_degree) - 1;
	const uint64_t base_length = uint64_t(data.count) * 64, sh_offset = HGSHeaderSize + base_length;
	const uint64_t sh_length = uint64_t(data.count) * non_dc * 16, total_size = sh_offset + sh_length;
	if (total_size > GaussianSplatMaxNativeBytes || total_size > std::numeric_limits<size_t>::max())
		return SetError(error, "Native Gaussian output exceeds its size budget");
	try {
		if (!output.Resize(size_t(total_size)))
			return SetError(error, "Cannot allocate native Gaussian output");
	} catch (const std::bad_alloc &) {
		return SetError(error, "Cannot allocate native Gaussian output");
	}
	output.SetCursor(0);
	output.Write("HGS1", 4);
	PutU16(output, 1);
	PutU16(output, 0);
	PutU32(output, HGSEndianMarker);
	PutU32(output, HGSHeaderSize);
	PutU64(output, total_size);
	PutU32(output, (data.antialiased ? HGSAntialiased : 0) | (data.normalized ? HGSNormalized : 0) | (data.sh_degree ? HGSHigherSH : 0));
	PutU32(output, data.sh_degree);
	PutU32(output, data.count);
	PutU32(output, data.source_count);
	PutU64(output, HGSHeaderSize);
	PutU64(output, base_length);
	PutU64(output, sh_offset);
	PutU64(output, sh_length);
	for (uint32_t axis = 0; axis < 3; ++axis)
		PutF32(output, data.bounds_min[axis]);
	for (uint32_t axis = 0; axis < 3; ++axis)
		PutF32(output, data.bounds_max[axis]);
	PutU32(output, 64);
	PutU32(output, 16);
	PutU32(output, non_dc);
	PutU32(output, 0);
	for (uint32_t i = 0; i < 8; ++i)
		PutU16(output, 0);
	for (float value : data.base)
		PutF32(output, value);
	for (float value : data.higher_sh)
		PutF32(output, value);
	if (output.GetCursor() != total_size)
		return SetError(error, "Internal HGS1 serialization length mismatch");
	return true;
}

bool WriteGaussianSplatDataToFile(const GaussianSplatData &data, const char *path, std::string *error) {
	if (!path || !*path)
		return SetError(error, "Empty output path");
	Data output;
	if (!WriteGaussianSplatData(data, output, error))
		return false;
	// The complete asset is validated and serialized before the destination is opened.
	if (!SaveDataToFile(path, output))
		return SetError(error, std::string("Cannot write '") + path + "'");
	return true;
}

} // namespace hg
