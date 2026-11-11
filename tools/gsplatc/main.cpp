// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/gaussian_splat_data.h"

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <string>

namespace {

bool ParseCount(const std::string &text, uint32_t &value) {
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

std::string InputExtension(const std::string &path) {
	const auto dot = path.find_last_of('.'), separator = path.find_last_of("/\\");
	if (dot == std::string::npos || (separator != std::string::npos && dot < separator))
		return {};
	std::string extension = path.substr(dot + 1);
	for (char &c : extension)
		c = char(std::tolower(static_cast<unsigned char>(c)));
	return extension;
}

void Usage() {
	std::cout << "Usage: gsplatc INPUT.ply|INPUT.splat|INPUT.spz OUTPUT.hgs [--max-count N] [--sh-degree 0..3] [--normalize] [--storage quantized|float32]\n"
		"Supported inputs: strict binary_little_endian PLY, headerless 32-byte .splat, and gzip SPZ versions 2/3 (SH degree 0..3).\n"
		"Default storage: quantized HGS2 with compressed blocks. Omitted --max-count retains all source splats; explicit reduction requires 1.."
		<< hg::GaussianSplatMaxCount << ". Reduction uses deterministic midpoint uniform selection.\n";
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 3) {
		Usage();
		return 2;
	}
	hg::GaussianSplatImportOptions options;
	hg::GaussianSplatWriteOptions write_options;
	hg::GaussianSplatWriteStats stats;
	std::string input, output;
	bool max_seen = false, degree_seen = false, normalize_seen = false, storage_seen = false;
	for (int i = 1; i < argc; ++i) {
		const std::string arg = argv[i];
		if (arg == "--max-count" || arg == "--sh-degree") {
			if (i + 1 >= argc) {
				std::cerr << "Missing value for " << arg << '\n';
				return 2;
			}
			uint32_t value;
			if (!ParseCount(argv[++i], value)) {
				std::cerr << "Invalid numeric value for " << arg << '\n';
				return 2;
			}
			if (arg == "--max-count") {
				if (max_seen || !value || value > hg::GaussianSplatMaxCount) {
					std::cerr << "--max-count must be unique and in 1.." << hg::GaussianSplatMaxCount << " (omit it to retain all source splats)\n";
					return 2;
				}
				max_seen = true;
				options.max_count = value;
			} else {
				if (degree_seen || value > 3) {
					std::cerr << "--sh-degree must be unique and in 0..3\n";
					return 2;
				}
				degree_seen = true;
				options.sh_degree = value;
			}
		} else if (arg == "--storage") {
			if (storage_seen || i + 1 >= argc) {
				std::cerr << "--storage must be unique and followed by quantized or float32\n";
				return 2;
			}
			storage_seen = true;
			const std::string storage = argv[++i];
			if (storage == "quantized")
				write_options.storage = hg::GaussianSplatStorage::Quantized;
			else if (storage == "float32")
				write_options.storage = hg::GaussianSplatStorage::Float32;
			else {
				std::cerr << "--storage must be quantized or float32\n";
				return 2;
			}
		} else if (arg == "--normalize") {
			if (normalize_seen) {
				std::cerr << "Duplicate --normalize\n";
				return 2;
			}
			normalize_seen = true;
			options.normalize = true;
		} else if (!arg.empty() && arg[0] == '-') {
			std::cerr << "Unknown option: " << arg << '\n';
			return 2;
		} else if (input.empty())
			input = arg;
		else if (output.empty())
			output = arg;
		else {
			std::cerr << "Unexpected extra positional argument: " << arg << '\n';
			return 2;
		}
	}
	if (input.empty() || output.empty() || input == output) {
		Usage();
		return 2;
	}
	const std::string extension = InputExtension(input);
	hg::GaussianSplatDataResult imported;
	if (extension == "ply")
		imported = hg::ImportGaussianSplatPLYFromFile(input.c_str(), options);
	else if (extension == "splat")
		imported = hg::ImportGaussianSplatBinaryFromFile(input.c_str(), options);
	else if (extension == "spz")
		imported = hg::ImportGaussianSplatSPZFromFile(input.c_str(), options);
	else {
		std::cerr << "gsplatc: Unsupported input extension for '" << input << "' (expected .ply, .splat or .spz)\n";
		return 2;
	}
	if (!imported.ok) {
		std::cerr << "gsplatc: " << imported.message << '\n';
		return 1;
	}
	std::string error;
	if (!hg::WriteGaussianSplatDataToFile(imported.data, output.c_str(), &error, write_options, &stats)) {
		std::cerr << "gsplatc: " << error << '\n';
		return 1;
	}
	const auto &data = imported.data;
	std::cout << "Source splats: " << data.source_count << "\nRetained splats: " << data.count << "\nSH degree: " << data.sh_degree
		<< "\nBounds min: " << data.bounds_min.x << ' ' << data.bounds_min.y << ' ' << data.bounds_min.z << "\nBounds max: " << data.bounds_max.x << ' '
		<< data.bounds_max.y << ' ' << data.bounds_max.z << "\nPostshot antialiasing: " << (data.antialiased ? 1 : 0) << "\nNormalized: "
		<< (data.normalized ? 1 : 0) << "\nStorage: " << (write_options.storage == hg::GaussianSplatStorage::Quantized ? "quantized" : "float32")
		<< "\nUncompressed block bytes: " << stats.uncompressed_bytes << "\nOutput bytes: " << stats.stored_bytes
		<< "\nMaximum quantization errors: position(L2)=" << stats.max_position_error << " scale(relative)=" << stats.max_scale_relative_error
		<< " opacity=" << stats.max_opacity_error << " DC=" << stats.max_dc_error << " SH=" << stats.max_sh_error
		<< " quaternion(sign-equivalent L2)=" << stats.max_quaternion_error
		<< "\nQuantization clipping: none (adaptive block ranges)\n";
	return 0;
}
