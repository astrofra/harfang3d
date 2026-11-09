-- Run from tutorials/. Exercise the actual single-file tutorial.
-- lua runner.lua MODULE_DIR TUTORIAL.lua [FRAMES=60] [SCREENSHOT_BASE]
local module_dir, tutorial = assert(arg[1]), assert(arg[2])
local limit, capture = tonumber(arg[3]) or 60, arg[4]
assert(limit >= 40)
package.cpath = module_dir .. '/?.dll;' .. module_dir .. '/?.so;' .. package.cpath
local hg = require('harfang')
local frame, submissions, resized, shutdown = 0, 0, 0, false
local original_open, original_frame = hg.IsWindowOpen, hg.Frame
local original_submit, original_shutdown = hg.SubmitGaussianSplats, hg.RenderShutdown
local original_load = hg.LoadGaussianSplatModelFromFile
hg.LoadGaussianSplatModelFromFile = function(path)
    local loaded = original_load(path)
    assert(loaded.ok, loaded.message)
    if tutorial == 'gaussian_splatting.lua' then
        assert(hg.GetGaussianSplatSourceCount(loaded.model) == 113648)
        assert(hg.GetGaussianSplatSHDegree(loaded.model) == 0)
    end
    return loaded
end
hg.IsWindowOpen = function(win)
    if frame == 10 then assert(hg.SetWindowClientSize(win, 960, 540)); resized = resized + 1 end
    if frame == 25 then assert(hg.SetWindowClientSize(win, 1280, 720)); resized = resized + 1 end
    return frame < limit and original_open(win)
end
hg.SubmitGaussianSplats = function(first, ...)
    local result, next_view = original_submit(first, ...)
    assert(result.ok, result.message)
    assert(next_view == first + 2, 'Compact native schedule must consume two views')
    submissions = submissions + 1
    return result, next_view
end
hg.Frame = function(...)
    frame = frame + 1
    if frame % 10 == 0 then collectgarbage('collect') end
    if capture and frame == limit - 10 then hg.RequestScreenShot(hg.InvalidFrameBufferHandle, capture) end
    return original_frame(...)
end
hg.RenderShutdown = function(...)
    original_shutdown(...)
    shutdown = true
end
dofile(tutorial)
assert(shutdown and frame == limit and submissions == frame and resized == 2, 'Incomplete tutorial run')
if capture then
    local picture = hg.Picture()
    assert(hg.LoadTGA(picture, capture .. '.tga'), 'Missing screenshot')
    assert(hg.SavePNG(picture, capture .. '.png'), 'Cannot save screenshot')
end
print(string.format('PASS actual tutorial: %s; %d native frames, two resizes, GC, shutdown', tutorial, frame))
