-- Speak a sentence with lib-say and HARFANG's raw LPCM bridge

hg = require("harfang")
say = require("say")

hg.InputInit()
hg.AudioInit()

local phrase = "This is Harfang speaking!"
local blob = select(1, say.synthesize(phrase, {
	lang = "en",
	format = "raw"
}))

-- LoadLPCMSound copies the PCM payload to the audio backend before returning.
local snd_ref = hg.LoadLPCMSound(blob:GetData(), blob:GetSize(), hg.AFF_LPCM_44KHZ_S16_Mono)
local src_ref = hg.PlayStereo(snd_ref, hg.StereoSourceState(1, hg.SR_Once))

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
