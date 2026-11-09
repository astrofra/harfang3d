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
    programs.sort_local = hg.LoadComputeProgramFromFile(assets .. '/core/gaussian_splatting/sort_local.sc')
    programs.gather = hg.LoadComputeProgramFromFile(assets .. '/core/gaussian_splatting/gather.sc')
    programs.encode = hg.LoadComputeProgramFromFile(assets .. '/encode.sc')
    programs.display = hg.LoadProgramFromFile(assets .. '/display')
    for _, name in ipairs({'radix_histogram', 'radix_prefix', 'radix_scatter'}) do
        programs[name] = hg.LoadComputeProgramFromFile(assets .. '/core/gaussian_splatting/' .. name .. '.sc')
    end
    local cases = assert(io.open(assets .. '/cases.txt'))
    for line in cases:lines() do
        local case, count, actual = line:match('(%S+) (%d+) (%d+)')
        count, actual = tonumber(count), tonumber(actual)
        local height = math.max(8, math.ceil(count / 4096) * 8)
        local input = hg.LoadComputeBufferFromFile(assets .. '/' .. case .. '_input.bin')
        local projected = hg.LoadComputeBufferFromFile(assets .. '/' .. case .. '_projected.bin')
        local a, b = hg.CreateComputeBuffer(count), hg.CreateComputeBuffer(count)
        local blocks = math.ceil(count / 256)
        local histogram = hg.CreateComputeBuffer(blocks * 16, hg.CBT_UInt4)
        local prefixes = hg.CreateComputeBuffer(blocks * 16, hg.CBT_UInt4)
        for _, buffer in ipairs({input, projected, a, b, histogram, prefixes}) do buffers[#buffers + 1] = buffer end
        local f = assert(io.open(assets .. '/' .. case .. '_expected.bin', 'rb'))
        local expected = f:read('a'); f:close()
        assert(#expected == count * 8)
        local flags = hg.TF_ComputeWrite | hg.TF_SamplerMinPoint | hg.TF_SamplerMagPoint | hg.TF_UClamp | hg.TF_VClamp
        local function atlas(name)
            local texture = hg.CreateTexture(512, height, name, flags, hg.TF_RGBA32F)
            textures[#textures + 1] = texture
            return texture
        end
        local model = hg.CreateComputeQuadModel(1); res:AddModel('quad-' .. case, model)
        local function capture(view, texture, name)
            local color = hg.CreateTexture(512, height, name, hg.TF_RenderTarget, hg.TF_RGBA8)
            local depth = hg.CreateTexture(512, height, name .. '-depth', hg.TF_RenderTarget, hg.TF_D24)
            local readback = hg.CreateTexture(512, height, name .. '-readback', hg.TF_ReadBack | hg.TF_BlitDestination, hg.TF_RGBA8)
            textures[#textures + 1] = depth
            textures[#textures + 1] = readback
            local ref = res:AddTexture(name, color)
            local fb = hg.CreateFrameBuffer(color, depth, name)
            framebuffers[#framebuffers + 1] = fb
            hg.SetViewFrameBuffer(view, fb.handle); hg.SetViewRect(view, 0, 0, 512, height)
            hg.SetViewClear(view, hg.CF_Color, hg.Color(0, 0, 0, 0))
            hg.DrawModel(view, model, programs.display, {}, {hg.MakeUniformSetTexture('s_atlas', texture, 0)},
                hg.Mat4.Identity, hg.ComputePremultipliedRenderState())
            local picture = hg.Picture(512, height, hg.PF_RGBA32)
            local ready = hg.CaptureTexture(view + 1, res, ref, readback, picture)
            return {picture = picture, ready = ready}
        end
        for _, optimized in ipairs({false, true}) do
            for _, compact in ipairs({false, true}) do
                local view, current, output = 0, input, a
                local function dispatch(program, x, y, values, images, bindings)
                    -- DispatchCompute returns the next view; deliberately reuse view zero
                    -- in compact mode, just as native DispatchComputeSameView does.
                    local ok, next_view = hg.DispatchCompute(view, program, x, y, 1, values, {}, images, bindings)
                    assert(ok and next_view == view + 1)
                    if not compact then view = next_view end
                end
                local function flip()
                    current = output; output = output == a and b or a
                end
                if optimized and count > 256 then
                    for shift = 0, 28, 4 do
                        local values = {hg.MakeUniformSetValue('u_radix', hg.Vec4(shift, count, blocks, 0))}
                        dispatch(programs.radix_histogram, blocks, 1, values, {},
                            {hg.MakeComputeBufferBinding(current, 0, hg.AC_Read), hg.MakeComputeBufferBinding(histogram, 1, hg.AC_Write)})
                        dispatch(programs.radix_prefix, 16, 1, values, {},
                            {hg.MakeComputeBufferBinding(histogram, 0, hg.AC_Read), hg.MakeComputeBufferBinding(prefixes, 1, hg.AC_Write)})
                        dispatch(programs.radix_scatter, blocks, 1, values, {},
                            {hg.MakeComputeBufferBinding(current, 0, hg.AC_Read), hg.MakeComputeBufferBinding(prefixes, 1, hg.AC_Read),
                             hg.MakeComputeBufferBinding(output, 2, hg.AC_Write)})
                        flip()
                    end
                elseif optimized and count > 1 then
                    dispatch(programs.sort_local, 1, 1, {hg.MakeUniformSetValue('u_sort', hg.Vec4(0, 0, count, 1))}, {},
                        {hg.MakeComputeBufferBinding(current, 0, hg.AC_Read), hg.MakeComputeBufferBinding(output, 1, hg.AC_Write)})
                    flip()
                elseif not optimized then
                    local k = 2
                    while k <= count do
                        local stride = k // 2
                        while stride >= 1 do
                            dispatch(programs.sort, math.ceil(count / 64), 1,
                                {hg.MakeUniformSetValue('u_sort', hg.Vec4(stride, k, count, 0))}, {},
                                {hg.MakeComputeBufferBinding(current, 0, hg.AC_Read), hg.MakeComputeBufferBinding(output, 1, hg.AC_Write)})
                            flip()
                            stride = stride // 2
                        end
                        k = k * 2
                    end
                end
                local ids, center, basis, color = atlas('IDs'), atlas('center'), atlas('basis'), atlas('color')
                dispatch(programs.gather, 64, height // 8,
                    {hg.MakeUniformSetValue('u_counts', hg.Vec4(actual, count, 512, height))},
                    {hg.MakeImageBinding(center, 2, hg.AC_Write), hg.MakeImageBinding(basis, 3, hg.AC_Write),
                     hg.MakeImageBinding(color, 4, hg.AC_Write)},
                    {hg.MakeComputeBufferBinding(projected, 0, hg.AC_Read), hg.MakeComputeBufferBinding(current, 1, hg.AC_Read)})
                dispatch(programs.encode, 64, height // 8, {hg.MakeUniformSetValue('u_encode', hg.Vec4(count, 0, 0, 0))}, {hg.MakeImageBinding(ids, 1, hg.AC_Write)},
                    {hg.MakeComputeBufferBinding(current, 0, hg.AC_Read)})
                if compact then view = 1 end
                local encoded = capture(view, ids, 'encoded-' .. tostring(compact) .. '-' .. tostring(optimized) .. '-' .. case)
                local gathered = capture(view + 2, center, 'gathered-' .. tostring(compact) .. '-' .. tostring(optimized) .. '-' .. case)
                local ready = math.max(encoded.ready, gathered.ready)
                local frame = hg.Frame()
                while frame < ready do hg.UpdateWindow(win); frame = hg.Frame() end
                for i = 0, count - 1 do
                    local want, visible = string.unpack('<I4I4', expected, i * 8 + 1)
                    local function check(picture, id, alpha, label)
                        local c = picture:GetPixelRGBA(i % 512, i // 512)
                        local got = math.floor(c.r * 255 + .5) + math.floor(c.g * 255 + .5) * 256 + math.floor(c.b * 255 + .5) * 65536
                        assert(got == id and math.abs(c.a - alpha) < 1 / 255,
                            string.format('%s compact=%s slot %d: got %d expected %d', label, tostring(compact), i, got, id))
                    end
                    check(encoded.picture, want, 1, 'sort')
                    local valid = visible ~= 0
                    check(gathered.picture, valid and want or 0, valid and 1 or 0, 'gather')
                end
                print('PASS GPU sort/gather: ' .. case .. '; compact=' .. tostring(compact) .. '; optimized=' .. tostring(optimized))
            end
        end
        -- Each case is complete and read back; release its resources before the next allocation.
        for _, fb in ipairs(framebuffers) do hg.DestroyFrameBuffer(fb) end
        framebuffers = {}
        res:DestroyAllModels(); res:DestroyAllTextures()
        for _, t in ipairs(textures) do hg.DestroyTexture(t) end
        textures = {}
        for _, b in ipairs(buffers) do hg.DestroyComputeBuffer(b) end
        buffers = {}
    end
    cases:close()
end
local ok, err = xpcall(main, debug.traceback)
for _, fb in ipairs(framebuffers) do hg.DestroyFrameBuffer(fb) end
res:DestroyAllModels(); res:DestroyAllTextures()
for _, t in ipairs(textures) do hg.DestroyTexture(t) end
for _, b in ipairs(buffers) do hg.DestroyComputeBuffer(b) end
for _, p in pairs(programs) do hg.DestroyProgram(p) end
hg.RenderShutdown(); hg.DestroyWindow(win); hg.WindowSystemShutdown()
if not ok then error(err) end
print('PASS GPU sort: reference and optimized, expanded and compact match CPU order (1..2097152 slots, float bits, ties, culling and padding)')
