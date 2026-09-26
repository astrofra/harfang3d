A scene-owned player that automatically crossfades compatible local transform animations.

Create a player with `scene:CreateInstanceAnimPlayer(node)` or
`scene:CreateAnimPlayer(clips)`. Configure `SetCrossFadeDuration` and optionally
`SetCrossFadeEasing`, then call `Play` when the desired animation changes.

```lua
local player = scene:CreateInstanceAnimPlayer(actor)
assert(player:IsValid())
player:SetCrossFadeDuration(hg.time_from_sec_f(0.2))
player:SetCrossFadeEasing(hg.E_SmoothStep)
player:Play("idle", hg.ALM_Loop)
-- Later:
player:Play("turn_left", hg.ALM_Once)
```

The normal scene update advances both clips independently. Both can loop or play
once. Interrupting a fade blends from its last displayed pose. Once-only clips
hold their endpoint during longer fades; `IsTransitioning` reports the fade
state independently of `Scene::IsPlaying`.

The default fade duration is zero; supported easing values are `E_Linear` and
`E_SmoothStep`. Clips must target one common set of nodes with position, rotation
and scale tracks only. Nested instance-selection timelines are unsupported.

Use `Stop` to stop playback and release ownership, or `scene:DestroyAnimPlayer`
to invalidate the player. Releasing a scripting wrapper does not stop playback.
