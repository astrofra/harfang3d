-- Run from tutorials/. Exercise preloading, GUI switching and lifetime in the actual tutorial.
-- lua runner.lua MODULE_DIR TUTORIAL.lua [FRAMES=60] [SCREENSHOT_BASE]
local module_dir, tutorial = assert(arg[1]), assert(arg[2])
local limit, capture = tonumber(arg[3]) or 60, arg[4]
assert(limit >= 60)
package.cpath = module_dir .. '/?.dll;' .. module_dir .. '/?.so;' .. package.cpath
local hg = require('harfang')
local frame, submissions, resized, shutdown = 0, 0, 0, false
local loaded_models, submitted_data = {}, {}
local visits, sh_degrees = {0, 0}, {}
local switches, gui_frames, destroyed_models, destroyed_data, gui_shutdown = 0, 0, 0, 0, false
local expected_model = 0
local original_open, original_frame = hg.IsWindowOpen, hg.Frame
local original_submit, original_shutdown = hg.SubmitGaussianSplats, hg.RenderShutdown
local original_load = hg.LoadGaussianSplatModelFromFile
hg.LoadGaussianSplatModelFromFile = function(path)
    assert(frame == 0, 'Models must be preloaded before the frame loop')
    local loaded = original_load(path)
    assert(loaded.ok, loaded.message)
    loaded_models[#loaded_models + 1] = loaded.model
    if #loaded_models == 1 then
        assert(hg.GetGaussianSplatSourceCount(loaded.model) == 113648)
        assert(hg.GetGaussianSplatSHDegree(loaded.model) == 0)
    else
        assert(hg.GetGaussianSplatSHDegree(loaded.model) == 3)
    end
    return loaded
end
local original_combo, original_slider = hg.ImGuiCombo, hg.ImGuiSliderInt
hg.ImGuiCombo = function(label, selected, items, ...)
    local changed, value = original_combo(label, selected, items, ...)
    assert(label == 'Model' and #items == 2)
    expected_model = frame >= 12 and frame < 36 and 1 or 0
    if selected ~= expected_model then switches = switches + 1 end
    return selected ~= expected_model, expected_model
end
hg.ImGuiSliderInt = function(label, degree, ...)
    original_slider(label, degree, ...)
    assert(label == 'SH degree' and expected_model == 1)
    local value = frame < 20 and 1 or 3
    return value ~= degree, value
end
local original_gui_frame = hg.ImGuiEndFrame
hg.ImGuiEndFrame = function(view)
    assert(view == 4, 'GUI must follow compute, draw and gizmo views')
    gui_frames = gui_frames + 1
    return original_gui_frame(view)
end
local original_destroy_model, original_destroy_data = hg.DestroyGaussianSplatModel, hg.DestroyGaussianSplatRenderData
hg.DestroyGaussianSplatModel = function(model)
    assert(frame == limit, 'Models must remain preloaded throughout switching')
    destroyed_models = destroyed_models + 1
    return original_destroy_model(model)
end
hg.DestroyGaussianSplatRenderData = function(data)
    assert(frame == limit)
    destroyed_data = destroyed_data + 1
    return original_destroy_data(data)
end
local original_gui_shutdown = hg.ImGuiShutdown
hg.ImGuiShutdown = function(...)
    gui_shutdown = true
    return original_gui_shutdown(...)
end

hg.IsWindowOpen = function(win)
    assert(#loaded_models == 2, 'Both models must be loaded during setup')
    if frame == 10 then assert(hg.SetWindowClientSize(win, 960, 540)); resized = resized + 1 end
    if frame == 25 then assert(hg.SetWindowClientSize(win, 1280, 720)); resized = resized + 1 end
    return frame < limit and original_open(win)
end
hg.SubmitGaussianSplats = function(first, viewport, view_state, instances, renderer, data, settings)
    local expected_renderer = package.config:sub(1, 1) == '\\' and 'Direct3D 11' or 'Metal'
    assert(hg.GetComputeCaps().renderer == expected_renderer, 'Unexpected actual tutorial renderer')
    local index = expected_model + 1
    assert(#instances == 1 and instances[1].id == index, 'Wrong model submitted after GUI switch')
    assert(hg.GetGaussianSplatCount(instances[1].model) == hg.GetGaussianSplatCount(loaded_models[index]))
    if submitted_data[index] then assert(submitted_data[index] == data, 'Render data must survive switching') end
    submitted_data[index] = data
    visits[index] = visits[index] + 1
    if index == 2 then sh_degrees[settings.max_sh_degree] = true end
    local result, next_view = original_submit(first, viewport, view_state, instances, renderer, data, settings)
    assert(result.ok, result.message)
    assert(result.count == hg.GetGaussianSplatCount(loaded_models[index]), 'Submission silently reduced the loaded model')
    assert(next_view == first + 2, 'Compact native schedule must consume two views')
    submissions = submissions + 1
    return result, next_view
end
hg.Frame = function(...)
    frame = frame + 1
    if frame % 10 == 0 then collectgarbage('collect') end
    if capture and frame == 30 then hg.RequestScreenShot(hg.InvalidFrameBufferHandle, capture .. '-saari') end
    if capture and frame == limit - 10 then hg.RequestScreenShot(hg.InvalidFrameBufferHandle, capture) end
    return original_frame(...)
end
hg.RenderShutdown = function(...)
    original_shutdown(...)
    shutdown = true
end
dofile(tutorial)
assert(shutdown and frame == limit and submissions == frame and resized == 2, 'Incomplete tutorial run')
assert(#loaded_models == 2 and switches == 2 and visits[1] > 0 and visits[2] > 0, 'Incomplete preload/switch coverage')
assert(submitted_data[1] ~= submitted_data[2] and sh_degrees[1] and sh_degrees[3], 'Missing independent state or SH selection')
assert(gui_frames == frame and gui_shutdown and destroyed_models == 2 and destroyed_data == 2, 'Incomplete resource cleanup')
if capture then
    for _, path in ipairs({capture, capture .. '-saari'}) do
        local picture = hg.Picture()
        assert(hg.LoadTGA(picture, path .. '.tga'), 'Missing screenshot')
        assert(hg.SavePNG(picture, path .. '.png'), 'Cannot save screenshot')
        -- Ignore the GUI/gizmo. Empty frames or a GUI-only capture must fail.
        local width, height = picture:GetWidth(), picture:GetHeight()
        local visible, green = 0, 0
        for y = math.floor(height * 0.25), math.floor(height * 0.85), 4 do
            for x = math.floor(width * 0.3), math.floor(width * 0.7), 4 do
                local color = picture:GetPixelRGBA(x, y)
                if math.max(color.r, color.g, color.b) > 0.12 then visible = visible + 1 end
                if color.g > color.r * 1.1 and color.g > color.b * 1.1 and color.g > 0.12 then
                    green = green + 1
                end
            end
        end
        assert(visible > 1000 and green > 100, 'Missing model content in screenshot: ' .. path)
        print(string.format('PASS image content: %s (%dx%d; visible=%d green=%d)', path, width, height, visible, green))
    end
end
print(string.format('PASS actual tutorial: %s; %d native frames, two preloads, two GUI switches, SH1/SH3, two resizes, GC and cleanup', tutorial, frame))
