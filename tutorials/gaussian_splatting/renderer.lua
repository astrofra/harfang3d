-- Shared Gaussian projection, GPU ordering and drawing for the tutorial and its tests.
return function(hg)
	local module_dir = debug.getinfo(1, 'S').source:sub(2):match('^(.*[/\\])') or './'
	local draw_gizmo = dofile(module_dir .. 'gizmo.lua')(hg)
	local Renderer = {}

	function Renderer.new(assets, preset_name)
		local metadata = assert(loadfile(assets .. '/metadata.luadata'))()
		assert(metadata.version == 1, 'Unsupported metadata version')

		local preset = assert(metadata.presets[preset_name], 'Unknown preset')
		local file = assert(io.open(assets .. '/' .. preset.file, 'rb'))
		local header = assert(file:read(64), 'Missing HGS0 header')
		file:close()

		local magic, version, stored_count, stride = string.unpack('<c4I4I4I4', header)
		assert(magic == 'HGS0' and version == 1 and stored_count == preset.count and stride == 64 and preset.offset == 64,
			'Invalid HGS0 dataset header')

		local count, padded = preset.count, 1
		while padded < count do
			padded = padded * 2
		end
		assert(count <= 1024 * 1024, 'Tutorial limit is one million Gaussians')

		local atlas_w = 512
		local atlas_h = math.max(8, math.ceil(padded / atlas_w / 8) * 8)
		local resources = hg.PipelineResources()
		local buffers, texture_objects, programs = {}, {}, {}

		local function buffer(b)
			assert(hg.IsValid(b), 'Compute allocation failed')
			buffers[#buffers + 1] = b
			return b
		end

		local function texture(w, h, name, flags, format)
			local t = hg.CreateTexture(w, h, name, flags, format)
			texture_objects[#texture_objects + 1] = t
			return t
		end

		local function value(name, x, y, z, w)
			return hg.MakeUniformSetValue(name, hg.Vec4(x, y, z, w))
		end

		local function binding(b, stage, access)
			return hg.MakeComputeBufferBinding(b, stage, access)
		end

		local caps = hg.GetComputeCaps()
		assert(caps.supported and caps.rgba32f_vertex_sampling, 'Compute / RGBA32F vertex sampling unavailable')
		assert(caps.max_bindings >= 5 and caps.max_views >= 160 and caps.max_texture_size >= atlas_w, 'Insufficient device limits')
		assert(caps.rgba32f_image_write or (caps.renderer == 'Metal' and not caps.image_write_reporting_complete),
			'RGBA32F image writes unavailable')

		-- load shaders and upload the dataset
		for _, name in ipairs({'project', 'sort', 'gather'}) do
			programs[name] = hg.LoadComputeProgramFromFile(assets .. '/shaders/' .. name .. '.sc')
		end
		programs.splat = hg.LoadProgramFromFile(assets .. '/shaders/splat')
		programs.gizmo = hg.LoadProgramFromFile(assets .. '/shaders/gizmo')

		local source = buffer(hg.LoadComputeBufferFromFile(assets .. '/' .. preset.file, hg.CBT_Float4, preset.offset))
		assert(hg.GetComputeBufferCount(source) == count * 4, 'Dataset size disagrees with metadata')

		local projected = buffer(hg.CreateComputeBuffer(count * 3))
		local keys = {buffer(hg.CreateComputeBuffer(padded)), buffer(hg.CreateComputeBuffer(padded))}

		-- attribute atlases and compute bindings
		local flags = hg.TF_ComputeWrite | hg.TF_SamplerMinPoint | hg.TF_SamplerMagPoint | hg.TF_UClamp | hg.TF_VClamp
		local center = texture(atlas_w, atlas_h, 'splat-center', flags, hg.TF_RGBA32F)
		local basis = texture(atlas_w, atlas_h, 'splat-basis', flags, hg.TF_RGBA32F)
		local color = texture(atlas_w, atlas_h, 'splat-color', flags, hg.TF_RGBA32F)

		local counts = value('u_counts', count, padded, atlas_w, atlas_h)
		local draw_values = {counts}
		local draw_textures = {
			hg.MakeUniformSetTexture('s_center', center, 0),
			hg.MakeUniformSetTexture('s_basis', basis, 1),
			hg.MakeUniformSetTexture('s_color', color, 2)
		}
		local images = {
			hg.MakeImageBinding(center, 2, hg.AC_Write),
			hg.MakeImageBinding(basis, 3, hg.AC_Write),
			hg.MakeImageBinding(color, 4, hg.AC_Write)
		}
		local project_buffers = {
			binding(source, 0, hg.AC_Read),
			binding(projected, 1, hg.AC_Write),
			binding(keys[1], 2, hg.AC_Write)
		}

		-- prebuild all sorting uniforms and bindings: no per-Gaussian Lua work
		local passes = {}
		local current = 1
		local k = 2

		while k <= padded do
			local stride = k // 2
			while stride >= 1 do
				local other = 3 - current
				passes[#passes + 1] = {
					values = {value('u_sort', stride, k, padded, 0)},
					buffers = {binding(keys[current], 0, hg.AC_Read), binding(keys[other], 1, hg.AC_Write)}
				}
				current = other
				stride = stride // 2
			end
			k = k * 2
		end
		assert(#passes + 4 < caps.max_views, 'Sort exceeds view budget')

		local gather_buffers = {binding(projected, 0, hg.AC_Read), binding(keys[current], 1, hg.AC_Read)}

		-- persistent quad model
		local model = hg.CreateComputeQuadModel(count)
		resources:AddModel('splat-quads', model)
		local state = hg.ComputePremultipliedRenderState()

		local function dispatch(view, program, values, imgs, bufs, x, y)
			local ok, next_view = hg.DispatchCompute(view, program, x or math.ceil(padded / 64), y or 1, 1,
				values, {}, imgs or {}, bufs)
			assert(ok, 'Compute dispatch rejected')
			return next_view
		end

		local self = {
			metadata = metadata,
			count = count,
			sort_passes = #passes,
			gpu_bytes = 172 * count + 32 * padded + 48 * atlas_w * atlas_h
		}

		function self:prepare(view, camera, width, height)
			local camera_right, camera_up, camera_forward = camera:basis()
			local rx, ry, rz = table.unpack(camera_right)
			local ux, uy, uz = table.unpack(camera_up)
			local fx, fy, fz = table.unpack(camera_forward)
			local px, py, pz = table.unpack(camera.eye)

			-- project, sort and gather the Gaussians
			local values = {
				counts,
				value('u_eye', px, py, pz, 0),
				value('u_right', rx, ry, rz, 0),
				value('u_up', ux, uy, uz, 0),
				value('u_forward', fx, fy, fz, 0),
				value('u_screen', width, height, height / (2 * math.tan(math.pi / 6)), 0.05)
			}
			view = dispatch(view, programs.project, values, {}, project_buffers)
			for _, pass in ipairs(passes) do
				view = dispatch(view, programs.sort, pass.values, {}, pass.buffers)
			end
			view = dispatch(view, programs.gather, draw_values, images, gather_buffers, atlas_w // 8, atlas_h // 8)
			return view
		end

		function self:draw(view, camera, width, height)
			local camera_right, camera_up = camera:basis()
			hg.SetViewRect(view, 0, 0, width, height)
			hg.SetViewClear(view, hg.CF_Color, hg.Color(0.025, 0.03, 0.04, 1))
			hg.SetViewMode(view, hg.VM_Sequential)

			hg.DrawModel(view, model, programs.splat, draw_values, draw_textures, hg.Mat4.Identity, state)
			draw_gizmo(view, programs.gizmo, state, width, height, camera_right, camera_up)
		end

		function self:destroy()
			resources:DestroyAllModels()
			resources:DestroyAllTextures()
			resources:DestroyAllPrograms()

			for _, b in ipairs(buffers) do
				hg.DestroyComputeBuffer(b)
			end
			for _, t in ipairs(texture_objects) do
				hg.DestroyTexture(t)
			end
			for _, p in pairs(programs) do
				hg.DestroyProgram(p)
			end
		end

		return self
	end

	return Renderer
end
