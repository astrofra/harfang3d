#include "cmd_line.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include "bc7enc/rdo_bc_encoder.h"
#include "bc7enc/utils.h"
#include "fmt/core.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

struct FormatInfo {
	DXGI_FORMAT dxgi_format;
	std::string desc;
};

static const std::map<std::string, FormatInfo> format_infos = {
	{"BC1_UNORM", {DXGI_FORMAT_BC1_UNORM, "(bc7enc+rdo) RGB565+1 bit alpha 4bpp"}},
	{"BC3_UNORM", {DXGI_FORMAT_BC3_UNORM, "(bc7enc+rdo) RGB565+A8 8bpp"}},
	{"BC4_UNORM", {DXGI_FORMAT_BC4_UNORM, "(bc7enc+rdo) R8 4bpp"}},
	{"BC5_UNORM", {DXGI_FORMAT_BC5_UNORM, "(bc7enc+rdo) R8G8 8bpp"}},
	{"BC7_UNORM", {DXGI_FORMAT_BC7_UNORM, "(bc7enc+rdo) RGBA8 8bpp"}},
	{"R8G8B8A8_UNORM", {DXGI_FORMAT_R8G8B8A8_UNORM, "RGBA8 32bpp"}},
	{"B8G8R8A8_UNORM", {DXGI_FORMAT_B8G8R8A8_UNORM, "BGRA8 32bpp"}},
};

static const std::map<std::string, std::string> format_aliases = {
	{"DXT1", "BC1_UNORM"},
	{"DXT5", "BC3_UNORM"},
	{"BC1", "BC1_UNORM"},
	{"BC3", "BC3_UNORM"},
	{"BC4", "BC4_UNORM"},
	{"BC5", "BC5_UNORM"},
	{"BC7", "BC7_UNORM"},
	{"RGBA", "R8G8B8A8_UNORM"},
	{"BGRA", "B8G8R8A8_UNORM"},
	{"RGBA8", "R8G8B8A8_UNORM"},
	{"BGRA8", "B8G8R8A8_UNORM"},
};

static void OutputUsage(const CmdLineFormat &cmd_format) {
	fmt::print("texc - Image to DDS - e.julien.niji@gmail.com\n");
	fmt::print("  Stupid CLI wrapper around: stb_image.h, stb_image_resize.h, bc7enc\n");
	fmt::print("Usage: texc {}\n", FormatCmdLineArgs(cmd_format));
}

static std::set<std::string> GetFormatAliases(const std::string &format) {
	std::set<std::string> aliases;
	for (const auto &i : format_aliases) {
		if (i.second == format) {
			aliases.insert(i.first);
		}
	}
	return aliases;
}

static std::string SolveFormatAlias(const std::string &format) {
	if (format_infos.find(format) != std::end(format_infos)) {
		return format; // not an alias
	}

	const auto &i = format_aliases.find(format);
	return i != std::end(format_aliases) ? i->second : format;
}

static DXGI_FORMAT GetDXGIFormat(const std::string &format) {
	const auto &i = format_infos.find(format);
	return i != std::end(format_infos) ? i->second.dxgi_format : DXGI_FORMAT_UNKNOWN;
}

static void OutputHelp(const CmdLineFormat &cmd_format) {
	fmt::print("\n{}\n", FormatCmdLineArgsDescription(cmd_format));
	fmt::print("Supported formats:\n");
	for (const auto &format_info : format_infos) {
		fmt::print("  - {}: {}", format_info.first, format_info.second.desc);

		const std::set<std::string> aliases = GetFormatAliases(format_info.first);

		if (!aliases.empty()) {
			fmt::print(" (aliases: {})\n", join(std::begin(aliases), std::end(aliases), ","));
		} else {
			fmt::print("\n");
		}
	}
}

static int get_pot(int x) {
	int r = 1;
	while (r < x) {
		r *= 2;
	}
	return r;
}

