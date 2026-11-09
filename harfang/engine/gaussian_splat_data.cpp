// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/gaussian_splat_data.h"

#include "engine/assets_rw_interface.h"
#include "foundation/file.h"
#include "foundation/rw_interface.h"

#include <miniz/miniz.h>

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

// HGS2 is specified in specifications/SPECS_GAUSSIAN_SPLATTING_HGS2.md.
constexpr size_t HGSHeaderSize = 128;
constexpr uint32_t HGSEndianMarker = 0x01020304u;
constexpr uint32_t HGSAntialiased = 1u, HGSNormalized = 2u, HGSHigherSH = 4u;
constexpr uint32_t HGSKnownFlags = HGSAntialiased | HGSNormalized | HGSHigherSH;
constexpr uint32_t HGSBlockSize = 256, HGSTableEntrySize = 32;
constexpr uint32_t HGSMaxSHCoefficients = 45;
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

void SetU16(uint8_t *p, uint16_t v) {
	p[0] = uint8_t(v);
	p[1] = uint8_t(v >> 8);
}
void SetU32(uint8_t *p, uint32_t v) {
	p[0] = uint8_t(v);
	p[1] = uint8_t(v >> 8);
	p[2] = uint8_t(v >> 16);
	p[3] = uint8_t(v >> 24);
}
void SetU64(uint8_t *p, uint64_t v) {
	SetU32(p, uint32_t(v));
	SetU32(p + 4, uint32_t(v >> 32));
}
void SetF32(uint8_t *p, float v) {
	uint32_t bits;
	std::memcpy(&bits, &v, sizeof(bits));
	SetU32(p, bits);
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

bool DecodeSPZQuaternion(const uint8_t *packed, bool smallest_three, float *rotation) {
	float xyzw[4] = {};
	if (smallest_three) {
		uint32_t components = GetU32(packed);
		const uint32_t largest = components >> 30;
		float sum_squares = 0.f;
		for (int i = 3; i >= 0; --i) {
			if (i == int(largest))
				continue;
			const uint32_t magnitude = components & 511u, negative = (components >> 9) & 1u;
			components >>= 10;
			xyzw[i] = 0.7071067811865475244f * float(magnitude) / 511.f;
			if (negative)
				xyzw[i] = -xyzw[i];
			sum_squares += xyzw[i] * xyzw[i];
		}
		xyzw[largest] = std::sqrt(1.0f - sum_squares);
	} else {
		for (uint32_t i = 0; i < 3; ++i)
			xyzw[i] = float(packed[i]) / 127.5f - 1.f;
		const float sum_squares = xyzw[0] * xyzw[0] + xyzw[1] * xyzw[1] + xyzw[2] * xyzw[2];
		xyzw[3] = std::sqrt(std::max(0.f, 1.f - sum_squares));
	}
	const float wxyz[] = {xyzw[3], xyzw[0], xyzw[1], xyzw[2]};
	return NormalizeQuaternion(wxyz, rotation);
}

bool ValidateCanonical(const GaussianSplatData &data, std::string &error) {
	if (!data.count || data.count > GaussianSplatMaxCount)
		return SetError(&error, "Gaussian count must be in 1.." + std::to_string(GaussianSplatMaxCount));
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

bool HGS2RawBlockSize(GaussianSplatStorage storage, uint32_t count, uint32_t non_dc, uint64_t &size) {
	if (storage == GaussianSplatStorage::Float32) {
		uint64_t base, sh;
		if (!CheckedMul(count, 64, base) || !CheckedMul(uint64_t(count) * non_dc, 16, sh) || !CheckedAdd(base, sh, size))
			return false;
		return true;
	}
	if (storage != GaussianSplatStorage::Quantized)
		return false;
	uint64_t metadata, records;
	if (!CheckedMul(non_dc, 12, metadata) || !CheckedAdd(72, metadata, metadata) ||
		!CheckedMul(count, 17 + uint64_t(non_dc) * 3, records) || !CheckedAdd(metadata, records, size))
		return false;
	return true;
}

bool HGS2CanonicalSize(uint32_t count, uint32_t non_dc, uint64_t &size) {
	uint64_t base, sh;
	return CheckedMul(count, 64, base) && CheckedMul(uint64_t(count) * non_dc, 16, sh) && CheckedAdd(base, sh, size);
}

bool ValidateImportOptions(const GaussianSplatImportOptions &options, std::string &error) {
	if (options.max_count > GaussianSplatMaxCount || options.sh_degree > 3)
		return SetError(&error, "Import max-count must be 0 (all source splats) or in 1.." + std::to_string(GaussianSplatMaxCount) +
			" and SH degree in 0..3");
	return true;
}

bool PrepareImport(GaussianSplatData &data, uint32_t source_count, uint32_t degree, const GaussianSplatImportOptions &options, std::string &error) {
	if (!options.max_count && source_count > GaussianSplatMaxCount)
		return SetError(&error, "Source Gaussian count " + std::to_string(source_count) + " exceeds the supported " +
			std::to_string(GaussianSplatMaxCount) + "-splat limit; explicitly set max_count (CLI: --max-count) to reduce the input");
	data.count = options.max_count ? std::min(source_count, options.max_count) : source_count;
	data.source_count = source_count;
	data.sh_degree = degree;
	const uint32_t non_dc = CoefficientCount(degree) - 1;
	uint64_t native_size;
	if (!HGS2CanonicalSize(data.count, non_dc, native_size))
		return SetError(&error, "Retained Gaussian expanded CPU payload length overflow");
	if (native_size > GaussianSplatMaxNativeBytes || native_size > std::numeric_limits<size_t>::max())
		return SetError(&error, "Retained Gaussian expanded CPU payload requires " + std::to_string(native_size) +
			" bytes, exceeding the 512 MiB budget; reduce max_count/sh_degree (CLI: --max-count/--sh-degree)");
	return true;
}

uint32_t HGS2HeaderCRC(const uint8_t *header) {
	const uint8_t zero[4] = {};
	mz_ulong crc = mz_crc32(MZ_CRC32_INIT, header, 104);
	crc = mz_crc32(crc, zero, sizeof(zero));
	crc = mz_crc32(crc, header + 108, HGSHeaderSize - 108);
	return uint32_t(crc);
}

uint32_t HGS2QuantizeUnit(double value, uint32_t max_code) {
	value = std::max(0.0, std::min(1.0, value));
	return uint32_t(std::floor(value * max_code + 0.5));
}

uint32_t HGS2EncodeLinear(float value, float min_value, float max_value, uint32_t max_code) {
	if (min_value == max_value || value == min_value)
		return 0;
	if (value == max_value)
		return max_code;
	return HGS2QuantizeUnit((double(value) - min_value) / (double(max_value) - min_value), max_code);
}

float HGS2DecodeLinear(float min_value, float max_value, uint32_t code, uint32_t max_code) {
	if (!code || min_value == max_value)
		return min_value;
	if (code >= max_code)
		return max_value;
	double value = double(min_value) + (double(max_value) - min_value) * (double(code) / max_code);
	value = std::max(double(min_value), std::min(double(max_value), value));
	return float(value);
}

uint32_t HGS2EncodeScale(float value, float min_value, float max_value) {
	if (min_value == max_value || value == min_value)
		return 0;
	if (value == max_value)
		return 255;
	const double min_log = std::log(double(min_value)), max_log = std::log(double(max_value));
	return HGS2QuantizeUnit((std::log(double(value)) - min_log) / (max_log - min_log), 255);
}

float HGS2DecodeScale(float min_value, float max_value, uint32_t code) {
	if (!code || min_value == max_value)
		return min_value;
	if (code >= 255)
		return max_value;
	const double min_log = std::log(double(min_value)), max_log = std::log(double(max_value));
	double value = std::exp(min_log + (max_log - min_log) * (double(code) / 255));
	value = std::max(double(min_value), std::min(double(max_value), value));
	return float(value);
}

int HGS2EncodeSH(float value, float absmax) {
	if (!absmax || !value)
		return 0;
	const double scaled = double(value) / absmax * 127.0;
	const int code = scaled < 0.0 ? int(std::ceil(scaled - 0.5)) : int(std::floor(scaled + 0.5));
	return std::max(-127, std::min(127, code));
}

bool HGS2EncodeQuaternion(const float *source, uint32_t &packed) {
	float wxyz[4];
	if (!NormalizeQuaternion(source, wxyz))
		return false;
	float xyzw[] = {wxyz[1], wxyz[2], wxyz[3], wxyz[0]};
	uint32_t largest = 0;
	for (uint32_t i = 1; i < 4; ++i)
		if (std::abs(xyzw[i]) > std::abs(xyzw[largest]))
			largest = i;
	if (xyzw[largest] < 0.f)
		for (float &value : xyzw)
			value = -value;
	packed = largest << 30;
	uint32_t shift = 0;
	for (int i = 3; i >= 0; --i) {
		if (i == int(largest))
			continue;
		const double magnitude = std::min(511.0, std::floor(std::abs(double(xyzw[i])) * (511.0 * 1.4142135623730950488) + 0.5));
		const uint32_t code = uint32_t(magnitude) | (xyzw[i] < 0.f ? 512u : 0u);
		packed |= code << shift;
		shift += 10;
	}
	return true;
}

void HGS2EncodeFloat32Block(const GaussianSplatData &data, uint32_t first, uint32_t count, std::vector<uint8_t> &raw) {
	size_t cursor = 0;
	for (uint32_t i = 0; i < count; ++i) {
		const float *source = data.base.data() + size_t(first + i) * 16;
		for (uint32_t j = 0; j < 16; ++j, cursor += 4)
			SetF32(raw.data() + cursor, source[j]);
	}
	const uint32_t non_dc = CoefficientCount(data.sh_degree) - 1;
	if (!non_dc)
		return;
	if (!non_dc)
		return;
	for (uint32_t i = 0; i < count; ++i) {
		const float *source = data.higher_sh.data() + size_t(first + i) * non_dc * 4;
		for (uint32_t j = 0; j < non_dc * 4; ++j, cursor += 4)
			SetF32(raw.data() + cursor, source[j]);
	}
}

bool HGS2EncodeQuantizedBlock(const GaussianSplatData &data, uint32_t first, uint32_t count, std::vector<uint8_t> &raw,
	std::string &error) {
	const uint32_t non_dc = CoefficientCount(data.sh_degree) - 1;
	uint64_t expected_size;
	if (!HGS2RawBlockSize(GaussianSplatStorage::Quantized, count, non_dc, expected_size) || raw.size() != expected_size)
		return SetError(&error, "Internal HGS2 quantized block size mismatch");
	std::array<float, 3> pos_min, pos_max, scale_min, scale_max, dc_min, dc_max;
	const float *initial = data.base.data() + size_t(first) * 16;
	for (uint32_t axis = 0; axis < 3; ++axis) {
		pos_min[axis] = pos_max[axis] = initial[axis];
		scale_min[axis] = scale_max[axis] = initial[4 + axis];
		dc_min[axis] = dc_max[axis] = initial[12 + axis];
	}
	std::array<float, HGSMaxSHCoefficients> sh_absmax{};
	for (uint32_t i = 0; i < count; ++i) {
		const float *record = data.base.data() + size_t(first + i) * 16;
		for (uint32_t axis = 0; axis < 3; ++axis) {
			pos_min[axis] = std::min(pos_min[axis], record[axis]);
			pos_max[axis] = std::max(pos_max[axis], record[axis]);
			scale_min[axis] = std::min(scale_min[axis], record[4 + axis]);
			scale_max[axis] = std::max(scale_max[axis], record[4 + axis]);
			dc_min[axis] = std::min(dc_min[axis], record[12 + axis]);
			dc_max[axis] = std::max(dc_max[axis], record[12 + axis]);
		}
		for (uint32_t k = 0; k < non_dc; ++k)
			for (uint32_t channel = 0; channel < 3; ++channel) {
				const float value = data.higher_sh[(size_t(first + i) * non_dc + k) * 4 + channel];
				sh_absmax[k * 3 + channel] = std::max(sh_absmax[k * 3 + channel], std::abs(value));
			}
	}

	size_t cursor = 0;
	for (uint32_t axis = 0; axis < 3; ++axis, cursor += 4)
		SetF32(raw.data() + cursor, pos_min[axis]);
	for (uint32_t axis = 0; axis < 3; ++axis, cursor += 4)
		SetF32(raw.data() + cursor, pos_max[axis]);
	for (uint32_t axis = 0; axis < 3; ++axis, cursor += 4)
		SetF32(raw.data() + cursor, scale_min[axis]);
	for (uint32_t axis = 0; axis < 3; ++axis, cursor += 4)
		SetF32(raw.data() + cursor, scale_max[axis]);
	for (uint32_t axis = 0; axis < 3; ++axis, cursor += 4)
		SetF32(raw.data() + cursor, dc_min[axis]);
	for (uint32_t axis = 0; axis < 3; ++axis, cursor += 4)
		SetF32(raw.data() + cursor, dc_max[axis]);
	for (uint32_t k = 0; k < non_dc; ++k)
		for (uint32_t channel = 0; channel < 3; ++channel, cursor += 4)
			SetF32(raw.data() + cursor, sh_absmax[k * 3 + channel]);

	for (uint32_t i = 0; i < count; ++i) {
		const float *record = data.base.data() + size_t(first + i) * 16;
		for (uint32_t axis = 0; axis < 3; ++axis, cursor += 2)
			SetU16(raw.data() + cursor, uint16_t(HGS2EncodeLinear(record[axis], pos_min[axis], pos_max[axis], 65535)));
	}
	for (uint32_t i = 0; i < count; ++i)
		raw[cursor++] = uint8_t(HGS2EncodeLinear(data.base[size_t(first + i) * 16 + 3], 0.f, 1.f, 255));
	for (uint32_t i = 0; i < count; ++i) {
		const float *record = data.base.data() + size_t(first + i) * 16;
		for (uint32_t axis = 0; axis < 3; ++axis)
			raw[cursor++] = uint8_t(HGS2EncodeScale(record[4 + axis], scale_min[axis], scale_max[axis]));
	}
	for (uint32_t i = 0; i < count; ++i, cursor += 4) {
		uint32_t packed;
		if (!HGS2EncodeQuaternion(data.base.data() + size_t(first + i) * 16 + 8, packed))
			return SetError(&error, "Cannot encode HGS2 quaternion");
		SetU32(raw.data() + cursor, packed);
	}
	for (uint32_t i = 0; i < count; ++i) {
		const float *record = data.base.data() + size_t(first + i) * 16;
		for (uint32_t channel = 0; channel < 3; ++channel)
			raw[cursor++] = uint8_t(HGS2EncodeLinear(record[12 + channel], dc_min[channel], dc_max[channel], 255));
	}
	for (uint32_t i = 0; i < count; ++i)
		for (uint32_t k = 0; k < non_dc; ++k)
			for (uint32_t channel = 0; channel < 3; ++channel) {
				const float value = data.higher_sh[(size_t(first + i) * non_dc + k) * 4 + channel];
				const int code = HGS2EncodeSH(value, sh_absmax[k * 3 + channel]);
				raw[cursor++] = uint8_t(code < 0 ? 256 + code : code);
			}
	if (cursor != raw.size())
		return SetError(&error, "Internal HGS2 quantized serialization length mismatch");
	return true;
}

bool HGS2DecodeQuantizedBlock(const uint8_t *raw, size_t raw_size, uint32_t count, uint32_t non_dc, float *base, float *higher_sh,
	std::string &error) {
	uint64_t expected_size;
	if (!HGS2RawBlockSize(GaussianSplatStorage::Quantized, count, non_dc, expected_size) || raw_size != expected_size)
		return SetError(&error, "HGS2 quantized block length mismatch");
	std::array<float, 3> pos_min, pos_max, scale_min, scale_max, dc_min, dc_max;
	for (uint32_t axis = 0; axis < 3; ++axis) {
		pos_min[axis] = GetF32(raw + axis * 4);
		pos_max[axis] = GetF32(raw + (3 + axis) * 4);
		scale_min[axis] = GetF32(raw + (6 + axis) * 4);
		scale_max[axis] = GetF32(raw + (9 + axis) * 4);
		dc_min[axis] = GetF32(raw + (12 + axis) * 4);
		dc_max[axis] = GetF32(raw + (15 + axis) * 4);
		if (!std::isfinite(pos_min[axis]) || !std::isfinite(pos_max[axis]) || pos_min[axis] > pos_max[axis] ||
			!std::isfinite(scale_min[axis]) || !std::isfinite(scale_max[axis]) || scale_min[axis] <= 0.f ||
			scale_min[axis] > scale_max[axis] || !std::isfinite(dc_min[axis]) || !std::isfinite(dc_max[axis]) ||
			dc_min[axis] > dc_max[axis])
			return SetError(&error, "Invalid HGS2 quantized position/scale/DC range");
	}
	std::array<float, HGSMaxSHCoefficients> sh_absmax{};
	for (uint32_t k = 0; k < non_dc; ++k)
		for (uint32_t channel = 0; channel < 3; ++channel) {
			const float value = GetF32(raw + 72 + (k * 3 + channel) * 4);
			if (!std::isfinite(value) || value < 0.f)
				return SetError(&error, "Invalid HGS2 quantized SH absolute maximum");
			sh_absmax[k * 3 + channel] = value;
		}

	const size_t positions = 72 + size_t(non_dc) * 12;
	const size_t alpha = positions + size_t(count) * 6;
	const size_t scales = alpha + count;
	const size_t rotations = scales + size_t(count) * 3;
	const size_t dc = rotations + size_t(count) * 4;
	const size_t sh = dc + size_t(count) * 3;
	for (uint32_t i = 0; i < count; ++i) {
		float *record = base + size_t(i) * 16;
		for (uint32_t axis = 0; axis < 3; ++axis)
			record[axis] = HGS2DecodeLinear(pos_min[axis], pos_max[axis], GetU16(raw + positions + (size_t(i) * 3 + axis) * 2), 65535);
		record[3] = HGS2DecodeLinear(0.f, 1.f, raw[alpha + i], 255);
		for (uint32_t axis = 0; axis < 3; ++axis)
			record[4 + axis] = HGS2DecodeScale(scale_min[axis], scale_max[axis], raw[scales + size_t(i) * 3 + axis]);
		record[7] = 0.f;
		if (!DecodeSPZQuaternion(raw + rotations + size_t(i) * 4, true, record + 8))
			return SetError(&error, "Invalid HGS2 quantized quaternion");
		for (uint32_t channel = 0; channel < 3; ++channel)
			record[12 + channel] = HGS2DecodeLinear(dc_min[channel], dc_max[channel], raw[dc + size_t(i) * 3 + channel], 255);
		record[15] = 0.f;
		for (uint32_t j = 0; j < 16; ++j)
			if (!std::isfinite(record[j]))
				return SetError(&error, "Non-finite HGS2 quantized base value");
		for (uint32_t k = 0; k < non_dc; ++k) {
			float *output = higher_sh + (size_t(i) * non_dc + k) * 4;
			output[3] = 0.f;
			for (uint32_t channel = 0; channel < 3; ++channel) {
				const uint8_t packed = raw[sh + (size_t(i) * non_dc + k) * 3 + channel];
				const int code = packed < 128 ? int(packed) : int(packed) - 256;
				const float absmax = sh_absmax[k * 3 + channel];
				if (code == -128 || (!absmax && code))
					return SetError(&error, "Invalid HGS2 quantized SH code");
				if (!absmax || !code)
					output[channel] = 0.f;
				else if (code == 127)
					output[channel] = absmax;
				else if (code == -127)
					output[channel] = -absmax;
				else {
					double value = double(absmax) * code / 127.0;
					value = std::max(-double(absmax), std::min(double(absmax), value));
					output[channel] = float(value);
				}
				if (!std::isfinite(output[channel]))
					return SetError(&error, "Non-finite HGS2 quantized SH value");
			}
		}
	}
	return true;
}

bool HGS2InflateBlock(const uint8_t *compressed, uint32_t compressed_size, uint32_t raw_size, uint32_t expected_crc,
	std::vector<uint8_t> &raw, std::string &error) {
	try {
		raw.resize(raw_size);
	} catch (const std::bad_alloc &) {
		return SetError(&error, "Insufficient memory for HGS2 block");
	}
	mz_stream stream{};
	stream.next_in = compressed;
	stream.avail_in = compressed_size;
	stream.next_out = raw.data();
	stream.avail_out = raw_size;
	int status = mz_inflateInit2(&stream, MZ_DEFAULT_WINDOW_BITS);
	if (status != MZ_OK)
		return SetError(&error, status == MZ_MEM_ERROR ? "Insufficient memory for HGS2 inflater" : "Cannot initialize HGS2 inflater");
	status = mz_inflate(&stream, MZ_FINISH);
	const mz_ulong consumed = stream.total_in, produced = stream.total_out;
	const int end_status = mz_inflateEnd(&stream);
	if (status == MZ_MEM_ERROR)
		return SetError(&error, "Insufficient memory while inflating HGS2 block");
	if (status != MZ_STREAM_END)
		return SetError(&error, "Corrupt/truncated HGS2 zlib stream or decompressed size overrun");
	if (end_status != MZ_OK)
		return SetError(&error, "Cannot finalize HGS2 inflater");
	if (consumed != compressed_size)
		return SetError(&error, "HGS2 compressed block has trailing bytes");
	if (produced != raw_size)
		return SetError(&error, "HGS2 decompressed block length mismatch");
	if (uint32_t(mz_crc32(MZ_CRC32_INIT, raw.data(), raw.size())) != expected_crc)
		return SetError(&error, "HGS2 raw block CRC32 mismatch");
	return true;
}

void HGS2UpdateStats(const GaussianSplatData &data, uint32_t first, uint32_t count, const std::vector<float> &decoded_base,
	const std::vector<float> &decoded_sh, GaussianSplatWriteStats &stats) {
	const uint32_t non_dc = CoefficientCount(data.sh_degree) - 1;
	for (uint32_t i = 0; i < count; ++i) {
		const float *source = data.base.data() + size_t(first + i) * 16;
		const float *decoded = decoded_base.data() + size_t(i) * 16;
		const double dx = double(source[0]) - decoded[0], dy = double(source[1]) - decoded[1], dz = double(source[2]) - decoded[2];
		stats.max_position_error = std::max(stats.max_position_error, std::sqrt(dx * dx + dy * dy + dz * dz));
		stats.max_opacity_error = std::max(stats.max_opacity_error, std::abs(double(source[3]) - decoded[3]));
		for (uint32_t axis = 0; axis < 3; ++axis) {
			stats.max_scale_relative_error = std::max(stats.max_scale_relative_error,
				std::abs(double(source[4 + axis]) - decoded[4 + axis]) / double(source[4 + axis]));
			stats.max_dc_error = std::max(stats.max_dc_error, std::abs(double(source[12 + axis]) - decoded[12 + axis]));
		}
		double difference = 0.0, negated_difference = 0.0;
		for (uint32_t j = 0; j < 4; ++j) {
			const double d = double(source[8 + j]) - decoded[8 + j], n = double(source[8 + j]) + decoded[8 + j];
			difference += d * d;
			negated_difference += n * n;
		}
		stats.max_quaternion_error = std::max(stats.max_quaternion_error, std::sqrt(std::min(difference, negated_difference)));
		for (uint32_t k = 0; k < non_dc; ++k)
			for (uint32_t channel = 0; channel < 3; ++channel) {
				const size_t index = (size_t(first + i) * non_dc + k) * 4 + channel;
				const float value = decoded_sh[(size_t(i) * non_dc + k) * 4 + channel];
				stats.max_sh_error = std::max(stats.max_sh_error, std::abs(double(data.higher_sh[index]) - value));
			}
	}
}

GaussianSplatDataResult ReadNative(const Data &input, uint64_t byte_cap) {
	if (!input.GetData())
		return ErrorResult("Missing HGS2 header");
	if (input.GetSize() >= 4 && (!std::memcmp(input.GetData(), "HGS0", 4) || !std::memcmp(input.GetData(), "HGS1", 4)))
		return ErrorResult("HGS0/HGS1 assets are no longer supported; recompile the asset to HGS2");
	if (input.GetSize() < HGSHeaderSize)
		return ErrorResult("Missing HGS2 header");
	if (input.GetSize() > byte_cap || input.GetSize() > GaussianSplatMaxNativeBytes)
		return ErrorResult("Native Gaussian asset exceeds the configured byte budget");
	const uint8_t *bytes = input.GetData();
	if (std::memcmp(bytes, "HGS2", 4) != 0)
		return ErrorResult("Unsupported Gaussian magic (expected HGS2; recompile HGS0/HGS1 assets)");
	if (GetU16(bytes + 4) != 2 || GetU16(bytes + 6) != 0)
		return ErrorResult("Unsupported HGS2 major/minor version");
	if (GetU32(bytes + 8) != HGSEndianMarker || GetU32(bytes + 12) != HGSHeaderSize)
		return ErrorResult("Invalid HGS2 endian marker or header size");
	const uint64_t total_size = GetU64(bytes + 16);
	if (total_size != input.GetSize() || total_size > GaussianSplatMaxNativeBytes)
		return ErrorResult("HGS2 total length is invalid or exceeds 512 MiB");
	const uint32_t flags = GetU32(bytes + 24), degree = GetU32(bytes + 28), count = GetU32(bytes + 32), source_count = GetU32(bytes + 36);
	if (flags & ~HGSKnownFlags)
		return ErrorResult("HGS2 contains unsupported required flags");
	if (degree > 3 || !count || count > GaussianSplatMaxCount || source_count < count)
		return ErrorResult("HGS2 count/source count/SH degree is invalid");
	if (!!(flags & HGSHigherSH) != (degree > 0))
		return ErrorResult("HGS2 higher-SH flag disagrees with SH degree");
	const uint32_t storage_value = GetU32(bytes + 40);
	if (storage_value > uint32_t(GaussianSplatStorage::Quantized))
		return ErrorResult("Unsupported HGS2 storage encoding");
	const GaussianSplatStorage storage = static_cast<GaussianSplatStorage>(storage_value);
	const uint32_t non_dc = CoefficientCount(degree) - 1;
	const uint32_t expected_block_count = uint32_t((uint64_t(count) + HGSBlockSize - 1) / HGSBlockSize);
	uint64_t table_size, expected_payload_offset, canonical_size;
	if (!CheckedMul(expected_block_count, HGSTableEntrySize, table_size) ||
		!CheckedAdd(HGSHeaderSize, table_size, expected_payload_offset) || !HGS2CanonicalSize(count, non_dc, canonical_size))
		return ErrorResult("HGS2 section length overflow");
	if (GetU32(bytes + 44) != HGSBlockSize || GetU32(bytes + 48) != expected_block_count || GetU32(bytes + 52) != HGSTableEntrySize ||
		GetU64(bytes + 56) != HGSHeaderSize || GetU64(bytes + 64) != expected_payload_offset)
		return ErrorResult("HGS2 block/table layout is not canonical");
	const uint64_t payload_offset = GetU64(bytes + 64);
	if (payload_offset > total_size)
		return ErrorResult("HGS2 payload offset exceeds the file length");
	if (canonical_size > GaussianSplatMaxNativeBytes || canonical_size > std::numeric_limits<size_t>::max())
		return ErrorResult("HGS2 expanded canonical payload exceeds 512 MiB");
	const Vec3 bounds_min{GetF32(bytes + 72), GetF32(bytes + 76), GetF32(bytes + 80)};
	const Vec3 bounds_max{GetF32(bytes + 84), GetF32(bytes + 88), GetF32(bytes + 92)};
	if (!IsFinite(bounds_min) || !IsFinite(bounds_max) || bounds_min.x > bounds_max.x || bounds_min.y > bounds_max.y ||
		bounds_min.z > bounds_max.z)
		return ErrorResult("HGS2 bounds are invalid");
	for (size_t i = 112; i < HGSHeaderSize; ++i)
		if (bytes[i] != 0)
			return ErrorResult("HGS2 reserved header bytes must be zero");
	if (HGS2HeaderCRC(bytes) != GetU32(bytes + 104))
		return ErrorResult("HGS2 header CRC32 mismatch");
	if (uint32_t(mz_crc32(MZ_CRC32_INIT, bytes + HGSHeaderSize, size_t(table_size))) != GetU32(bytes + 108))
		return ErrorResult("HGS2 table CRC32 mismatch");

	uint64_t expected_offset = payload_offset, uncompressed_size = 0;
	for (uint32_t block = 0; block < expected_block_count; ++block) {
		const uint8_t *entry = bytes + HGSHeaderSize + size_t(block) * HGSTableEntrySize;
		const uint32_t first = block * HGSBlockSize;
		const uint32_t block_count = std::min(HGSBlockSize, count - first);
		uint64_t expected_raw_size;
		if (!HGS2RawBlockSize(storage, block_count, non_dc, expected_raw_size) || expected_raw_size > std::numeric_limits<uint32_t>::max())
			return ErrorResult("HGS2 raw block length overflow");
		const uint64_t offset = GetU64(entry);
		const uint32_t compressed_size = GetU32(entry + 8), raw_size = GetU32(entry + 12);
		if (offset != expected_offset || !compressed_size || raw_size != expected_raw_size || GetU32(entry + 16) != block_count ||
			GetU64(entry + 24) != 0)
			return ErrorResult("HGS2 table entry is invalid or non-canonical");
		if (expected_offset > total_size || compressed_size > total_size - expected_offset)
			return ErrorResult("HGS2 compressed block exceeds the file length");
		if (!CheckedAdd(expected_offset, compressed_size, expected_offset) || !CheckedAdd(uncompressed_size, raw_size, uncompressed_size))
			return ErrorResult("HGS2 payload length overflow");
	}
	if (expected_offset != total_size)
		return ErrorResult("HGS2 payload blocks are not consecutive or the file has trailing bytes");
	if (uncompressed_size != GetU64(bytes + 96))
		return ErrorResult("HGS2 total uncompressed length mismatch");

	GaussianSplatData data;
	data.count = count;
	data.source_count = source_count;
	data.sh_degree = degree;
	data.antialiased = (flags & HGSAntialiased) != 0;
	data.normalized = (flags & HGSNormalized) != 0;
	data.bounds_min = bounds_min;
	data.bounds_max = bounds_max;
	std::vector<uint8_t> raw;
	try {
		data.base.resize(size_t(count) * 16);
		data.higher_sh.resize(size_t(count) * non_dc * 4);
	} catch (const std::bad_alloc &) {
		return ErrorResult("Insufficient memory for native Gaussian payload");
	}
	for (uint32_t block = 0; block < expected_block_count; ++block) {
		const uint8_t *entry = bytes + HGSHeaderSize + size_t(block) * HGSTableEntrySize;
		const uint32_t first = block * HGSBlockSize;
		const uint32_t block_count = std::min(HGSBlockSize, count - first);
		std::string codec_error;
		if (!HGS2InflateBlock(bytes + size_t(GetU64(entry)), GetU32(entry + 8), GetU32(entry + 12), GetU32(entry + 20), raw, codec_error))
			return ErrorResult("HGS2 block " + std::to_string(block) + ": " + codec_error);
		if (storage == GaussianSplatStorage::Float32) {
			const uint8_t *cursor = raw.data();
			for (uint32_t i = 0; i < block_count; ++i)
				for (uint32_t j = 0; j < 16; ++j, cursor += 4)
					data.base[(size_t(first + i) * 16) + j] = GetF32(cursor);
			for (uint32_t i = 0; i < block_count; ++i)
				for (uint32_t j = 0; j < non_dc * 4; ++j, cursor += 4)
					data.higher_sh[(size_t(first + i) * non_dc * 4) + j] = GetF32(cursor);
		} else {
			float *higher = non_dc ? data.higher_sh.data() + size_t(first) * non_dc * 4 : nullptr;
			if (!HGS2DecodeQuantizedBlock(raw.data(), raw.size(), block_count, non_dc, data.base.data() + size_t(first) * 16, higher,
				codec_error))
				return ErrorResult("HGS2 block " + std::to_string(block) + ": " + codec_error);
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

bool InflateSPZGzip(const Data &input, std::vector<uint8_t> &output, std::string &error) {
	output.clear();
	const uint8_t *bytes = input.GetData();
	const size_t size = input.GetSize();
	if (!bytes || size < 18)
		return SetError(&error, "Empty or truncated SPZ gzip stream");
	if (bytes[0] != 0x1f || bytes[1] != 0x8b)
		return SetError(&error, "SPZ input does not have a gzip header");
	if (bytes[2] != 8)
		return SetError(&error, "SPZ gzip compression method is not DEFLATE");
	const uint8_t flags = bytes[3];
	if (flags & 0xe0)
		return SetError(&error, "SPZ gzip header has reserved flags");

	const size_t footer_offset = size - 8;
	size_t cursor = 10;
	if (flags & 4) { // FEXTRA
		if (cursor > footer_offset || footer_offset - cursor < 2)
			return SetError(&error, "Truncated SPZ gzip FEXTRA length");
		const uint16_t extra_size = GetU16(bytes + cursor);
		cursor += 2;
		if (extra_size > footer_offset - cursor)
			return SetError(&error, "Truncated SPZ gzip FEXTRA payload");
		cursor += extra_size;
	}
	for (uint8_t flag : {uint8_t(8), uint8_t(16)}) { // FNAME, FCOMMENT
		if (!(flags & flag))
			continue;
		const void *terminator = std::memchr(bytes + cursor, 0, footer_offset - cursor);
		if (!terminator)
			return SetError(&error, "Unterminated SPZ gzip FNAME/FCOMMENT field");
		cursor = size_t(static_cast<const uint8_t *>(terminator) - bytes) + 1;
	}
	if (flags & 2) { // FHCRC
		if (cursor > footer_offset || footer_offset - cursor < 2)
			return SetError(&error, "Truncated SPZ gzip header CRC");
		const uint16_t expected = GetU16(bytes + cursor);
		const uint16_t actual = uint16_t(mz_crc32(MZ_CRC32_INIT, bytes, cursor) & 0xffffu);
		if (actual != expected)
			return SetError(&error, "SPZ gzip header CRC mismatch");
		cursor += 2;
	}
	if (cursor >= footer_offset)
		return SetError(&error, "Missing SPZ raw DEFLATE payload");

	const uint32_t expected_crc = GetU32(bytes + footer_offset);
	const uint32_t payload_size = GetU32(bytes + footer_offset + 4);
	if (payload_size > GaussianSplatMaxNativeBytes)
		return SetError(&error, "SPZ decompressed payload exceeds the 512 MiB budget");
	try {
		output.resize(payload_size);
	} catch (const std::bad_alloc &) {
		return SetError(&error, "Insufficient memory for SPZ decompressed payload");
	}

	const size_t deflate_size = footer_offset - cursor;
	mz_stream stream{};
	stream.next_in = bytes + cursor;
	stream.avail_in = static_cast<unsigned int>(deflate_size);
	std::array<uint8_t, 1> empty_output{};
	stream.next_out = payload_size ? output.data() : empty_output.data();
	stream.avail_out = payload_size ? payload_size : 1u;
	int status = mz_inflateInit2(&stream, -MZ_DEFAULT_WINDOW_BITS);
	if (status != MZ_OK)
		return SetError(&error, status == MZ_MEM_ERROR ? "Insufficient memory for SPZ inflater" : "Cannot initialize SPZ raw DEFLATE inflater");
	status = mz_inflate(&stream, MZ_FINISH);
	const mz_ulong consumed = stream.total_in, produced = stream.total_out;
	const int end_status = mz_inflateEnd(&stream);
	if (status == MZ_MEM_ERROR)
		return SetError(&error, "Insufficient memory while inflating SPZ payload");
	if (status != MZ_STREAM_END)
		return SetError(&error, "Corrupt/truncated SPZ DEFLATE stream or decompressed size overrun");
	if (end_status != MZ_OK)
		return SetError(&error, "Cannot finalize SPZ DEFLATE inflater");
	if (produced != payload_size)
		return SetError(&error, "SPZ gzip ISIZE does not match the decompressed payload size");
	if (consumed != deflate_size)
		return SetError(&error, "SPZ gzip has extra DEFLATE bytes, concatenated members, or trailing data");
	const uint32_t actual_crc = uint32_t(mz_crc32(MZ_CRC32_INIT, payload_size ? output.data() : empty_output.data(), payload_size));
	if (actual_crc != expected_crc)
		return SetError(&error, "SPZ gzip payload CRC32 mismatch");
	return true;
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
	std::string error;
	if (!ValidateImportOptions(options, error))
		return ErrorResult(error);

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
	if (!PrepareImport(data, source_count, degree, options, error))
		return ErrorResult(error);
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
	std::string error;
	if (!ValidateImportOptions(options, error))
		return ErrorResult(error);

	const uint32_t source_count = uint32_t(input.GetSize() / SplatRecordSize);
	GaussianSplatData data;
	if (!PrepareImport(data, source_count, 0, options, error))
		return ErrorResult(error);
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

GaussianSplatDataResult ImportGaussianSplatSPZ(const Data &input, const GaussianSplatImportOptions &options) {
	if (input.GetSize() > GaussianSplatMaxSPZBytes)
		return ErrorResult("Compressed SPZ input exceeds the 1 GiB input budget");
	std::string error;
	if (!ValidateImportOptions(options, error))
		return ErrorResult(error);

	if (input.GetData() && input.GetSize() >= 8 && std::memcmp(input.GetData(), "NGSP", 4) == 0)
		return ErrorResult("Unsupported plaintext SPZ container (including v4/ZSTD); only gzip SPZ versions 2 and 3 are supported");

	std::vector<uint8_t> payload;
	if (!InflateSPZGzip(input, payload, error))
		return ErrorResult(error);
	if (payload.size() < 16)
		return ErrorResult("Truncated SPZ NGSP header");
	const uint8_t *header = payload.data();
	if (std::memcmp(header, "NGSP", 4) != 0)
		return ErrorResult("Invalid SPZ magic (expected NGSP)");
	const uint32_t version = GetU32(header + 4), source_count = GetU32(header + 8);
	if (version != 2 && version != 3)
		return ErrorResult("Unsupported SPZ version " + std::to_string(version) + "; only gzip versions 2 and 3 are supported");
	const uint32_t source_degree = header[12], fractional_bits = header[13], flags = header[14];
	if (source_degree > 3)
		return ErrorResult("Unsupported SPZ SH degree " + std::to_string(source_degree) + "; only degrees 0..3 are supported");
	if (flags & 2)
		return ErrorResult("SPZ extensions are not supported");
	if (flags & ~1u)
		return ErrorResult("Unsupported SPZ flags " + std::to_string(flags) + "; only the antialiasing flag is supported");
	if (header[15] != 0)
		return ErrorResult("SPZ reserved header byte 15 must be zero");
	if (fractional_bits > 24)
		return ErrorResult("SPZ fractionalBits must be in 0..24");
	if (!source_count)
		return ErrorResult("SPZ point count must be non-zero");

	const uint32_t source_non_dc = CoefficientCount(source_degree) - 1;
	const uint32_t source_sh_stride = source_non_dc * 3;
	const uint32_t rotation_stride = version == 2 ? 3 : 4;
	const std::array<uint64_t, 6> strides = {9, 1, 3, 3, rotation_stride, source_sh_stride};
	std::array<uint64_t, 6> offsets{};
	uint64_t expected_size = 16;
	for (size_t i = 0; i < offsets.size(); ++i) {
		uint64_t section_size;
		offsets[i] = expected_size;
		if (!CheckedMul(source_count, strides[i], section_size) || !CheckedAdd(expected_size, section_size, expected_size))
			return ErrorResult("SPZ packed payload length overflow");
	}
	if (expected_size != payload.size() || expected_size > GaussianSplatMaxNativeBytes)
		return ErrorResult("SPZ packed payload length does not exactly match its header or exceeds 512 MiB");

	GaussianSplatData data;
	if (!PrepareImport(data, source_count, std::min(source_degree, options.sh_degree), options, error))
		return ErrorResult(error);
	data.antialiased = (flags & 1) != 0;
	const uint32_t non_dc = CoefficientCount(data.sh_degree) - 1;
	try {
		data.base.resize(size_t(data.count) * 16);
		data.higher_sh.resize(size_t(data.count) * non_dc * 4);
	} catch (const std::bad_alloc &) {
		return ErrorResult("Insufficient memory for retained SPZ Gaussian payload");
	}

	const uint8_t *positions = payload.data() + offsets[0];
	const uint8_t *alphas = payload.data() + offsets[1];
	const uint8_t *colors = payload.data() + offsets[2];
	const uint8_t *scales = payload.data() + offsets[3];
	const uint8_t *rotations = payload.data() + offsets[4];
	const uint8_t *sh = payload.data() + offsets[5];
	const float position_scale = std::ldexp(1.0f, -int(fractional_bits));
	uint32_t retained = 0;
	Vec3 bounds_min, bounds_max;
	Vec3 source_min, source_max;
	for (uint32_t source = 0; source < source_count; ++source) {
		const uint64_t numerator = (uint64_t(retained) * 2 + 1) * source_count;
		const uint32_t selected_index = retained < data.count ? uint32_t(numerator / (uint64_t(data.count) * 2)) : source_count;
		const bool selected = retained < data.count && source == selected_index;
		std::array<float, 16> discarded{};
		float *base = selected ? data.base.data() + size_t(retained) * 16 : discarded.data();
		for (uint32_t axis = 0; axis < 3; ++axis) {
			const uint8_t *packed_position = positions + size_t(source) * 9 + axis * 3;
			const uint32_t raw = uint32_t(packed_position[0]) | (uint32_t(packed_position[1]) << 8) | (uint32_t(packed_position[2]) << 16);
			int32_t fixed = int32_t(raw & 0x7fffffu);
			if (raw & 0x800000u)
				fixed -= 0x800000;
			base[axis] = float(fixed) * position_scale;
			base[12 + axis] = float((double(colors[size_t(source) * 3 + axis]) / 255.0 - 0.5) / 0.15);
			if (!std::isfinite(base[axis]) || !std::isfinite(base[12 + axis]))
				return ErrorResult("Non-finite SPZ position/DC color at source record " + std::to_string(source));
			if (!RobustScale(float(scales[size_t(source) * 3 + axis]) / 16.f - 10.f, base[4 + axis]))
				return ErrorResult("Invalid SPZ exponential scale at source record " + std::to_string(source));
		}
		base[3] = float(alphas[source]) / 255.f;
		if (!std::isfinite(base[3]) || base[3] < 0.f || base[3] > 1.f)
			return ErrorResult("Invalid SPZ alpha at source record " + std::to_string(source));
		if (!DecodeSPZQuaternion(rotations + size_t(source) * rotation_stride, version == 3, base + 8))
			return ErrorResult("Zero/invalid SPZ quaternion at source record " + std::to_string(source));
		for (uint32_t axis = 0; axis < 3; ++axis) {
			source_min[axis] = source ? std::min(source_min[axis], base[axis]) : base[axis];
			source_max[axis] = source ? std::max(source_max[axis], base[axis]) : base[axis];
		}

		if (selected) {
			for (uint32_t k = 0; k < non_dc; ++k)
				for (uint32_t channel = 0; channel < 3; ++channel) {
					const uint8_t packed_sh = sh[size_t(source) * source_sh_stride + k * 3 + channel];
					float &value = data.higher_sh[(size_t(retained) * non_dc + k) * 4 + channel];
					value = (float(packed_sh) - 128.f) / 128.f;
					if (!std::isfinite(value))
						return ErrorResult("Non-finite SPZ SH coefficient at source record " + std::to_string(source));
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
		return ErrorResult("Internal midpoint selection did not retain the requested SPZ count");
	data.bounds_min = bounds_min;
	data.bounds_max = bounds_max;
	return FinalizeGaussianSplatImport(data, source_min, source_max, options.normalize);
}

GaussianSplatDataResult ImportGaussianSplatSPZFromFile(const char *path, const GaussianSplatImportOptions &options) {
	Data input;
	std::string error;
	if (!LoadBoundedFile(path, GaussianSplatMaxSPZBytes, input, error))
		return ErrorResult(error);
	return ImportGaussianSplatSPZ(input, options);
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

bool WriteGaussianSplatData(const GaussianSplatData &data, Data &output, std::string *error, const GaussianSplatWriteOptions &options,
	GaussianSplatWriteStats *stats) {
	output.Reset();
	if (stats)
		*stats = {};
	std::string validation_error;
	if (!ValidateCanonical(data, validation_error))
		return SetError(error, validation_error);
	if (options.storage != GaussianSplatStorage::Float32 && options.storage != GaussianSplatStorage::Quantized)
		return SetError(error, "Unsupported HGS2 storage encoding");
	const uint32_t non_dc = CoefficientCount(data.sh_degree) - 1;
	const uint32_t block_count = (data.count + HGSBlockSize - 1) / HGSBlockSize;
	uint64_t table_size, payload_offset, canonical_size;
	if (!CheckedMul(block_count, HGSTableEntrySize, table_size) || !CheckedAdd(HGSHeaderSize, table_size, payload_offset) ||
		!HGS2CanonicalSize(data.count, non_dc, canonical_size))
		return SetError(error, "HGS2 output length overflow");
	if (canonical_size > GaussianSplatMaxNativeBytes || canonical_size > std::numeric_limits<size_t>::max())
		return SetError(error, "HGS2 expanded canonical payload exceeds 512 MiB");
	if (payload_offset > GaussianSplatMaxNativeBytes)
		return SetError(error, "Native Gaussian output exceeds its size budget");
	try {
		std::vector<uint8_t> table(size_t(table_size), 0), raw;
		std::vector<std::vector<uint8_t>> compressed_blocks(block_count);
		std::vector<float> decoded_base, decoded_sh;
		GaussianSplatWriteStats actual_stats;
		uint64_t payload_size = 0, total_size = payload_offset;
		for (uint32_t block = 0; block < block_count; ++block) {
			const uint32_t first = block * HGSBlockSize;
			const uint32_t count = std::min(HGSBlockSize, data.count - first);
			uint64_t raw_size;
			if (!HGS2RawBlockSize(options.storage, count, non_dc, raw_size) || raw_size > std::numeric_limits<uint32_t>::max())
				return SetError(error, "HGS2 raw block length overflow");
			raw.resize(size_t(raw_size));
			std::string codec_error;
			if (options.storage == GaussianSplatStorage::Float32)
				HGS2EncodeFloat32Block(data, first, count, raw);
			else if (!HGS2EncodeQuantizedBlock(data, first, count, raw, codec_error))
				return SetError(error, codec_error);
			if (stats && options.storage == GaussianSplatStorage::Quantized) {
				decoded_base.resize(size_t(count) * 16);
				decoded_sh.resize(size_t(count) * non_dc * 4);
				float *higher = non_dc ? decoded_sh.data() : nullptr;
				if (!HGS2DecodeQuantizedBlock(raw.data(), raw.size(), count, non_dc, decoded_base.data(), higher, codec_error))
					return SetError(error, "Internal HGS2 quantization decode failed: " + codec_error);
				HGS2UpdateStats(data, first, count, decoded_base, decoded_sh, actual_stats);
			}
			if (!CheckedAdd(actual_stats.uncompressed_bytes, raw_size, actual_stats.uncompressed_bytes))
				return SetError(error, "HGS2 uncompressed length overflow");
			const uint32_t raw_crc = uint32_t(mz_crc32(MZ_CRC32_INIT, raw.data(), raw.size()));
			mz_ulong compressed_size = mz_compressBound(static_cast<mz_ulong>(raw.size()));
			std::vector<uint8_t> &compressed = compressed_blocks[block];
			compressed.resize(size_t(compressed_size));
			const int status = mz_compress2(compressed.data(), &compressed_size, raw.data(), static_cast<mz_ulong>(raw.size()), 6);
			if (status != MZ_OK)
				return SetError(error, status == MZ_MEM_ERROR ? "Insufficient memory while compressing HGS2 block" : "Cannot compress HGS2 block");
			if (!compressed_size || compressed_size > std::numeric_limits<uint32_t>::max())
				return SetError(error, "Invalid HGS2 compressed block length");
			compressed.resize(size_t(compressed_size));
			uint64_t next_payload_size;
			if (!CheckedAdd(payload_size, compressed_size, next_payload_size) || !CheckedAdd(payload_offset, next_payload_size, total_size))
				return SetError(error, "HGS2 output length overflow");
			if (total_size > GaussianSplatMaxNativeBytes || total_size > std::numeric_limits<size_t>::max())
				return SetError(error, "Native Gaussian output exceeds its size budget");
			uint8_t *entry = table.data() + size_t(block) * HGSTableEntrySize;
			SetU64(entry, payload_offset + payload_size);
			SetU32(entry + 8, uint32_t(compressed_size));
			SetU32(entry + 12, uint32_t(raw_size));
			SetU32(entry + 16, count);
			SetU32(entry + 20, raw_crc);
			payload_size = next_payload_size;
		}
		const uint32_t table_crc = uint32_t(mz_crc32(MZ_CRC32_INIT, table.data(), table.size()));
		if (!output.Resize(size_t(total_size)))
			return SetError(error, "Cannot allocate native Gaussian output");
		output.SetCursor(0);
		output.Write("HGS2", 4);
		PutU16(output, 2);
		PutU16(output, 0);
		PutU32(output, HGSEndianMarker);
		PutU32(output, HGSHeaderSize);
		PutU64(output, total_size);
		PutU32(output, (data.antialiased ? HGSAntialiased : 0) | (data.normalized ? HGSNormalized : 0) | (data.sh_degree ? HGSHigherSH : 0));
		PutU32(output, data.sh_degree);
		PutU32(output, data.count);
		PutU32(output, data.source_count);
		PutU32(output, uint32_t(options.storage));
		PutU32(output, HGSBlockSize);
		PutU32(output, block_count);
		PutU32(output, HGSTableEntrySize);
		PutU64(output, HGSHeaderSize);
		PutU64(output, payload_offset);
		for (uint32_t axis = 0; axis < 3; ++axis)
			PutF32(output, data.bounds_min[axis]);
		for (uint32_t axis = 0; axis < 3; ++axis)
			PutF32(output, data.bounds_max[axis]);
		PutU64(output, actual_stats.uncompressed_bytes);
		PutU32(output, 0);
		PutU32(output, table_crc);
		PutU64(output, 0);
		PutU64(output, 0);
		output.Write(table.data(), table.size());
		for (const std::vector<uint8_t> &compressed : compressed_blocks)
			output.Write(compressed.data(), compressed.size());
		if (output.GetCursor() != total_size)
			return SetError(error, "Internal HGS2 serialization length mismatch");
		SetU32(output.GetData() + 104, HGS2HeaderCRC(output.GetData()));
		actual_stats.stored_bytes = total_size;
		if (stats)
			*stats = actual_stats;
		return true;
	} catch (const std::bad_alloc &) {
		output.Reset();
		return SetError(error, "Cannot allocate HGS2 codec buffers");
	}
}

bool WriteGaussianSplatDataToFile(const GaussianSplatData &data, const char *path, std::string *error, const GaussianSplatWriteOptions &options,
	GaussianSplatWriteStats *stats) {
	if (!path || !*path)
		return SetError(error, "Empty output path");
	Data output;
	if (!WriteGaussianSplatData(data, output, error, options, stats))
		return false;
	// The complete asset is validated and serialized before the destination is opened.
	if (!SaveDataToFile(path, output))
		return SetError(error, std::string("Cannot write '") + path + "'");
	return true;
}

} // namespace hg
