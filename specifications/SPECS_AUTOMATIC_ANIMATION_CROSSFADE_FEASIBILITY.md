# Automatic Animation Crossfade Feasibility

Date: 2026-09-25

Status: feasibility study and proposed runtime contract; not implemented.

Method: static inspection of engine code, bindings, application code and source
scene JSON. No modified engine was built, no visual playback comparison was run,
and no performance measurements were taken. Effort and performance estimates
below are engineering estimates.

Engine revision inspected: `c3dd8a4501418f77ea37481f197391cd5d786335`.
Reference application revision: `d03b2e2abaeed82b03ff85128d67595ba7fb2a40`, at
`C:/works/projects/demo-martian-melodies/src`.

## Feasibility verdict

**Automatic crossfading is feasible, with moderate engine integration work.**
The recommended implementation is an optional, scene-owned animation player
associated with an instance or an explicit group of animated nodes. Calling
`Play` on that player starts the requested clip and automatically fades from its
current output over a configured duration. Both clips can independently loop or
play once. Existing `Scene::PlayAnim` calls retain their current behavior.

The essential engine change is to separate **sampling animation values** from
**writing those values into the scene**. Two clips must produce independent
poses, which are blended and applied once. Starting two existing animations
simultaneously does not perform this operation.

**The supplied astronaut also needs an asset conversion.** Its current master
scene switches between five separate character instances. Their animation
tracks target different skeleton nodes, so adding playback weights alone would
not create a skeletal crossfade. The practical route is one visible character
and one skeleton with all five clips bound to it. The inspected hierarchies make
this plausible, but skin bind poses and visual equivalence still need validation.

The recommended asset workflow is a **Lua assembly tool backed by a small C++
animation import API**. It should copy clips from separate scenes onto a validated
common hierarchy and save an ordinary reusable `.scn`. This can be delivered and
validated before the crossfade player itself.

An initial release should cover local position, rotation and scale on a common
node hierarchy, including interruption and all four loop/once combinations.
Arbitrary scene timelines, nested animation-selection tracks and a general
animation graph should be separate extensions.

## Requested behavior

The trigger is a new playback request, not the natural end of an animation:

1. A is playing, either once or in a loop.
2. The application requests B on the same player.
3. A keeps advancing from its current time. B starts at its requested entry time.
4. During a configurable interval, A's weight decreases from one to zero and B's
   increases from zero to one.
5. B continues alone with its own loop mode and playback time.

A looping A must not prevent an immediate transition to B. A non-looping B must
not start over when the transition finishes. Selecting which clip follows a
completed clip remains application logic; automatic queuing is not required.

## What the current implementation actually does

### Playback and evaluation

| Source | Observed behavior | Consequence |
| --- | --- | --- |
| [scene.h](../harfang/engine/scene.h), `ScenePlayAnim`, around lines 893-906 | Stores binding, clock, range, fixed-point speed, pause flag, loop mode and easing; no blend weight or player ownership. | Needs additional runtime state. |
| [scene.cpp](../harfang/engine/scene.cpp), `PlayAnim`, around lines 2370-2391 | Binds a clip, initializes playback and appends it to `play_anims`. | There is no unique scene-wide "current animation". |
| `scene.cpp`, `UpdatePlayingAnims`, around lines 2397-2447 | Advances each playback and immediately evaluates it into the scene. | Overlapping properties are overwritten in traversal order, not blended. Do not rely on "last started wins": storage traversal is not an ownership contract. |
| `scene.cpp`, `EvaluateBoundAnim`, around lines 2053-2284 | Writes transforms, lights, camera FOV, material values, environment properties and node enable state; also recursively evaluates instance animations. | A second evaluation can observe or overwrite the first one's effects. |
| [animation.h](../harfang/engine/animation.h), `Evaluate*`, around lines 109-168 | Track samplers already return values; empty tracks return false and out-of-range samples use endpoint keys. | Reusable basis for a sampler that produces poses without modifying the scene. |
| [bind_harfang.py](../binding/bind_harfang.py), around lines 1289-1301 | Exposes play, stop, status and animation updates; no weighted pose sampling or player API. | A small script wrapper around existing play calls is insufficient. |

The existing `easing` argument belongs to a single clip's playback time path. It
is not a crossfade weight. The new transition curve must be a separate setting;
its correctness must not depend on the existing easing block.

`ALM_Loop` wraps within the clip range. `ALM_Once` clamps to its terminal time,
evaluates that endpoint and removes the playback reference during the same
update. `ALM_Infinite` keeps advancing without wrapping or automatic removal;
ordinary value tracks then hold their endpoint values. It is distinct from
`ALM_Loop`.

