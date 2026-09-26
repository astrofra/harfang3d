# Animation crossfade implementation

Date: 2026-09-26

This document describes the implemented C++, Lua and Python API. The earlier
[feasibility study](SPECS_AUTOMATIC_ANIMATION_CROSSFADE_FEASIBILITY.md) records the
design investigation; proposed signatures there are not an API reference.

## Runtime API

Create one scene-owned player for a loaded instance containing compatible clips:

```lua
local player = scene:CreateInstanceAnimPlayer(actor)
assert(player:IsValid())
assert(player:SetCrossFadeDuration(hg.time_from_sec_f(0.20)))
assert(player:SetCrossFadeEasing(hg.E_SmoothStep))

local idle = player:Play("idle", hg.ALM_Loop)
-- Later, on a state change:
local turn = player:Play("turn_left", hg.ALM_Once)
-- Continue the normal scene update each frame; no separate player update.
```

The Python equivalent uses `scene.CreateInstanceAnimPlayer(actor)` and
`player.Play("idle", hg.ALM_Loop)`. C++ uses `hg::ALM_Loop` and
`hg::E_SmoothStep`. These are HARFANG's existing enum constants, exposed at module
level in the bound APIs. There are no string-valued modes or new enum conventions.

| Operation | Contract |
| --- | --- |
| `Scene::CreateAnimPlayer(const std::vector<SceneAnimRef> &clips)` | Explicit clip set in this scene; names must be unique. |
| `Scene::CreateInstanceAnimPlayer(const Node &node)` | Uses the instance view's clips; all targets must belong to that view. |
| `Scene::DestroyAnimPlayer(const SceneAnimPlayer &player)` | Stops playback and invalidates all wrappers for this player. |
| `SceneAnimPlayer::IsValid()` | Whether the scene still owns this player. |
| `SetCrossFadeDuration(time_ns duration)` / `GetCrossFadeDuration()` | Default zero, meaning an immediate switch; negative values are rejected. |
| `SetCrossFadeEasing(Easing easing)` / `GetCrossFadeEasing()` | Default `E_Linear`; `E_SmoothStep` is also supported. Other easing values are rejected. |
| `Play(const std::string &name, AnimLoopMode loop_mode = ALM_Once, bool restart = false)` | Returns the incoming `ScenePlayAnimRef`, or an invalid reference on rejection. |
| `IsTransitioning()` | Whether the fade has remaining time, independently of clip completion. |
| `Stop()` | Stops both requests, releases channel ownership and leaves the last pose in the scene. The player can be reused. |

Duration and easing setters return a Boolean. Invalid requests warn and leave the
accepted playback unchanged. Configuration changes apply to the next request;
they do not reshape an existing transition. Playback runs through
`Scene::UpdatePlayingAnims`, including the normal `Scene::Update` and
`SceneUpdateSystems` paths. Dropping a Lua/Python wrapper does not stop a
scene-owned player: use `Stop` or `DestroyAnimPlayer` explicitly.

## Transition semantics

The first request starts directly. Later requests crossfade from the player's
last committed output. The new clip starts at its authored `t_start`; the outgoing
clip continues from its current time. Both advance at normal speed. There is no
phase synchronization, animation queue or automatic selection of the next clip.

| Outgoing A | Incoming B | Behavior |
| --- | --- | --- |
| `ALM_Loop` | `ALM_Loop` | Both wrap independently; B continues after the fade. |
| `ALM_Loop` | `ALM_Once` | A wraps; B reaches its endpoint once. |
| `ALM_Once` | `ALM_Loop` | A holds its endpoint if it ends during the fade; B wraps. |
| `ALM_Once` | `ALM_Once` | Each holds its endpoint if needed; B never restarts at fade completion. |

`ALM_Infinite` is supported with the existing convention: time advances without
wrapping or automatic completion; track evaluation holds the final value beyond
the last key. A zero-length clip can play once or infinitely, but cannot loop.
Looping uses `[t_start, t_end)`. Large time steps wrap with bounded arithmetic;
negative time steps are treated as zero by this player.

For an ordinary uninterrupted transition, with elapsed time `t` and duration `D`:

```text
u = clamp(t / D, 0, 1)
w = u                           for E_Linear
w = u*u*(3 - 2*u)               for E_SmoothStep
position, scale = lerp(A, B, w)
rotation        = normalize(shortest_arc_slerp(A, B, w))
```

Clips are sampled independently before committing the blended local transforms.
Euler rotation tracks are converted to quaternions for blending. The existing
transform hierarchy and skinning pipeline then consume the result.

Interrupting A-to-B with C captures the currently displayed mixed pose and fades
from that frozen pose to the advancing C. It does not jump to B or retain an
unbounded chain of sources. Several requests before the next update resolve to
the latest request, still using the last committed pose as their source. Repeating
the same active target and loop mode is idempotent unless `restart=true`; a
completed once-only target starts again when requested.

## Completion, ownership and lifetime

`Scene::IsPlaying(ref)` reports whether that request's clock is active. An
`ALM_Once` handle becomes inactive at its endpoint even when a longer fade still
needs the endpoint pose. Use `IsTransitioning` to observe fade completion. An
outgoing handle also becomes inactive when its contribution is retired.

The final incoming pose remains applied until another request or `Stop`. Stopping
the active incoming handle through `Scene::StopAnim` stops its player. Stopping
an active outgoing handle freezes its last sampled pose while the fade continues.
Expired handles are harmless; use `player:Stop()` to release a completed player.
`Scene::StopAllAnims` also stops all players.

