// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/compute.h"
#include "engine/gaussian_splat_data.h"
#include "foundation/rect.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hg {

struct GaussianSplatModelStorage;
struct GaussianSplatRendererStorage;
struct GaussianSplatRenderDataStorage;

// Copies share ownership. Explicit destruction invalidates all aliases.
struct GaussianSplatModel { std::shared_ptr<GaussianSplatModelStorage> storage; };
struct GaussianSplatRenderer { std::shared_ptr<GaussianSplatRendererStorage> storage; };
struct GaussianSplatRenderData { std::shared_ptr<GaussianSplatRenderDataStorage> storage; };

struct GaussianSplatInstance {
	GaussianSplatModel model;
	Mat4 world{Mat4::Identity};
	uint32_t id{0};
	float opacity{1.f};
};

GaussianSplatInstance MakeGaussianSplatInstance(const GaussianSplatModel &model, const Mat4 &world = Mat4::Identity, uint32_t id = 0, float opacity = 1.f);

struct GaussianSplatLoadOptions {
	uint32_t max_file_bytes{uint32_t(GaussianSplatMaxNativeBytes)};
};

struct GaussianSplatLoadResult {
	bool ok{false};
	std::string message;
	GaussianSplatModel model;
};

struct GaussianSplatRenderSettings {
	uint32_t max_sh_degree{3};
	uint32_t view_limit{256};
	uint32_t max_splats{GaussianSplatMaxCount};
	bool depth_test{true};
	bool compact_views{true};
	float max_pixel_radius{256.f};
};

struct GaussianSplatSubmitResult {
	bool ok{false};
	std::string message;
	bgfx::ViewId draw_view{0};
	uint32_t count{0}, dispatches{0};
	uint64_t gpu_bytes{0};
	float max_pixel_radius{0.f}; // Effective per-axis standard-deviation cap; an appearance/overdraw approximation.
};

GaussianSplatLoadResult LoadGaussianSplatModelFromFile(const char *path, const GaussianSplatLoadOptions &options = {});
GaussianSplatLoadResult LoadGaussianSplatModelFromAssets(const char *name, const GaussianSplatLoadOptions &options = {});
GaussianSplatLoadResult LoadGaussianSplatModelFromData(const Data &data, const GaussianSplatLoadOptions &options = {});

GaussianSplatRenderer CreateGaussianSplatRendererFromFile(const char *shader_root);
GaussianSplatRenderer CreateGaussianSplatRendererFromAssets(const char *shader_root);
GaussianSplatRenderData CreateGaussianSplatRenderData();

bool IsValid(const GaussianSplatModel &model);
bool IsValid(const GaussianSplatRenderer &renderer);
bool IsValid(const GaussianSplatRenderData &render_data);

uint32_t GetGaussianSplatCount(const GaussianSplatModel &model);
uint32_t GetGaussianSplatSourceCount(const GaussianSplatModel &model);
uint32_t GetGaussianSplatSHDegree(const GaussianSplatModel &model);
Vec3 GetGaussianSplatBoundsMin(const GaussianSplatModel &model);
Vec3 GetGaussianSplatBoundsMax(const GaussianSplatModel &model);
bool IsGaussianSplatAntialiased(const GaussianSplatModel &model);

// Owns compute/graphics views, never clears the target and never calls Frame.
// Only rigid view transforms, positive-uniform-scale instance transforms and standard/off-axis
// perspective projections are supported. Sorting/rasterization use center depth; source-domain
// color and the pixel-radius cap are explicit approximations, not arbitrary mesh/AAA equivalence.
// On validation/allocation failure next_view is unchanged and no command is submitted.
GaussianSplatSubmitResult SubmitGaussianSplats(bgfx::ViewId &next_view, const iRect &rect, const ViewState &view_state,
	const std::vector<GaussianSplatInstance> &instances, GaussianSplatRenderer &renderer, GaussianSplatRenderData &render_data,
	const GaussianSplatRenderSettings &settings = {}, bgfx::FrameBufferHandle target = BGFX_INVALID_HANDLE);

void DestroyGaussianSplatModel(GaussianSplatModel &model);
void DestroyGaussianSplatRenderer(GaussianSplatRenderer &renderer);
void DestroyGaussianSplatRenderData(GaussianSplatRenderData &render_data);
void ReleaseGaussianSplatResources(); // RenderShutdown calls this before ReleaseComputeResources.

} // namespace hg
