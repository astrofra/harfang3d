-- Native Gaussian model loader, renderer, validation and ownership integration probe.
-- Positional arguments: module directory, compiled shader directory, fixture directory, dx11|metal.
local module_dir, shaders, fixture_dir, renderer_name = assert(arg[1]), assert(arg[2]), assert(arg[3]), arg[4] or 'metal'
assert(renderer_name == 'dx11' or renderer_name == 'metal', 'renderer must be dx11 or metal')
package.cpath = module_dir .. '/?.so;' .. module_dir .. '/?.dll;' .. package.cpath
local hg = require('harfang')

hg.WindowSystemInit()
local win = hg.RenderInit('Native Gaussian splat probe (automatic)', 320, 200,
    renderer_name == 'dx11' and hg.RT_Direct3D11 or hg.RT_Metal)
assert(win, 'RenderInit failed')
assert(hg.GetComputeCaps().renderer == (renderer_name == 'metal' and 'Metal' or 'Direct3D 11'),
    'Renderer fallback is not valid qualification evidence')

local resources = hg.PipelineResources()
local targets, models, renderers, render_data_handles = {}, {}, {}, {}
local mesh_program
local rect = hg.IntRect(0, 0, 64, 64)
local black = {0, 0, 0, 0}
local sentinel = {0.125, 0.25, 0.375, 0.5}

