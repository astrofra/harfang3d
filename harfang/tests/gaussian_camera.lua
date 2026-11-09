-- Numerical checks for the interactive camera, independent of rendering/input devices.
-- lua gaussian_camera.lua MODULE_DIR TUTORIAL.lua
local module_dir,tutorial=assert(arg[1]),assert(arg[2])
package.cpath=module_dir..'/?.dll;'..module_dir..'/?.so;'..package.cpath
local hg=require('harfang')
local file=assert(io.open(tutorial,'rb'))
local source=file:read('*a'); file:close()
local camera_source=assert(source:match('^(.-)\nhg%.InputInit%(%)'),'Missing camera section')
local Camera=assert(load(camera_source..'\nreturn Camera','@'..tutorial))()
-- Native math uses 32-bit floats.
local function near(a,b) assert(math.abs(a-b)<1e-5,string.format('%.12f != %.12f',a,b)) end
local camera=Camera.new(hg.Vec3(-1,-1,-1),hg.Vec3(1,1,1),0,0)
local drag={active=true,orbit=true,drag_valid=true,x=150,y=100,dx=50,dy=0,width=200,height=200,wheel=0}
camera:update(drag)
local r,u,f=camera:basis()
near(f.x,.5); near(f.y,0); near(f.z,math.sqrt(.75))
near(camera.eye.x,-1.7); near(camera.eye.z,-3.4*math.sqrt(.75))
-- Reverse the same sphere path: no drift or radius change.
drag.x,drag.dx=100,-50; camera:update(drag)
near(camera.eye.x,0); near(camera.eye.z,-3.4)
-- Focus loss must ignore rotate, zoom, pan and reset.
drag.active=false; drag.wheel=100; drag.reset=true; camera:update(drag)
near(camera.eye.z,-3.4); near(camera.radius,3.4)
camera:update({active=true,wheel=1000}); near(camera.radius,.15)
camera:update({active=true,wheel=-1000}); near(camera.radius,50)
camera:reset()
camera:update({active=true,pan=true,drag_valid=true,dx=10,dy=0,height=200,wheel=0})
near(camera.target.x,-10*2*3.4*math.tan(math.pi/6)/200)
near(camera.eye.x,camera.target.x)
-- Quaternion normalization keeps the camera orthonormal during extended dragging.
drag.active=true; drag.reset=false; drag.wheel=0; drag.x=140; drag.y=130; drag.dx=3; drag.dy=-2
for _=1,10000 do camera:update(drag) end
r,u,f=camera:basis()
for _,v in ipairs({r,u,f}) do near(hg.Len2(v),1) end
near(hg.Dot(r,u),0); near(hg.Dot(r,f),0); near(hg.Dot(u,f),0)
camera:reset(); near(camera.eye.x,0); near(camera.eye.z,-3.4)
-- Preserve the original yaw/pitch signs and composition order.
local posed=Camera.new(nil,nil,.4,-.18)
local _,_,front=posed:basis()
near(front.x,math.sin(.4)*math.cos(-.18))
near(front.y,math.sin(-.18))
near(front.z,math.cos(.4)*math.cos(-.18))
-- An exactly antipodal drag must leave the orientation valid.
camera:update({active=true,orbit=true,drag_valid=true,x=200,y=100,dx=200,dy=0,width=200,height=200})
near(camera.eye.x,0); near(camera.eye.z,-3.4)
print('PASS native trackball: known rotation, reversible drag, focus gate, zoom bounds, pan, reset, orthonormality, yaw/pitch and antipodal drag')