static bool bc7enc_to_dds(
	int quality, unsigned char **mips_data, const size_t mip_count, int width, int height, const DXGI_FORMAT dxgi_format, const std::string &output_filename) {
	std::vector<rdo_bc::rdo_bc_encoder> encoder(mip_count);

	rdo_bc::rdo_bc_params rp;

	rp.m_rdo_max_threads = std::max(1u, std::thread::hardware_concurrency());
	rp.m_status_output = false;

	rp.m_bc1_quality_level = (18 * quality) / 10;
	rp.m_bc7_uber_level = (4 * quality) / 10;
	rp.m_bc7enc_max_partitions_to_scan = (64 * quality) / 10;

	rp.m_bc345_search_rad = (8 * quality) / 10;
	rp.m_use_hq_bc345 = quality > 5;

	rp.m_dxgi_format = dxgi_format;

	//
	int mip_width = width, mip_height = height;

	for (size_t mip = 0; mip < mip_count; ++mip) {
		utils::image_u8 image;

		image.init(mip_width, mip_height);
		memcpy(image.get_pixels().data(), mips_data[mip], mip_width * mip_height * 4);

		//
		if (!encoder[mip].init(image, rp)) {
			return false;
		}

		if (mip == 0) {
			if (encoder[mip].get_has_alpha()) {
				fmt::print("Source image has alpha\n");
			} else {
				fmt::print("Source image has no alpha\n");
			}
		}

		fmt::print("Encoding mip {}\n", mip);

		if (!encoder[mip].encode()) {
			fmt::print("Failed to encode\n");
			return false;
		}

		mip_width = std::max(1, mip_width / 2);
		mip_height = std::max(1, mip_height / 2);
	}

	//
	const bool force_dx10_dds = true;

	size_t pixel_format_bpp = 0;

	switch (dxgi_format) {
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC4_UNORM:
			pixel_format_bpp = 4;
			break;

		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC7_UNORM:
			pixel_format_bpp = 8;
			break;
	}

	std::vector<const void *> mip_blocks(mip_count);
	for (size_t mip = 0; mip < mip_count; ++mip) {
		mip_blocks[mip] = encoder[mip].get_blocks();
	}

	if (!utils::save_dds(output_filename.c_str(), encoder[0].get_orig_width(), encoder[0].get_orig_height(), mip_blocks.data(), mip_blocks.size(),
			pixel_format_bpp, rp.m_dxgi_format, rp.m_perceptual, force_dx10_dds)) {
		fmt::print("Failed to save output texture\n");
		return false;
	}

	return true;
}

static bool raw_to_dds(
	unsigned char **mips_data, const size_t mip_count, int width, int height, const DXGI_FORMAT dxgi_format, const std::string &output_filename) {
	const bool force_dx10_dds = true;

	size_t pixel_format_bpp = 0;

	switch (dxgi_format) {
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			pixel_format_bpp = 32;
			break;

		default:
			fmt::print("Unsupported RAW format\n");
			return false;
	}

	std::vector<const void *> mip_blocks(mip_count);
	for (size_t mip = 0; mip < mip_count; ++mip) {
		mip_blocks[mip] = mips_data[mip];
	}

	if (!utils::save_dds(output_filename.c_str(), width, height, mip_blocks.data(), mip_blocks.size(), pixel_format_bpp, dxgi_format, true, force_dx10_dds)) {
		fmt::print("Failed to save output texture\n");
		return false;
	}

	return true;
}