A player reserves the union of animated local position, rotation and scale
channels in its clip set at creation. Missing channels in a clip use the reference
pose captured at creation, never values left behind by another clip. Two players
or a legacy playback may not write overlapping reserved channels. Conflicting
creation/play requests are rejected, including legacy enable/nested tracks on
ancestors of the rig. Distinct instances can play independently. Direct script,
physics or editor writes to owned channels are not intercepted; callers must
coordinate these themselves.

Destroying a target node, referenced animation, instance content or scene
invalidates affected players and their handles. Instance setup/reload invalidates
players tied to its previous view. Channel bindings and references must remain
unchanged during playback; destroy and recreate the player before editing the
underlying clip/track structures in C++.

## Supported assets

This implementation blends direct local transform tracks on common target nodes.
It rejects scene-global tracks, enable/discrete tracks, material/light/camera
tracks and nested `instance_anim` timelines. It is not a skeletal retargeter and
does not blend separate renderable copies of a character. Unsupported clips fail
at player creation rather than partially participating in a fade.

The runtime supports authored position, rotation and scale tracks, including
sparse clips. It does not extract root motion, correct foot sliding, synchronize
gait phases, implement additive layers, or offer playback speed/seek controls.
An authored loop can still have a discontinuity at its own wrap boundary.

## Offline animation import

The following functions are available in C++, Lua and Python:

```cpp
SceneAnimInfo GetSceneAnimInfo(const Scene &scene, SceneAnimRef ref);
SceneAnimNodeMap BuildSceneAnimNodeMap(const Scene &source, const Scene &destination);
SceneAnimImportResult ImportSceneAnim(const Scene &source, SceneAnimRef source_anim,
    Scene &destination, const SceneAnimNodeMap &node_map, const std::string &name,
    bool preserve_missing_trs = true);
```

`SceneAnimInfo` contains `valid`, `name`, `t_start`, `t_end` and `frame_duration`.
It is a value snapshot, not an editable view of a clip.

`BuildSceneAnimNodeMap` matches authored nodes by complete root-relative name
paths. It reports ambiguous paths and missing targets. Its result contains
`success`, `message`, `source_nodes` and `destination_nodes`. For renamed but
otherwise compatible hierarchies, construct `SceneAnimNodeMap`, supply both node
vectors and set `success=true`; import still validates the supplied map.

`ImportSceneAnim` validates scene membership, unique targets, parent relationships,
clip compatibility and destination name collisions before publishing copied
tracks. Every animated source node must be mapped, together with its ancestry.
The result exposes `success`, `message`, `anim`, `copied_channels` and
`completed_channels`. Copies remain usable after clearing the source scene.
Validation failures do not add destination animations.

With `preserve_missing_trs=true`, missing or empty channels become constant tracks
from the source scene's current local transform values. This includes mapped
nodes with no animation tracks at all. Load the source in its authored default
pose before import. Setting this option to false keeps the original sparse
tracks, whose missing channels will use the destination player's reference pose.
The importer does not verify mesh bind matrices or rewrite skinning data: the
authoring tool must establish skeleton/mesh compatibility first.

Load authoring scenes with `LSSF_Nodes | LSSF_Anims | LSSF_DoNotLoadResources` for
headless import, then save with `SaveSceneJsonToFile`. The output uses the existing
scene format and normal `assetc` path. `LSSF_DoNotLoadResources` also defers
automatic nested-instance setup; explicitly set up an instance if a headless
test needs its view. `Node::SetupInstanceFromAssets` now forwards its `flags`
argument, as the file-loading variant already did.

## Martian Melodies integration

The demo generates `assets/generated/anims/cosmonaut_blendable.scn` from the five
original cosmonaut scenes. `main_scenery.scn` references that common rig. A Python
build driver validates the shared mesh resource and bone order; a Lua assembly
tool imports clips through the native API and compares five sampled poses per
clip against their source. The source scenes remain authoring inputs.

After import validation, the tool removes accumulated hip heading from the two
turn clips in the generated output. This is specific to the demo: its controller
owns placement rotation and advances continuously to the target heading. It
avoids a heading jump when the generated turn pose blends into walk or idle.

The controller configures a 200 ms smoothstep fade and uses metadata from the
loaded instance for clip durations. The normal asset build runs the assembly tool
before `assetc`. See the demo's `src/documentation/astronaut-wander-animations.md`
for commands and validation tools.

## Validation and limits

Validation on Windows x64 Release covers:

- C++ animation and scene regression groups, plus `engine.scene_animation`:
  four loop/once combinations, endpoints shorter than fades, interruptions,
  repeated/restarted requests, sparse channels, quaternion shortest arcs, easing,
  ownership, stop/clear/destruction, large time steps and binary import roundtrip.
- Lua and Python binding builds and API smoke checks using existing enum constants.
- Source-to-import pose comparisons for all five demo clips, including absent
  channels, followed by save/reload validation and heading normalization checks.
- The actual demo controller on compiled assets for 120 simulated seconds each
  at 30, 60 and 120 FPS, and two independent instances with destruction of one.
- Rendered captures of all five clips and six transitions for inspection of mesh,
  attachment and pose continuity.
- A bounded 600-frame run of the full demo with its normal controller and compiled
  assets, using the non-AAA rendering path and muted audio. Its existing scenes
  reference three unavailable resources (two title textures and
  `props/metal_tents/metal_tent_40.geo`); those warnings are outside the animation
  integration.

This is functional validation, not a performance benchmark. No new animation
serialization format or renderer/shader feature is required. The demo's previous
`assetc` binary predates the engine's current scene format, so its local compiler
and Lua module are updated together and the compiled scenes are rebuilt.
