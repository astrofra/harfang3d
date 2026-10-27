#include "engine/compute.h"
#include "engine/assets_rw_interface.h"
#include "foundation/log.h"
#include <algorithm>

namespace hg {

struct ComputeBufferStorage {
	bgfx::DynamicVertexBufferHandle handle = BGFX_INVALID_HANDLE;
	uint32_t count{0};
	bool writable{false};
	~ComputeBufferStorage() {
		if (IsRenderUp() && bgfx::isValid(handle))
			bgfx::destroy(handle);
	}
};
static std::vector<std::weak_ptr<ComputeBufferStorage>> compute_buffers;

ComputeCaps GetComputeCaps() {
	ComputeCaps out;
	if (!IsRenderUp()) return out;
	const auto &caps = *bgfx::getCaps();
	out.supported = (caps.supported & BGFX_CAPS_COMPUTE) != 0;
	out.rgba32f_vertex_sampling = (caps.formats[bgfx::TextureFormat::RGBA32F] & BGFX_CAPS_FORMAT_TEXTURE_VERTEX) != 0;
	out.rgba32f_image_write = (caps.formats[bgfx::TextureFormat::RGBA32F] & BGFX_CAPS_FORMAT_TEXTURE_IMAGE_WRITE) != 0;
	out.image_write_reporting_complete = caps.rendererType != bgfx::RendererType::Metal;
	out.max_bindings = caps.limits.maxComputeBindings;
	out.max_views = caps.limits.maxViews;
	out.max_texture_size = caps.limits.maxTextureSize;
	out.renderer = bgfx::getRendererName(caps.rendererType);
	return out;
}

ComputeFrameStats GetComputeFrameStats() {
    ComputeFrameStats out;
    if (!IsRenderUp()) return out;
    const auto &stats = *bgfx::getStats();
    out.gpu_timing_available = stats.gpuTimerFreq > 0 && stats.gpuTimeBegin > 0 && stats.gpuTimeEnd >= stats.gpuTimeBegin;
    if (out.gpu_timing_available) {
        out.gpu_timestamp = stats.gpuTimeEnd;
        out.gpu_ms = double(stats.gpuTimeEnd - stats.gpuTimeBegin) * 1000.0 / double(stats.gpuTimerFreq);
    }
    out.draw_calls = stats.numDraw;
    out.compute_calls = stats.numCompute;
    return out;
}

static ComputeBuffer MakeBuffer(uint32_t count, ComputeBufferType type, const void *data) {
	if (!GetComputeCaps().supported || count == 0 || count > (256 * 1024 * 1024 / 16) || (type != CBT_Float4 && type != CBT_UInt4)) {
		warn("Invalid compute buffer count/type or compute unavailable (maximum 256 MiB)");
		return {};
	}
	bgfx::VertexLayout layout;
	layout.begin().add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float).end();
	const uint16_t flags = BGFX_BUFFER_COMPUTE_FORMAT_32X4 |
		(type == CBT_Float4 ? BGFX_BUFFER_COMPUTE_TYPE_FLOAT : BGFX_BUFFER_COMPUTE_TYPE_UINT) |
		(data ? BGFX_BUFFER_COMPUTE_READ : BGFX_BUFFER_COMPUTE_READ_WRITE);
	auto storage = std::make_shared<ComputeBufferStorage>();
	storage->count = count;
	storage->writable = data == nullptr;
	storage->handle = data ? bgfx::createDynamicVertexBuffer(bgfx::copy(data, count * 16), layout, flags) :
		bgfx::createDynamicVertexBuffer(count, layout, flags);
	if (!bgfx::isValid(storage->handle)) return {};
	compute_buffers.erase(std::remove_if(compute_buffers.begin(), compute_buffers.end(),
		[](const std::weak_ptr<ComputeBufferStorage> &p) { return p.expired(); }), compute_buffers.end());
	compute_buffers.push_back(storage);
	return {storage};
}

ComputeBuffer CreateComputeBuffer(uint32_t count, ComputeBufferType type) { return MakeBuffer(count, type, nullptr); }

ComputeBuffer CreateComputeBufferFromData(const Data &data, ComputeBufferType type, uint32_t offset) {
	if (offset >= data.GetSize() || (data.GetSize() - offset) % 16 || data.GetSize() - offset > 256 * 1024 * 1024) {
		warn("Compute input must contain complete 16-byte records after its offset, maximum 256 MiB");
		return {};
	}
	return MakeBuffer(uint32_t((data.GetSize() - offset) / 16), type, data.GetData() + offset);
}

ComputeBuffer LoadComputeBufferFromFile(const char *path, ComputeBufferType type, uint32_t offset) {
	Data data;
	if (!LoadDataFromFile(path, data)) return {};
	return CreateComputeBufferFromData(data, type, offset);
}

ComputeBuffer LoadComputeBufferFromAssets(const char *path, ComputeBufferType type, uint32_t offset) {
	ScopedReadHandle handle(g_assets_read_provider, path, true);
	if (!g_assets_reader.is_valid(handle)) return {};
	const auto data = LoadData(g_assets_reader, handle);
	return CreateComputeBufferFromData(data, type, offset);
}

