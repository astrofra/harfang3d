-- Saari (compiled from SPZ through assetc): native Gaussian loading, SH evaluation, projection, sorting and drawing.
-- Drag left to orbit, wheel to dolly, drag right/middle to pan, R to reset.
-- Keys 1..4 select SH degree 0..3. Escape closes the window.
local lua_dir = (arg[-1] or ''):match('^(.*[/\\])')
if lua_dir then package.cpath = lua_dir .. '?.dll;' .. lua_dir .. '?.so;' .. package.cpath end
local hg = require('harfang')
assert(hg.SubmitGaussianSplats, 'Rebuild HARFANG with the native Gaussian API first')
-- Pure Lua camera math. Quaternion arcball permits free rotation, including roll.
local Camera={}; Camera.__index=Camera
local function clamp(x,a,b) return math.max(a,math.min(b,x)) end
local function normalize(q)
    local n=math.sqrt(q[1]^2+q[2]^2+q[3]^2+q[4]^2)
    if n<1e-12 then return {1,0,0,0} end
    return {q[1]/n,q[2]/n,q[3]/n,q[4]/n}
end
local function multiply(a,b)
    return normalize({a[1]*b[1]-a[2]*b[2]-a[3]*b[3]-a[4]*b[4],
        a[1]*b[2]+a[2]*b[1]+a[3]*b[4]-a[4]*b[3],
        a[1]*b[3]-a[2]*b[4]+a[3]*b[1]+a[4]*b[2],
        a[1]*b[4]+a[2]*b[3]-a[3]*b[2]+a[4]*b[1]})
end
local function rotate(q,x,y,z)
    local tx,ty,tz=2*(q[3]*z-q[4]*y),2*(q[4]*x-q[2]*z),2*(q[2]*y-q[3]*x)
    return {x+q[1]*tx+q[3]*tz-q[4]*ty,y+q[1]*ty+q[4]*tx-q[2]*tz,z+q[1]*tz+q[2]*ty-q[3]*tx}
end
local function orientation(yaw,pitch)
    return multiply({math.cos(yaw/2),0,math.sin(yaw/2),0},{math.cos(pitch/2),-math.sin(pitch/2),0,0})
end
local function sphere(x,y,w,h)
    local r=math.max(1,math.min(w,h)*0.5)
    x,y=(x-w*.5)/r,(y-h*.5)/r
    local d=x*x+y*y
    if d>1 then return {x/math.sqrt(d),y/math.sqrt(d),0} end
    return {x,y,-math.sqrt(1-d)}
end
function Camera.new(bounds_min,bounds_max,yaw,pitch)
    local lo,hi=bounds_min or {-1,-1,-1},bounds_max or {1,1,1}
    local c=setmetatable({center={(lo[1]+hi[1])*.5,(lo[2]+hi[2])*.5,(lo[3]+hi[3])*.5},
        initial_radius=math.max(hi[1]-lo[1],hi[2]-lo[2],hi[3]-lo[3])*1.7,
        initial_yaw=yaw or .4,initial_pitch=pitch or -.18},Camera)
    c:reset(); return c
end
function Camera:basis() return rotate(self.q,1,0,0),rotate(self.q,0,1,0),rotate(self.q,0,0,1) end
function Camera:orbit_eye()
    local _,_,f=self:basis()
    self.eye={self.target[1]-f[1]*self.radius,self.target[2]-f[2]*self.radius,self.target[3]-f[3]*self.radius}
end
function Camera:reset()
    self.radius=math.max(.5,self.initial_radius)
    self.target={table.unpack(self.center)}
    self.yaw,self.pitch=self.initial_yaw,self.initial_pitch
    self.q=orientation(self.yaw,self.pitch); self:orbit_eye()
end
function Camera:set_pose(eye,yaw,pitch)
    self.eye=eye; self.q=orientation(yaw,pitch)
end
function Camera:update(input)
    if input.active then
        if input.reset then self:reset() end
        if input.orbit and input.drag_valid then
            local a=sphere(input.x-input.dx,input.y-input.dy,input.width,input.height)
            local b=sphere(input.x,input.y,input.width,input.height)
            local dot=clamp(a[1]*b[1]+a[2]*b[2]+a[3]*b[3],-1,1)
            -- Inverse object rotation: move the camera around the target.
            local delta={1+dot,b[2]*a[3]-b[3]*a[2],b[3]*a[1]-b[1]*a[3],b[1]*a[2]-b[2]*a[1]}
            if dot>-.999999 then self.q=multiply(self.q,normalize(delta)) end
        end
        self.radius=clamp(self.radius*math.exp(-(input.wheel or 0)*.12),.15,50)
        if input.pan and input.drag_valid then
            local r,u=self:basis()
            local scale=2*self.radius*math.tan(math.pi/6)/math.max(input.height,1)
            for j=1,3 do self.target[j]=self.target[j]-(r[j]*input.dx+u[j]*input.dy)*scale end
        end
        self:orbit_eye()
    end
