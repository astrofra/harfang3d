-- Speak a sentence with lib-say and HARFANG's raw LPCM bridge

hg = require("harfang")

-- If Lua cannot find the module, extend package.cpath so it can locate say.dll.
local ok, say = pcall(require, "say")
if not ok then
	print("This tutorial requires the 'say' Lua module from lib-say.")
	print("Build lib-say first, then make sure say.dll and lua54.dll are reachable from package.cpath / PATH.")
	return
end

hg.InputInit()
hg.AudioInit()

if hg.LoadLPCMSound == nil then
	print("This tutorial requires a HARFANG Lua runtime exposing hg.LoadLPCMSound().")
	print("Rebuild or refresh tutorials/hg_lua so it includes the latest audio raw PCM bridge.")
	hg.AudioShutdown()
	hg.InputShutdown()
	return
end

local phrase = "This is Harfang speaking!"
local blob, info = say.synthesize(phrase, {
	lang = "en",
	format = "raw"
})

if info.sample_rate ~= 44100 or info.channels ~= 1 or info.bits_per_sample ~= 16 or info.pcm_encoding ~= "s16le" then
	print("Unsupported lib-say output format.")
	print("Expected raw mono 44.1kHz signed 16-bit little-endian PCM.")
	hg.AudioShutdown()
	hg.InputShutdown()
	return
end

-- LoadLPCMSound copies the PCM payload to the audio backend before returning.
local snd_ref = hg.LoadLPCMSound(blob:GetData(), blob:GetSize(), hg.AFF_LPCM_44KHZ_S16_Mono)
if snd_ref == hg.SND_Invalid then
	print("Failed to upload synthesized speech to HARFANG.")
	hg.AudioShutdown()
	hg.InputShutdown()
	return
end

local src_ref = hg.PlayStereo(snd_ref, hg.StereoSourceState(1, hg.SR_Once))
if src_ref == hg.SRC_Invalid then
	print("Failed to start audio playback.")
	hg.UnloadSound(snd_ref)
	hg.AudioShutdown()
	hg.InputShutdown()
	return
end

print('Speaking: "' .. phrase .. '"')
print("Press Escape to stop playback and exit.")

while hg.GetSourceState(src_ref) ~= hg.SS_Stopped and not hg.ReadKeyboard("raw"):Key(hg.K_Escape) do
	hg.TickClock()
	hg.Sleep(hg.time_from_ms(10))
end

hg.StopSource(src_ref)
hg.UnloadSound(snd_ref)

hg.AudioShutdown()
hg.InputShutdown()
