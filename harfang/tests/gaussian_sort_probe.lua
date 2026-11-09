-- Independent CPU order and projected-ID oracle, expanded and compact schedules.
local assets, module, renderer = assert(arg[1]), assert(arg[2]), assert(arg[3])
assert(renderer == 'metal' or renderer == 'dx11')
package.cpath = module .. '/?.so;' .. module .. '/?.dll;' .. package.cpath
local hg = require('harfang')
hg.WindowSystemInit()
local win = hg.RenderInit('Gaussian GPU sort oracle', 320, 200,
    renderer == 'metal' and hg.RT_Metal or hg.RT_Direct3D11)
assert(win)
assert(hg.GetComputeCaps().renderer == (renderer == 'metal' and 'Metal' or 'Direct3D 11'),
    'Renderer fallback is not valid qualification evidence')
local res = hg.PipelineResources()
local textures, buffers, programs, framebuffers = {}, {}, {}, {}
local function main()
    programs.sort = hg.LoadComputeProgramFromFile(assets .. '/core/gaussian_splatting/sort.sc')
    programs.gather = hg.LoadComputeProgramFromFile(assets .. '/core/gaussian_splatting/gather.sc')
    programs.encode = hg.LoadComputeProgramFromFile(assets .. '/encode.sc')
    programs.display = hg.LoadProgramFromFile(assets .. '/display')
    local input = hg.LoadComputeBufferFromFile(assets .. '/input.bin')
    local projected = hg.LoadComputeBufferFromFile(assets .. '/projected.bin')
    local a, b = hg.CreateComputeBuffer(131072), hg.CreateComputeBuffer(131072)
    buffers = {input, projected, a, b}
    local f = assert(io.open(assets .. '/expected.bin', 'rb'))
    local expected = f:read('a'); f:close()
    assert(#expected == 131072 * 4)
    local flags = hg.TF_ComputeWrite | hg.TF_SamplerMinPoint | hg.TF_SamplerMagPoint | hg.TF_UClamp | hg.TF_VClamp
    local function atlas(name)
        local texture = hg.CreateTexture(512, 256, name, flags, hg.TF_RGBA32F)
        textures[#textures + 1] = texture
        return texture
    end
    local model = hg.CreateComputeQuadModel(1); res:AddModel('quad', model)
    local function capture(view, texture, name)
        local color = hg.CreateTexture(512, 256, name, hg.TF_RenderTarget, hg.TF_RGBA8)
        local depth = hg.CreateTexture(512, 256, name .. '-depth', hg.TF_RenderTarget, hg.TF_D24)
        local readback = hg.CreateTexture(512, 256, name .. '-readback', hg.TF_ReadBack | hg.TF_BlitDestination, hg.TF_RGBA8)
        textures[#textures + 1] = depth
        textures[#textures + 1] = readback
        local ref = res:AddTexture(name, color)
        local fb = hg.CreateFrameBuffer(color, depth, name)
        framebuffers[#framebuffers + 1] = fb
        hg.SetViewFrameBuffer(view, fb.handle); hg.SetViewRect(view, 0, 0, 512, 256)
        hg.SetViewClear(view, hg.CF_Color, hg.Color(0, 0, 0, 0))
        hg.DrawModel(view, model, programs.display, {}, {hg.MakeUniformSetTexture('s_atlas', texture, 0)},
            hg.Mat4.Identity, hg.ComputePremultipliedRenderState())
        local picture = hg.Picture(512, 256, hg.PF_RGBA32)
        local ready = hg.CaptureTexture(view + 1, res, ref, readback, picture)
        return {picture = picture, ready = ready}
    end
    for _, compact in ipairs({false, true}) do
        local view, current, output = 0, input, a
        local function dispatch(program, x, y, values, images, bindings)
            -- DispatchCompute returns the next view; deliberately reuse view zero
            -- in compact mode, just as native DispatchComputeSameView does.
            local ok, next_view = hg.DispatchCompute(view, program, x, y, 1, values, {}, images, bindings)
            assert(ok and next_view == view + 1)
            if not compact then view = next_view end
        end
        local k = 2
        while k <= 131072 do
            local stride = k // 2
            while stride >= 1 do
                dispatch(programs.sort, 2048, 1,
                    {hg.MakeUniformSetValue('u_sort', hg.Vec4(stride, k, 131072, 0))}, {},
                    {hg.MakeComputeBufferBinding(current, 0, hg.AC_Read), hg.MakeComputeBufferBinding(output, 1, hg.AC_Write)})
                current = output; output = output == a and b or a
                stride = stride // 2
            end
            k = k * 2
        end
        local ids, center, basis, color = atlas('IDs'), atlas('center'), atlas('basis'), atlas('color')
        dispatch(programs.gather, 64, 32,
            {hg.MakeUniformSetValue('u_counts', hg.Vec4(113648, 131072, 512, 256))},
            {hg.MakeImageBinding(center, 2, hg.AC_Write), hg.MakeImageBinding(basis, 3, hg.AC_Write),
             hg.MakeImageBinding(color, 4, hg.AC_Write)},
            {hg.MakeComputeBufferBinding(projected, 0, hg.AC_Read), hg.MakeComputeBufferBinding(current, 1, hg.AC_Read)})
        dispatch(programs.encode, 64, 32, {}, {hg.MakeImageBinding(ids, 1, hg.AC_Write)},
            {hg.MakeComputeBufferBinding(current, 0, hg.AC_Read)})
        if compact then view = 1 end
        local encoded = capture(view, ids, 'encoded-' .. tostring(compact))
        local gathered = capture(view + 2, center, 'gathered-' .. tostring(compact))
        local ready = math.max(encoded.ready, gathered.ready)
        local frame = hg.Frame()
        while frame < ready do hg.UpdateWindow(win); frame = hg.Frame() end
        for i = 0, 131071 do
            local want = string.unpack('<I4', expected, i * 4 + 1)
            local function check(picture, id, alpha, label)
                local c = picture:GetPixelRGBA(i % 512, i // 512)
                local got = math.floor(c.r * 255 + .5) + math.floor(c.g * 255 + .5) * 256 + math.floor(c.b * 255 + .5) * 65536
                assert(got == id and math.abs(c.a - alpha) < 1 / 255,
                    string.format('%s compact=%s slot %d: got %d expected %d', label, tostring(compact), i, got, id))
            end
            check(encoded.picture, want, 1, 'sort')
            local valid = want < 113648 and want % 7 ~= 0 and (want * 73) % 4096 > 0
            check(gathered.picture, valid and want or 0, valid and 1 or 0, 'gather')
        end
        print('PASS GPU sort/gather: all 131072 IDs; compact=' .. tostring(compact))
    end
end
local ok, err = xpcall(main, debug.traceback)
for _, fb in ipairs(framebuffers) do hg.DestroyFrameBuffer(fb) end
res:DestroyAllModels(); res:DestroyAllTextures()
for _, t in ipairs(textures) do hg.DestroyTexture(t) end
for _, b in ipairs(buffers) do hg.DestroyComputeBuffer(b) end
for _, p in pairs(programs) do hg.DestroyProgram(p) end
hg.RenderShutdown(); hg.DestroyWindow(win); hg.WindowSystemShutdown()
if not ok then error(err) end
print('PASS GPU sort: expanded and compact sort/gather match CPU order (131072 slots, ties, culled entries and padding)')
