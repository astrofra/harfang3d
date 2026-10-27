-- Numerical checks for the interactive camera, independent of rendering/input devices.
local Camera=dofile(assert(arg[1]))
local function near(a,b) assert(math.abs(a-b)<1e-9,string.format('%.12f != %.12f',a,b)) end
local camera=Camera.new({-1,-1,-1},{1,1,1},0,0)
local drag={active=true,orbit=true,drag_valid=true,x=150,y=100,dx=50,dy=0,width=200,height=200,wheel=0}
camera:update(drag)
local r,u,f=camera:basis()
near(f[1],.5); near(f[2],0); near(f[3],math.sqrt(.75))
near(camera.eye[1],-1.7); near(camera.eye[3],-3.4*math.sqrt(.75))
-- Reverse the same sphere path: no drift or radius change.
drag.x,drag.dx=100,-50; camera:update(drag)
near(camera.eye[1],0); near(camera.eye[3],-3.4)
-- Focus loss must ignore rotate, zoom, pan and reset.
drag.active=false; drag.wheel=100; drag.reset=true; camera:update(drag)
near(camera.eye[3],-3.4); near(camera.radius,3.4)
camera:update({active=true,wheel=1000}); near(camera.radius,.15)
camera:update({active=true,wheel=-1000}); near(camera.radius,50)
camera:reset()
camera:update({active=true,pan=true,drag_valid=true,dx=10,dy=0,height=200,wheel=0})
near(camera.target[1],-10*2*3.4*math.tan(math.pi/6)/200)
near(camera.eye[1],camera.target[1])
-- Quaternion normalization keeps the camera orthonormal during extended dragging.
drag.active=true; drag.reset=false; drag.wheel=0; drag.x=140; drag.y=130; drag.dx=3; drag.dy=-2
for _=1,10000 do camera:update(drag) end
r,u,f=camera:basis()
for _,v in ipairs({r,u,f}) do near(v[1]^2+v[2]^2+v[3]^2,1) end
near(r[1]*u[1]+r[2]*u[2]+r[3]*u[3],0)
near(r[1]*f[1]+r[2]*f[2]+r[3]*f[3],0)
camera:reset(); near(camera.eye[1],0); near(camera.eye[3],-3.4)
print('PASS trackball: known rotation, reversible drag, focus gate, zoom bounds, pan, reset and orthonormality')
