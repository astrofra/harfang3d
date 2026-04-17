-- Stream a MOD/XM/S3M/IT tracker module with the XMP audio plugin.
-- Test asset: ftp://ftp.modland.com//pub/modules/Protracker/4-Mat/4-mat's madness.mod
-- Put the file at tutorials/resources/music/4-mat's madness.mod before running this script.

hg = require("harfang")

local module_path = "resources/music/4-mat's madness.mod"

if not hg.Exists(module_path) then
	print("Missing module file: " .. module_path)
	print("Download ftp://ftp.modland.com//pub/modules/Protracker/4-Mat/4-mat's madness.mod into tutorials/resources/music/")
	return
end

local plugin_names
if package.config:sub(1, 1) == "\\" then
	plugin_names = {"audio_xmp.dll"}
else
	plugin_names = {"audio_xmp.so", "audio_xmp.dylib"}
end

local streamer = nil
for _, plugin_name in ipairs(plugin_names) do
	local candidate = hg.MakeAudioStreamer(plugin_name)
	if hg.IsValid(candidate) then
		streamer = candidate
		break
	end
end

if streamer == nil then
	print("audio_xmp plugin not found. Build with HG_ENABLE_XMP_AUDIO and keep the plugin next to the Lua runtime.")
	return
end

hg.InputInit()
hg.AudioInit()

if streamer:Startup() == 0 then
	print("audio_xmp plugin failed to start")
	hg.AudioShutdown()
	hg.InputShutdown()
	return
end

local src_ref = hg.StreamAudioFileStereo(streamer, module_path, hg.StereoSourceState(1, hg.SR_Loop))
if src_ref == hg.SRC_Invalid then
	print("Failed to stream " .. module_path)
	streamer:Shutdown()
	hg.AudioShutdown()
	hg.InputShutdown()
	return
end

local angle = 0
while not hg.ReadKeyboard("raw"):Key(hg.K_Escape) do
	angle = angle + hg.time_to_sec_f(hg.TickClock()) * 0.5
	hg.SetSourcePanning(src_ref, math.sin(angle))
end

hg.StopSource(src_ref)
streamer:Shutdown()

hg.AudioShutdown()
hg.InputShutdown()
