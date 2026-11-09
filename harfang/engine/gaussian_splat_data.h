// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/data.h"
#include "foundation/vector3.h"

#include <cstdint>
#include <string>
#include <vector>

namespace hg {

static constexpr uint32_t GaussianSplatMaxCount = 1024 * 1024;
static constexpr uint32_t GaussianSplatDefaultImportCount = 250000;
static constexpr uint64_t GaussianSplatMaxNativeBytes = 512 * 1024 * 1024;
static constexpr uint64_t GaussianSplatMaxPLYBytes = 1024 * 1024 * 1024;
static constexpr uint64_t GaussianSplatMaxBinaryBytes = 1024 * 1024 * 1024;
static constexpr uint64_t GaussianSplatMaxSPZBytes = 1024 * 1024 * 1024;

// Canonical CPU representation. base contains 16 floats per splat:
// position.xyz/opacity, linear_scale.xyz/0, rotation.wxyz, dc.rgb/0.
// higher_sh contains vec4(rgb, 0) for each non-DC coefficient, coefficient-major per splat.
struct GaussianSplatData {
	std::vector<float> base, higher_sh;
	uint32_t count{0}, source_count{0}, sh_degree{0};
	Vec3 bounds_min{Vec3::Zero}, bounds_max{Vec3::Zero};
	bool antialiased{false}, normalized{false};
};

struct GaussianSplatImportOptions {
	uint32_t max_count{GaussianSplatDefaultImportCount};
	uint32_t sh_degree{3}; // maximum retained degree; never invents missing bands
	bool normalize{false};
};

struct GaussianSplatDataResult {
	bool ok{false};
	std::string message;
	GaussianSplatData data;
};

// Strict Graphdeco/Postshot binary_little_endian float PLY import.
GaussianSplatDataResult ImportGaussianSplatPLY(const Data &input, const GaussianSplatImportOptions &options = {});
GaussianSplatDataResult ImportGaussianSplatPLYFromFile(const char *path, const GaussianSplatImportOptions &options = {});

// Strict gzip SPZ version 2/3 import with SH degree 0..3 and no coordinate conversion.
GaussianSplatDataResult ImportGaussianSplatSPZ(const Data &input, const GaussianSplatImportOptions &options = {});
GaussianSplatDataResult ImportGaussianSplatSPZFromFile(const char *path, const GaussianSplatImportOptions &options = {});

// Strict headerless 32-byte .splat import: LE float32 position/linear scale, RGBA and packed WXYZ bytes.
GaussianSplatDataResult ImportGaussianSplatBinary(const Data &input, const GaussianSplatImportOptions &options = {});
GaussianSplatDataResult ImportGaussianSplatBinaryFromFile(const char *path, const GaussianSplatImportOptions &options = {});

// Versioned HGS1 native asset codec. No GPU objects are created by these functions.
GaussianSplatDataResult ReadGaussianSplatData(const Data &input);
GaussianSplatDataResult ReadGaussianSplatDataFromFile(const char *path);
GaussianSplatDataResult ReadGaussianSplatDataFromAssets(const char *name);
bool WriteGaussianSplatData(const GaussianSplatData &data, Data &output, std::string *error = nullptr);
bool WriteGaussianSplatDataToFile(const GaussianSplatData &data, const char *path, std::string *error = nullptr);

} // namespace hg