end

hg.InputInit()
hg.WindowSystemInit()
local windows = package.config:sub(1, 1) == '\\'
local assets = 'resources_compiled'
local win = hg.RenderInit('HARFANG - Saari native Gaussian splatting', 1280, 720,
    windows and hg.RT_Direct3D11 or hg.RT_Metal)
assert(win, 'RenderInit failed')
local renderer, data, model

local function main()
    renderer = hg.CreateGaussianSplatRendererFromFile(assets .. '/core/gaussian_splatting')
    assert(hg.IsValid(renderer), 'Missing compiled Gaussian shaders; compile resources with assetc first')
    local loaded = hg.LoadGaussianSplatModelFromFile(assets .. '/gaussian_splatting_native/saari-probe.hgs')
    assert(loaded.ok, loaded.message)
    model = loaded.model
    data = hg.CreateGaussianSplatRenderData()
    local instance = hg.GaussianSplatInstance(model)
    instance.model, instance.id = model, 1
    -- The preservation capture uses Z up; rotate the whole instance, including SH.
    instance.world = hg.RotationMat4(hg.Vec3(-math.pi / 2, 0, 0))
    local instances = {instance}
    local lo, hi = hg.GetGaussianSplatBoundsMin(model), hg.GetGaussianSplatBoundsMax(model)
    local camera = Camera.new({lo.x, lo.z, -hi.y}, {hi.x, hi.z, -lo.y})
    -- Saari includes a distant sky shell. Start inside it, looking at the island.
    camera.center = {0, -0.35, 0}
    camera.initial_radius, camera.initial_pitch = 0.85, -0.5
    camera:reset()
    local settings = hg.GaussianSplatRenderSettings()
    local keyboard, mouse = hg.Keyboard(), hg.Mouse()
    local width, height, was_focused = 0, 0, false
    print(string.format('Saari: %d / %d splats; stored SH degree %d', hg.GetGaussianSplatCount(model),
        hg.GetGaussianSplatSourceCount(model), hg.GetGaussianSplatSHDegree(model)))

    while hg.IsWindowOpen(win) do
        keyboard:Update()
        mouse:Update()
        local focused = hg.WindowHasFocus(win)
        if focused and keyboard:Down(hg.K_Escape) then break end
        local sized, w, h = hg.GetWindowFrameBufferSize(win)
        if sized and w > 0 and h > 0 then
            if w ~= width or h ~= height then
                width, height = w, h
                hg.RenderReset(width, height, 0)
            end
            local client_ok, client_w, client_h = hg.GetWindowClientSize(win)
            assert(client_ok)
            camera:update({active = focused, reset = keyboard:Pressed(hg.K_R),
                orbit = mouse:Down(hg.MB_0), pan = mouse:Down(hg.MB_1) or mouse:Down(hg.MB_2),
                drag_valid = was_focused and not mouse:Pressed(hg.MB_0) and not mouse:Pressed(hg.MB_1)
                    and not mouse:Pressed(hg.MB_2),
                x = mouse:X(), y = mouse:Y(), dx = mouse:DtX(), dy = mouse:DtY(),
                width = client_w, height = client_h, wheel = mouse:Wheel()})
            if focused then
                for degree, key in ipairs({hg.K_1, hg.K_2, hg.K_3, hg.K_4}) do
                    if keyboard:Pressed(key) then settings.max_sh_degree = degree - 1 end
                end
            end
            local _, up, forward = camera:basis()
            local camera_world = hg.Mat4LookTowardUp(hg.Vec3(table.unpack(camera.eye)),
                hg.Vec3(table.unpack(forward)), hg.Vec3(table.unpack(up)))
            local view_state = hg.ComputePerspectiveViewState(camera_world, math.pi / 3, 0.01, 100,
                hg.ComputeAspectRatioX(width, height))
            -- Clearing belongs to the application; the native renderer preserves the target.
            hg.SetViewRect(0, 0, 0, width, height)
            hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color(0.025, 0.03, 0.04, 1), 1)
            hg.Touch(0)
            local result, next_view = hg.SubmitGaussianSplats(1, hg.IntRect(0, 0, width, height),
                view_state, instances, renderer, data, settings)
            assert(result.ok, result.message)
            hg.Frame()
        else
            hg.Sleep(hg.time_from_ms(10))
        end
        was_focused = focused
        hg.UpdateWindow(win)
    end
end

local ok, message = xpcall(main, debug.traceback)
if not ok then io.stderr:write(tostring(message) .. '\n') end
if data then hg.DestroyGaussianSplatRenderData(data) end
if renderer then hg.DestroyGaussianSplatRenderer(renderer) end
if model then hg.DestroyGaussianSplatModel(model) end
hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
hg.InputShutdown()
if not ok then error(message) end
