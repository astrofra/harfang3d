-- Automated Gaussian renderer tests and benchmarks; the tutorial takes no arguments.

-- command-line options
local opt = {preset = 'full', renderer = 'metal', frames = 0, width = 1280, height = 720}
local i = 1

while i <= #arg do
	local key = arg[i]:match('^%-%-(.+)$')
	assert(key, 'Expected --option, got ' .. arg[i])

	if key == 'route' or key == 'resize-test' then
		opt[key] = true
	else
		i = i + 1
		opt[key] = assert(arg[i], 'Missing value for ' .. key)
	end

	i = i + 1
end

assert(opt.assets and opt.module, '--assets COMPILED_DIRECTORY and --module MODULE_DIRECTORY are required')
package.cpath = opt.module .. '/?.so;' .. opt.module .. '/?.dll;' .. package.cpath

local hg = require('harfang')

local script_dir = arg[0]:match('^(.*[/\\])') or './'
local tutorial_dir = script_dir .. '../../tutorials/'
local Camera = dofile(tutorial_dir .. 'gaussian_splatting/camera.lua')
local Renderer = dofile(tutorial_dir .. 'gaussian_splatting/renderer.lua')(hg)

local frames = tonumber(opt.frames)
-- initialize input and rendering
hg.InputInit()
hg.WindowSystemInit()

assert(opt.renderer == 'metal' or opt.renderer == 'dx11', 'This tutorial currently qualifies Metal; dx11 is experimental')
local win = hg.RenderInit('HARFANG Gaussian splatting', tonumber(opt.width), tonumber(opt.height),
	opt.renderer == 'metal' and hg.RT_Metal or hg.RT_Direct3D11)
assert(win, 'RenderInit failed')

