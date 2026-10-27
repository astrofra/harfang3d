#pragma once

#include "engine/render_pipeline.h"
#include <memory>

namespace hg {

struct ComputeCaps {
	bool supported{false}, rgba32f_vertex_sampling{false}, rgba32f_image_write{false}, image_write_reporting_complete{true};
	uint32_t max_bindings{0}, max_views{0}, max_texture_size{0};
	std::string renderer;
};
ComputeCaps GetComputeCaps();

// Latest completed backend timing; asynchronous and not necessarily the current frame.
struct ComputeFrameStats {
    bool gpu_timing_available{false};
    int64_t gpu_timestamp{0};
    double gpu_ms{0};
    uint32_t draw_calls{0}, compute_calls{0};
};
ComputeFrameStats GetComputeFrameStats();

enum ComputeBufferType { CBT_Float4, CBT_UInt4 };
struct ComputeBufferStorage;
// Copies share ownership; explicit destruction invalidates every copy/binding.
struct ComputeBuffer { std::shared_ptr<ComputeBufferStorage> storage; };
ComputeBuffer CreateComputeBuffer(uint32_t count, ComputeBufferType type = CBT_Float4);
ComputeBuffer CreateComputeBufferFromData(const Data &data, ComputeBufferType type = CBT_Float4, uint32_t offset = 0);
ComputeBuffer LoadComputeBufferFromFile(const char *path, ComputeBufferType type = CBT_Float4, uint32_t offset = 0);
ComputeBuffer LoadComputeBufferFromAssets(const char *path, ComputeBufferType type = CBT_Float4, uint32_t offset = 0);
bool IsValid(const ComputeBuffer &buffer);
uint32_t GetComputeBufferCount(const ComputeBuffer &buffer);
void DestroyComputeBuffer(ComputeBuffer &buffer);
void ReleaseComputeResources(); // Called before renderer shutdown, including for surviving Lua userdata.

struct ComputeBufferBinding {
	ComputeBuffer buffer;
	uint32_t stage{0};
	bgfx::Access::Enum access{bgfx::Access::Read};
};
ComputeBufferBinding MakeComputeBufferBinding(const ComputeBuffer &buffer, uint32_t stage, bgfx::Access::Enum access);

struct ImageBinding {
	Texture texture;
	uint32_t stage{0};
	uint8_t mip{0};
	bgfx::Access::Enum access{bgfx::Access::Read};
};
ImageBinding MakeImageBinding(const Texture &texture, uint32_t stage, bgfx::Access::Enum access, uint8_t mip = 0);

// Returns false without submission or view advancement when validation fails.
bool DispatchCompute(bgfx::ViewId &view_id, bgfx::ProgramHandle program, uint32_t x, uint32_t y = 1, uint32_t z = 1,
	const std::vector<UniformSetValue> &values = {}, const std::vector<UniformSetTexture> &textures = {},
	const std::vector<ImageBinding> &images = {}, const std::vector<ComputeBufferBinding> &buffers = {}, const std::string &debug_name = {});

// Persistent 16-bit batches; vertex POSITION = (corner.x, corner.y, sorted slot).
Model CreateComputeQuadModel(uint32_t count);
RenderState ComputePremultipliedRenderState(); // RGB/A writes, no depth test/write or face culling.

} // namespace hg