`IsPlaying` currently tests playback-reference validity. Removing a playback
does not restore the scene's original values: its last writes remain until
something else changes them.

### Instances and rendering

[Node::GetInstanceSceneAnim](../harfang/engine/node.cpp) resolves an animation
through the node's `SceneView`; it returns a scene animation reference, not a
separate animation player. A `SceneAnim` contains explicit `NodeRef`/`AnimRef`
pairs. The [JSON scene loader](../harfang/engine/scene_load_json.cpp), around
lines 986-999, remaps these references when loading an instance. Runtime blending
should therefore match resolved node/property identities, not global node names.

Both `Scene::Update` and the implementations in
[scene_systems.cpp](../harfang/engine/scene_systems.cpp) call
`UpdatePlayingAnims` before the final world-matrix computation. With physics,
animation evaluation also precedes physics synchronization. This is the correct
place to integrate pose blending so all update paths consume the same result.
Scene scripts and physics can still modify transforms later in the frame; that
ordering must remain explicit.

Skinning already obtains bone matrices from scene transforms, through
`Scene::GetModelDisplayLists` and the scene rendering pipeline. Blending local
bone transforms before world-matrix computation should therefore require no
new skinning shader or duplicate draw of the character. This is an architectural
conclusion, not a measured result.

For public context, the [upstream scene implementation](https://github.com/harfang3d/harfang3d/blob/main/harfang/engine/scene.cpp)
also contains the playback entry points. The local revision identified above is
the authority for this study; upstream line numbers and behavior may differ.

## Findings specific to the Martian Melodies astronaut

### Application behavior

In `src/astronaut_wander.lua`, `play_anim` (lines 51-72) obtains the instance
animation, stops `state.playing_anim_ref`, then calls `scene:PlayAnim`:

- `idle`, `walk` and the reserved `crouch_walk` use `ALM_Loop`.
- `turn_left` and `turn_right` use `ALM_Once`.
- Repeated requests for the same name are ignored unless forced to restart.
- Turning uses both clip motion and scripted parent yaw compensation.

There is **no existing crossfade** in that helper. The application's
`src/documentation/astronaut-wander-animations.md` explicitly describes the
absence of blending. In `src/main.lua`, lines 445-446, the controller runs just
before `scene:Update(dt)`.

### The master scene switches whole instances

`src/assets/main_scenery.scn` points the `astronaut` node to
`anims/cosmonaut_master.scn`. That master scene contains these five instances:

| Master node | Referenced source scene | Actual source clip | Animated node bindings |
| --- | --- | --- | --- |
| `idle` | `anims/rifle_idle/rifle_idle.scn` | `idle` | 18 |
| `rifle_walk` | `anims/rifle_walk/rifle_walk.scn` | `riffle_walk` | 26 |
| `rifle_turn_left` | `anims/rifle_turn_left/rifle_turn_left.scn` | `turn_left` | 20 |
| `rifle_turn_right` | `anims/rifle_turn_right/rifle_turn_right.scn` | `turn_right` | 20 |
| `crouch_walk` | `anims/rifle_crouch_walk/rifle_crouch_walk.scn` | `crouch_walk` | 20 |

The master clips mainly contain boolean `Enable` tracks and `instance_anim`
selection keys. They enable the selected copy, disable other copies and evaluate
the selected child clip. The source spelling `riffle_walk` is intentional in
this table: it differs from the master's logical name `walk`.

All five inspected source scenes contain 27 nodes, one object with 22 bone
references, and the same node names and named parent relationships. Their
animation tracks use quaternion rotations and Vec3 position/scale. However:

- Their actual runtime node references are different.
- Their serialized initial transforms differ, notably for rotations.
- Their track coverage differs, so copying only existing tracks can leave
  missing channels at an incorrect default or at a previous clip's value.
- Matching names and hierarchy does not prove equality of the geometry's skin
  bind matrices, bone ordering, coordinate conventions or attachment alignment.

Blending the master `Enable` flags cannot produce an intermediate body pose.
Enabling both copies would render two characters; fading their materials would
be an image dissolve with different depth/shadow behavior. Neither supplies the
requested skeletal transition.

### Required asset preparation

Prepare a new common-rig scene with a reproducible offline conversion:

1. Select a canonical mesh and hierarchy, preserving its bone references and
   attachment nodes. Validate skin bind matrices and source coordinate systems.
2. Map source nodes onto that hierarchy using validated relative paths and
   parent relationships. Name matching alone must reject ambiguity.
3. Copy each clip's tracks and remap their node references. Expose the logical
   names `idle`, `walk`, `turn_left`, `turn_right`, `crouch_walk` on that instance.
4. For missing channels, preserve each source scene's effective initial value,
   using constant keys where necessary. Convert serialized units through the
   normal loader/converter conventions, including rotations. If bind-space
   differences remain, rebake or retarget instead of assuming a raw copy works.
5. Remove the five-copy enable/disable timeline from the new playback path.
6. Compile generated source assets through `assetc` and load only compiled
   output at runtime. Retain the original sources for comparison.

The structural inspection supports this conversion, but standalone playback of
each migrated clip must be compared with its original before testing blending.
The engine feature is independently useful for assets already authored with
multiple clips on one rig.

## Assembling clips through C++ and Lua

### Existing building blocks and the binding gap

The proposed tool does not require creating animation storage from scratch:

| Capability | C++ today | Lua bindings today |
| --- | --- | --- |
| Load and save scene files | `LoadSceneFromFile`, `SaveSceneJsonToFile`, `SaveSceneBinaryToFile` | Exposed. |
| Enumerate nodes and inspect names/parents | Scene/node/transform APIs | Exposed, including `SceneView::GetNodes`. |
| Find/list animation references | `GetSceneAnim`, `GetSceneAnims` | Exposed. |
| Inspect clip metadata and its node bindings | `GetSceneAnim(ref)` returns `SceneAnim*` | That inspection overload and the editable `SceneAnim` structure are not exposed. |
| Read/copy tracks and insert them | `GetAnim`, `AddAnim`, `AddSceneAnim` | Not exposed in the inspected binding definition. |
| Duplicate a clip in its current scene | `DuplicateSceneAnim` | Not exposed; also insufficient for cross-scene import. |

`Scene::DuplicateSceneAnim`, around lines 2451-2479 of `scene.cpp`, already
illustrates value-copying `Anim` objects and constructing a new `SceneAnim`.
However, it preserves the original target nodes and adds tracks to the same
scene. The missing reusable operation is **copy tracks into a destination scene
and remap every target node**.

Exposing every mutable track template would make a larger public binding
surface. For this use case, prefer a small validated native import operation,
plus metadata and node-mapping helpers. Lua should orchestrate file selection,
clip naming, mapping overrides, batch processing, reports and saving; C++ should
own track copying, default completion and reference correctness.

### Proposed import API

These names and types are design proposals, not current API:

```cpp
SceneAnimInfo GetSceneAnimInfo(const Scene &scene, SceneAnimRef anim);

SceneAnimImportResult ImportSceneAnim(
    const Scene &source, SceneAnimRef source_anim,
    Scene &destination,
    const std::vector<SceneAnimNodeMapping> &node_map,
    const SceneAnimImportOptions &options);
```

`SceneAnimInfo` should provide the name, time range, frame duration, animated
nodes and property kinds without exposing mutable internal pointers.
`SceneAnimNodeMapping` should pair scene-aware source and destination `Node`
wrappers. Plain numeric indices are insufficient: two scenes can have identical
index/generation values referring to different nodes.

`SceneAnimImportOptions` minimally needs the destination name and a
`preserve_missing_trs` option, enabled for this tool. The initial name-collision
policy is failure, with no implicit replacement. The result reports success,
the destination animation reference, copied/completed channel counts and
actionable diagnostics. A `BuildSceneAnimNodeMap` convenience helper can match
relative paths and validate parent correspondence; allow explicit overrides.

The native import contract should be:

1. Validate source/destination scene membership, all references, unique target
   mapping, clip range, supported track kinds and destination name before
   publishing any changes. Require mapping coverage for the intended whole rig,
   including nodes with no tracks in that particular clip.
2. Preserve times, key values, interpolation parameters, rotation representation
   flags and frame duration. Clone track data into destination-owned `Anim`
   objects; never retain references to source scene storage. Preserve relevant
   animation semantics while clearing runtime instantiation flags on new
   standalone assets. The current serializers skip assets marked instantiated.
3. Replace each source node reference with its mapped destination reference.
   Mapping is identity correspondence, not automatic retargeting: report known
   hierarchy/space mismatches. Skin bind compatibility remains an asset-level
   check and is not proven by this API.
4. With `preserve_missing_trs`, add constant position/rotation/scale channels
   using the unplayed source scene's local defaults wherever source channels are
   absent or empty. Include mapped nodes lacking an entire `NodeAnim`. Do not
   fill from destination defaults or a source scene already changed by playback.
   Match the clip's active rotation representation when generating constants.
5. Initially accept direct transform clips. Reject scene-global animation,
   enable/disable selection tracks and `instance_anim` dependencies with clear
   diagnostics; the tool should import the five source clips, not master clips.
6. Leave destination hierarchy, mesh, materials, bone bindings and initial pose
   intact. Add only animation data. On failure, roll back staged insertions so
   the destination contains no partial clip or orphan tracks.

This primitive can later gain other property types. Broadening its scope is
independent of adding crossfading to playback.

### Lua tool workflow

Load the canonical source scene as ordinary nodes, without its animations, into
a fresh destination scene. Load each animation source into a separate temporary
scene. Both `mixamorig:Hips` and the mesh node are roots in these files, so mapping
must cover the complete scene hierarchy/forest, not only descendants of the mesh
node. The outer gameplay placement node is not part of the imported rig.

The following is illustrative tool pseudocode. The authoring loader is a tool
helper over existing scene I/O; mapping/import types are the proposed additions:

```lua
local destination = load_source_scene_for_authoring(canonical_path, false)

local clips = {
    {"rifle_idle/rifle_idle.scn", "idle", "idle"},
    {"rifle_walk/rifle_walk.scn", "riffle_walk", "walk"},
    {"rifle_turn_left/rifle_turn_left.scn", "turn_left", "turn_left"},
    {"rifle_turn_right/rifle_turn_right.scn", "turn_right", "turn_right"},
    {"rifle_crouch_walk/rifle_crouch_walk.scn", "crouch_walk", "crouch_walk"}
}

for _, clip in ipairs(clips) do
    local source = load_source_scene_for_authoring(anim_root .. clip[1], true)
    local mapping = hg.BuildSceneAnimNodeMap(source, destination)
    assert(mapping.success, mapping.message)

    local options = hg.SceneAnimImportOptions()
    options.name = clip[3]
    options.preserve_missing_trs = true
    local result = hg.ImportSceneAnim(
        source, source:GetSceneAnim(clip[2]), destination, mapping.nodes, options)
    assert(result.success, result.message)
    source:Clear() -- imported tracks must remain valid independently
end

assert(hg.SaveSceneJsonToFile(output_path, destination, resources))
```

Use a manifest with explicit source paths and names, recreate the destination
on each run, and write to a separate generated output. Save only after the entire
batch succeeds; write to a temporary output and replace the generated file only
after successful serialization and reload validation. Source files remain input.

A proposed generated asset is
`assets/generated/anims/cosmonaut_blendable.scn`. The normal asset build compiles
it, and the main scene's astronaut instance can then reference that single asset.
The offline tool reads authoring scene files deliberately; application playback
and visual previews use compiled assets and resolved resource dependencies.

The first tool milestone is useful even with today's hard-switch playback:
load the generated scene, find all five logical clips, play each individually,
and confirm it matches its original. Do not concatenate the clips end-to-end;
store them as separate named animations with their own time ranges. Loop/once
remains a playback choice.

This workflow avoids a manual DCC merge when the existing rigs are compatible.
If skin bind matrices or animation spaces differ, the report must identify the
incompatible inputs; a validated rebake/retargeting pass is then additional work.

## Recommended API and ownership

Use a scene-owned player with an instance-scoped convenience constructor. Its
runtime record belongs to the scene; a Lua/C++ wrapper only references it.
Dropping a script variable must not implicitly stop an otherwise active player.
Destroying the instance or scene invalidates the wrapper safely.

The following is **proposed API**, not available in the inspected bindings:

```lua
-- After loading the converted common-rig instance:
local actor = scene:GetNode("astronaut")
local player = scene:CreateInstanceAnimPlayer(actor)
player:SetCrossFadeDuration(hg.time_from_sec_f(0.20))

local idle_ref = player:Play("idle", hg.ALM_Loop)

-- On a later state change: retains the outgoing clip automatically.
local turn_ref = player:Play("turn_left", hg.ALM_Once)

-- Can also interrupt a transition already in progress.
local walk_ref = player:Play("walk", hg.ALM_Loop)
```

`0.20` seconds is an initial tuning value for this actor, not an engine-wide
default. Player creation starts with a zero crossfade duration until configured.

Proposed minimal surface:

| Operation | Contract |
| --- | --- |
| `Scene::CreateInstanceAnimPlayer(node)` | Creates one player for that instance and resolves its clips within that view. Rejects duplicate ownership. |
| `player.SetCrossFadeDuration(time_ns)` | Sets the default for subsequent requests; does not retime a transition already underway. |
| `player.Play(name, loop_mode = ALM_Once, restart = false)` | Validates and starts or transitions to a clip; returns a `ScenePlayAnimRef` for that request. |
| `player.IsTransitioning()` | Reports whether a transition envelope remains active. |
| `player.Stop()` | Cancels live clips and transitions, releases held-pose ownership and leaves the last applied values in place. |
| `Scene::DestroyAnimPlayer(player)` | Releases player resources and invalidates its wrapper. |

An options overload can later expose a per-request fade duration, playback
range, initial time, speed and transition curve. A C++ factory for an explicitly
declared node/property scope can reuse the same implementation outside scene
instances. Neither extension is necessary to demonstrate the first release.

Each player has one current target and, during an ordinary transition, one
outgoing clip. The player defines which animation is "current". A global
"crossfade every call to Scene::PlayAnim" flag would incorrectly couple actors,
camera animations and independent scene effects.

Existing unscoped playback remains unchanged when no player owns its targets.
For the initial player path, reject conflicting animation ownership during
validation, including existing on-instantiate playback. An application can
explicitly stop that playback before creating its player. New legacy playback
requests overlapping a registered player's channels should be diagnosed and
rejected, rather than silently fighting the player's output. Direct application
transform writes remain possible; their ordering is the application's contract.

## Crossfade semantics

### Two clip clocks and one transition clock

Let `D` be the fade duration, `e` elapsed scene time since the request and `w`
the incoming weight:

```text
u = clamp(e / D, 0, 1)
w = u                         # linear default
position = lerp(position_A(tA), position_B(tB), w)
scale    = lerp(scale_A(tA),    scale_B(tB),    w)
rotation = normalize(slerp(rotation_A(tA), rotation_B(tB), w))
```

Advance `tA` and `tB` with their own speeds and loop modes. Advance `e` with the
nonnegative `dt` passed to the animation update, independently of clip lengths
and clip speeds. This follows scene time, including deliberate slow motion; it
does not read a separate wall clock. `dt == 0` leaves the transition unchanged.

At the request boundary, `w == 0` and the output is continuous with A. After an
update of `dt`, the output represents both advanced clips at `e + dt`; it need
not render an extra frame with zero weight. On completion, B retains its already
advanced clock. Do not restart B or delay its clock until the fade has ended.

Use local transforms. Normalize source quaternions; convert sampled Euler
rotations into quaternions before blending, then convert the result back to the
engine's current Euler TRS representation when committing. HARFANG already has
`QuaternionFromEuler`, `ToEuler` and a shortest-path `Slerp` in
[quaternion.cpp](../harfang/foundation/quaternion.cpp). Preserve the existing
interpolation inside each clip. Do not interpolate matrices element by element.

The first version can use only a linear envelope. If curves are added, require
bounded weights with exact endpoints; an overshooting easing curve is not a
valid default blend weight. Optional phase synchronization is separate: equal
weights do not imply matching foot-contact phases.

### Loop and once combinations

| Outgoing A | Incoming B | During the fade | After the fade |
| --- | --- | --- | --- |
| Loop | Loop | Both advance and wrap independently. | B continues looping at its advanced time. |
| Loop | Once | A wraps; B advances towards its end. | B continues once, or holds its endpoint if already finished. |
| Once | Loop | A advances, then holds its endpoint if reached; B wraps. | B continues looping. |
| Once | Once | Each advances and independently holds its endpoint on completion. | B continues once or remains at its endpoint. |

Holding an outgoing endpoint is essential when A ends before its weight reaches
zero. Deleting all source pose state at that point would make the transition
jump. Likewise, if B is shorter than `D`, retain B's endpoint while the envelope
finishes. Do not shorten `D` implicitly: the application may choose a shorter
duration for brief actions.

For example, if A has 50 ms remaining, B lasts 100 ms and `D = 200 ms`, A holds
its endpoint after 50 ms, B holds its endpoint after 100 ms, and the blend still
reaches B at 200 ms. B has completed playback at 100 ms even though the transition
continues for another 100 ms.

If A finished before B was requested, fade from the player's held final output.
On first playback with no previous output, start B directly. A fade from an
explicit reference pose can be an optional later behavior.

### Completion, handles and cancellation

Separate clip lifetime from pose contribution lifetime:

- Preserve `Scene::IsPlaying(ref)` as reference-validity status. A managed
  once-only request expires when its clip reaches the end, after its endpoint
  has been sampled; its final pose can outlive that reference internally.
- A looping outgoing reference expires when its fade contribution is retired.
- A completed incoming pose stays owned and applied by the player until another
  request or an explicit stop, while `IsPlaying` for its request is false.
- `IsTransitioning` can therefore be true while B's `IsPlaying` is false. A game
  state that must wait for both can test both conditions.
- Stopping the current target via `StopAnim(ref)` cancels that player's active
  transition and releases its held output. Stopping a still-live outgoing
  reference freezes its current source pose while the fade to B continues.
  Invalid or already expired references remain harmless no-ops.
- `StopAllAnims`, player destruction, instance destruction and scene clearing
  must cancel managed playback and release held outputs as well as legacy plays.

These are proposed semantics for the new path. They avoid keeping a once-only
playback artificially "playing" just because its pose is still needed.

### Interrupted transitions and repeated requests

For a new C request during A-to-B, capture the player's last committed blended
local pose and fade from that fixed pose to advancing C. Retire A and B's live
requests. This preserves pose continuity and bounds work to one captured pose
plus C. Velocity continuity is not guaranteed; preserving it requires a more
advanced transition algorithm or a bounded blend tree.

Do not use B alone as the new source: B may still have a low weight. Also do not
capture arbitrary world transforms after physics or unrelated scripts have
modified them. Use the animation player's own committed local output. Several
requests before the next update use that same output, with the latest accepted
request becoming the target; intermediate unrendered requests are retired.

A request for the same active target, mode and options is idempotent unless
`restart` is true. A request for an already completed target starts it again.
A mode/options change is a new request. A forced restart crossfades to a fresh
clock, or switches immediately when the fade duration is zero.

### Time edge cases

- A zero duration performs an immediate switch without division. Reject negative
  durations and invalid clip references before changing the current playback.
- Validate `t_end > t_start` for looping ranges. Existing wrap loops can fail to
  progress for a zero-length range; the new path must reject it. Treat an explicitly
  supported single-pose non-looping clip as an endpoint sample.
- A large `dt` must handle multiple wraps, terminal samples and a fully completed
  fade in one update. Prefer bounded modulo wrapping over repeated subtraction.
- Initially support forward playback at normal speed in the minimal API. If
  speed/range controls are exposed, validate finite representable speed values,
  define reverse entry at `t_end`, and clamp reverse once-playback at `t_start`.
  The legacy record stores speed in a signed fixed-point byte; do not silently
  promise arbitrary floating-point rates or reuse its start initialization for
  reverse once-playback.
- A zero clip speed freezes that clip, not the transition. If player pause is
  added, it freezes both clip clocks and the envelope. A future per-clip pause
  affects only the selected clip clock.
- `ALM_Infinite` can retain its unbounded clock semantics if exposed; it must not
  be treated as a synonym for looping. Negative scene `dt`/reverse timeline
  scrubbing is outside the initial contract.

## Sampling, channel coverage and scope

Introduce a reusable runtime pose buffer containing local position, quaternion
rotation and scale with channel-validity information. Bind and validate tracks
when a request is accepted, rather than searching by node name every frame.

The player owns an explicit node/property domain. For the instance convenience
API, derive it from the instance's validated transform clips and keep it stable
for the player's lifetime. Capture reference values for that domain before
player playback begins. Refreshing the clip set or hierarchy requires explicit
revalidation and a new reference capture; do not let it change silently.

For each sample, fill missing owned channels from that stable reference:

| Channel coverage | Blend behavior |
| --- | --- |
| Present in both clips | Blend independently sampled A and B values. |
| Present only in A | Fade A towards the reference value. |
| Present only in B | Fade from the reference value towards B. |
| Present in neither, but inside the player's declared domain | Keep the reference value. |
| Outside the declared domain | Do not write it. |

Never use a previous frame's blended output as the normal missing-channel
default: it introduces feedback and makes results depend on history and frame
rate. A captured interruption pose is a deliberate, fixed exception. Asset
conversion must bake source-specific constant values where they differ from
the common reference, as is possible with the astronaut's sparse clips.

Exclude the outer actor placement transform from this domain when application
logic owns movement and heading. Two players can run independently only with
disjoint owned channels. Layers, masks, additive animation and shared-channel
weight normalization can be added later with explicit composition rules.

### Scene properties beyond the initial transform scope

The term "scene animation" covers more than skeleton transforms in HARFANG.
The sampler architecture can later support the following, but the first release
must validate and reject unsupported clips instead of silently ignoring tracks:

| Property kind | Possible extension policy |
| --- | --- |
| Lights, FOV, colors, material Vec4 values | Blend numeric values after sampling and enforce applicable value constraints. |
| Scene fog and ambient values | Blend only with explicit scene-global ownership; an instance should not claim them implicitly. |
| Boolean enable and other discrete properties | Select one value using a documented threshold or handoff rule; there is no numeric pose interpolation for these values. |
| Nested `instance_anim` tracks | Resolve child clips and sample recursively into independent buffers, with depth/cycle guards and defined child clock policies. |
| Events, callbacks or animation-selection side effects | Define ownership and delivery rules so two contributing clips do not trigger unintended duplicate effects. No new event system is required for the initial release. |

An actor being a scene instance does not itself require support for nested
selection tracks. A converted common-rig instance can expose ordinary transform
clips directly. The current astronaut master is an example of an unsupported
nested/discrete clip and should receive a useful diagnostic.

## Engine integration plan

1. **Add a pose sampler and commit operation.** Reuse the existing track
   evaluators and binding target tables. The transform sampler must have no scene
   writes, node enable operations or nested playback side effects. Reuse sampled
   values in the legacy evaluator where safe, with regression coverage for its
   existing behavior.
2. **Add scene-owned player records.** Store current and outgoing clip states,
   transition elapsed/duration, reference pose, sampled poses, last output and
   optional interruption snapshot. Use generational references for public
   handles and validate node/component references before committing.
3. **Integrate once in `UpdatePlayingAnims`.** Classify managed and legacy
   playback explicitly. Managed clips must not also pass through the legacy
   direct-write evaluator. Advance clocks, sample endpoints, blend and commit,
   then retire expired references. Keep the existing matrix/physics order.
4. **Handle lifecycle and animation edits.** Update `Clear`, `StopAllAnims`,
   `NodeDestroyInstance`, `DestroyViewContent` and animation destruction paths.
   `GarbageCollectAnims` currently scans scene animation assets; new bindings and
   caches must not outlive their inputs. On deletion/reload of a required asset,
   cancel the affected player safely or rebind through an explicit operation.
   Do not keep invalid raw pointers or re-use a destroyed node's storage slot.
5. **Expose the API through `binding/bind_harfang.py`.** Generate and smoke-test
   Lua and Python, and Squirrel where enabled. Keep defaults and reference
   lifetime behavior aligned with C++.
6. **Document and demonstrate it.** Add a minimal instance example and API docs,
   then migrate the astronaut helper once the new assets are validated.

Keep the new runtime state unserialized initially. No `.scn` or animation format
change is inherently required for blending transform clips, and existing assets
remain usable on the legacy path. Converted astronaut assets are a separate
content change. Rebuild consumers of affected C++ classes and bindings: source
compatibility does not imply binary compatibility if the layout of `Scene`
changes.

## Astronaut controller migration and orientation risk

Replace the helper's stop/play pair with one player request and preserve its
same-name and force-restart behavior. Store the player's returned request ref
where the controller currently stores `playing_anim_ref`. Initialize the player
after loading the converted instance, and keep the existing once-per-frame
`scene:Update(dt)` flow. Do not add a second animation time step in Lua.

The turn code deserves its own integration test. It compensates for authored
internal yaw (`-163` degrees for left, `153` for right), then assigns the logical
target yaw to the outer transform at the state boundary. Under a hard switch,
the outgoing rig disappears. Under a fade, some of its internal rotation can
remain visible while the outer transform already uses the new yaw. Excluding
the outer transform from blending does not, by itself, fix that mismatch.

For the first visual prototype, validate idle/walk fades before turn transitions.
Then choose and validate one consistent orientation policy:

- Normalize/rebake accumulated heading out of the turn clips and drive the full
  desired heading trajectory in the application; or
- Retain the authored root rotation and adapt the visual-root compensation to
  the blended orientation throughout the transition, including interruption.

The acceptance criterion is continuity of the composed world orientation,
not just smooth local quaternion interpolation. Foot sliding, mismatched gait
phase and root-motion transfer are not automatically solved by a pose crossfade.
The current duration timer and `IsPlaying` condition must also follow the chosen
state-transition policy: clip completion and fade completion are separate facts.

## Alternatives and effort

| Approach | Feasibility and tradeoff |
| --- | --- |
| Stop A and play B | Already implemented; no intermediate pose. |
| Run two existing `PlayAnim` calls | Does not blend overlapping values and can leave both plays alive. |
| Lua snapshot interpolation after scene evaluation | Can demonstrate a fixed-pose-to-B fade, but requires correct update ordering and access to every affected node. It does not keep A advancing; the current separate-rig assets remain a problem. |
| Independent sampling scenes controlled from Lua | Technically possible as a prototype, but duplicates scene state and adds mapping, lifetime and scripting overhead. Poor fit for an automatic engine mode. |
| Native player with pose buffers | Recommended: explicit ownership, predictable cost, shared implementation across languages. |
| Full animation graph, state machine and retargeting system | Broader capabilities, substantially larger scope than this request. |

Estimated work for one developer familiar with the codebase, with working build
and asset tools already available:

| Milestone | Estimated developer days | Reviewable result |
| --- | --- | --- |
| Transform sampling and two live clip prototype | 2-4 | Synthetic common-rig A/B blend with independent clocks. |
| Ownership, completion, interruption and lifecycle | 3-5 | Automatic player with bounded state and defined edge behavior. |
| Bindings, docs and regression validation | 2-4 | C++/Lua/Python feature and enabled-language smoke checks. |
| C++ clip import API and Lua assembly tool | 3-6 | Validated mapping, independent track copies, default completion and repeatable generated scene. |
| Astronaut asset validation and controller integration | 2-4 | All five clips preserved; state helper uses player; heading transitions checked. |
| **Total initial scope** | **12-23** | Reusable assembly workflow, validated transform crossfade and astronaut demonstration. |

This range excludes asset reauthoring if skin/bind-space incompatibilities are
found, build-environment repair and general nested timeline blending. A transform
prototype can establish engine feasibility before committing to the entire
integration. Generic numeric/discrete scene animation support should be
estimated after the prototype exposes its binding and ownership requirements.

## Performance expectations

For a common-rig actor, an ordinary transition samples two clips instead of one
and blends one pose. Approximate work is `sample(A) + sample(B) + O(P)`, where
`P` is the number of owned property channels. HARFANG's current key interval
lookup scans keys, so sampling cost also depends on key count; it is not strictly
constant per channel. Do not interpret this as a doubling of total frame time.

Outside transitions, only the active clip needs sampling. Pose buffers and
reference values require `O(P)` memory per player. The interruption snapshot
policy prevents the number of sampled clips from growing with repeated requests.
Bindings and buffers should be reused without steady-state heap allocation.

One final skeleton and mesh remain rendered per actor. The converted astronaut
may also avoid retaining multiple instance hierarchies, although GPU resource
sharing and actual memory savings have not been measured. Benchmark CPU sampling,
blending, allocation count and retained memory for 1, 10 and 100 actors, both
inside and outside transitions, before setting a supported scale target.

## Validation and acceptance criteria

Use the existing `engine.animation` and `engine.scene` test groups in
[harfang/tests](../harfang/tests), with small synthetic tracks and controlled
time steps. Add tests to verify behavior, rather than merely the proposed data
structure.

The assembly tool also needs tests independent of blending: mismatched/ambiguous
maps, unrelated scenes with coincident numeric node references, duplicate clip
names, sparse default completion, unsupported nested clips, source scene
destruction immediately after import, failed-import rollback and a save/reload
round trip preserving all five names, ranges and poses. Rebuilding from the same
manifest must not append duplicate clips or alter input files.

| Case | Required observation |
| --- | --- |
| Four loop/once combinations | Correct independent sample times, endpoint holds and target continuation. |
| Mid-clip A-to-B request | First transition boundary preserves A; half-duration gives the expected blend of both advanced poses; completion equals B at its advanced time. |
| Source/target shorter than fade | No missing contribution, snap or premature envelope termination. |
| Request after a once-only clip ended | Uses held output even though the old playback reference is invalid. |
| Interrupted A-to-B-to-C, including repeated same-frame calls | Output is continuous at each accepted boundary; retained state remains bounded. |
| Same target and forced restart | Idempotent requests do not restart clocks; explicit restart does. |
| Zero duration, zero/large `dt`, invalid ranges | Defined immediate switch or rejection; no divide-by-zero, unbounded loop or dropped terminal sample. |
| Quaternion sign and Euler wrap cases | `q` and `-q` produce the same orientation; shortest-path rotation does not take a long detour around +/-180 degrees. |
| Sparse clips | Deterministic fallback with no previous-frame feedback or leaked pose from a prior clip. |
| Two instances with identical node names | Each player changes only its own resolved nodes. |
| Stop, clear, asset deletion and instance reload | No stale handles, retained writes, leaks or access to reused node storage. |
| Legacy animation and ownership conflict | Unmanaged behavior remains unchanged outside owned channels; conflicts are diagnosed. |
| Unsupported master/discrete/nested clips | Clear rejection without disturbing the current accepted playback. |
| All scene update entry points | Same blend result via `Scene::Update` and appropriate `SceneUpdateSystems` variants; no double advancement. |
| Large step versus equivalent smaller steps | Equivalent final pose/time for deterministic clips within numeric tolerance, absent intervening requests. |

For the visual demonstration, compare original and converted clips individually,
then record idle-to-walk, walk-to-idle, idle-to-turn, turn-to-walk and interrupted
turns at 30, 60 and 120 updates per second. Confirm one visible character,
preserved attachments, stable skinning and continuous world heading. Exercise
the reserved crouch clip to cover the different track coverage too.

The work is ready to ship when ordinary callers request one animation on a
player, the engine owns the crossfade automatically, loop/once behavior matches
the contract, all original non-player playback remains valid, and the converted
astronaut demonstrates pose and heading continuity. This study establishes the
implementation route and the asset dependency; those runtime acceptance checks
remain to be performed during implementation.