int main(int argc, const char **args) {
	CmdLineFormat cmd_format = {
		{
			{"-resize-npot", "Resize texture to nearest power of two", false},
			{"-generate-mips", "Generate mips", false},
			{"-help", "Output help", false},
		},
		{
			{"-max-size", "Maximum texture size (0 = unlimited)", true},
			{"-format", "Output texture format", true},
			{"-quality", "Control encoder quality where 0=fastest, 10=best quality. Default is 0.", true},
		},
		{
			{"input", "Input image"},
			{"output", "Output texture", true},
		},
		{
			{"-r", "-resize-npot"},
			{"-m", "-generate-mips"},
			{"-h", "-help"},

			{"-s", "-max-size"},
			{"-f", "-format"},
			{"-q", "-quality"},
		},
	};

	CmdLineContent cmd_content;
	if (!ParseCmdLine({args + 1, args + argc}, cmd_format, cmd_content)) {
		OutputUsage(cmd_format);
		return -1;
	}

	//
	if (GetCmdLineFlagValue(cmd_content, "-help")) {
		OutputUsage(cmd_format);
		OutputHelp(cmd_format);
		return 0;
	}

	//
	if (cmd_content.positionals.size() < 1) {
		OutputUsage(cmd_format);
		return -2;
	}

	const std::string input_filename = cmd_content.positionals[0];

	std::string output_filename;

	if (cmd_content.positionals.size() > 1) {
		output_filename = cmd_content.positionals[1];
	} else {
		output_filename = input_filename + ".dds";
	}

	//
	const std::string format = SolveFormatAlias(GetCmdLineSingleValue(cmd_content, "-format", "BC7"));
	const DXGI_FORMAT dxgi_format = GetDXGIFormat(format);

	if (dxgi_format == DXGI_FORMAT_UNKNOWN) {
		fmt::print("Unsupported format {}\n", format);
		return -4;
	}

	int max_size, quality;
	try {
		max_size = GetCmdLineSingleValue(cmd_content, "-max-size", 0);
		quality = GetCmdLineSingleValue(cmd_content, "-quality", 0);
	} catch (const std::exception &) {
		fmt::print("Invalid maximum size or quality\n");
		return -2;
	}
	if (max_size < 0 || quality < 0 || quality > 10) {
		fmt::print("Maximum size must be non-negative and quality must be between 0 and 10\n");
		return -2;
	}
	const bool resize_npot = GetCmdLineFlagValue(cmd_content, "-resize-npot");
	// Rounding up to a power of two must not exceed a non-power-of-two size limit.
	if (resize_npot && max_size > 0) {
		int pot_max = 1;
		while (pot_max <= max_size / 2)
			pot_max *= 2;
		max_size = pot_max;
	}

	// load input image
	const std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();

	fmt::print("Loading input image {}\n", input_filename);

	int in_width, in_height, in_n_comp;
	std::vector<unsigned char *> in_mips_data(1);
	in_mips_data[0] = stbi_load(input_filename.c_str(), &in_width, &in_height, &in_n_comp, 4);

	if (in_mips_data[0] == nullptr) {
		fmt::print("Failed to load input image {}\n", input_filename);
		return -3;
	}

	fmt::print("Loaded input image {}x{}x{}\n", in_width, in_height, in_n_comp);

	//
	if (max_size > 0) {
		if (in_width > max_size || in_height > max_size) {
			fmt::print("Enforcing maximum size constraint\n");

			int w = in_width, h = in_height;

			if (in_width > in_height) {
				w = max_size;
				h = std::max(1, int((int64_t(in_height) * max_size + in_width / 2) / in_width));
			} else {
				h = max_size;
				w = std::max(1, int((int64_t(in_width) * max_size + in_height / 2) / in_height));
			}

			fmt::print("Resizing to {}x{}\n", w, h);

			unsigned char *out_data = (unsigned char *)malloc(w * h * 4);
			stbir_resize_uint8_linear(in_mips_data[0], in_width, in_height, 0, out_data, w, h, 0, STBIR_4CHANNEL);
			stbi_image_free(in_mips_data[0]);

			in_mips_data[0] = out_data;
			in_width = w;
			in_height = h;
		}
	}

	//
	if (resize_npot) {
		fmt::print("Enforcing power of two constraint\n");

		const int pot_width = get_pot(in_width), pot_height = get_pot(in_height);

		if (pot_width == in_width && pot_height == in_height) {
			fmt::print("No need to resize\n");
		} else {
			fmt::print("Resizing to {}x{}\n", pot_width, pot_height);

			unsigned char *out_data = (unsigned char *)malloc(pot_width * pot_height * 4);
			stbir_resize_uint8_linear(in_mips_data[0], in_width, in_height, 0, out_data, pot_width, pot_height, 0, STBIR_4CHANNEL);
			stbi_image_free(in_mips_data[0]);

			in_mips_data[0] = out_data;
			in_width = pot_width;
			in_height = pot_height;
		}
	}

	// RGBA to BGRA
	if (dxgi_format == DXGI_FORMAT_B8G8R8A8_UNORM) {
		for (int y = 0; y < in_height; ++y) {
			uint8_t *p = in_mips_data[0] + y * in_width * 4;
			for (int x = 0; x < in_width; ++x) {
				const uint8_t t = p[0];
				p[0] = p[2];
				p[2] = t;
				p += 4;
			}
		}
	}

	// generate mip levels
	const bool generate_mips = GetCmdLineFlagValue(cmd_content, "-generate-mips");

	if (generate_mips) {
		int out_w = in_width, out_h = in_height;

		int mip = 0;
		for (; out_w > 1 || out_h > 1; ++mip) {
			unsigned char *in_data = in_mips_data[mip];
			int in_w = out_w, in_h = out_h;

			out_w = std::max(1, out_w / 2);
			out_h = std::max(1, out_h / 2);

			unsigned char *out_data = (unsigned char *)malloc(out_w * out_h * 4);
			stbir_resize_uint8_linear(in_data, in_w, in_h, 0, out_data, out_w, out_h, 0, STBIR_4CHANNEL);

			in_mips_data.push_back(out_data);
		}

		fmt::print("Generated {} mips\n", mip);
	}

	//
	switch (dxgi_format) {
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC7_UNORM: {
			if (!bc7enc_to_dds(quality, in_mips_data.data(), in_mips_data.size(), in_width, in_height, dxgi_format, output_filename)) {
				fmt::print("Failed to convert texture\n");
				return -5;
			}
		} break;

		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			if (!raw_to_dds(in_mips_data.data(), in_mips_data.size(), in_width, in_height, dxgi_format, output_filename)) {
				fmt::print("Failed to save texture\n");
				return -5;
			}
			break;

		default:
			fmt::print("Unsupported format {}\n", format);
			return -4;
	}

	//
	const std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
	fmt::print("Elapsed time: {} seconds\n", elapsed_seconds.count());

	for (unsigned char *mip_data : in_mips_data) {
		stbi_image_free(mip_data);
	}
	return 0;
}
