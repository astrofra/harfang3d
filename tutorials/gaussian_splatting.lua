-- Houseplant by Marcel Padilla (CC BY 4.0): native Gaussian loading and rendering.
-- Drag left to orbit, wheel to dolly, drag right/middle to pan, R to reset.
-- One Lua source; compile resources with assetc before running. Escape closes the window.

local hg = require("harfang")

-- initialize rendering
hg.InputInit()
hg.WindowSystemInit()

local windows = package.config:sub(1, 1) == "\\"
local assets = "resources_compiled"
local win = hg.RenderInit('HARFANG - Houseplant Gaussian splatting', 1280, 720,
	windows and hg.RT_Direct3D11 or hg.RT_Metal)

local renderer, data, model, gizmo_program

-- world orientation gizmo
-- Drawn after the splats in a separate sequential view.
local function create_gizmo()
	local vertices = hg.Vertices(hg.VertexLayoutPosFloatColorUInt8(), 128)
	local colors = {hg.Color(1, 0, 0), hg.Color(0, 1, 0), hg.Color(0, 0.35, 1)}
	local axes = {hg.Vec3.Right, hg.Vec3.Up, hg.Vec3.Front}
	local glyphs = {
		{{-3, -4, 3, 4}, {-3, 4, 3, -4}},
		{{-3, 4, 0, 0}, {3, 4, 0, 0}, {0, 0, 0, -4}},
		{{-3, 4, 3, 4}, {3, 4, -3, -4}, {-3, -4, 3, -4}}
	}

	return function(view, program, state, width, height, right, up)
		vertices:Clear()
		local index = 0
		local origin_x, origin_y = width - 76, 76

		local function line(x1, y1, x2, y2, color)
			vertices:Begin(index):SetPos(hg.Vec3(x1 / width * 2 - 1, y1 / height * 2 - 1, 0)):SetColor0(color):End()
			vertices:Begin(index + 1):SetPos(hg.Vec3(x2 / width * 2 - 1, y2 / height * 2 - 1, 0)):SetColor0(color):End()
			index = index + 2
		end

		for axis = 1, 3 do
			local dx, dy = hg.Dot(right, axes[axis]) * 42, hg.Dot(up, axes[axis]) * 42
			local length = math.sqrt(dx * dx + dy * dy)
			local x, y = origin_x + dx, origin_y + dy
			line(origin_x, origin_y, x, y, colors[axis])

			local nx = length > 1 and dx / length or 0.707
			local ny = length > 1 and dy / length or 0.707
			line(x, y, x - nx * 6 - ny * 3, y - ny * 6 + nx * 3, colors[axis])
			line(x, y, x - nx * 6 + ny * 3, y - ny * 6 - nx * 3, colors[axis])

			x, y = x + nx * 10, y + ny * 10
			for _, segment in ipairs(glyphs[axis]) do
				line(x + segment[1], y + segment[2], x + segment[3], y + segment[4], colors[axis])
			end
		end

		hg.DrawLines(view, vertices, program, state)
	end
end

local function main()
	-- load model
	renderer = hg.CreateGaussianSplatRendererFromFile(assets .. "/core/gaussian_splatting")

	local loaded = hg.LoadGaussianSplatModelFromFile(assets .. "/gaussian_splatting/source/plant.hgs")
	model = loaded.model
	data = hg.CreateGaussianSplatRenderData()

	local instance = hg.GaussianSplatInstance(model)
	instance.model, instance.id = model, 1
	-- The source is -Y up; a proper X rotation transforms positions and covariances together.
	instance.world = hg.RotationMat4(hg.Vec3(math.pi, 0, 0))
	local instances = {instance}

	-- camera and input
	local lo, hi = hg.GetGaussianSplatBoundsMin(model), hg.GetGaussianSplatBoundsMax(model)
	local extent = hi - lo
	local distance = math.max(0.5, math.max(extent.x, extent.y, extent.z) * 1.7)
	local target = instance.world * ((lo + hi) * 0.5)
	local camera = hg.TrackballController(target, distance, hg.QuaternionFromEuler(0.18, 0.4, 0, hg.RO_YXZ))
	local fov = math.pi / 3
	local keyboard, mouse = hg.Keyboard(), hg.Mouse()
	local width, height, was_focused = 0, 0, false

	gizmo_program = hg.LoadProgramFromFile(assets .. "/gaussian_splatting/shaders/gizmo")
	local draw_gizmo = create_gizmo()
	local gizmo_state = hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Disabled, hg.FC_Disabled, false)
	local settings = hg.GaussianSplatRenderSettings()

	print(string.format("Houseplant: %d / %d splats; stored SH degree %d", hg.GetGaussianSplatCount(model),
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

			-- update camera
			local _, client_w, client_h = hg.GetWindowClientSize(win)
			if focused then
				if keyboard:Pressed(hg.K_R) then
					camera:Reset()
				end
				camera:Update(mouse, client_w, client_h, fov, was_focused)
			end

			local camera_world = camera:ComputeWorld()
			local view_state = hg.ComputePerspectiveViewState(camera_world, fov, 0.01, 100,
				hg.ComputeAspectRatioX(width, height))

			-- draw Gaussian splats
			-- Clearing belongs to the application; the native renderer preserves the target.
			hg.SetViewRect(0, 0, 0, width, height)
			hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color(0.025, 0.03, 0.04, 1), 1)
			hg.Touch(0)

			local _, next_view = hg.SubmitGaussianSplats(1, hg.IntRect(0, 0, width, height),
				view_state, instances, renderer, data, settings)

			-- draw orientation gizmo
			hg.SetViewRect(next_view, 0, 0, width, height)
			hg.SetViewClear(next_view, 0)
			hg.SetViewMode(next_view, hg.VM_Sequential)
			draw_gizmo(next_view, gizmo_program, gizmo_state, width, height, hg.GetX(camera_world), hg.GetY(camera_world))

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
if gizmo_program then
	hg.DestroyProgram(gizmo_program)
end
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
