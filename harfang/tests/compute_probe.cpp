// HARFANG(R). Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/render_pipeline.h"
#include "platform/window_system.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

constexpr uint16_t grid_size = 16, target_size = 128;
constexpr uint32_t record_count = grid_size * grid_size;
constexpr uint64_t sampler_flags = BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT |
	BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

struct Vertex { float x, y, z, u, v; };

// Resources die while the renderer is still running, including on setup errors.
struct Resources {
	bgfx::ProgramHandle producer = BGFX_INVALID_HANDLE, consumer = BGFX_INVALID_HANDLE, display = BGFX_INVALID_HANDLE;
	bgfx::VertexBufferHandle input = BGFX_INVALID_HANDLE, vertices = BGFX_INVALID_HANDLE;
	bgfx::DynamicVertexBufferHandle scratch = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle indices = BGFX_INVALID_HANDLE;
	bgfx::TextureHandle atlas = BGFX_INVALID_HANDLE, color = BGFX_INVALID_HANDLE, readback = BGFX_INVALID_HANDLE;
	bgfx::FrameBufferHandle framebuffer = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle seed = BGFX_INVALID_HANDLE, sampler = BGFX_INVALID_HANDLE;

	~Resources() {
		bgfx_Destroy(framebuffer); // Does not own color.
		bgfx_Destroy(producer); bgfx_Destroy(consumer); bgfx_Destroy(display);
		bgfx_Destroy(input); bgfx_Destroy(scratch); bgfx_Destroy(vertices); bgfx_Destroy(indices);
		bgfx_Destroy(atlas); bgfx_Destroy(color); bgfx_Destroy(readback);
		bgfx_Destroy(seed); bgfx_Destroy(sampler);
	}
};

void Require(bool condition, const char *message) {
	if (!condition)
		throw std::runtime_error(message);
}