local function make_target(name)
    local color = hg.CreateTexture(64, 64, name .. '-color', hg.TF_RenderTarget, hg.TF_RGBA8)
    local depth = hg.CreateTexture(64, 64, name .. '-depth', hg.TF_RenderTarget, hg.TF_D24)
    local framebuffer = hg.CreateFrameBuffer(color, depth, name)
    local readback = hg.CreateTexture(64, 64, name .. '-readback',
        hg.TF_ReadBack | hg.TF_BlitDestination, hg.TF_RGBA8)
    assert(hg.IsValid(color) and hg.IsValid(depth) and hg.IsValid(framebuffer) and hg.IsValid(readback),
        'Target allocation failed: ' .. name)
    local target = {
        color = color, depth = depth, fb = framebuffer, readback = readback,
        ref = resources:AddTexture(name .. '-color', color), picture = hg.Picture(64, 64, hg.PF_RGBA32),
    }
    targets[#targets + 1] = target
    return target
end

local function clear_target(target, view, color, depth)
    hg.SetViewFrameBuffer(view, target.fb.handle)
    hg.SetViewRect(view, 0, 0, 64, 64)
    hg.SetViewClear(view, hg.CF_Color | hg.CF_Depth,
        hg.Color(color[1], color[2], color[3], color[4]), depth)
    hg.SetViewMode(view, hg.VM_Sequential)
    hg.Touch(view) -- A clear alone does not force bgfx to process a view.
end

local function make_settings()
    local settings = hg.GaussianSplatRenderSettings()
    settings.max_sh_degree = 3
    settings.view_limit = 256
    settings.max_splats = 1048576
    settings.depth_test = true
    settings.compact_views = true
    settings.max_pixel_radius = 256
    return settings
end

local function make_instance(model, id, world, opacity)
    local instance = hg.GaussianSplatInstance(model)
    instance.model = model
    instance.world = world or hg.Mat4.Identity
    instance.id = id
    instance.opacity = opacity or 1
    return instance
end

local function make_view_state(world)
    return hg.ComputePerspectiveViewState(world or hg.Mat4.Identity, hg.Deg(60), 0.05, 100, hg.Vec2(1, 1))
end

local function load_model(name)
    local result = hg.LoadGaussianSplatModelFromFile(fixture_dir .. '/' .. name .. '.hgs')
    assert(result.ok, 'Load ' .. name .. ': ' .. tostring(result.message))
    assert(hg.IsValid(result.model), 'Load returned an invalid model: ' .. name)
    models[#models + 1] = result.model
    return result.model
end

local function submit(view, target, view_state, instances, renderer, render_data, settings)
    local result, next_view = hg.SubmitGaussianSplats(view, rect, view_state, instances,
        renderer, render_data, settings, target.fb.handle)
    assert(result ~= nil and next_view ~= nil, 'SubmitGaussianSplats must return a result and next view')
    return result, next_view
end

local function submit_ok(view, target, view_state, instances, renderer, render_data, settings)
    local result, next_view = submit(view, target, view_state, instances, renderer, render_data, settings)
    assert(result.ok, 'Gaussian submission failed: ' .. tostring(result.message))
    assert(next_view > view, 'Successful nonempty submission did not advance the view')
    assert(result.draw_view == next_view - 1, 'Graphics view is not the final sequential view')
    return result, next_view
end

local function submit_fail(view, target, view_state, instances, renderer, render_data, settings, label)
    local result, next_view = submit(view, target, view_state, instances, renderer, render_data, settings)
    assert(not result.ok, label .. ': invalid submission unexpectedly succeeded')
    assert(next_view == view, label .. ': failed submission advanced the view')
end

local function capture(target, view)
    local ready, next_view = hg.CaptureTexture(view, resources, target.ref, target.readback, target.picture)
    assert(next_view == view + 1, 'CaptureTexture consumed an unexpected number of views')
    target.ready = ready
    return next_view
end

local function wait_for_captures()
    local ready = 0
    for _, target in ipairs(targets) do
        if target.ready then ready = math.max(ready, target.ready) end
    end
    if ready == 0 then return end
    local frame = hg.Frame()
    local bounded_frames = 0
    while frame < ready do
        hg.UpdateWindow(win)
        frame = hg.Frame()
        bounded_frames = bounded_frames + 1
        assert(bounded_frames <= 32, 'Capture readback did not become ready within 32 frames')
    end
    for _, target in ipairs(targets) do target.ready = nil end
end

local function expect_pixel(target, x, y, expected, tolerance, label)
    local color = target.picture:GetPixelRGBA(x, y)
    local actual = {color.r, color.g, color.b, color.a}
    for channel = 1, 4 do
        assert(math.abs(actual[channel] - expected[channel]) <= tolerance,
            string.format('%s pixel %d,%d channel %d: got %.6f expected %.6f (RGBA %.4f %.4f %.4f %.4f)',
                label, x, y, channel, actual[channel], expected[channel], actual[1], actual[2], actual[3], actual[4]))
    end
end

local function expect_picture(target, expected, tolerance, label)
    for y = 0, 63 do
        for x = 0, 63 do
            expect_pixel(target, x, y, expected, tolerance, label)
        end
    end
end

local function expect_same(a, b, label)
    for y = 0, 63 do
        for x = 0, 63 do
            local ca, cb = a.picture:GetPixelRGBA(x, y), b.picture:GetPixelRGBA(x, y)
            local aa, bb = {ca.r, ca.g, ca.b, ca.a}, {cb.r, cb.g, cb.b, cb.a}
            for channel = 1, 4 do
                assert(math.abs(aa[channel] - bb[channel]) <= 1.01 / 255,
                    string.format('%s differs at %d,%d channel %d: %.6f vs %.6f',
                        label, x, y, channel, aa[channel], bb[channel]))
            end
        end
    end
end

local function save(target, name)
    assert(hg.SavePNG(target.picture, fixture_dir .. '/' .. name .. '.png'), 'SavePNG failed: ' .. name)
end

local function probe()
    local caps = hg.GetComputeCaps()
    assert(caps.supported and caps.rgba32f_vertex_sampling, 'Required compute / RGBA32F sampling support unavailable')
    assert(caps.max_views >= 32, 'Insufficient view count for native Gaussian probe')

    -- Malformed native files must be rejected by both available loader routes.
    local malformed = {'magic', 'legacy', 'length', 'count', 'sh_offsets', 'storage', 'block_size', 'raw_length', 'overlap', 'reserved', 'raw_crc', 'header_crc', 'table_crc', 'trailing', 'truncated', 'nan', 'zero_quat', 'compressed_trailing', 'inflate_overrun', 'sh_code'}
    local from_data_available = hg.Data ~= nil and hg.LoadDataFromFile ~= nil and hg.LoadGaussianSplatModelFromData ~= nil
    for _, name in ipairs(malformed) do
        local path = fixture_dir .. '/malformed_' .. name .. '.hgs'
        local result = hg.LoadGaussianSplatModelFromFile(path)
        assert(not result.ok, 'Malformed native file accepted: ' .. name)
        assert(not hg.IsValid(result.model), 'Malformed native file returned a valid model: ' .. name)
        if from_data_available then
            local data = hg.Data()
            assert(hg.LoadDataFromFile(path, data), 'Data file loading failed: ' .. name)
            result = hg.LoadGaussianSplatModelFromData(data)
            assert(not result.ok, 'Malformed native Data accepted: ' .. name)
            assert(not hg.IsValid(result.model), 'Malformed native Data returned a valid model: ' .. name)
        end
    end
    if from_data_available then
        print('PASS malformed HGS rejection through File and FromData loaders')
    else
        print('INFO LoadGaussianSplatModelFromData/Data file binding unavailable; only File loader rejection was exercised')
    end

    local red, green = load_model('red'), load_model('green')
    local sh1, invisible = load_model('sh1'), load_model('invisible')
    assert(hg.GetGaussianSplatCount(red) == 1 and hg.GetGaussianSplatSourceCount(red) == 1, 'Red model counts')
    assert(hg.GetGaussianSplatSHDegree(red) == 0 and hg.GetGaussianSplatSHDegree(sh1) == 1, 'Model SH degrees')
    assert(not hg.IsGaussianSplatAntialiased(red) and not hg.IsGaussianSplatAntialiased(sh1), 'Fixtures must be non-AA')
    assert(hg.GetGaussianSplatCount(invisible) == 3, 'Invisible/padding fixture count')
    local bounds_min, bounds_max = hg.GetGaussianSplatBoundsMin(red), hg.GetGaussianSplatBoundsMax(red)
    assert(math.abs(bounds_min.z - 3) < 1e-6 and math.abs(bounds_max.z - 3) < 1e-6, 'Model bounds')

    local renderer = hg.CreateGaussianSplatRendererFromFile(shaders)
    assert(hg.IsValid(renderer), 'Native Gaussian renderer creation failed')
    renderers[#renderers + 1] = renderer
    local render_data = hg.CreateGaussianSplatRenderData()
    local second_render_data = hg.CreateGaussianSplatRenderData()
    assert(hg.IsValid(render_data) and hg.IsValid(second_render_data), 'Render data creation failed')
    render_data_handles[#render_data_handles + 1] = render_data
    render_data_handles[#render_data_handles + 1] = second_render_data
    local identity_view = make_view_state(hg.Mat4.Identity)
    local preflight_target = make_target('preflight')

    -- Every preflight rejection must leave the caller's view ID untouched.
    local settings = make_settings()
    settings.view_limit = 1
    submit_fail(7, preflight_target, identity_view, {make_instance(red, 1)}, renderer, render_data, settings,
        'view_limit')
    settings = make_settings()
    settings.max_splats = 0
    submit_fail(7, preflight_target, identity_view, {make_instance(red, 1)}, renderer, render_data, settings,
        'max_splats/count')
    settings = make_settings()
    submit_fail(7, preflight_target, identity_view,
        {make_instance(red, 9), make_instance(green, 9)}, renderer, render_data, settings, 'duplicate instance IDs')
    submit_fail(7, preflight_target, identity_view,
        {make_instance(red, 1, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(0, 0, 0), hg.Vec3(1, 2, 1)))},
        renderer, render_data, settings, 'nonuniform transform')
    submit_fail(7, preflight_target, identity_view,
        {make_instance(red, 1, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(0, 0, 0), hg.Vec3(-1, -1, -1)))},
        renderer, render_data, settings, 'negative transform')
    submit_fail(7, preflight_target, identity_view,
        {make_instance(red, 1, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(0, 0, 0), hg.Vec3(1, 1, -1)))},
        renderer, render_data, settings, 'reflection transform')

    local destroyed_renderer = hg.CreateGaussianSplatRendererFromFile(shaders)
    renderers[#renderers + 1] = destroyed_renderer
    local renderer_alias = destroyed_renderer
    hg.DestroyGaussianSplatRenderer(destroyed_renderer)
    assert(not hg.IsValid(destroyed_renderer) and not hg.IsValid(renderer_alias), 'Renderer destroy aliases')
    submit_fail(7, preflight_target, identity_view, {make_instance(red, 1)}, destroyed_renderer, render_data,
        make_settings(), 'destroyed renderer')
    local destroyed_data = hg.CreateGaussianSplatRenderData()
    render_data_handles[#render_data_handles + 1] = destroyed_data
    local data_alias = destroyed_data
    hg.DestroyGaussianSplatRenderData(destroyed_data)
    assert(not hg.IsValid(destroyed_data) and not hg.IsValid(data_alias), 'Render data destroy aliases')
    submit_fail(7, preflight_target, identity_view, {make_instance(red, 1)}, renderer, destroyed_data,
        make_settings(), 'destroyed render data')

    -- Empty submission succeeds without a draw/view and preserves the cleared target.
    local empty = make_target('empty')
    clear_target(empty, 0, sentinel, 1)
    local empty_result, view = submit(1, empty, identity_view, {}, renderer, render_data, make_settings())
    assert(empty_result.ok and view == 1 and empty_result.count == 0 and empty_result.dispatches == 0,
        'Empty instances must succeed without consuming views')
    view = capture(empty, view)
    wait_for_captures()
    expect_picture(empty, sentinel, 1.01 / 255, 'empty submission/no implicit clear')

    -- Three invisible records force sort padding while preserving every sentinel pixel.
    local padded = make_target('all_invisible_padding')
    clear_target(padded, 0, sentinel, 1)
    local _, next_view = submit_ok(1, padded, identity_view, {make_instance(invisible, 1)},
        renderer, render_data, make_settings())
    capture(padded, next_view)
    wait_for_captures()
    expect_picture(padded, sentinel, 1.01 / 255, 'all-invisible splats and padding')

    -- Back-to-front order is global, independent of input order. Compact and separate schedules match.
    local order_a, order_b, separate = make_target('order_a'), make_target('order_b'), make_target('separate')
    clear_target(order_a, 0, black, 1)
    local start = 1
    local result_a
    result_a, view = submit_ok(start, order_a, identity_view,
        {make_instance(red, 1), make_instance(green, 2)}, renderer, render_data, make_settings())
    assert(view - start == 2 and result_a.count == 2, 'Compact submission must consume exactly two views')
    view = capture(order_a, view)
    clear_target(order_b, view, black, 1)
    start = view + 1
    local result_b
    result_b, view = submit_ok(start, order_b, identity_view,
        {make_instance(green, 2), make_instance(red, 1)}, renderer, render_data, make_settings())
    assert(view - start == 2 and result_b.count == 2, 'Second compact submission view count')
    view = capture(order_b, view)
    clear_target(separate, view, black, 1)
    start = view + 1
    local separate_settings = make_settings()
    separate_settings.compact_views = false
    local result_separate
    result_separate, view = submit_ok(start, separate, identity_view,
        {make_instance(green, 2), make_instance(red, 1)}, renderer, render_data, separate_settings)
    assert(view - start > 2, 'Separate schedule did not expose per-dispatch views')
    assert(view == start + result_separate.dispatches + 1, 'Separate view count does not match dispatch count')
    capture(separate, view)
    wait_for_captures()
    for _, target in ipairs({order_a, order_b, separate}) do
        expect_pixel(target, 32, 32, {0.25, 0.5, 0, 0.75}, 0.025, 'global back-to-front compositing')
        expect_pixel(target, 0, 0, black, 1.01 / 255, 'native submission must not clear target')
    end
    expect_same(order_a, order_b, 'input list order changed global sorting')
    expect_same(order_a, separate, 'compact versus separate view schedule')
    save(order_a, 'order_compact.png')
    save(separate, 'order_separate.png')

    -- Equal depths use explicit instance IDs, not input order. Red is translated from z=3 to z=2.
    local tie_a, tie_b = make_target('tie_a'), make_target('tie_b')
    local red_at_two = hg.TranslationMat4(hg.Vec3(0, 0, -1))
    clear_target(tie_a, 0, black, 1)
    local tie_instances_a = {make_instance(red, 22, red_at_two), make_instance(green, 11)}
    local _, tie_view = submit_ok(1, tie_a, identity_view, tie_instances_a, renderer, render_data, make_settings())
    tie_view = capture(tie_a, tie_view)
    clear_target(tie_b, tie_view, black, 1)
    local tie_instances_b = {make_instance(green, 11), make_instance(red, 22, red_at_two)}
    local _, tie_next = submit_ok(tie_view + 1, tie_b, identity_view, tie_instances_b,
        renderer, render_data, make_settings())
    capture(tie_b, tie_next)
    wait_for_captures()
    for _, target in ipairs({tie_a, tie_b}) do
        -- ID 11 green is drawn before ID 22 red in the stable equal-depth order.
        expect_pixel(target, 32, 32, {0.5, 0.25, 0, 0.75}, 0.025, 'stable explicit instance ID order')
    end
    expect_same(tie_a, tie_b, 'equal-depth input order independence')
    save(tie_a, 'stable_instance_ids.png')

    -- Prefilled depth=0 rejects all fragments when enabled; disabling depth or clearing to 1 reveals them.
    local depth_on, depth_off, depth_one = make_target('depth_zero_on'), make_target('depth_zero_off'), make_target('depth_one_on')
    clear_target(depth_on, 0, black, 0)
    local _, depth_view = submit_ok(1, depth_on, identity_view,
        {make_instance(red, 1), make_instance(green, 2)}, renderer, render_data, make_settings())
    depth_view = capture(depth_on, depth_view)
    clear_target(depth_off, depth_view, black, 0)
    local no_depth_settings = make_settings()
    no_depth_settings.depth_test = false
    local _, depth_next = submit_ok(depth_view + 1, depth_off, identity_view,
        {make_instance(red, 1), make_instance(green, 2)}, renderer, render_data, no_depth_settings)
    depth_view = capture(depth_off, depth_next)
    clear_target(depth_one, depth_view, black, 1)
    local _, depth_next_one = submit_ok(depth_view + 1, depth_one, identity_view,
        {make_instance(red, 1), make_instance(green, 2)}, renderer, render_data, make_settings())
    capture(depth_one, depth_next_one)
    wait_for_captures()
    expect_picture(depth_on, black, 1.01 / 255, 'prefilled depth-zero rejection')
    expect_pixel(depth_off, 32, 32, {0.25, 0.5, 0, 0.75}, 0.025, 'depth_test=false with prefilled depth zero')
    expect_pixel(depth_one, 32, 32, {0.25, 0.5, 0, 0.75}, 0.025, 'prefilled depth-one visibility')
    expect_same(depth_off, depth_one, 'depth rejection control images')
    save(depth_on, 'depth_zero_rejected.png')
    save(depth_off, 'depth_zero_disabled.png')

    -- Conventional opaque geometry between green (z=2) and red (z=3).
    -- Sharing its depth keeps green visible and rejects red; constant splat depth fails this.
    mesh_program = hg.LoadProgramFromFile((arg[5] or shaders) .. '/pos_rgb')
    assert(mesh_program, 'Missing compiled conventional mesh shader')
    local layout = hg.VertexLayout()
    layout:Begin():Add(hg.A_Position, 3, hg.AT_Float):Add(hg.A_Color0, 4, hg.AT_Uint8, true):End()
    local triangle = hg.Vertices(layout, 3)
    for i, point in ipairs({{-4, -4, 2.5}, {4, -4, 2.5}, {0, 4, 2.5}}) do
        triangle:Begin(i - 1):SetPos(hg.Vec3(table.unpack(point))):SetColor0(hg.Color(0, 0, 1, 1)):End()
    end
    local mesh_target = make_target('opaque_mesh')
    clear_target(mesh_target, 0, black, 1)
    hg.SetViewTransform(0, identity_view.view, identity_view.proj)
    hg.SetTransform(hg.Mat4.Identity)
    hg.DrawTriangles(0, triangle, mesh_program, hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Less, hg.FC_Disabled, true))
    local _, mesh_view = submit_ok(1, mesh_target, identity_view,
        {make_instance(red, 1), make_instance(green, 2)}, renderer, render_data, make_settings())
    capture(mesh_target, mesh_view)
    wait_for_captures()
    expect_pixel(mesh_target, 32, 32, {0, 0.5, 0.5, 1}, 0.025, 'opaque triangle between two Gaussian models')
    save(mesh_target, 'opaque_mesh_composition')

    -- Independent render data, cameras and targets are submitted before one shared frame.
    local independent_a, independent_b = make_target('independent_a'), make_target('independent_b')
    clear_target(independent_a, 0, black, 1)
    local _, independent_view = submit_ok(1, independent_a, identity_view, {make_instance(red, 1)},
        renderer, render_data, make_settings())
    independent_view = capture(independent_a, independent_view)
    clear_target(independent_b, independent_view, black, 1)
    local shifted_world = hg.TransformationMat4(hg.Vec3(0.75, 0, 0), hg.Vec3(0, 0, 0))
    local shifted_view = make_view_state(shifted_world)
    local _, independent_next = submit_ok(independent_view + 1, independent_b, shifted_view, {make_instance(green, 2)},
        renderer, second_render_data, make_settings())
    capture(independent_b, independent_next)
    wait_for_captures()
    expect_pixel(independent_a, 32, 32, {0.5, 0, 0, 0.5}, 0.025, 'first independent target overwritten')
    expect_pixel(independent_b, 32, 32, black, 0.025, 'second camera did not shift splat')
    expect_pixel(independent_b, 11, 32, {0, 0.5, 0, 0.5}, 0.05, 'second independent camera/target')
    save(independent_a, 'independent_a.png')
    save(independent_b, 'independent_b.png')

    -- Graphdeco SH1 k=3 is the x basis. Direction is normalized(splat-camera).
    local sh_plus, sh_minus = make_target('sh_camera_plus_x'), make_target('sh_camera_minus_x')
    local plus_view = make_view_state(hg.Mat4LookAt(hg.Vec3(3, 0, 3), hg.Vec3(0, 0, 3)))
    local minus_view = make_view_state(hg.Mat4LookAt(hg.Vec3(-3, 0, 3), hg.Vec3(0, 0, 3)))
    clear_target(sh_plus, 0, black, 1)
    local _, sh_view = submit_ok(1, sh_plus, plus_view, {make_instance(sh1, 1)}, renderer, render_data, make_settings())
    sh_view = capture(sh_plus, sh_view)
    clear_target(sh_minus, sh_view, black, 1)
    local _, sh_next = submit_ok(sh_view + 1, sh_minus, minus_view, {make_instance(sh1, 1)},
        renderer, second_render_data, make_settings())
    capture(sh_minus, sh_next)
    wait_for_captures()
    -- k=3 is -C1*x: a positive coefficient brightens red for the -x direction.
    expect_pixel(sh_plus, 32, 32, {0.35, 0, 0, 0.5}, 0.04, 'SH1 +x camera uses -x direction')
    expect_pixel(sh_minus, 32, 32, {0.15, 0, 0, 0.5}, 0.04, 'SH1 -x camera uses +x direction')
    save(sh_plus, 'sh1_camera_plus_x.png')
    save(sh_minus, 'sh1_camera_minus_x.png')

    -- A visible record at source index 1 checks stored SH3 stride when evaluating only SH1.
    local sh3 = load_model('sh3')
    local sh3_full, sh3_low = make_target('sh3_full'), make_target('sh3_low')
    clear_target(sh3_full, 0, black, 1)
    local _, full_view = submit_ok(1, sh3_full, identity_view, {make_instance(sh3, 1)},
        renderer, render_data, make_settings())
    full_view = capture(sh3_full, full_view)
    clear_target(sh3_low, full_view, black, 1)
    local low_settings = make_settings()
    low_settings.max_sh_degree = 1
    local _, low_view = submit_ok(full_view + 1, sh3_low, plus_view, {make_instance(sh3, 1)},
        renderer, second_render_data, low_settings)
    capture(sh3_low, low_view)
    wait_for_captures()
    expect_pixel(sh3_full, 32, 32, {0.25, 0.35, 0, 0.5}, 0.025, 'degree-3 k=12 evaluation')
    expect_pixel(sh3_low, 32, 32, {0.35, 0.25, 0, 0.5}, 0.025, 'SH1 evaluation from stored SH3 source index 1')

    -- The instance retains its model even when the load result and model local are collected.
    local owned_target = make_target('instance_ownership')
    local loaded = hg.LoadGaussianSplatModelFromFile(fixture_dir .. '/red.hgs')
    assert(loaded.ok, loaded.message)
    local owned_instance = make_instance(loaded.model, 77)
    loaded = nil
    collectgarbage('collect')
    collectgarbage('collect')
    assert(hg.IsValid(owned_instance.model), 'Instance did not retain model ownership across Lua GC')
    clear_target(owned_target, 0, black, 1)
    local _, owned_view = submit_ok(1, owned_target, identity_view, {owned_instance},
        renderer, render_data, make_settings())
    local owned_next = capture(owned_target, owned_view)
    wait_for_captures()
    expect_pixel(owned_target, 32, 32, {0.5, 0, 0, 0.5}, 0.025, 'owned model rendered before alias destruction')
    local model_alias = owned_instance.model
    hg.DestroyGaussianSplatModel(owned_instance.model)
    assert(not hg.IsValid(owned_instance.model) and not hg.IsValid(model_alias), 'Model destroy must invalidate aliases')
    submit_fail(owned_next, owned_target, identity_view, {owned_instance}, renderer, render_data, make_settings(),
        'destroyed model')

    -- Deliberately leave one native handle alive to verify RenderShutdown invalidation.
    local surviving_load = hg.LoadGaussianSplatModelFromFile(fixture_dir .. '/red.hgs')
    assert(surviving_load.ok and hg.IsValid(surviving_load.model), 'Survivor model load failed')
    return surviving_load.model
end

local success, survivor = xpcall(probe, debug.traceback)

-- Framebuffers are destroyed before their color/depth textures. Color textures are resource-owned.
for _, target in ipairs(targets) do
    if target.fb and hg.IsValid(target.fb) then hg.DestroyFrameBuffer(target.fb) end
end
resources:DestroyAllTextures()
for _, target in ipairs(targets) do
    if target.depth and hg.IsValid(target.depth) then hg.DestroyTexture(target.depth) end
    if target.readback and hg.IsValid(target.readback) then hg.DestroyTexture(target.readback) end
end
for _, data in ipairs(render_data_handles) do
    if hg.IsValid(data) then hg.DestroyGaussianSplatRenderData(data) end
end
for _, model in ipairs(models) do
    if hg.IsValid(model) then hg.DestroyGaussianSplatModel(model) end
end
for _, renderer in ipairs(renderers) do
    if hg.IsValid(renderer) then hg.DestroyGaussianSplatRenderer(renderer) end
end
collectgarbage('collect')
if mesh_program then hg.DestroyProgram(mesh_program) end
hg.RenderShutdown() -- Also releases the deliberately surviving Gaussian model.
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
if not success then error(survivor) end
assert(not hg.IsValid(survivor), 'RenderShutdown must invalidate a surviving model userdata')
survivor = nil
collectgarbage('collect')
print('PASS Gaussian splat native probe: loaders, global/stable sorting, depth rejection, schedules, independent targets, SH1, preflight and ownership')
