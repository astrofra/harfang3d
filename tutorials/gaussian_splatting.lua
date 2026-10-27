-- Lua/Metal Gaussian splatting with an orbital trackball: see GAUSSIAN_SPLATTING.md for build and controls.
local opt = {preset='full', renderer='metal', frames=0, width=1280, height=720}
local i=1
while i<=#arg do
    local key=arg[i]:match('^%-%-(.+)$'); assert(key,'Expected --option, got '..arg[i])
    if key=='route' or key=='resize-test' then opt[key]=true else i=i+1; opt[key]=assert(arg[i],'Missing value for '..key) end
    i=i+1
end
assert(opt.assets and opt.module, '--assets COMPILED_DIRECTORY and --module MODULE_DIRECTORY are required')
package.cpath=opt.module..'/?.so;'..opt.module..'/?.dll;'..package.cpath
local hg=require('harfang')
local script_dir=arg[0]:match('^(.*[/\\])') or './'
local Camera=dofile(script_dir..'gaussian_splatting/camera.lua')
local draw_gizmo=dofile(script_dir..'gaussian_splatting/gizmo.lua')(hg)
local metadata=assert(loadfile(opt.assets..'/metadata.luadata'))()
assert(metadata.version==1,'Unsupported metadata version')
local preset=assert(metadata.presets[opt.preset],'Unknown preset')
local file=assert(io.open(opt.assets..'/'..preset.file,'rb'))
local header=assert(file:read(64),'Missing HGS0 header'); file:close()
local magic,version,stored_count,stride=string.unpack('<c4I4I4I4',header)
assert(magic=='HGS0' and version==1 and stored_count==preset.count and stride==64 and preset.offset==64,'Invalid HGS0 dataset header')
local count,padded=preset.count,1
while padded<count do padded=padded*2 end
assert(count<=1024*1024,'Tutorial limit is one million Gaussians')
local atlas_w=512
local atlas_h=math.max(8,math.ceil(padded/atlas_w/8)*8)
local frames=tonumber(opt.frames)
hg.InputInit(); hg.WindowSystemInit()
assert(opt.renderer=='metal' or opt.renderer=='dx11','This tutorial currently qualifies Metal; dx11 is experimental')
local win=hg.RenderInit('HARFANG Gaussian splatting',tonumber(opt.width),tonumber(opt.height),opt.renderer=='metal' and hg.RT_Metal or hg.RT_Direct3D11)
assert(win,'RenderInit failed')
local resources=hg.PipelineResources()
local buffers,texture_objects,programs={},{},{}
local capture_fb,capture_depth
local function buffer(b) assert(hg.IsValid(b),'Compute allocation failed'); buffers[#buffers+1]=b; return b end
local function texture(w,h,name,flags,format)
    local t=hg.CreateTexture(w,h,name,flags,format); texture_objects[#texture_objects+1]=t; return t
end
local function value(name,x,y,z,w) return hg.MakeUniformSetValue(name,hg.Vec4(x,y,z,w)) end
local function binding(b,stage,access) return hg.MakeComputeBufferBinding(b,stage,access) end
local function main()
    local caps=hg.GetComputeCaps()
    assert(caps.supported and caps.rgba32f_vertex_sampling,'Compute / RGBA32F vertex sampling unavailable')
    assert(caps.max_bindings>=5 and caps.max_views>=160 and caps.max_texture_size>=atlas_w,'Insufficient device limits')
    assert(caps.rgba32f_image_write or (caps.renderer=='Metal' and not caps.image_write_reporting_complete),'RGBA32F image writes unavailable')
    for _,name in ipairs({'project','sort','gather'}) do programs[name]=hg.LoadComputeProgramFromFile(opt.assets..'/shaders/'..name..'.sc') end
    programs.splat=hg.LoadProgramFromFile(opt.assets..'/shaders/splat')
    programs.gizmo=hg.LoadProgramFromFile(opt.assets..'/shaders/gizmo')
    local source=buffer(hg.LoadComputeBufferFromFile(opt.assets..'/'..preset.file,hg.CBT_Float4,preset.offset))
    assert(hg.GetComputeBufferCount(source)==count*4,'Dataset size disagrees with metadata')
    local projected=buffer(hg.CreateComputeBuffer(count*3))
    local keys={buffer(hg.CreateComputeBuffer(padded)),buffer(hg.CreateComputeBuffer(padded))}
    local flags=hg.TF_ComputeWrite|hg.TF_SamplerMinPoint|hg.TF_SamplerMagPoint|hg.TF_UClamp|hg.TF_VClamp
    local center=texture(atlas_w,atlas_h,'splat-center',flags,hg.TF_RGBA32F)
    local basis=texture(atlas_w,atlas_h,'splat-basis',flags,hg.TF_RGBA32F)
    local color=texture(atlas_w,atlas_h,'splat-color',flags,hg.TF_RGBA32F)
    local counts=value('u_counts',count,padded,atlas_w,atlas_h)
    local draw_values={counts}
    local draw_textures={hg.MakeUniformSetTexture('s_center',center,0),hg.MakeUniformSetTexture('s_basis',basis,1),hg.MakeUniformSetTexture('s_color',color,2)}
    local images={hg.MakeImageBinding(center,2,hg.AC_Write),hg.MakeImageBinding(basis,3,hg.AC_Write),hg.MakeImageBinding(color,4,hg.AC_Write)}
    local project_buffers={binding(source,0,hg.AC_Read),binding(projected,1,hg.AC_Write),binding(keys[1],2,hg.AC_Write)}
    -- Prebuild all sorting uniforms and bindings: no per-Gaussian Lua work.
    local passes={}
    local current=1
    local k=2
    while k<=padded do
        local stride=k//2
        while stride>=1 do
            local other=3-current
            passes[#passes+1]={values={value('u_sort',stride,k,padded,0)},buffers={binding(keys[current],0,hg.AC_Read),binding(keys[other],1,hg.AC_Write)}}
            current=other; stride=stride//2
        end
        k=k*2
    end
    assert(#passes+4<caps.max_views,'Sort exceeds view budget')
    local gather_buffers={binding(projected,0,hg.AC_Read),binding(keys[current],1,hg.AC_Read)}
    local model=hg.CreateComputeQuadModel(count)
    resources:AddModel('splat-quads',model)
    local state=hg.ComputePremultipliedRenderState()
    local keyboard,mouse=hg.Keyboard(),hg.Mouse()
    local camera=Camera.new(metadata.bounds_min,metadata.bounds_max,tonumber(opt.yaw),tonumber(opt.pitch))
    local camera_right,camera_up,camera_forward=camera:basis()
    local width,height=0,0
    local last=hg.time_now()
    local timings={}
    local cpu_times={}
    local gpu_times={}
    local last_gpu_timestamp=0
    local draw_calls,compute_calls=0,0
    local n=0
    local resized=0
    local was_focused=false
    local gpu_bytes=172*count+32*padded+48*atlas_w*atlas_h
    print(string.format('Estimated persistent GPU payload: %.2f MiB (excludes driver allocations)',gpu_bytes/1048576))
    local function dispatch(view,program,values,imgs,bufs,x,y)
        local ok,next_view=hg.DispatchCompute(view,program,x or math.ceil(padded/64),y or 1,1,values,{},imgs or {},bufs)
        assert(ok,'Compute dispatch rejected'); return next_view
    end
    local function draw(view)
        hg.SetViewRect(view,0,0,width,height)
        hg.SetViewClear(view,hg.CF_Color,hg.Color(0.025,0.03,0.04,1))
        hg.SetViewMode(view,hg.VM_Sequential)
        hg.DrawModel(view,model,programs.splat,draw_values,draw_textures,hg.Mat4.Identity,state)
        draw_gizmo(view,programs.gizmo,state,width,height,camera_right,camera_up)
    end
    while hg.IsWindowOpen(win) and (frames==0 or n<frames) do
        keyboard:Update(); mouse:Update()
        local focused=hg.WindowHasFocus(win)
        local active=focused and not opt.route
        if focused and keyboard:Down(hg.K_Escape) then break end
        if opt['resize-test'] then
            if n==30 then assert(hg.SetWindowClientSize(win,tonumber(opt.width)+80,tonumber(opt.height)+40)) end
            if n==60 then assert(hg.SetWindowClientSize(win,tonumber(opt.width),tonumber(opt.height))) end
        end
        local now=hg.time_now()
        local elapsed=hg.time_to_sec_f(now-last); last=now
        local sized,w,h=hg.GetWindowFrameBufferSize(win)
        assert(sized,'Unable to query drawable size')
        if w>0 and h>0 then
            if w~=width or h~=height then
                width,height=w,h
                resized=resized+1
                hg.RenderReset(width,height,0)
                print(string.format('Drawable %dx%d; %s; %d splats; %d bitonic passes',width,height,caps.renderer,count,#passes))
            end
            local client_ok,client_w,client_h=hg.GetWindowClientSize(win)
            assert(client_ok)
            camera:update({active=active,reset=keyboard:Pressed(hg.K_R),
                orbit=mouse:Down(hg.MB_0),pan=mouse:Down(hg.MB_1) or mouse:Down(hg.MB_2),
                drag_valid=was_focused and not mouse:Pressed(hg.MB_0) and not mouse:Pressed(hg.MB_1) and not mouse:Pressed(hg.MB_2),
                x=mouse:X(),y=mouse:Y(),dx=mouse:DtX(),dy=mouse:DtY(),width=client_w,height=client_h,wheel=mouse:Wheel()})
            if opt.route then
                local t=n/math.max(frames-1,1)
                local angle=t*math.pi*2
                local radius=3.4-1.8*math.sin(angle)^8
                local py=0.5*math.sin(angle*2)
                local yaw,pitch=-angle,-math.atan(py/radius)
                if t>0.82 and t<0.90 then yaw=yaw+math.pi*math.sin((t-0.82)/0.08*math.pi)^2 end
                camera:set_pose({math.sin(angle)*radius,py,-math.cos(angle)*radius},yaw,pitch)
            end
            camera_right,camera_up,camera_forward=camera:basis()
            local rx,ry,rz=table.unpack(camera_right)
            local ux,uy,uz=table.unpack(camera_up)
            local fx,fy,fz=table.unpack(camera_forward)
            local px,py,pz=table.unpack(camera.eye)
            local start=hg.time_now()
            local values={counts,value('u_eye',px,py,pz,0),value('u_right',rx,ry,rz,0),value('u_up',ux,uy,uz,0),value('u_forward',fx,fy,fz,0),value('u_screen',width,height,height/(2*math.tan(math.pi/6)),0.05)}
            local view=dispatch(0,programs.project,values,{},project_buffers)
            for _,pass in ipairs(passes) do view=dispatch(view,programs.sort,pass.values,{},pass.buffers) end
            view=dispatch(view,programs.gather,draw_values,images,gather_buffers,atlas_w//8,atlas_h//8)
            draw(view)
            -- Capture only on the last frame; no readbacks in the normal render loop.
            local ready,picture,readback
            if opt.capture and frames>0 and n==frames-1 then
                local target=hg.CreateTexture(width,height,'capture-target',hg.TF_RenderTarget,hg.TF_RGBA8)
                local ref=resources:AddTexture('capture-target',target)
                capture_depth=texture(width,height,'capture-depth',hg.TF_RenderTarget,hg.TF_D24)
                capture_fb=hg.CreateFrameBuffer(target,capture_depth,'capture')
                hg.SetViewFrameBuffer(view+1,capture_fb.handle); draw(view+1)
                readback=texture(width,height,'capture-readback',hg.TF_ReadBack|hg.TF_BlitDestination,hg.TF_RGBA8)
                picture=hg.Picture(width,height,hg.PF_RGBA32)
                ready=hg.CaptureTexture(view+2,resources,ref,readback,picture)
            end
            local cpu=hg.time_to_ms_f(hg.time_now()-start)
            local stats=hg.GetComputeFrameStats()
            draw_calls,compute_calls=stats.draw_calls,stats.compute_calls
            if n>=60 and stats.gpu_timing_available and stats.gpu_timestamp~=last_gpu_timestamp then
                gpu_times[#gpu_times+1]=stats.gpu_ms
                last_gpu_timestamp=stats.gpu_timestamp
            end
            local frame=hg.Frame()
            if ready then
                while frame<ready do hg.UpdateWindow(win); frame=hg.Frame() end
                assert(hg.SavePNG(picture,opt.capture),'Screenshot save failed')
            end
            if n>=60 then timings[#timings+1]=elapsed*1000; cpu_times[#cpu_times+1]=cpu end
            n=n+1
            if n%60==0 then hg.SetWindowTitle(win,string.format('GSplat %s | %d | %.1f ms | LMB orbit | wheel zoom | RMB pan | R reset | Esc quit',caps.renderer,count,elapsed*1000)) end
        else
            hg.Sleep(hg.time_from_ms(10))
        end
        was_focused=focused
        hg.UpdateWindow(win)
    end
    if opt['resize-test'] then assert(resized>=3,'Resize stress did not exercise both size changes') end
    local measured_ms=0; for _,ms in ipairs(timings) do measured_ms=measured_ms+ms end
    table.sort(timings); table.sort(cpu_times); table.sort(gpu_times)
    local function percentile(t,q) return #t>0 and t[math.max(1,math.ceil(#t*q))] or 0 end
    local report=string.format('{"renderer":"%s","preset":"%s","count":%d,"width":%d,"height":%d,"frames":%d,"samples":%d,"route":%s,"sort_passes":%d,"frame_ms_p50":%.4f,"frame_ms_p95":%.4f,"submit_ms_p50":%.4f,"submit_ms_p95":%.4f}',caps.renderer,opt.preset,count,width,height,n,#timings,tostring(opt.route or false),#passes,percentile(timings,0.5),percentile(timings,0.95),percentile(cpu_times,0.5),percentile(cpu_times,0.95))
    report=report:sub(1,-2)..string.format(',"gpu_samples":%d,"gpu_ms_p50":%.4f,"gpu_ms_p95":%.4f,"draw_calls":%d,"compute_calls":%d}',#gpu_times,percentile(gpu_times,0.5),percentile(gpu_times,0.95),draw_calls,compute_calls)
    report=report:sub(1,-2)..string.format(',"measured_seconds":%.3f,"estimated_gpu_bytes":%d,"resize_count":%d}',measured_ms/1000,gpu_bytes,resized)
    print(report)
    if opt.report then local f=assert(io.open(opt.report,'w')); f:write(report,'\n'); f:close() end
end
local success,err=xpcall(main,debug.traceback)
if capture_fb then hg.DestroyFrameBuffer(capture_fb) end
resources:DestroyAllModels(); resources:DestroyAllTextures(); resources:DestroyAllPrograms()
for _,b in ipairs(buffers) do hg.DestroyComputeBuffer(b) end
for _,t in ipairs(texture_objects) do hg.DestroyTexture(t) end
for _,p in pairs(programs) do hg.DestroyProgram(p) end
hg.RenderShutdown(); hg.DestroyWindow(win); hg.WindowSystemShutdown(); hg.InputShutdown()
if not success then error(err) end
print('PASS Gaussian splatting viewer: normal shutdown')
