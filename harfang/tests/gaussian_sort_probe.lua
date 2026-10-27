local assets,module,renderer=arg[1],arg[2],arg[3]
package.cpath=module..'/?.so;'..module..'/?.dll;'..package.cpath
local hg=require('harfang')
hg.WindowSystemInit()
local win=hg.RenderInit('Gaussian GPU sort oracle (automatic)',320,200,renderer=='metal' and hg.RT_Metal or hg.RT_Direct3D11)
assert(win)
local res=hg.PipelineResources()
local textures,buffers,programs={},{},{}
local fb
local function main()
    programs.sort=hg.LoadComputeProgramFromFile(assets..'/sort.sc')
    programs.encode=hg.LoadComputeProgramFromFile(assets..'/encode.sc')
    programs.display=hg.LoadProgramFromFile(assets..'/display')
    local input=hg.LoadComputeBufferFromFile(assets..'/input.bin')
    local a,b=hg.CreateComputeBuffer(131072),hg.CreateComputeBuffer(131072)
    buffers={input,a,b}
    local current,output=input,a
    local view=0
    local k=2
    while k<=131072 do
        local stride=k//2
        while stride>=1 do
            local ok
            ok,view=hg.DispatchCompute(view,programs.sort,2048,1,1,
                {hg.MakeUniformSetValue('u_sort',hg.Vec4(stride,k,131072,0))},{},{},
                {hg.MakeComputeBufferBinding(current,0,hg.AC_Read),hg.MakeComputeBufferBinding(output,1,hg.AC_Write)})
            assert(ok)
            current=output; output=output==a and b or a
            stride=stride//2
        end
        k=k*2
    end
    local flags=hg.TF_ComputeWrite|hg.TF_SamplerMinPoint|hg.TF_SamplerMagPoint|hg.TF_UClamp|hg.TF_VClamp
    local atlas=hg.CreateTexture(512,256,'encoded IDs',flags,hg.TF_RGBA32F)
    local color=hg.CreateTexture(512,256,'sort target',hg.TF_RenderTarget,hg.TF_RGBA8)
    local depth=hg.CreateTexture(512,256,'sort depth',hg.TF_RenderTarget,hg.TF_D24)
    local readback=hg.CreateTexture(512,256,'sort readback',hg.TF_ReadBack|hg.TF_BlitDestination,hg.TF_RGBA8)
    textures={atlas,depth,readback}
    local color_ref=res:AddTexture('sort target',color)
    fb=hg.CreateFrameBuffer(color,depth,'sort target')
    local ok
    ok,view=hg.DispatchCompute(view,programs.encode,64,32,1,{},{},
        {hg.MakeImageBinding(atlas,1,hg.AC_Write)},{hg.MakeComputeBufferBinding(current,0,hg.AC_Read)})
    assert(ok)
    local model=hg.CreateComputeQuadModel(1); res:AddModel('quad',model)
    hg.SetViewFrameBuffer(view,fb.handle); hg.SetViewRect(view,0,0,512,256)
    hg.SetViewClear(view,hg.CF_Color,hg.Color(0,0,0,0))
    hg.DrawModel(view,model,programs.display,{}, {hg.MakeUniformSetTexture('s_atlas',atlas,0)},hg.Mat4.Identity,hg.ComputePremultipliedRenderState())
    local picture=hg.Picture(512,256,hg.PF_RGBA32)
    local ready=hg.CaptureTexture(view+1,res,color_ref,readback,picture)
    local frame=hg.Frame()
    while frame<ready do hg.UpdateWindow(win); frame=hg.Frame() end
    local f=assert(io.open(assets..'/expected.bin','rb')); local expected=f:read('a'); f:close()
    assert(#expected==131072*4)
    for i=0,131071 do
        local c=picture:GetPixelRGBA(i%512,i//512)
        local id=math.floor(c.r*255+.5)+math.floor(c.g*255+.5)*256+math.floor(c.b*255+.5)*65536
        local want=string.unpack('<I4',expected,i*4+1)
        assert(id==want,string.format('sort slot %d: got %d expected %d',i,id,want))
    end
end
local ok,err=xpcall(main,debug.traceback)
if fb then hg.DestroyFrameBuffer(fb) end
res:DestroyAllModels(); res:DestroyAllTextures()
for _,t in ipairs(textures) do hg.DestroyTexture(t) end
for _,b in ipairs(buffers) do hg.DestroyComputeBuffer(b) end
for _,p in pairs(programs) do hg.DestroyProgram(p) end
hg.RenderShutdown(); hg.DestroyWindow(win); hg.WindowSystemShutdown()
if not ok then error(err) end
print('PASS GPU sort: all 131072 IDs match CPU ordering (113648 records, culled entries, equal depths and padding)')
