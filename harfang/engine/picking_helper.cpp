// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/picking_helper.h"

#include <algorithm>

namespace hg {

void PickingHelper::Create(int w, int h) {
	if (width == w && height == h && bgfx::isValid(fb))
		return;

	Destroy();
	if (w <= 0 || h <= 0) return;

	color = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8,
		BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
	color_read = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8,
		BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP |
			BGFX_SAMPLER_V_CLAMP);

	depth = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24S8,
		BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
	// Metal depth textures use private storage (D24S8 can also become D32FS8).
	// Read back the shader's RGB24 depth output, never the native depth texture.
	depth_color = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8,
		BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
	depth_read = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8,
		BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP |
			BGFX_SAMPLER_V_CLAMP);

	bgfx::TextureHandle texs[3] = {color, depth_color, depth};
	fb = bgfx::createFrameBuffer(3, texs, false);

	width = w;
	height = h;
}

void PickingHelper::Destroy() {
	if (bgfx::isValid(fb)) bgfx::destroy(fb);
	fb = BGFX_INVALID_HANDLE;
	if (bgfx::isValid(color)) bgfx::destroy(color);
	if (bgfx::isValid(color_read)) bgfx::destroy(color_read);
	color = BGFX_INVALID_HANDLE;
	color_read = BGFX_INVALID_HANDLE;

	if (bgfx::isValid(depth)) bgfx::destroy(depth);
	if (bgfx::isValid(depth_color)) bgfx::destroy(depth_color);
	if (bgfx::isValid(depth_read)) bgfx::destroy(depth_read);
	depth = BGFX_INVALID_HANDLE;
	depth_color = BGFX_INVALID_HANDLE;
	depth_read = BGFX_INVALID_HANDLE;

	width = height = 0;
}

void PickingHelper::Readback(bgfx::ViewId &view_id, PickingResults &res) const {
	res.width = width;
	res.height = height;

	res.color_data.resize(width * height * 4);
	res.depth_data.resize(width * height * 4);

	bgfx::touch(view_id);

	bgfx::blit(view_id, color_read, 0, 0, color);
	res.result_frame = bgfx::readTexture(color_read, res.color_data.data());

	++view_id;

	bgfx::blit(view_id, depth_read, 0, 0, depth_color);
	res.result_frame = std::max(res.result_frame, bgfx::readTexture(depth_read, res.depth_data.data()));
	++view_id;
}

PickingHelper::~PickingHelper() { Destroy(); }

} // namespace hg