local resources = hg.PipelineResources()
local texture_objects = {}
local capture_fb, capture_depth, renderer
local function texture(w, h, name, flags, format)
	local t = hg.CreateTexture(w, h, name, flags, format)
	texture_objects[#texture_objects + 1] = t
	return t
end

local function main()
	renderer = Renderer.new(opt.assets, opt.preset)
	local caps = hg.GetComputeCaps()
	local metadata, count = renderer.metadata, renderer.count
	local keyboard, mouse = hg.Keyboard(), hg.Mouse()
	local camera = Camera.new(metadata.bounds_min, metadata.bounds_max, tonumber(opt.yaw), tonumber(opt.pitch))

	local width, height = 0, 0
	local last = hg.time_now()
	local timings = {}
	local cpu_times = {}
	local gpu_times = {}
	local last_gpu_timestamp = 0
	local draw_calls, compute_calls = 0, 0
	local n = 0
	local resized = 0
	local was_focused = false
	local gpu_bytes = renderer.gpu_bytes
	print(string.format('Estimated persistent GPU payload: %.2f MiB (excludes driver allocations)', gpu_bytes / 1048576))

	local function draw(view)
		renderer:draw(view, camera, width, height)
	end
	-- main loop
	while hg.IsWindowOpen(win) and (frames == 0 or n < frames) do
		keyboard:Update()
		mouse:Update()

		local focused = hg.WindowHasFocus(win)
		local active = focused and not opt.route
		if focused and keyboard:Down(hg.K_Escape) then
			break
		end

		if opt['resize-test'] then
			if n == 30 then
				assert(hg.SetWindowClientSize(win, tonumber(opt.width) + 80, tonumber(opt.height) + 40))
			end
			if n == 60 then
				assert(hg.SetWindowClientSize(win, tonumber(opt.width), tonumber(opt.height)))
			end
		end

		local now = hg.time_now()
		local elapsed = hg.time_to_sec_f(now - last)
		last = now

		local sized, w, h = hg.GetWindowFrameBufferSize(win)
		assert(sized, 'Unable to query drawable size')

		if w > 0 and h > 0 then
			-- update the drawable size and camera
			if w ~= width or h ~= height then
				width, height = w, h
				resized = resized + 1
				hg.RenderReset(width, height, 0)
				print(string.format('Drawable %dx%d; %s; %d splats; %d bitonic passes', width, height, caps.renderer, count, renderer.sort_passes))
			end

			local client_ok, client_w, client_h = hg.GetWindowClientSize(win)
			assert(client_ok)

			camera:update({
				active = active,
				reset = keyboard:Pressed(hg.K_R),
				orbit = mouse:Down(hg.MB_0),
				pan = mouse:Down(hg.MB_1) or mouse:Down(hg.MB_2),
				drag_valid = was_focused and not mouse:Pressed(hg.MB_0) and not mouse:Pressed(hg.MB_1) and not mouse:Pressed(hg.MB_2),
				x = mouse:X(), y = mouse:Y(),
				dx = mouse:DtX(), dy = mouse:DtY(),
				width = client_w, height = client_h,
				wheel = mouse:Wheel()
			})

			-- repeatable camera route for performance measurements
			if opt.route then
				local t = n / math.max(frames - 1, 1)
				local angle = t * math.pi * 2
				local radius = 3.4 - 1.8 * math.sin(angle) ^ 8
				local py = 0.5 * math.sin(angle * 2)
				local yaw, pitch = -angle, -math.atan(py / radius)
				if t > 0.82 and t < 0.90 then
					yaw = yaw + math.pi * math.sin((t - 0.82) / 0.08 * math.pi) ^ 2
				end
				camera:set_pose({math.sin(angle) * radius, py, -math.cos(angle) * radius}, yaw, pitch)
			end

			local start = hg.time_now()
			local view = renderer:prepare(0, camera, width, height)
			draw(view)

			-- capture only on the last frame; no readbacks in the normal render loop
			local ready, picture, readback
			if opt.capture and frames > 0 and n == frames - 1 then
				local target = hg.CreateTexture(width, height, 'capture-target', hg.TF_RenderTarget, hg.TF_RGBA8)
				local ref = resources:AddTexture('capture-target', target)
				capture_depth = texture(width, height, 'capture-depth', hg.TF_RenderTarget, hg.TF_D24)
				capture_fb = hg.CreateFrameBuffer(target, capture_depth, 'capture')

				hg.SetViewFrameBuffer(view + 1, capture_fb.handle)
				draw(view + 1)

				readback = texture(width, height, 'capture-readback', hg.TF_ReadBack | hg.TF_BlitDestination, hg.TF_RGBA8)
				picture = hg.Picture(width, height, hg.PF_RGBA32)
				ready = hg.CaptureTexture(view + 2, resources, ref, readback, picture)
			end

			-- collect timings and present the frame
			local cpu = hg.time_to_ms_f(hg.time_now() - start)
			local stats = hg.GetComputeFrameStats()
			draw_calls, compute_calls = stats.draw_calls, stats.compute_calls
			if n >= 60 and stats.gpu_timing_available and stats.gpu_timestamp ~= last_gpu_timestamp then
				gpu_times[#gpu_times + 1] = stats.gpu_ms
				last_gpu_timestamp = stats.gpu_timestamp
			end

			local frame = hg.Frame()
			if ready then
				while frame < ready do
					hg.UpdateWindow(win)
					frame = hg.Frame()
				end
				assert(hg.SavePNG(picture, opt.capture), 'Screenshot save failed')
			end

			if n >= 60 then
				timings[#timings + 1] = elapsed * 1000
				cpu_times[#cpu_times + 1] = cpu
			end
			n = n + 1

			if n % 60 == 0 then
				hg.SetWindowTitle(win, string.format('GSplat %s | %d | %.1f ms | LMB orbit | wheel zoom | RMB pan | R reset | Esc quit',
					caps.renderer, count, elapsed * 1000))
			end
		else
			hg.Sleep(hg.time_from_ms(10))
		end

		was_focused = focused
		hg.UpdateWindow(win)
	end

	-- report validation and performance results
	if opt['resize-test'] then
		assert(resized >= 3, 'Resize stress did not exercise both size changes')
	end

	local measured_ms = 0
	for _, ms in ipairs(timings) do
		measured_ms = measured_ms + ms
	end
	table.sort(timings)
	table.sort(cpu_times)
	table.sort(gpu_times)

	local function percentile(t, q)
		return #t > 0 and t[math.max(1, math.ceil(#t * q))] or 0
	end

	local report = string.format('{"renderer":"%s","preset":"%s","count":%d,"width":%d,"height":%d,"frames":%d,"samples":%d,"route":%s,"sort_passes":%d,"frame_ms_p50":%.4f,"frame_ms_p95":%.4f,"submit_ms_p50":%.4f,"submit_ms_p95":%.4f}',
		caps.renderer, opt.preset, count, width, height, n, #timings, tostring(opt.route or false), renderer.sort_passes,
		percentile(timings, 0.5), percentile(timings, 0.95), percentile(cpu_times, 0.5), percentile(cpu_times, 0.95))
	report = report:sub(1, -2) .. string.format(',"gpu_samples":%d,"gpu_ms_p50":%.4f,"gpu_ms_p95":%.4f,"draw_calls":%d,"compute_calls":%d}',
		#gpu_times, percentile(gpu_times, 0.5), percentile(gpu_times, 0.95), draw_calls, compute_calls)
	report = report:sub(1, -2) .. string.format(',"measured_seconds":%.3f,"estimated_gpu_bytes":%d,"resize_count":%d}',
		measured_ms / 1000, gpu_bytes, resized)
	print(report)

	if opt.report then
		local f = assert(io.open(opt.report, 'w'))
		f:write(report, '\n')
		f:close()
	end
end

local success, err = xpcall(main, debug.traceback)
if capture_fb then hg.DestroyFrameBuffer(capture_fb) end
resources:DestroyAllTextures()
for _, t in ipairs(texture_objects) do hg.DestroyTexture(t) end
if renderer then renderer:destroy() end
hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
hg.InputShutdown()
if not success then error(err) end
print('PASS Gaussian splatting viewer: normal shutdown')