bool RunProbe(hg::Window *window, const std::string &assets, const std::string &negative_control) {
	const auto &caps = *bgfx::getCaps();
	std::printf("Renderer=%s vendor=0x%04x device=0x%04x maxComputeBindings=%u maxViews=%u maxTextureSize=%u\n",
		bgfx::getRendererName(caps.rendererType), caps.vendorId, caps.deviceId,
		caps.limits.maxComputeBindings, caps.limits.maxViews, caps.limits.maxTextureSize);
	const uint64_t required = BGFX_CAPS_COMPUTE | BGFX_CAPS_TEXTURE_BLIT | BGFX_CAPS_TEXTURE_READ_BACK;
	Require((caps.supported & required) == required, "Missing compute, texture blit, or readback capability");
	Require(caps.limits.maxComputeBindings >= 2 && caps.limits.maxViews >= 4, "Insufficient binding/view limits");
	const uint32_t format_required = BGFX_CAPS_FORMAT_TEXTURE_VERTEX;
	std::printf("RGBA32F format caps=0x%08x\n", caps.formats[bgfx::TextureFormat::RGBA32F]);
	Require((caps.formats[bgfx::TextureFormat::RGBA32F] & format_required) == format_required,
		"RGBA32F vertex texture fetch is required");
	if (!(caps.formats[bgfx::TextureFormat::RGBA32F] & BGFX_CAPS_FORMAT_TEXTURE_IMAGE_WRITE)) {
		// This pinned Metal backend never reports IMAGE_READ/WRITE format bits.
		// Only this diagnostic probe may proceed to empirically test the path;
		// it does not change capabilities or claim support for other devices.
		Require(caps.rendererType == bgfx::RendererType::Metal, "RGBA32F image writes are not advertised");
		std::puts("NOTE: pinned Metal image-write capability is unreported; testing it explicitly");
	}
	Require(bgfx::isTextureValid(0, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_COMPUTE_WRITE | sampler_flags),
		"RGBA32F atlas flags unsupported");
	Require(bgfx::isTextureValid(0, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | sampler_flags),
		"RGBA8 render target unsupported");
	Require(bgfx::isTextureValid(0, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_BLIT_DST),
		"RGBA8 readback texture unsupported");

	Resources r;
	r.producer = hg::LoadComputeProgramFromFile((assets + "/producer.sc").c_str());
	r.consumer = hg::LoadComputeProgramFromFile((assets + "/consumer.sc").c_str());
	r.display = hg::LoadProgramFromFile((assets + "/display").c_str());
	Require(bgfx::isValid(r.producer) && bgfx::isValid(r.consumer) && bgfx::isValid(r.display),
		"Cannot load programs; compile probe assets for the requested renderer first");

	std::array<std::array<float, 4>, record_count> input;
	for (uint32_t i = 0; i < record_count; ++i)
		input[i] = {{float((i * 17 + 13) % 200 + 20), float((i * 29 + 7) % 200 + 20),
			float((i * 43 + 3) % 200 + 20), float(64 + i)}};
	bgfx::VertexLayout data_layout;
	data_layout.begin().add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float).end();
	const uint16_t buffer_format = BGFX_BUFFER_COMPUTE_FORMAT_32X4 | BGFX_BUFFER_COMPUTE_TYPE_FLOAT;
	r.input = bgfx::createVertexBuffer(bgfx::copy(input.data(), uint32_t(sizeof(input))), data_layout,
		buffer_format | BGFX_BUFFER_COMPUTE_READ);
	// No CPU upload/update for a compute-written buffer.
	r.scratch = bgfx::createDynamicVertexBuffer(record_count, data_layout, buffer_format | BGFX_BUFFER_COMPUTE_READ_WRITE);
	r.atlas = bgfx::createTexture2D(grid_size, grid_size, false, 1, bgfx::TextureFormat::RGBA32F,
		BGFX_TEXTURE_COMPUTE_WRITE | sampler_flags);
	r.color = bgfx::createTexture2D(target_size, target_size, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | sampler_flags);
	r.readback = bgfx::createTexture2D(target_size, target_size, false, 1, bgfx::TextureFormat::RGBA8,
		BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_BLIT_DST);
	Require(bgfx::isValid(r.input) && bgfx::isValid(r.scratch) && bgfx::isValid(r.atlas) &&
		bgfx::isValid(r.color) && bgfx::isValid(r.readback), "Cannot allocate compute probe resources");
	r.framebuffer = bgfx::createFrameBuffer(1, &r.color, false);
	r.seed = bgfx::createUniform("u_seed", bgfx::UniformType::Vec4);
	r.sampler = bgfx::createUniform("s_atlas", bgfx::UniformType::Sampler);

	std::array<Vertex, record_count * 4> vertices;
	std::array<uint16_t, record_count * 6> indices;
	for (uint16_t i = 0; i < record_count; ++i) {
		const auto x = i % grid_size, y = i / grid_size;
		const float left = -1.f + 2.f * x / grid_size, top = 1.f - 2.f * y / grid_size;
		const float right = left + 2.f / grid_size, bottom = top - 2.f / grid_size;
		const float u = (x + 0.5f) / grid_size, v = (y + 0.5f) / grid_size;
		vertices[i * 4 + 0] = {left, top, 0.5f, u, v};
		vertices[i * 4 + 1] = {right, top, 0.5f, u, v};
		vertices[i * 4 + 2] = {right, bottom, 0.5f, u, v};
		vertices[i * 4 + 3] = {left, bottom, 0.5f, u, v};
		const uint16_t quad[] = {0, 1, 2, 0, 2, 3};
		for (int j = 0; j < 6; ++j)
			indices[i * 6 + j] = uint16_t(i * 4 + quad[j]);
	}
	bgfx::VertexLayout vertex_layout;
	vertex_layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();
	r.vertices = bgfx::createVertexBuffer(bgfx::copy(vertices.data(), uint32_t(sizeof(vertices))), vertex_layout);
	r.indices = bgfx::createIndexBuffer(bgfx::copy(indices.data(), uint32_t(sizeof(indices))));
	Require(bgfx::isValid(r.framebuffer) && bgfx::isValid(r.seed) && bgfx::isValid(r.sampler) &&
		bgfx::isValid(r.vertices) && bgfx::isValid(r.indices), "Cannot allocate draw resources");

	bgfx::setViewName(0, "Probe producer: input to buffer");
	bgfx::setViewName(1, "Probe consumer: buffer to image");
	bgfx::setViewName(2, "Probe vertex texture fetch");
	bgfx::setViewName(3, "Probe readback blit");
	bgfx::setViewFrameBuffer(2, r.framebuffer);
	bgfx::setViewRect(2, 0, 0, target_size, target_size);
	bgfx::setViewClear(2, BGFX_CLEAR_COLOR, 0xff00ffff);
	bgfx::setViewMode(2, bgfx::ViewMode::Sequential);

	std::array<uint8_t, target_size * target_size * 4> pixels;
	const int seeds[] = {0, 1, 3, 7};
	for (const int seed : seeds) {
		const float uniform[] = {float(seed), 0.f, 0.f, 0.f};
		if (seed == 0 || negative_control != "skip-producer") {
			bgfx::setUniform(r.seed, uniform);
			bgfx::setBuffer(0, r.input, bgfx::Access::Read);
			bgfx::setBuffer(1, r.scratch, bgfx::Access::Write);
			bgfx::dispatch(0, r.producer, record_count / 64);
		}
		if (seed == 0 || negative_control != "skip-consumer") {
			bgfx::setBuffer(0, r.scratch, bgfx::Access::Read);
			bgfx::setImage(1, r.atlas, 0, bgfx::Access::Write);
			bgfx::dispatch(1, r.consumer, grid_size / 8, grid_size / 8);
		}
		bgfx::setTexture(0, r.sampler, r.atlas);
		bgfx::setVertexBuffer(0, r.vertices);
		bgfx::setIndexBuffer(r.indices);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A); // No blending, culling or depth test.
		bgfx::submit(2, r.display);
		bgfx::blit(3, r.readback, 0, 0, r.color);
		pixels.fill(0);
		const uint32_t ready_frame = bgfx::readTexture(r.readback, pixels.data());
		uint32_t frame = bgfx::frame(); // First frame boundary after the complete dependency chain.
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		while (frame < ready_frame) {
			Require(std::chrono::steady_clock::now() < deadline, "GPU readback timed out");
			hg::UpdateWindow(window);
			frame = bgfx::frame();
		}
		uint32_t mismatches = 0;
		for (uint32_t i = 0; i < record_count; ++i) {
			const auto scratch_index = (i * 17 + 3) % record_count;
			const auto source = (scratch_index * 13) % record_count;
			const uint32_t x = (i % grid_size) * 8 + 4;
			uint32_t y = (i / grid_size) * 8 + 4;
			if (caps.originBottomLeft)
				y = target_size - 1 - y;
			const auto pixel = &pixels[(y * target_size + x) * 4];
			for (int channel = 0; channel < 4; ++channel) {
				const int expected = channel == 3 ? 255 : int(input[source][channel]) + seed * (channel + 1);
				if (std::abs(int(pixel[channel]) - expected) > 1) {
					if (mismatches++ == 0)
						std::printf("Mismatch seed=%d cell=%u channel=%d expected=%d actual=%u\n", seed, i, channel, expected, pixel[channel]);
				}
			}
		}
		if (mismatches) {
			std::printf("FAIL pixel mismatch: %u channels at seed=%d\n", mismatches, seed);
			return false;
		}
		std::printf("PASS seed=%d: all 256 buffer records reached vertex fetch and rendered pixels\n", seed);
	}
	std::puts("PASS compute probe");
	return true;
}

} // namespace

