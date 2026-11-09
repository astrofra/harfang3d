-- Saari (compiled from SPZ through assetc): native Gaussian loading, SH evaluation, projection, sorting and drawing.
-- Drag left to orbit, wheel to dolly, drag right/middle to pan, R to reset.
-- Keys 1..4 select SH degree 0..3. Escape closes the window.

local hg = require("harfang")

-- initialize rendering
hg.InputInit()
hg.WindowSystemInit()

local windows = package.config:sub(1, 1) == "\\"
local assets = "resources_compiled"
local win = hg.RenderInit('HARFANG - Saari native Gaussian splatting', 1280, 720,
	windows and hg.RT_Direct3D11 or hg.RT_Metal)

local renderer, data, model

local function main()
	-- load model
	renderer = hg.CreateGaussianSplatRendererFromFile(assets .. "/core/gaussian_splatting")

	local loaded = hg.LoadGaussianSplatModelFromFile(assets .. "/gaussian_splatting_native/saari-probe.hgs")
	model = loaded.model
	data = hg.CreateGaussianSplatRenderData()

	local instance = hg.GaussianSplatInstance(model)
	instance.model, instance.id = model, 1
	-- The preservation capture uses Z up; rotate the whole instance, including SH.
	instance.world = hg.RotationMat4(hg.Vec3(-math.pi / 2, 0, 0))
	local instances = {instance}

	-- camera and input
	-- Saari includes a distant sky shell. Start inside it, looking at the island.
	local camera = hg.TrackballController(hg.Vec3(0, -0.35, 0), 0.85,
		hg.QuaternionFromEuler(0.5, 0.4, 0, hg.RO_YXZ))
	local fov = math.pi / 3
	local keyboard, mouse = hg.Keyboard(), hg.Mouse()
	local width, height, was_focused = 0, 0, false
	local settings = hg.GaussianSplatRenderSettings()

	print(string.format("Saari: %d / %d splats; stored SH degree %d", hg.GetGaussianSplatCount(model),
		hg.GetGaussianSplatSourceCount(model), hg.GetGaussianSplatSHDegree(model)))

	-- main loop
	while hg.IsWindowOpen(win) do
		keyboard:Update()
		mouse:Update()
		local focused = hg.WindowHasFocus(win)

		if focused and keyboard:Down(hg.K_Escape) then
			break
		end

		local sized, w, h = hg.GetWindowFrameBufferSize(win)
		if sized and w > 0 and h > 0 then
			if w ~= width or h ~= height then
				width, height = w, h
				hg.RenderReset(width, height, 0)
			end

			-- update camera and SH degree
			local _, client_w, client_h = hg.GetWindowClientSize(win)
			if focused then
				if keyboard:Pressed(hg.K_R) then
					camera:Reset()
				end
				camera:Update(mouse, client_w, client_h, fov, was_focused)

				for degree, key in ipairs({hg.K_1, hg.K_2, hg.K_3, hg.K_4}) do
					if keyboard:Pressed(key) then
						settings.max_sh_degree = degree - 1
					end
				end
			end

			local camera_world = camera:ComputeWorld()
			local view_state = hg.ComputePerspectiveViewState(camera_world, fov, 0.01, 100,
				hg.ComputeAspectRatioX(width, height))

			-- draw Gaussian splats
			-- Clearing belongs to the application; the native renderer preserves the target.
			hg.SetViewRect(0, 0, 0, width, height)
			hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color(0.025, 0.03, 0.04, 1), 1)
			hg.Touch(0)

			hg.SubmitGaussianSplats(1, hg.IntRect(0, 0, width, height),
				view_state, instances, renderer, data, settings)

			hg.Frame()
		else
			hg.Sleep(hg.time_from_ms(10))
		end

		was_focused = focused
		hg.UpdateWindow(win)
	end
end

local ok, message = xpcall(main, debug.traceback)
if not ok then
	io.stderr:write(tostring(message) .. "\n")
end

-- shutdown
if data then
	hg.DestroyGaussianSplatRenderData(data)
end
if renderer then
	hg.DestroyGaussianSplatRenderer(renderer)
end
if model then
	hg.DestroyGaussianSplatModel(model)
end

hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
hg.InputShutdown()

if not ok then
	error(message)
end
