-- Gaussian splatting with a trackball camera and an RGB axis gizmo.
-- Left drag: orbit, wheel: zoom, right/middle drag: pan, R: reset, Escape: quit.

-- find HARFANG beside the Lua executable when using an installed runtime
local lua_dir = (arg[-1] or ''):match('^(.*[/\\])')
if lua_dir then
	package.cpath = lua_dir .. '?.so;' .. lua_dir .. '?.dll;' .. package.cpath
end

hg = require('harfang')

local Camera = dofile('gaussian_splatting/camera.lua')
local Renderer = dofile('gaussian_splatting/renderer.lua')(hg)

hg.InputInit()
hg.WindowSystemInit()

res_x, res_y = 1280, 720
renderer_name = package.config:sub(1, 1) == '\\' and 'dx11' or 'metal'
renderer_type = renderer_name == 'metal' and hg.RT_Metal or hg.RT_Direct3D11
win = hg.RenderInit('Harfang - Gaussian Splatting', res_x, res_y, renderer_type)
assert(win, 'RenderInit failed')

-- load the bundled Houseplant from precompiled resources
splat = Renderer.new('resources_compiled/gaussian_splatting/' .. renderer_name, 'full')
camera = Camera.new(splat.metadata.bounds_min, splat.metadata.bounds_max)
keyboard = hg.Keyboard()
mouse = hg.Mouse()
was_focused = false

-- main loop
while hg.IsWindowOpen(win) do
	keyboard:Update()
	mouse:Update()

	local focused = hg.WindowHasFocus(win)
	if focused and keyboard:Down(hg.K_Escape) then
		break
	end

	local sized, width, height = hg.GetWindowFrameBufferSize(win)
	if sized and width > 0 and height > 0 then
		if width ~= res_x or height ~= res_y then
			res_x, res_y = width, height
			hg.RenderReset(res_x, res_y, 0)
		end

		local client_ok, client_w, client_h = hg.GetWindowClientSize(win)
		assert(client_ok)
		camera:update({
			active = focused,
			reset = keyboard:Pressed(hg.K_R),
			orbit = mouse:Down(hg.MB_0),
			pan = mouse:Down(hg.MB_1) or mouse:Down(hg.MB_2),
			drag_valid = was_focused and not mouse:Pressed(hg.MB_0) and not mouse:Pressed(hg.MB_1) and not mouse:Pressed(hg.MB_2),
			x = mouse:X(), y = mouse:Y(),
			dx = mouse:DtX(), dy = mouse:DtY(),
			width = client_w, height = client_h,
			wheel = mouse:Wheel()
		})

		local view_id = splat:prepare(0, camera, res_x, res_y)
		splat:draw(view_id, camera, res_x, res_y)

		hg.Frame()
	else
		hg.Sleep(hg.time_from_ms(10))
	end

	was_focused = focused
	hg.UpdateWindow(win)
end

splat:destroy()
hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
hg.InputShutdown()
