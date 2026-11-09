-- Headless binding checks for the native trackball controller.
-- lua gaussian_camera.lua MODULE_DIR
local module_dir = assert(arg[1])
package.cpath = module_dir .. "/?.dll;" .. module_dir .. "/?.so;" .. package.cpath
local hg = require("harfang")

local function near(a, b)
	assert(math.abs(a - b) < 1e-5, string.format("%.12f != %.12f", a, b))
end

local camera = hg.TrackballController(hg.Vec3(0, 0, 0), 3.4)
camera:Update(true, false, 150, 100, 50, 0, 0, 200, 200)
local world = camera:ComputeWorld()
near(hg.GetZ(world).x, 0.5)
near(hg.GetZ(world).z, math.sqrt(0.75))
near(hg.GetT(world).x, -1.7)
near(hg.GetT(world).z, -3.4 * math.sqrt(0.75))

-- Reverse the sphere path, then exercise dolly, pan and reset.
camera:Update(true, false, 100, 100, -50, 0, 0, 200, 200)
near(hg.GetT(camera:ComputeWorld()).x, 0)
near(hg.GetT(camera:ComputeWorld()).z, -3.4)
camera:Update(false, false, 0, 0, 0, 0, 1000, 200, 200)
near(camera.distance, 0.15)
camera:Update(false, false, 0, 0, 0, 0, -1000, 200, 200)
near(camera.distance, 50)
camera:Reset()
camera:Update(false, true, 0, 0, 10, 0, 0, 200, 200, math.pi / 3)
near(camera.target.x, -10 * 2 * 3.4 * math.tan(math.pi / 6) / 200)
camera:Reset()
near(camera.target.x, 0)

-- Explicit orientation, independent copies, editable settings and preserved reset pose.
local posed = hg.TrackballController(hg.Vec3(2, 3, 4), 100, hg.QuaternionFromEuler(0.18, 0.4, 0, hg.RO_YXZ))
posed.max_distance = 1000
local copy = hg.TrackballController(posed)
copy.target = hg.Vec3(0, 0, 0)
copy.distance = 5
copy.orientation = hg.Quaternion()
copy:Reset()
near(copy.target.x, 2)
near(copy.distance, 100)
near(copy.max_distance, 1000)
near(posed.target.x, 2)
local front = hg.GetZ(copy:ComputeWorld())
near(front.x, math.sin(0.4) * math.cos(-0.18))
near(front.y, math.sin(-0.18))
near(front.z, math.cos(0.4) * math.cos(-0.18))

-- Dispatch the Mouse overload with default, explicit FOV and explicit focus guard arguments.
local mouse = hg.Mouse("trackball-no-input")
local default_camera = hg.TrackballController()
default_camera:Update(mouse, 200, 200)
default_camera:Update(mouse, 200, 200, math.pi / 3)
default_camera:Update(mouse, 200, 200, math.pi / 3, false)
near(hg.GetT(default_camera:ComputeWorld()).z, -3.4)

for _ = 1, 10000 do
	camera:Update(true, false, 140, 130, 3, -2, 0, 200, 200)
end
world = camera:ComputeWorld()
local right, up, forward = hg.GetX(world), hg.GetY(world), hg.GetZ(world)
near(hg.Len2(right), 1)
near(hg.Len2(up), 1)
near(hg.Len2(forward), 1)
near(hg.Dot(right, up), 0)
near(hg.Dot(right, forward), 0)
near(hg.Dot(up, forward), 0)
print("PASS native trackball bindings: constructors, copies, both Update overloads, rotation, dolly, pan, reset and stability")
