-- Developer benchmark: identical full-count Saari route, no GUI, no readback.
-- MODULE_DIR COMPILED_ASSETS REPORT.json [motion|static] [FRAMES=240] [SECONDS=0] [dx11|metal]
local module, assets, report = assert(arg[1]), assert(arg[2]), assert(arg[3])
local mode, minimum_frames, minimum_seconds = arg[4] or 'motion', tonumber(arg[5]) or 240, tonumber(arg[6]) or 0
local backend = arg[7] or 'dx11'
assert(mode == 'motion' or mode == 'static')
assert(backend == 'dx11' or backend == 'metal')
package.cpath = module .. '/?.dll;' .. module .. '/?.so;' .. package.cpath
local hg = require('harfang')
hg.WindowSystemInit()
local win = hg.RenderInit('Native Gaussian benchmark', 1280, 720,
	backend == 'metal' and hg.RT_Metal or hg.RT_Direct3D11)
assert(win)
assert(hg.GetComputeCaps().renderer == (backend == 'metal' and 'Metal' or 'Direct3D 11'))
local loaded = hg.LoadGaussianSplatModelFromFile(assets .. '/gaussian_splatting/source/saari-probe.hgs')
assert(loaded.ok, loaded.message)
local renderer = hg.CreateGaussianSplatRendererFromFile(assets .. '/core/gaussian_splatting')
assert(hg.IsValid(renderer))
local data, settings = hg.CreateGaussianSplatRenderData(), hg.GaussianSplatRenderSettings()
local instance = hg.GaussianSplatInstance(loaded.model)
instance.id = 1
instance.world = hg.RotationMat4(hg.Vec3(-math.pi / 2, 0, 0))
local instances = {instance}
local camera = hg.TrackballController(hg.Vec3(0, -0.35, 0), 0.85)
local sized, width, height = hg.GetWindowFrameBufferSize(win)
assert(sized)
hg.RenderReset(width, height, 0)
local rect = hg.IntRect(0, 0, width, height)
local cpu, submit, gpu, dispatch = {}, {}, {}, {}
local last_stamp, frame, first_sample, gpu_bytes = -1, 0, nil, 0
local function milliseconds(start)
	return hg.time_to_ms_f(hg.time_now() - start)
end
local ok, err = xpcall(function()
	while frame < 60 + minimum_frames or (first_sample and milliseconds(first_sample) < minimum_seconds * 1000) or (frame - 60) % 240 ~= 0 do
		local start = hg.time_now()
		local phase = mode == 'motion' and ((frame % 240) / 240 * math.pi * 2) or 0
		camera.orientation = hg.QuaternionFromEuler(0.5 + 0.15 * math.sin(phase * 2), 0.4 + 0.4 * math.sin(phase), 0, hg.RO_YXZ)
		camera.distance = 0.85 * (1 + 0.1 * math.sin(phase))
		local state = hg.ComputePerspectiveViewState(camera:ComputeWorld(), math.pi / 3, 0.01, 100,
			hg.ComputeAspectRatioX(width, height))
		hg.SetViewRect(0, 0, 0, width, height)
		hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color(0.025, 0.03, 0.04, 1), 1)
		hg.Touch(0)
		local before_submit = hg.time_now()
		local result = hg.SubmitGaussianSplats(1, rect, state, instances, renderer, data, settings)
		local submit_ms = milliseconds(before_submit)
		assert(result.ok, result.message)
		assert(result.count == hg.GetGaussianSplatCount(loaded.model))
		gpu_bytes = result.gpu_bytes
		hg.Frame()
		hg.UpdateWindow(win)
		if frame >= 60 then
			first_sample = first_sample or start
			cpu[#cpu + 1], submit[#submit + 1] = milliseconds(start), submit_ms
			dispatch[#dispatch + 1] = result.dispatches
			local stats = hg.GetComputeFrameStats()
			if stats.gpu_timing_available and stats.gpu_timestamp ~= last_stamp then
				gpu[#gpu + 1] = stats.gpu_ms
				last_stamp = stats.gpu_timestamp
			end
		end
		frame = frame + 1
	end
	local elapsed = milliseconds(first_sample) / 1000
	local function percentile(values, fraction)
		table.sort(values)
		return #values > 0 and values[math.max(1, math.ceil(#values * fraction))] or -1
	end
	local text = string.format('{\n  "mode": "%s", "renderer": "%s", "count": %d, "width": %d, "height": %d,\n  "frames": %d, "seconds": %.6f, "gpu_samples": %d, "gpu_bytes": %d,\n  "frame_ms_p50": %.6f, "frame_ms_p95": %.6f, "submit_ms_p50": %.6f,\n  "gpu_ms_p50": %.6f, "gpu_ms_p95": %.6f, "dispatches_p50": %.0f\n}\n',
		mode, hg.GetComputeCaps().renderer, hg.GetGaussianSplatCount(loaded.model), width, height,
		#cpu, elapsed, #gpu, gpu_bytes, percentile(cpu, 0.5), percentile(cpu, 0.95), percentile(submit, 0.5),
		percentile(gpu, 0.5), percentile(gpu, 0.95), percentile(dispatch, 0.5))
	local file = assert(io.open(report, 'w'))
	file:write(text)
	file:close()
	print(text)
end, debug.traceback)
hg.DestroyGaussianSplatRenderData(data)
hg.DestroyGaussianSplatRenderer(renderer)
hg.DestroyGaussianSplatModel(loaded.model)
hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
if not ok then error(err) end