bool IsValid(const ComputeBuffer &buffer) { return buffer.storage && bgfx::isValid(buffer.storage->handle); }
uint32_t GetComputeBufferCount(const ComputeBuffer &buffer) { return IsValid(buffer) ? buffer.storage->count : 0; }

void DestroyComputeBuffer(ComputeBuffer &buffer) {
	if (IsValid(buffer)) {
		if (IsRenderUp()) bgfx::destroy(buffer.storage->handle);
		buffer.storage->handle = BGFX_INVALID_HANDLE;
	}
}

void ReleaseComputeResources() {
	for (const auto &weak : compute_buffers) {
		ComputeBuffer buffer{weak.lock()};
		DestroyComputeBuffer(buffer);
	}
	compute_buffers.clear();
}

ComputeBufferBinding MakeComputeBufferBinding(const ComputeBuffer &buffer, uint32_t stage, bgfx::Access::Enum access) {
	return {buffer, stage, access};
}
ImageBinding MakeImageBinding(const Texture &texture, uint32_t stage, bgfx::Access::Enum access, uint8_t mip) {
	return {texture, stage, mip, access};
}

bool DispatchCompute(bgfx::ViewId &view_id, bgfx::ProgramHandle program, uint32_t x, uint32_t y, uint32_t z,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures,
	const std::vector<ImageBinding> &images, const std::vector<ComputeBufferBinding> &buffers, const std::string &debug_name) {
	const auto caps = GetComputeCaps();
	if (!caps.supported || !bgfx::isValid(program) || view_id >= caps.max_views || !x || !y || !z || x > 65535 || y > 65535 || z > 65535) {
		warn("Invalid compute dispatch program, view, group counts or compute unavailable");
		return false;
	}
	std::vector<bool> stages(caps.max_bindings, false);
	auto reserve = [&](uint32_t stage) {
		if (stage >= stages.size() || stages[stage]) return false;
		stages[stage] = true;
		return true;
	};
	for (const auto &b : buffers)
		if (!IsValid(b.buffer) || b.access < bgfx::Access::Read || b.access >= bgfx::Access::Count || (b.access != bgfx::Access::Read && !b.buffer.storage->writable) || !reserve(b.stage)) {
			warn("Invalid/destroyed/read-only compute buffer or duplicate/out-of-range stage"); return false;
		}
	for (const auto &i : images)
		if (!bgfx::isValid(i.texture.handle) || i.access < bgfx::Access::Read || i.access >= bgfx::Access::Count || !reserve(i.stage) ||
			(i.access != bgfx::Access::Read && !(i.texture.flags & BGFX_TEXTURE_COMPUTE_WRITE))) {
			warn("Invalid compute image, missing compute-write flag or duplicate/out-of-range stage"); return false;
		}
	for (const auto &t : textures)
		if (!bgfx::isValid(t.texture.handle) || !reserve(t.stage)) {
			warn("Invalid compute sampler or duplicate/out-of-range stage"); return false;
		}
	if (!debug_name.empty()) bgfx::setViewName(view_id, debug_name.c_str());
	SetUniforms(values, textures);
	for (const auto &b : buffers) bgfx::setBuffer(uint8_t(b.stage), b.buffer.storage->handle, b.access);
	for (const auto &i : images) bgfx::setImage(uint8_t(i.stage), i.texture.handle, i.mip, i.access);
	bgfx::dispatch(view_id++, program, x, y, z);
	return true;
}

Model CreateComputeQuadModel(uint32_t count) {
	Model model;
	if (!IsRenderUp() || !count || count > 1024 * 1024) return model;
	bgfx::VertexLayout layout;
	layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();
	for (uint32_t base = 0; base < count; base += 16384) {
		const uint32_t n = std::min(16384u, count - base);
		std::vector<float> vertices(n * 12);
		std::vector<uint16_t> indices(n * 6);
		const float corners[][2] = {{-1,-1}, {1,-1}, {1,1}, {-1,1}};
		const uint16_t quad[] = {0,1,2,0,2,3};
		for (uint32_t i = 0; i < n; ++i) {
			for (uint32_t j = 0; j < 4; ++j) {
				vertices[i*12+j*3] = corners[j][0];
				vertices[i*12+j*3+1] = corners[j][1];
				vertices[i*12+j*3+2] = float(base+i);
			}
			for (uint32_t j = 0; j < 6; ++j) indices[i*6+j] = uint16_t(i*4+quad[j]);
		}
		DisplayList list;
		list.vertex_buffer = bgfx::createVertexBuffer(bgfx::copy(vertices.data(), uint32_t(vertices.size()*sizeof(float))), layout);
		list.index_buffer = bgfx::createIndexBuffer(bgfx::copy(indices.data(), uint32_t(indices.size()*sizeof(uint16_t))));
		model.lists.push_back(list);
		if (!bgfx::isValid(list.vertex_buffer) || !bgfx::isValid(list.index_buffer)) { Destroy(model); return model; }
	}
	return model;
}

RenderState ComputePremultipliedRenderState() {
	RenderState state;
	state.state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	return state;
}

} // namespace hg
