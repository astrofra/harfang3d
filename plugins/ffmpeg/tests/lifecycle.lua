-- Run with the matching HARFANG Lua runtime: lifecycle.lua <plugin path> <video path>.
-- Use an external timeout to detect a deadlock in the native Close call.
local hg = require("harfang")
local plugin = assert(arg[1], "Missing plugin path")
local streamer = hg.MakeVideoStreamer(plugin)
local video = assert(arg[2], "Missing video path")
assert(hg.IsValid(streamer) and streamer:Startup() ~= 0)

for iteration = 1, 100 do
	local handle = streamer:Open(video)
	assert(handle ~= 0)
	assert(streamer:GetFrame(handle, 0, 0, 0, 0, hg.VFF_UNKNOWN) == 0, "Frame exposed before decoding")
	assert(streamer:Play(handle) ~= 0)
	assert(streamer:Close(handle) ~= 0)
end

streamer:Shutdown()
print("PASS FFmpeg lifecycle: 100 immediate play/close cycles; no frame before decoding")