int main(int argc, char **argv) {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	if (argc < 3 || argc > 4) {
		std::fprintf(stderr, "Usage: test_compute_probe metal|dx11|gl COMPILED_ASSETS [skip-producer|skip-consumer|flip-before]\n");
		return 2;
	}
	const std::string renderer = argv[1], negative = argc == 4 ? argv[3] : "";
	bgfx::RendererType::Enum type;
	if (renderer == "metal") type = bgfx::RendererType::Metal;
	else if (renderer == "dx11") type = bgfx::RendererType::Direct3D11;
	else if (renderer == "gl") type = bgfx::RendererType::OpenGL;
	else { std::fprintf(stderr, "Unknown renderer: %s\n", renderer.c_str()); return 2; }
	if (!negative.empty() && negative != "skip-producer" && negative != "skip-consumer" && negative != "flip-before") {
		std::fprintf(stderr, "Unknown probe mode\n"); return 2;
	}
	hg::WindowSystemInit();
	auto window = hg::RenderInit("HARFANG compute feasibility probe", 320, 200, type);
	if (!window) {
		std::fprintf(stderr, "Cannot initialize requested renderer %s\n", renderer.c_str());
		hg::WindowSystemShutdown();
		return 2;
	}
	int result = 2;
	try {
		Require(bgfx::getRendererType() == type, "Renderer fallback is not valid qualification evidence");
		if (negative == "flip-before") {
			int width, height;
			Require(hg::GetWindowFrameBufferSize(window, width, height), "Cannot query framebuffer size");
			bgfx::reset(uint32_t(width), uint32_t(height), BGFX_RESET_FLUSH_AFTER_RENDER);
		}
		result = RunProbe(window, argv[2], negative) ? 0 : 1;
	} catch (const std::exception &error) {
		std::fprintf(stderr, "SETUP ERROR: %s\n", error.what());
	}
	hg::RenderShutdown();
	hg::DestroyWindow(window);
	hg::WindowSystemShutdown();
	return result;
}
