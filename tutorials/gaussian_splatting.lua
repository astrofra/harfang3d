-- Native Gaussian splatting: preload Houseplant and Saari, then select either model in ImGui.
-- Houseplant by Marcel Padilla (CC BY 4.0); Saari is a local preservation capture.
-- Drag left to orbit, wheel to dolly, drag right/middle to pan, R to reset.
-- One Lua source; compile resources with assetc before running. Escape closes the window.

local hg = require("harfang")

-- initialize rendering
hg.InputInit()
hg.WindowSystemInit()

local windows = package.config:sub(1, 1) == "\\"
local assets = "resources_compiled"
local win = hg.RenderInit('HARFANG - Gaussian splatting', 1280, 720,
	windows and hg.RT_Direct3D11 or hg.RT_Metal)

local renderer, gizmo_program
local scenes = {}
local imgui_initialized = false

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
	-- preload both models and keep their cameras and render data independent
	renderer = hg.CreateGaussianSplatRendererFromFile(assets .. "/core/gaussian_splatting")
	local model_names = {"Houseplant", "Saari"}
	local model_paths = {
		assets .. "/gaussian_splatting/source/plant.hgs",
		assets .. "/gaussian_splatting/source/saari-probe.hgs"
	}
	local model_rotations = {hg.Vec3(math.pi, 0, 0), hg.Vec3(-math.pi / 2, 0, 0)}

	for index, path in ipairs(model_paths) do
		local loaded = hg.LoadGaussianSplatModelFromFile(path)
		local model = loaded.model
		local scene = {
			model = model,
			data = hg.CreateGaussianSplatRenderData(),
			settings = hg.GaussianSplatRenderSettings(),
			sh_degree = hg.GetGaussianSplatSHDegree(model)
		}
		scenes[index] = scene

		-- Transform source positions, covariances and SH directions together.
		local instance = hg.GaussianSplatInstance(model)
		instance.id = index
		instance.world = hg.RotationMat4(model_rotations[index])
		scene.instances = {instance}
		scene.settings.max_sh_degree = scene.sh_degree

		if index == 1 then
			local lo, hi = hg.GetGaussianSplatBoundsMin(model), hg.GetGaussianSplatBoundsMax(model)
			local extent = hi - lo
			local distance = math.max(0.5, math.max(extent.x, extent.y, extent.z) * 1.7)
			local target = instance.world * ((lo + hi) * 0.5)
			scene.camera = hg.TrackballController(target, distance, hg.QuaternionFromEuler(0.18, 0.4, 0, hg.RO_YXZ))
		else
			-- Start inside Saari's distant sky shell, looking at the island.
			scene.camera = hg.TrackballController(hg.Vec3(0, -0.35, 0), 0.85,
				hg.QuaternionFromEuler(0.5, 0.4, 0, hg.RO_YXZ))
		end

		print(string.format("%s: %d / %d splats; stored SH degree %d", model_names[index],
			hg.GetGaussianSplatCount(model), hg.GetGaussianSplatSourceCount(model), scene.sh_degree))
	end

	-- initialize ImGui
	local imgui_program = hg.LoadProgramFromFile(assets .. "/core/shader/imgui")
	local imgui_image_program = hg.LoadProgramFromFile(assets .. "/core/shader/imgui_image")
	hg.ImGuiInit(16, imgui_program, imgui_image_program)
	imgui_initialized = true

	gizmo_program = hg.LoadProgramFromFile(assets .. "/gaussian_splatting/shaders/gizmo")
	local draw_gizmo = create_gizmo()
	local gizmo_state = hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Disabled, hg.FC_Disabled, false)
	local selected_model = 0 -- ImGui uses zero-based indices.
	local fov = math.pi / 3
	local keyboard, mouse = hg.Keyboard(), hg.Mouse()
	local width, height, was_camera_active = 0, 0, false

	-- main loop
	while hg.IsWindowOpen(win) do
		keyboard:Update()
		mouse:Update()
		local dt = hg.TickClock()
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

			-- model selection and spherical harmonics
			hg.ImGuiBeginFrame(width, height, dt, mouse:GetState(), keyboard:GetState())
			hg.ImGuiSetNextWindowPos(hg.Vec2(16, 16), hg.ImGuiCond_Once)
			hg.ImGuiSetNextWindowSize(hg.Vec2(360, 220), hg.ImGuiCond_Once)
			local previous_model = selected_model
			if hg.ImGuiBegin("Gaussian splatting") then
				local _, model_index = hg.ImGuiCombo("Model", selected_model, model_names)
				selected_model = model_index
				local scene = scenes[selected_model + 1]
				hg.ImGuiText(string.format("%d splats - SH%d stored", hg.GetGaussianSplatCount(scene.model), scene.sh_degree))
				if scene.sh_degree > 0 then
					local _, degree = hg.ImGuiSliderInt("SH degree", scene.settings.max_sh_degree, 0, scene.sh_degree, "%d")
					scene.settings.max_sh_degree = degree
				else
					hg.ImGuiText("SH degree: 0")
				end
				if hg.ImGuiButton("Reset camera") then
					scene.camera:Reset()
				end
				hg.ImGuiSeparator()
				hg.ImGuiText("Left drag: orbit / wheel: dolly")
				hg.ImGuiText("Right or middle drag: pan / R: reset")
			end
			hg.ImGuiEnd()

			-- update the active camera only when ImGui is not using the mouse
			local scene = scenes[selected_model + 1]
			local camera = scene.camera
			local camera_active = focused and not hg.ImGuiWantCaptureMouse()
			local _, client_w, client_h = hg.GetWindowClientSize(win)
			if camera_active then
				if keyboard:Pressed(hg.K_R) then
					camera:Reset()
				end
				for degree, key in ipairs({hg.K_1, hg.K_2, hg.K_3, hg.K_4}) do
					if keyboard:Pressed(key) then
						scene.settings.max_sh_degree = math.min(degree - 1, scene.sh_degree)
					end
				end
				camera:Update(mouse, client_w, client_h, fov, was_camera_active and selected_model == previous_model)
			end
			was_camera_active = camera_active

			local camera_world = camera:ComputeWorld()
			local view_state = hg.ComputePerspectiveViewState(camera_world, fov, 0.01, 100,
				hg.ComputeAspectRatioX(width, height))

			-- draw Gaussian splats
			-- Clearing belongs to the application; the native renderer preserves the target.
			hg.SetViewRect(0, 0, 0, width, height)
			hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color(0.025, 0.03, 0.04, 1), 1)
			hg.Touch(0)

			local _, next_view = hg.SubmitGaussianSplats(1, hg.IntRect(0, 0, width, height),
				view_state, scene.instances, renderer, scene.data, scene.settings)

			-- draw orientation gizmo
			hg.SetViewRect(next_view, 0, 0, width, height)
			hg.SetViewClear(next_view, 0)
			hg.SetViewMode(next_view, hg.VM_Sequential)
			draw_gizmo(next_view, gizmo_program, gizmo_state, width, height, hg.GetX(camera_world), hg.GetY(camera_world))

			-- render ImGui after the splats and orientation overlay
			hg.SetViewClear(next_view + 1, 0)
			hg.ImGuiEndFrame(next_view + 1)
			hg.Frame()
		else
			was_camera_active = false
			hg.Sleep(hg.time_from_ms(10))
		end
		hg.UpdateWindow(win)
	end
end

local ok, message = xpcall(main, debug.traceback)
if not ok then
	io.stderr:write(tostring(message) .. "\n")
end

-- shutdown
if imgui_initialized then
	hg.ImGuiShutdown()
end
if gizmo_program then
	hg.DestroyProgram(gizmo_program)
end
for _, scene in ipairs(scenes) do
	hg.DestroyGaussianSplatRenderData(scene.data)
	hg.DestroyGaussianSplatModel(scene.model)
end
if renderer then
	hg.DestroyGaussianSplatRenderer(renderer)
end

hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
hg.InputShutdown()

if not ok then
	error(message)
end
