// HARFANG(R) Copyright (C) 2026 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/gaussian_splat.h"

#include "engine/assets_rw_interface.h"
#include "foundation/file_rw_interface.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <set>
#include <utility>

namespace hg {

struct GaussianScratch {
	ComputeBuffer projected, keys[2];
	Texture center, basis, color;
	Model quads;
	uint32_t capacity{0}, atlas_width{0}, atlas_height{0};
	uint64_t gpu_bytes{0};
	~GaussianScratch();
};

struct GaussianSplatModelStorage {
	ComputeBuffer base, higher_sh;
	uint32_t count{0}, source_count{0}, sh_degree{0};
	Vec3 bounds_min{Vec3::Zero}, bounds_max{Vec3::Zero};
	uint64_t gpu_bytes{0};
	bool antialiased{false}, valid{false};
	~GaussianSplatModelStorage();
};

struct GaussianSplatRendererStorage {
	bgfx::ProgramHandle project{BGFX_INVALID_HANDLE}, sort{BGFX_INVALID_HANDLE}, gather{BGFX_INVALID_HANDLE}, clear{BGFX_INVALID_HANDLE},
		splat{BGFX_INVALID_HANDLE};
	ComputeBuffer dummy_sh;
	bool valid{false};
	~GaussianSplatRendererStorage();
};

struct GaussianSplatRenderDataStorage {
	std::shared_ptr<GaussianScratch> scratch;
	std::vector<GaussianSplatModel> submitted_models;
	GaussianSplatRenderer submitted_renderer;
	bool valid{false};
	~GaussianSplatRenderDataStorage();
};

namespace {

void DestroyModelStorage(GaussianSplatModelStorage &storage);
void DestroyRendererStorage(GaussianSplatRendererStorage &storage);
void ReleaseScratch(GaussianScratch &scratch);

std::vector<std::weak_ptr<GaussianSplatModelStorage>> model_registry;
std::vector<std::weak_ptr<GaussianSplatRendererStorage>> renderer_registry;
std::vector<std::weak_ptr<GaussianSplatRenderDataStorage>> render_data_registry;

template <typename T> void RegisterStorage(std::vector<std::weak_ptr<T>> &registry, const std::shared_ptr<T> &storage) {
	registry.erase(std::remove_if(registry.begin(), registry.end(), [](const std::weak_ptr<T> &p) { return p.expired(); }), registry.end());
	registry.push_back(storage);
}

GaussianSplatLoadResult LoadError(const std::string &message) {
	GaussianSplatLoadResult result;
	result.message = message;
	return result;
}

GaussianSplatSubmitResult SubmitError(bgfx::ViewId view, const std::string &message) {
	GaussianSplatSubmitResult result;
	result.message = message;
	result.draw_view = view;
	return result;
}

bool SetError(std::string *error, const std::string &message) {
	if (error)
		*error = message;
	return false;
}

bool IsFinite(const Vec3 &v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

std::string ShaderPath(const char *root, const char *name) {
	if (!root || !*root)
		return name;
	return PathJoin({root, name});
}

void ReleaseScratch(GaussianScratch &scratch) {
	DestroyComputeBuffer(scratch.projected);
	DestroyComputeBuffer(scratch.keys[0]);
	DestroyComputeBuffer(scratch.keys[1]);
	if (IsRenderUp()) {
		Destroy(scratch.center);
		Destroy(scratch.basis);
		Destroy(scratch.color);
		Destroy(scratch.quads);
	} else {
		scratch.center.handle = scratch.basis.handle = scratch.color.handle = BGFX_INVALID_HANDLE;
		for (auto &list : scratch.quads.lists) {
			list.index_buffer = BGFX_INVALID_HANDLE;
			list.vertex_buffer = BGFX_INVALID_HANDLE;
		}
		scratch.quads.lists.clear();
	}
	scratch.capacity = 0;
}

uint32_t NextPowerOfTwo(uint32_t value) {
	uint32_t result = 1;
	while (result < value)
		result <<= 1;
	return result;
}

uint32_t SortPassCount(uint32_t padded) {
	uint32_t count = 0;
	for (uint32_t k = 2; k <= padded; k <<= 1)
		for (uint32_t stride = k >> 1; stride; stride >>= 1)
			++count;
	return count;
}

uint64_t ScratchSize(uint32_t capacity, uint32_t atlas_width, uint32_t atlas_height) {
	return uint64_t(capacity) * 3 * 16 + uint64_t(capacity) * 2 * 16 + uint64_t(atlas_width) * atlas_height * 3 * 16 + uint64_t(capacity) * 60;
}

std::shared_ptr<GaussianScratch> CreateScratch(uint32_t capacity) {
	std::shared_ptr<GaussianScratch> scratch;
	try {
		scratch = std::make_shared<GaussianScratch>();
	} catch (const std::bad_alloc &) {
		return {};
	}
	scratch->capacity = capacity;
	scratch->atlas_width = 512;
	scratch->atlas_height = std::max(8u, ((capacity + scratch->atlas_width - 1) / scratch->atlas_width + 7) & ~7u);
	scratch->projected = CreateComputeBuffer(capacity * 3);
	scratch->keys[0] = CreateComputeBuffer(capacity);
	scratch->keys[1] = CreateComputeBuffer(capacity);
	const uint64_t flags = BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
	scratch->center = CreateTexture(scratch->atlas_width, scratch->atlas_height, "gsplat-center", flags, bgfx::TextureFormat::RGBA32F);
	scratch->basis = CreateTexture(scratch->atlas_width, scratch->atlas_height, "gsplat-basis", flags, bgfx::TextureFormat::RGBA32F);
	scratch->color = CreateTexture(scratch->atlas_width, scratch->atlas_height, "gsplat-color", flags, bgfx::TextureFormat::RGBA32F);
	scratch->quads = CreateComputeQuadModel(capacity);
	bool valid_quads = !scratch->quads.lists.empty();
	for (const auto &list : scratch->quads.lists)
		valid_quads = valid_quads && bgfx::isValid(list.vertex_buffer) && bgfx::isValid(list.index_buffer);
	if (!IsValid(scratch->projected) || !IsValid(scratch->keys[0]) || !IsValid(scratch->keys[1]) || !bgfx::isValid(scratch->center.handle) ||
		!bgfx::isValid(scratch->basis.handle) || !bgfx::isValid(scratch->color.handle) || !valid_quads)
		return {};
	scratch->gpu_bytes = ScratchSize(capacity, scratch->atlas_width, scratch->atlas_height);
	return scratch;
}

bool ScratchUsable(const std::shared_ptr<GaussianScratch> &scratch, uint32_t capacity) {
	if (!scratch || scratch->capacity < capacity || !IsValid(scratch->projected) || !IsValid(scratch->keys[0]) || !IsValid(scratch->keys[1]) ||
		!bgfx::isValid(scratch->center.handle) || !bgfx::isValid(scratch->basis.handle) || !bgfx::isValid(scratch->color.handle) || scratch->quads.lists.empty())
		return false;
	for (const auto &list : scratch->quads.lists)
		if (!bgfx::isValid(list.vertex_buffer) || !bgfx::isValid(list.index_buffer))
			return false;
	return true;
}

struct TransformInfo {
	Vec4 sh_rows[3]{Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0)};
	float scale{1.f};
};

float Dot3(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 Cross3(const Vec3 &a, const Vec3 &b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

bool ValidateTransform(const Mat4 &world, TransformInfo &info, bool unit_scale, std::string &error) {
	for (const auto &row : world.m)
		for (float value : row)
			if (!std::isfinite(value))
				return SetError(&error, "Transform contains a non-finite value"), false;
	const Vec3 columns[] = {GetX(world), GetY(world), GetZ(world)};
	const double lengths[] = {Dot3(columns[0], columns[0]), Dot3(columns[1], columns[1]), Dot3(columns[2], columns[2])};
	if (lengths[0] <= 0.0 || lengths[1] <= 0.0 || lengths[2] <= 0.0 || !std::isfinite(lengths[0]) || !std::isfinite(lengths[1]) ||
		!std::isfinite(lengths[2]))
		return SetError(&error, "Transform has a singular axis"), false;
	const double scale2 = (lengths[0] + lengths[1] + lengths[2]) / 3.0;
	const double tolerance = scale2 * 0.0002;
	if (std::abs(lengths[0] - scale2) > tolerance || std::abs(lengths[1] - scale2) > tolerance || std::abs(lengths[2] - scale2) > tolerance ||
		std::abs(Dot3(columns[0], columns[1])) > tolerance || std::abs(Dot3(columns[0], columns[2])) > tolerance ||
		std::abs(Dot3(columns[1], columns[2])) > tolerance)
		return SetError(&error, "Only orthogonal positive-uniform-scale transforms are supported"), false;
	if (Dot3(columns[0], Cross3(columns[1], columns[2])) <= 0.0)
		return SetError(&error, "Mirrored transforms are not supported"), false;
	info.scale = float(std::sqrt(scale2));
	if (!std::isfinite(info.scale) || info.scale <= 0.f || (unit_scale && std::abs(info.scale - 1.f) > 0.001f))
		return SetError(&error, unit_scale ? "View matrix must be rigid" : "Transform scale is invalid"), false;
	for (uint32_t axis = 0; axis < 3; ++axis)
		info.sh_rows[axis] = Vec4(columns[axis] / info.scale, 0.f);
	return true;
}

bool ValidateProjection(const Mat44 &proj, bool homogeneous_depth, std::string &error) {
	for (const auto &row : proj.m)
		for (float value : row)
			if (!std::isfinite(value))
				return SetError(&error, "Projection contains a non-finite value"), false;
	const float zeros[] = {proj.m[0][1], proj.m[1][0], proj.m[2][0], proj.m[2][1], proj.m[3][0], proj.m[3][1]};
	for (float value : zeros)
		if (std::abs(value) > 0.00001f)
			return SetError(&error, "Only standard/off-axis perspective projections are supported"), false;
	if (std::abs(proj.m[3][2] - 1.f) > 0.00001f || std::abs(proj.m[3][3]) > 0.00001f || std::abs(proj.m[0][0]) < 0.000001f ||
		std::abs(proj.m[1][1]) < 0.000001f || proj.m[2][2] <= 0.f || proj.m[2][3] >= 0.f)
		return SetError(&error, "Unsupported or reversed perspective projection"), false;
	const float a = proj.m[2][2], b = proj.m[2][3];
	const float near = homogeneous_depth ? -b / (a + 1.f) : -b / a;
	const float far = -b / (a - 1.f);
	if (!std::isfinite(near) || !std::isfinite(far) || near <= 0.f || far <= near)
		return SetError(&error, "Perspective near/far range is invalid"), false;
	return true;
}

struct InstanceWork {
	const GaussianSplatInstance *instance{nullptr};
	uint32_t offset{0};
	TransformInfo transform;
};

using Values = std::vector<UniformSetValue>;
using Buffers = std::vector<ComputeBufferBinding>;

void AddCameraValues(Values &values, const ViewState &view_state, const Vec3 &eye, int width, int height, bool homogeneous_depth) {
	values.push_back(MakeUniformSetValue("u_eye", Vec4(eye, 0.f)));
	for (uint32_t row = 0; row < 3; ++row)
		values.push_back(MakeUniformSetValue(("u_view" + std::to_string(row)).c_str(), GetRow(view_state.view, row)));
	for (uint32_t row = 0; row < 4; ++row)
		values.push_back(MakeUniformSetValue(("u_proj" + std::to_string(row)).c_str(), GetRow(view_state.proj, row)));
	values.push_back(MakeUniformSetValue("u_screen", Vec4(float(width), float(height), 0.f, 0.f)));
	values.push_back(MakeUniformSetValue("u_depth_range", Vec4(homogeneous_depth ? -1.f : 0.f, 1.f, 0.f, 0.f)));
}

GaussianSplatLoadResult UploadModel(GaussianSplatDataResult &&parsed) {
	if (!parsed.ok)
		return LoadError(parsed.message);
	if (!IsRenderUp() || !GetComputeCaps().supported)
		return LoadError("Gaussian model upload requires an initialized compute-capable renderer");
	std::shared_ptr<GaussianSplatModelStorage> storage;
	try {
		storage = std::make_shared<GaussianSplatModelStorage>();
	} catch (const std::bad_alloc &) {
		return LoadError("Cannot allocate Gaussian model storage");
	}
	auto &source = parsed.data;
	storage->count = source.count;
	storage->source_count = source.source_count;
	storage->sh_degree = source.sh_degree;
	storage->bounds_min = source.bounds_min;
	storage->bounds_max = source.bounds_max;
	storage->antialiased = source.antialiased;
	storage->gpu_bytes = uint64_t(source.count) * 64 + uint64_t(source.higher_sh.size()) * 4;
	{
		Data bytes(source.base.data(), source.base.size() * sizeof(float));
		storage->base = CreateComputeBufferFromData(bytes, CBT_Float4);
	}
	if (!IsValid(storage->base) || GetComputeBufferCount(storage->base) != source.count * 4)
		return LoadError("Cannot allocate Gaussian base GPU buffer");
	if (source.sh_degree) {
		Data bytes(source.higher_sh.data(), source.higher_sh.size() * sizeof(float));
		storage->higher_sh = CreateComputeBufferFromData(bytes, CBT_Float4);
		if (!IsValid(storage->higher_sh) || GetComputeBufferCount(storage->higher_sh) != source.higher_sh.size() / 4)
			return LoadError("Cannot allocate Gaussian SH GPU buffer");
	}
	storage->valid = true;
	RegisterStorage(model_registry, storage);
	GaussianSplatLoadResult result;
	result.ok = true;
	result.message = "OK";
	result.model.storage = std::move(storage);
	return result;
}

GaussianSplatLoadResult LoadModel(const Reader &reader, const ReadProvider &provider, const char *path, const GaussianSplatLoadOptions &options) {
	if (!path || !*path || !options.max_file_bytes)
		return LoadError("Empty Gaussian path or zero byte budget");
	ScopedReadHandle handle(provider, path, true);
	if (!reader.is_valid(handle))
		return LoadError(std::string("Cannot open Gaussian asset '") + path + "'");
	const size_t size = reader.size(handle);
	if (size > options.max_file_bytes || size > GaussianSplatMaxNativeBytes)
		return LoadError("Gaussian asset exceeds the configured byte budget");
	const Data data = LoadData(reader, handle);
	if (data.GetSize() != size)
		return LoadError("Gaussian asset read was truncated");
	return UploadModel(ReadGaussianSplatData(data));
}

GaussianSplatRenderer CreateRenderer(const Reader &reader, const ReadProvider &provider, const char *shader_root) {
	if (!IsRenderUp() || !shader_root)
		return {};
	std::shared_ptr<GaussianSplatRendererStorage> storage;
	try {
		storage = std::make_shared<GaussianSplatRendererStorage>();
	} catch (const std::bad_alloc &) {
		return {};
	}
	const auto project = ShaderPath(shader_root, "project.sc"), sort = ShaderPath(shader_root, "sort.sc");
	const auto gather = ShaderPath(shader_root, "gather.sc"), clear = ShaderPath(shader_root, "clear.sc"), splat = ShaderPath(shader_root, "splat");
	storage->project = LoadComputeProgram(reader, provider, project.c_str());
	storage->sort = LoadComputeProgram(reader, provider, sort.c_str());
	storage->gather = LoadComputeProgram(reader, provider, gather.c_str());
	storage->clear = LoadComputeProgram(reader, provider, clear.c_str());
	storage->splat = LoadProgram(reader, provider, splat.c_str());
	const float zeros[] = {0.f, 0.f, 0.f, 0.f};
	const Data dummy_data(zeros, sizeof(zeros));
	storage->dummy_sh = CreateComputeBufferFromData(dummy_data, CBT_Float4);
	if (!bgfx::isValid(storage->project) || !bgfx::isValid(storage->sort) || !bgfx::isValid(storage->gather) || !bgfx::isValid(storage->clear) ||
		!bgfx::isValid(storage->splat) || !IsValid(storage->dummy_sh))
		return {};
	storage->valid = true;
	RegisterStorage(renderer_registry, storage);
	GaussianSplatRenderer renderer;
	renderer.storage = std::move(storage);
	return renderer;
}

} // namespace

GaussianSplatModelStorage::~GaussianSplatModelStorage() { DestroyModelStorage(*this); }
GaussianSplatRendererStorage::~GaussianSplatRendererStorage() { DestroyRendererStorage(*this); }
GaussianSplatRenderDataStorage::~GaussianSplatRenderDataStorage() {
	valid = false;
	scratch.reset();
	submitted_models.clear();
	submitted_renderer = {};
}

namespace {

void DestroyModelStorage(GaussianSplatModelStorage &storage) {
	storage.valid = false;
	DestroyComputeBuffer(storage.base);
	DestroyComputeBuffer(storage.higher_sh);
}

void DestroyRendererStorage(GaussianSplatRendererStorage &storage) {
	storage.valid = false;
	if (IsRenderUp()) {
		if (bgfx::isValid(storage.project))
			bgfx::destroy(storage.project);
		if (bgfx::isValid(storage.sort))
			bgfx::destroy(storage.sort);
		if (bgfx::isValid(storage.gather))
			bgfx::destroy(storage.gather);
		if (bgfx::isValid(storage.clear))
			bgfx::destroy(storage.clear);
		if (bgfx::isValid(storage.splat))
			bgfx::destroy(storage.splat);
	}
	storage.project = storage.sort = storage.gather = storage.clear = storage.splat = BGFX_INVALID_HANDLE;
	DestroyComputeBuffer(storage.dummy_sh);
}

} // namespace

GaussianScratch::~GaussianScratch() { ReleaseScratch(*this); }

GaussianSplatInstance MakeGaussianSplatInstance(const GaussianSplatModel &model, const Mat4 &world, uint32_t id, float opacity) {
	return {model, world, id, opacity};
}

GaussianSplatLoadResult LoadGaussianSplatModelFromFile(const char *path, const GaussianSplatLoadOptions &options) {
	return LoadModel(g_file_reader, g_file_read_provider, path, options);
}
GaussianSplatLoadResult LoadGaussianSplatModelFromAssets(const char *name, const GaussianSplatLoadOptions &options) {
	return LoadModel(g_assets_reader, g_assets_read_provider, name, options);
}
GaussianSplatLoadResult LoadGaussianSplatModelFromData(const Data &data, const GaussianSplatLoadOptions &options) {
	if (!options.max_file_bytes || data.GetSize() > options.max_file_bytes)
		return LoadError("Gaussian data exceeds the configured byte budget");
	return UploadModel(ReadGaussianSplatData(data));
}

GaussianSplatRenderer CreateGaussianSplatRendererFromFile(const char *shader_root) {
	return CreateRenderer(g_file_reader, g_file_read_provider, shader_root);
}
GaussianSplatRenderer CreateGaussianSplatRendererFromAssets(const char *shader_root) {
	return CreateRenderer(g_assets_reader, g_assets_read_provider, shader_root);
}
GaussianSplatRenderData CreateGaussianSplatRenderData() {
	GaussianSplatRenderData data;
	try {
		data.storage = std::make_shared<GaussianSplatRenderDataStorage>();
		data.storage->valid = true;
		RegisterStorage(render_data_registry, data.storage);
	} catch (const std::bad_alloc &) {
		data.storage.reset();
	}
	return data;
}

bool IsValid(const GaussianSplatModel &model) {
	return model.storage && model.storage->valid && IsValid(model.storage->base) && (!model.storage->sh_degree || IsValid(model.storage->higher_sh));
}
bool IsValid(const GaussianSplatRenderer &renderer) {
	return renderer.storage && renderer.storage->valid && bgfx::isValid(renderer.storage->project) && bgfx::isValid(renderer.storage->sort) &&
		bgfx::isValid(renderer.storage->gather) && bgfx::isValid(renderer.storage->clear) && bgfx::isValid(renderer.storage->splat) &&
		IsValid(renderer.storage->dummy_sh);
}
bool IsValid(const GaussianSplatRenderData &render_data) { return render_data.storage && render_data.storage->valid; }

uint32_t GetGaussianSplatCount(const GaussianSplatModel &model) { return IsValid(model) ? model.storage->count : 0; }
uint32_t GetGaussianSplatSourceCount(const GaussianSplatModel &model) { return IsValid(model) ? model.storage->source_count : 0; }
uint32_t GetGaussianSplatSHDegree(const GaussianSplatModel &model) { return IsValid(model) ? model.storage->sh_degree : 0; }
Vec3 GetGaussianSplatBoundsMin(const GaussianSplatModel &model) { return IsValid(model) ? model.storage->bounds_min : Vec3::Zero; }
Vec3 GetGaussianSplatBoundsMax(const GaussianSplatModel &model) { return IsValid(model) ? model.storage->bounds_max : Vec3::Zero; }
bool IsGaussianSplatAntialiased(const GaussianSplatModel &model) { return IsValid(model) && model.storage->antialiased; }

GaussianSplatSubmitResult SubmitGaussianSplats(bgfx::ViewId &next_view, const iRect &rect, const ViewState &view_state,
	const std::vector<GaussianSplatInstance> &instances, GaussianSplatRenderer &renderer, GaussianSplatRenderData &render_data,
	const GaussianSplatRenderSettings &settings, bgfx::FrameBufferHandle target) {
	const bgfx::ViewId initial_view = next_view;
	if (instances.empty()) {
		GaussianSplatSubmitResult result;
		result.ok = true;
		result.message = "OK (empty)";
		result.draw_view = initial_view;
		return result;
	}
	if (!IsRenderUp() || !IsValid(renderer) || !IsValid(render_data))
		return SubmitError(initial_view, "Invalid Gaussian renderer/render data or renderer is shut down");
	if (settings.max_sh_degree > 3 || !settings.max_splats || settings.max_splats > GaussianSplatMaxCount || !settings.view_limit ||
		!std::isfinite(settings.max_pixel_radius) || settings.max_pixel_radius <= 0.f)
		return SubmitError(initial_view, "Invalid Gaussian render settings");
	const int64_t width64 = int64_t(rect.ex) - int64_t(rect.sx), height64 = int64_t(rect.ey) - int64_t(rect.sy);
	if (width64 <= 0 || height64 <= 0 || width64 > 65535 || height64 > 65535 || rect.sx < -32768 || rect.sx > 32767 || rect.sy < -32768 || rect.sy > 32767)
		return SubmitError(initial_view, "Gaussian viewport is empty or outside BGFX limits");
	const int width = int(width64), height = int(height64);
	const auto caps = GetComputeCaps();
	if (!caps.supported || !caps.rgba32f_vertex_sampling || caps.max_bindings < 5)
		return SubmitError(initial_view, "Compute, RGBA32F vertex sampling or five compute bindings are unavailable");
	if (!caps.rgba32f_image_write && caps.image_write_reporting_complete)
		return SubmitError(initial_view, "RGBA32F image writes are unavailable");
	std::string validation_error;
	TransformInfo view_transform;
	if (!ValidateTransform(view_state.view, view_transform, true, validation_error))
		return SubmitError(initial_view, "Invalid view matrix: " + validation_error);
	if (!ValidateProjection(view_state.proj, bgfx::getCaps()->homogeneousDepth, validation_error))
		return SubmitError(initial_view, validation_error);
	const Vec3 eye = GetT(InverseFast(view_state.view));
	if (!IsFinite(eye))
		return SubmitError(initial_view, "Camera position is non-finite");

	std::vector<InstanceWork> work;
	try {
		work.reserve(instances.size());
	} catch (const std::bad_alloc &) {
		return SubmitError(initial_view, "Cannot allocate Gaussian instance descriptors");
	}
	uint64_t total = 0;
	for (const auto &instance : instances) {
		if (!IsValid(instance.model))
			return SubmitError(initial_view, "Gaussian instance has an invalid/destroyed model");
		if (!std::isfinite(instance.opacity) || instance.opacity < 0.f || instance.opacity > 1.f)
			return SubmitError(initial_view, "Gaussian instance opacity must be finite in [0,1]");
		InstanceWork item;
		item.instance = &instance;
		if (!ValidateTransform(instance.world, item.transform, false, validation_error))
			return SubmitError(initial_view, "Invalid Gaussian instance " + std::to_string(instance.id) + ": " + validation_error);
		total += instance.model.storage->count;
		if (total > settings.max_splats || total > GaussianSplatMaxCount)
			return SubmitError(initial_view, "Gaussian submission exceeds max_splats/1048576");
		work.push_back(item);
	}
	std::sort(work.begin(), work.end(), [](const InstanceWork &a, const InstanceWork &b) { return a.instance->id < b.instance->id; });
	for (size_t i = 1; i < work.size(); ++i)
		if (work[i - 1].instance->id == work[i].instance->id)
			return SubmitError(initial_view, "Duplicate Gaussian instance id " + std::to_string(work[i].instance->id));
	uint32_t offset = 0;
	for (auto &item : work) {
		item.offset = offset;
		offset += item.instance->model.storage->count;
	}
	const uint32_t count = uint32_t(total), padded = NextPowerOfTwo(count), sort_passes = SortPassCount(padded);
	uint64_t unique_gpu_bytes = 0;
	try {
		std::set<const GaussianSplatModelStorage *> unique_models;
		for (const auto &item : work)
			if (unique_models.insert(item.instance->model.storage.get()).second)
				unique_gpu_bytes += item.instance->model.storage->gpu_bytes;
	} catch (const std::bad_alloc &) {
		return SubmitError(initial_view, "Cannot account Gaussian model GPU resources");
	}
	const uint64_t dispatch_count = uint64_t(1) + work.size() + sort_passes + 1;
	const uint64_t view_count = settings.compact_views ? 2 : dispatch_count + 1;
	const uint64_t view_limit = std::min<uint64_t>(settings.view_limit, caps.max_views);
	if (dispatch_count > std::numeric_limits<uint32_t>::max() || uint64_t(initial_view) + view_count > view_limit)
		return SubmitError(initial_view, "Gaussian submission exceeds the exclusive view limit");
	const uint32_t atlas_width = 512, atlas_height = std::max(8u, ((padded + atlas_width - 1) / atlas_width + 7) & ~7u);
	if (caps.max_texture_size < atlas_width || caps.max_texture_size < atlas_height)
		return SubmitError(initial_view, "Gaussian attribute atlas exceeds device texture limits");
	if (!ScratchUsable(render_data.storage->scratch, padded)) {
		auto scratch = CreateScratch(padded);
		if (!scratch)
			return SubmitError(initial_view, "Cannot allocate Gaussian persistent scratch resources");
		render_data.storage->scratch = std::move(scratch);
	}
	auto scratch = render_data.storage->scratch;
	try {
		render_data.storage->submitted_models.clear();
		render_data.storage->submitted_models.reserve(work.size());
		for (const auto &item : work)
			render_data.storage->submitted_models.push_back(item.instance->model);
	} catch (const std::bad_alloc &) {
		return SubmitError(initial_view, "Cannot retain Gaussian models for submission");
	}
	render_data.storage->submitted_renderer = renderer;

	bgfx::ViewId work_view = initial_view;
	const bgfx::ViewId compact_view = initial_view;
	uint32_t submitted_dispatches = 0;
	auto dispatch = [&](bgfx::ProgramHandle program, uint32_t x, uint32_t y, const Values &values, const std::vector<ImageBinding> &images,
						const Buffers &buffers, const char *name) {
		const bool ok = settings.compact_views ? DispatchComputeSameView(compact_view, program, x, y, 1, values, {}, images, buffers, name) :
			DispatchCompute(work_view, program, x, y, 1, values, {}, images, buffers, name);
		if (ok)
			++submitted_dispatches;
		return ok;
	};
	auto dispatch_failure = [&](const char *name) {
		next_view = settings.compact_views ? bgfx::ViewId(initial_view + 1) : work_view;
		return SubmitError(next_view, std::string("Gaussian compute dispatch failed: ") + name);
	};

	Values clear_values{MakeUniformSetValue("u_counts", Vec4(float(padded), 0.f, 0.f, 0.f))};
	Buffers clear_buffers{MakeComputeBufferBinding(scratch->keys[0], 0, bgfx::Access::Write)};
	if (!dispatch(renderer.storage->clear, (padded + 63) / 64, 1, clear_values, {}, clear_buffers, "gsplat-clear"))
		return dispatch_failure("clear");

	for (const auto &item : work) {
		const auto &model = *item.instance->model.storage;
		const uint32_t degree = std::min(model.sh_degree, settings.max_sh_degree), non_dc = (degree + 1) * (degree + 1) - 1;
		const uint32_t stored_non_dc = (model.sh_degree + 1) * (model.sh_degree + 1) - 1;
		Values values;
		values.reserve(24);
		values.push_back(MakeUniformSetValue("u_counts", Vec4(float(model.count), float(item.offset), float(padded), float(stored_non_dc))));
		AddCameraValues(values, view_state, eye, width, height, bgfx::getCaps()->homogeneousDepth);
		for (uint32_t row = 0; row < 3; ++row)
			values.push_back(MakeUniformSetValue(("u_world" + std::to_string(row)).c_str(), GetRow(item.instance->world, row)));
		for (uint32_t row = 0; row < 3; ++row)
			values.push_back(MakeUniformSetValue(("u_sh" + std::to_string(row)).c_str(), item.transform.sh_rows[row]));
		values.push_back(MakeUniformSetValue("u_params", Vec4(float(degree), float(non_dc), model.antialiased ? 1.f : 0.f, settings.max_pixel_radius)));
		values.push_back(MakeUniformSetValue("u_instance", Vec4(item.instance->opacity, 0.f, 0.f, 0.f)));
		const ComputeBuffer &sh = degree ? model.higher_sh : renderer.storage->dummy_sh;
		Buffers buffers{MakeComputeBufferBinding(model.base, 0, bgfx::Access::Read), MakeComputeBufferBinding(scratch->projected, 1, bgfx::Access::Write),
			MakeComputeBufferBinding(scratch->keys[0], 2, bgfx::Access::Write), MakeComputeBufferBinding(sh, 3, bgfx::Access::Read)};
		if (!dispatch(renderer.storage->project, (model.count + 63) / 64, 1, values, {}, buffers, "gsplat-project"))
			return dispatch_failure("project");
	}

	uint32_t current = 0;
	for (uint32_t k = 2; k <= padded; k <<= 1) {
		for (uint32_t stride = k >> 1; stride; stride >>= 1) {
			Values values{MakeUniformSetValue("u_sort", Vec4(float(stride), float(k), float(padded), 0.f))};
			Buffers buffers{MakeComputeBufferBinding(scratch->keys[current], 0, bgfx::Access::Read),
				MakeComputeBufferBinding(scratch->keys[current ^ 1], 1, bgfx::Access::Write)};
			if (!dispatch(renderer.storage->sort, (padded + 63) / 64, 1, values, {}, buffers, "gsplat-sort"))
				return dispatch_failure("sort");
			current ^= 1;
		}
	}

	Values gather_values{MakeUniformSetValue("u_counts", Vec4(float(count), float(padded), float(scratch->atlas_width), float(scratch->atlas_height)))};
	Buffers gather_buffers{MakeComputeBufferBinding(scratch->projected, 0, bgfx::Access::Read), MakeComputeBufferBinding(scratch->keys[current], 1, bgfx::Access::Read)};
	std::vector<ImageBinding> gather_images{MakeImageBinding(scratch->center, 2, bgfx::Access::Write), MakeImageBinding(scratch->basis, 3, bgfx::Access::Write),
		MakeImageBinding(scratch->color, 4, bgfx::Access::Write)};
	if (!dispatch(renderer.storage->gather, scratch->atlas_width / 8, scratch->atlas_height / 8, gather_values, gather_images, gather_buffers, "gsplat-gather"))
		return dispatch_failure("gather");
	if (!settings.compact_views)
		next_view = work_view;
	else
		next_view = bgfx::ViewId(initial_view + 1);

	const bgfx::ViewId draw_view = next_view++;
	bgfx::setViewRect(draw_view, int16_t(rect.sx), int16_t(rect.sy), uint16_t(width), uint16_t(height));
	bgfx::setViewTransform(draw_view, to_bgfx(view_state.view).data(), to_bgfx(view_state.proj).data());
	bgfx::setViewFrameBuffer(draw_view, target);
	bgfx::setViewMode(draw_view, bgfx::ViewMode::Sequential);
	bgfx::setViewClear(draw_view, BGFX_CLEAR_NONE);
	Values draw_values{MakeUniformSetValue("u_counts", Vec4(float(count), float(padded), float(scratch->atlas_width), float(scratch->atlas_height)))};
	std::vector<UniformSetTexture> draw_textures{MakeUniformSetTexture("s_center", scratch->center, 0), MakeUniformSetTexture("s_basis", scratch->basis, 1),
		MakeUniformSetTexture("s_color", scratch->color, 2)};
	RenderState state = ComputePremultipliedRenderState();
	if (settings.depth_test)
		state.state |= BGFX_STATE_DEPTH_TEST_LESS;
	// The quad buffers are retained at capacity, but only active sorted slots are drawn.
	const auto identity = to_bgfx(Mat4::Identity);
	for (size_t list_index = 0; list_index < scratch->quads.lists.size(); ++list_index) {
		const uint64_t base = uint64_t(list_index) * 16384;
		if (base >= count)
			break;
		const uint32_t active = uint32_t(std::min<uint64_t>(16384, uint64_t(count) - base));
		const auto &list = scratch->quads.lists[list_index];
		bgfx::setTransform(identity.data());
		SetUniforms(draw_values, draw_textures);
		bgfx::setIndexBuffer(list.index_buffer, 0, active * 6);
		bgfx::setVertexBuffer(0, list.vertex_buffer, 0, active * 4);
		bgfx::setState(state.state, state.rgba);
		bgfx::submit(draw_view, renderer.storage->splat, 0);
	}

	const uint64_t gpu_bytes = scratch->gpu_bytes + unique_gpu_bytes;
	GaussianSplatSubmitResult result;
	result.ok = true;
	result.message = "OK";
	result.draw_view = draw_view;
	result.count = count;
	result.dispatches = submitted_dispatches;
	result.gpu_bytes = gpu_bytes;
	result.max_pixel_radius = settings.max_pixel_radius;
	return result;
}

void DestroyGaussianSplatModel(GaussianSplatModel &model) {
	if (model.storage)
		DestroyModelStorage(*model.storage);
}
void DestroyGaussianSplatRenderer(GaussianSplatRenderer &renderer) {
	if (renderer.storage)
		DestroyRendererStorage(*renderer.storage);
}
void DestroyGaussianSplatRenderData(GaussianSplatRenderData &render_data) {
	if (render_data.storage) {
		render_data.storage->valid = false;
		render_data.storage->scratch.reset();
		render_data.storage->submitted_models.clear();
		render_data.storage->submitted_renderer = {};
	}
}

void ReleaseGaussianSplatResources() {
	for (const auto &weak : model_registry)
		if (auto storage = weak.lock())
			DestroyModelStorage(*storage);
	for (const auto &weak : renderer_registry)
		if (auto storage = weak.lock())
			DestroyRendererStorage(*storage);
	for (const auto &weak : render_data_registry)
		if (auto storage = weak.lock()) {
			storage->valid = false;
			storage->scratch.reset();
			storage->submitted_models.clear();
			storage->submitted_renderer = {};
		}
	model_registry.clear();
	renderer_registry.clear();
	render_data_registry.clear();
}

} // namespace hg
