-- Houseplant by Marcel Padilla (CC BY 4.0): native Gaussian loading and rendering.
-- Drag left to orbit, wheel to dolly, drag right/middle to pan, R to reset.
-- One Lua source; compile resources with assetc before running. Escape closes the window.

local lua_dir = (arg[-1] or ""):match("^(.*[/\\])")
if lua_dir then
	package.cpath = lua_dir .. "?.dll;" .. lua_dir .. "?.so;" .. package.cpath
end

local hg = require("harfang")
assert(hg.SubmitGaussianSplats, "Rebuild HARFANG with the native Gaussian API first")

-- arcball camera
-- Native vectors and quaternions permit free rotation, including roll.
local Camera = {}
Camera.__index = Camera

local function sphere(x, y, width, height)
	local radius = math.max(1, math.min(width, height) * 0.5)
	x, y = (x - width * 0.5) / radius, (y - height * 0.5) / radius
	local distance_squared = x * x + y * y

	if distance_squared > 1 then
		return hg.Normalize(hg.Vec3(x, y, 0))
	end

	return hg.Vec3(x, y, -math.sqrt(1 - distance_squared))
end

function Camera.new(bounds_min, bounds_max, yaw, pitch)
	local lo = bounds_min or hg.Vec3(-1, -1, -1)
	local hi = bounds_max or hg.Vec3(1, 1, 1)
	local extent = hi - lo
	local camera = setmetatable({
		center = (lo + hi) * 0.5,
		initial_radius = math.max(extent.x, extent.y, extent.z) * 1.7,
		initial_yaw = yaw or 0.4,
		initial_pitch = pitch or -0.18
	}, Camera)

	camera:reset()
	return camera
end

function Camera:basis()
	return hg.Rotate(self.q, hg.Vec3.Right), hg.Rotate(self.q, hg.Vec3.Up), hg.Rotate(self.q, hg.Vec3.Front)
end

function Camera:orbit_eye()
	local _, _, forward = self:basis()
	self.eye = self.target - forward * self.radius
end

function Camera:reset()
	self.radius = math.max(0.5, self.initial_radius)
	self.target = hg.Vec3(self.center)
	self.q = hg.QuaternionFromEuler(-self.initial_pitch, self.initial_yaw, 0, hg.RO_YXZ)
	self:orbit_eye()
end

function Camera:set_pose(eye, yaw, pitch)
	self.eye = eye
	self.q = hg.QuaternionFromEuler(-pitch, yaw, 0, hg.RO_YXZ)
end

function Camera:update(input)
	if input.active then
		if input.reset then
			self:reset()
		end

		if input.orbit and input.drag_valid then
			local a = sphere(input.x - input.dx, input.y - input.dy, input.width, input.height)
			local b = sphere(input.x, input.y, input.width, input.height)
			local dot = hg.Clamp(hg.Dot(a, b), -1, 1)

			-- Inverse object rotation: move the camera around the target.
			if dot > -0.999999 then
				local axis = hg.Cross(b, a)
				local delta = hg.Normalize(hg.Quaternion(axis.x, axis.y, axis.z, 1 + dot))
				self.q = hg.Normalize(self.q * delta)
			end
		end

		self.radius = hg.Clamp(self.radius * math.exp(-(input.wheel or 0) * 0.12), 0.15, 50)

		if input.pan and input.drag_valid then
			local right, up = self:basis()
			local scale = 2 * self.radius * math.tan(math.pi / 6) / math.max(input.height, 1)
			self.target = self.target - (right * input.dx + up * input.dy) * scale
		end

		self:orbit_eye()
	end
end

-- initialize rendering
hg.InputInit()
hg.WindowSystemInit()

local windows = package.config:sub(1, 1) == "\\"
local assets = "resources_compiled"
local win = hg.RenderInit('HARFANG - Houseplant Gaussian splatting', 1280, 720,
	windows and hg.RT_Direct3D11 or hg.RT_Metal)
assert(win, "RenderInit failed")

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
	assert(hg.IsValid(renderer), "Missing compiled Gaussian shaders; compile resources with assetc first")

	local loaded = hg.LoadGaussianSplatModelFromFile(assets .. "/gaussian_splatting/source/plant.hgs")
	assert(loaded.ok, loaded.message)
	model = loaded.model
	data = hg.CreateGaussianSplatRenderData()

	local instance = hg.GaussianSplatInstance(model)
	instance.model, instance.id = model, 1
	-- The source is -Y up; a proper X rotation transforms positions and covariances together.
	instance.world = hg.RotationMat4(hg.Vec3(math.pi, 0, 0))
	local instances = {instance}

	-- camera and input
	local lo, hi = hg.GetGaussianSplatBoundsMin(model), hg.GetGaussianSplatBoundsMax(model)
	local camera = Camera.new(hg.Vec3(lo.x, -hi.y, -hi.z), hg.Vec3(hi.x, -lo.y, -lo.z))
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
			local client_ok, client_w, client_h = hg.GetWindowClientSize(win)
			assert(client_ok)
			camera:update({
				active = focused,
				reset = keyboard:Pressed(hg.K_R),
				orbit = mouse:Down(hg.MB_0),
				pan = mouse:Down(hg.MB_1) or mouse:Down(hg.MB_2),
				drag_valid = was_focused and not mouse:Pressed(hg.MB_0) and not mouse:Pressed(hg.MB_1)
					and not mouse:Pressed(hg.MB_2),
				x = mouse:X(),
				y = mouse:Y(),
				dx = mouse:DtX(),
				dy = mouse:DtY(),
				width = client_w,
				height = client_h,
				wheel = mouse:Wheel()
			})

			local right, up, forward = camera:basis()
			local camera_world = hg.Mat4LookTowardUp(camera.eye, forward, up)
			local view_state = hg.ComputePerspectiveViewState(camera_world, math.pi / 3, 0.01, 100,
				hg.ComputeAspectRatioX(width, height))

			-- draw Gaussian splats
			-- Clearing belongs to the application; the native renderer preserves the target.
			hg.SetViewRect(0, 0, 0, width, height)
			hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color(0.025, 0.03, 0.04, 1), 1)
			hg.Touch(0)

			local result, next_view = hg.SubmitGaussianSplats(1, hg.IntRect(0, 0, width, height),
				view_state, instances, renderer, data, settings)
			assert(result.ok, result.message)

			-- draw orientation gizmo
			hg.SetViewRect(next_view, 0, 0, width, height)
			hg.SetViewClear(next_view, 0)
			hg.SetViewMode(next_view, hg.VM_Sequential)
			draw_gizmo(next_view, gizmo_program, gizmo_state, width, height, right, up)

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
