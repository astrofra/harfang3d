-- Stream a MOD/XM/S3M/IT tracker module with the XMP audio plugin.

hg = require("harfang")

local module_path = "resources/sounds/4-mat's madness.mod"

if not hg.Exists(module_path) then
	print("Missing module file: " .. module_path)
	return
end

hg.InputInit()
hg.AudioInit()

local src_ref = hg.StreamModuleFileStereo(module_path, hg.StereoSourceState(1, hg.SR_Loop))
if src_ref == hg.SRC_Invalid then
	print("Failed to stream " .. module_path)
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

hg.AudioShutdown()
hg.InputShutdown()
