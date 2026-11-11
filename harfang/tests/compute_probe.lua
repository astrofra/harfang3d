-- Native M0 oracle exercised through the generated Lua bindings.
local assets, module_dir, renderer = assert(arg[1]), assert(arg[2]), arg[3] or 'metal'
package.cpath = module_dir .. '/?.so;' .. module_dir .. '/?.dll;' .. package.cpath
local hg = require('harfang')
hg.WindowSystemInit()
local win = hg.RenderInit('Lua compute probe (automatic)', 320, 200,
    renderer == 'dx11' and hg.RT_Direct3D11 or hg.RT_Metal)
assert(win)

local function probe()
    local caps = hg.GetComputeCaps()
    assert(caps.supported and caps.rgba32f_vertex_sampling)
    assert(caps.renderer == (renderer == 'dx11' and 'Direct3D 11' or 'Metal'))
    local producer = hg.LoadComputeProgramFromFile(assets .. '/producer.sc')
    local consumer = hg.LoadComputeProgramFromFile(assets .. '/consumer.sc')
    local display = hg.LoadProgramFromFile(assets .. '/display_lua')
    local input = hg.LoadComputeBufferFromFile(assets .. '/input.bin')
    local scratch = hg.CreateComputeBuffer(256)
    assert(hg.IsValid(input) and hg.GetComputeBufferCount(input) == 256)
    assert(hg.IsValid(scratch))
    local flags = hg.TF_SamplerMinPoint | hg.TF_SamplerMagPoint | hg.TF_UClamp | hg.TF_VClamp
    local atlas = hg.CreateTexture(16, 16, 'probe-atlas', flags | hg.TF_ComputeWrite, hg.TF_RGBA32F)
    local color_target = hg.CreateTexture(128, 128, 'probe-target', hg.TF_RenderTarget, hg.TF_RGBA8)
    local depth = hg.CreateTexture(128, 128, 'probe-depth', hg.TF_RenderTarget, hg.TF_D24)
    local target = hg.CreateFrameBuffer(color_target, depth, 'probe-target')
    local resources = hg.PipelineResources()
    local target_ref = resources:AddTexture('probe-target', color_target)
    local readback = hg.CreateTexture(128, 128, 'probe-readback', hg.TF_ReadBack | hg.TF_BlitDestination, hg.TF_RGBA8)
    local picture = hg.Picture(128, 128, hg.PF_RGBA32)
    local model = hg.CreateComputeQuadModel(256)
    local model_ref = resources:AddModel('probe-model', model)
    local state = hg.ComputePremultipliedRenderState()
    local producer_buffers = {hg.MakeComputeBufferBinding(input, 0, hg.AC_Read), hg.MakeComputeBufferBinding(scratch, 1, hg.AC_Write)}
    local consumer_buffers = {hg.MakeComputeBufferBinding(scratch, 0, hg.AC_Read)}
    local images = {hg.MakeImageBinding(atlas, 1, hg.AC_Write)}
    local textures = {hg.MakeUniformSetTexture('s_atlas', atlas, 0)}

    -- Failure must leave the view unchanged, before binding/submitting anything.
    local ok, view = hg.DispatchCompute(0, producer, 0)
    assert(not ok and view == 0)
    ok, view = hg.DispatchCompute(0, producer, 4, 1, 1, {}, {}, {}, {hg.MakeComputeBufferBinding(input, 0, hg.AC_Write)})
    assert(not ok and view == 0)
    ok, view = hg.DispatchCompute(0, producer, 4, 1, 1, {}, {}, {}, {hg.MakeComputeBufferBinding(scratch, caps.max_bindings, hg.AC_Write)})
    assert(not ok and view == 0)
    input = nil
    collectgarbage('collect') -- Binding lists must retain the input's storage.

    for _, seed in ipairs({0, 1, 3, 7}) do
        local values = {hg.MakeUniformSetValue('u_seed', hg.Vec4(seed, 0, 0, 0))}
        ok, view = hg.DispatchCompute(0, producer, 4, 1, 1, values, {}, {}, producer_buffers, 'Lua producer')
        assert(ok and view == 1)
        ok, view = hg.DispatchCompute(view, consumer, 2, 2, 1, {}, {}, images, consumer_buffers, 'Lua consumer')
        assert(ok and view == 2)
        hg.SetViewFrameBuffer(view, target.handle)
        hg.SetViewRect(view, 0, 0, 128, 128)
        hg.SetViewClear(view, hg.CF_Color, hg.Color(0, 0, 0, 0))
        hg.SetViewMode(view, hg.VM_Sequential)
        hg.DrawModel(view, model, display, {}, textures, hg.Mat4.Identity, state)
        local ready, next_view = hg.CaptureTexture(view + 1, resources, target_ref, readback, picture)
        assert(next_view == view + 2)
        local frame = hg.Frame()
        while frame < ready do
            hg.UpdateWindow(win)
            frame = hg.Frame()
        end
        for i = 0, 255 do
            local source = (((i * 17 + 3) % 256) * 13) % 256
            local expected = {((source * 17 + 13) % 200 + 20 + seed) / 510,
                ((source * 29 + 7) % 200 + 20 + seed * 2) / 510,
                ((source * 43 + 3) % 200 + 20 + seed * 3) / 510, 0.5}
            local color = picture:GetPixelRGBA((i % 16) * 8 + 4, math.floor(i / 16) * 8 + 4)
            for channel, value in ipairs({color.r, color.g, color.b, color.a}) do
                assert(math.abs(value - expected[channel]) < 1.5 / 255, 'Lua pixel mismatch at cell ' .. i)
            end
        end
    end
    hg.DestroyComputeBuffer(scratch)
    hg.DestroyComputeBuffer(scratch) -- Idempotent, including copies retained by lists.
    ok, view = hg.DispatchCompute(0, consumer, 2, 2, 1, {}, {}, images, consumer_buffers)
    assert(not ok and view == 0)
    resources:DestroyModel(model_ref)
    hg.DestroyTexture(atlas)
    hg.DestroyTexture(readback)
    -- PipelineResources owns this texture; framebuffer must be destroyed first.
    hg.DestroyFrameBuffer(target)
    resources:DestroyAllTextures()
    hg.DestroyTexture(depth)
    hg.DestroyProgram(producer)
    hg.DestroyProgram(consumer)
    hg.DestroyProgram(display)
    return hg.CreateComputeBuffer(16)
end

local success, survivor = xpcall(probe, debug.traceback)
collectgarbage('collect')
hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
if not success then error(survivor) end
assert(not hg.IsValid(survivor), 'Shutdown must invalidate surviving userdata')
survivor = nil
collectgarbage('collect')
print('PASS Lua compute probe: pixels, premultiplied blending, validation and ownership')
