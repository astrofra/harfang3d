# HARFANG to glTF animation export audit

Date: 2026-11-25

Audited revision: `87c361bc2ec191a15549a2301e6e5f9c97777827` (`main`).

Scope: animation discovery, track selection, transform conversion, interpolation, node targeting, clip timing, and deformation support in [gltf_exporter.cpp](../tools/gltf_converter/gltf_exporter.cpp). No production code was modified for this audit.

## Verdict

**The exporter does export animations, but the implementation is incomplete and can silently lose essential motion.** `ExportMotions` is called after node export and before serialization. It creates named glTF clips, samplers, channels, time accessors, and output accessors.

The decisive limitation is that it exports **at most one track per `SceneAnim::node_anims` entry**: the first quaternion track, or otherwise the first Vec3 track. A node animation containing rotation, position, and scale therefore loses two of its three transform tracks.

This is observable in the previously generated `car_engine` exports: `Take 001` contains 29 rotation channels over ten seconds, while all 29 position and 29 scale tracks are omitted. Six of those position tracks actually vary. The export consequently cannot reproduce the complete native animation, even if a viewer accepts the file and plays the rotations.

The quaternion data is present; an apparently static viewer is not by itself proof that no animation was exported. Viewer playback configuration and tolerance of invalid accessor bounds are separate questions and were not tested here.

## Evidence from car_engine

Source: `tutorials/resources/car_engine/engine.scn`.

Examined outputs: `build/gltf-vertex-audit/after/engine.gltf` and `engine.glb`, relative to the workspace root. These are the real exports produced during [vertex-sharing validation](GLTF_EXPORTER_VERTEX_SHARING_VALIDATION.md), not a simulation of animation serialization.

| Property | Native scene | glTF and GLB |
| --- | ---: | ---: |
| Named clips | 1, `Take 001` | 1, `Take 001` |
| Clip time interval | 0–10 seconds | Key timestamps span 0–10 seconds |
| Node animation bindings | 29 | 29 channels |
| Quaternion rotation tracks | 29 | 29 rotation channels |
| Position tracks | 29 | 0 translation channels |
| Scale tracks | 29 | 0 scale channels |
| Quaternion keys per track | 81 | 81 |
| Varying position tracks | 6 | 0 |
| Varying scale tracks | 0 | 0 |

There are **87 source transform tracks and only 29 output channels**. Of the 58 omitted tracks, 52 are constant and six vary. Constant tracks are not automatically redundant: they can override a node's base transform or establish a clip-specific pose. A future exporter may remove them only after proving that omission preserves the intended pose.

The six missing varying position tracks target:

| Node | Local Y range (max minus min) | Local Z range (max minus min) |
| --- | ---: | ---: |
| `Dummy004` | 5.055296 | 5.086757 |
| `Dummy002` | 5.091329 | 5.055168 |
| `Dummy006` | 5.049382 | 5.101239 |
| `Dummy008` | 5.055535 | 5.085111 |
| `Dummy010` | 5.055300 | 5.086761 |
| `Dummy012` | 5.091333 | 5.055196 |

These are native local-coordinate spans, not world-space displacement or a claim about physical units. Each of these position tracks has 81 keys; the other 23 position tracks have one key. Every scale track has one key.

Accessor decoding also confirmed:

- All 29 exported channels target names matching their native node bindings.
- All quaternion input times match native nanoseconds converted to float seconds.
- All 29 output quaternion streams vary.
- The largest observed deviation of squared quaternion length from one is approximately `2.08e-7`.
- Comparing normalized exported keys with the reflected source orientation `(-x, -y, z, w)`, allowing quaternion sign equivalence, gives a maximum angular discrepancy of approximately `6.82e-5` radians. This checks stored key orientations, not motion between keys.

No fresh scene export or visual playback test was performed during this audit. Findings about scale-only tracks, Euler-only tracks, filtering, and malformed input below come from source inspection, not executed synthetic scenes.

## Current export path

Line references below refer to the audited revision.

1. `ExportGltfScene` loads the scene and exports nodes, recording native-node-to-glTF-node indices (`gltf_exporter.cpp`, lines 1080–1082).
2. It calls `ExportMotions(model, scene)` at line 1141.
3. `ExportMotions` iterates `scene.GetSceneAnims()` and each clip's `node_anims` (196–205).
4. Entries without Vec3 or quaternion tracks are skipped (208–209).
5. If quaternion tracks exist, it exports `quat_tracks[0]` (221–299). Otherwise, it exports `vec3_tracks[0]` (300–371).
6. It associates that single sampler with the node, names the clip, and appends clips that have channels (374–383).

Names and multiple top-level clips are therefore supported. There is no animation-export command-line switch to enable: the export path calls this code unconditionally.

## Findings

### A1 — High: transform tracks are silently dropped

The quaternion branch and Vec3 branch are mutually exclusive, and both hardcode index zero. The implementation does not iterate tracks by semantic target.

Examples:

- Quaternion rotation plus position and scale: only quaternion rotation survives.
- Position and scale without quaternion rotation: only the first Vec3 track survives.
- An unsupported first Vec3 target can prevent a later valid position track from being considered.

**Observed on car_engine:** all 58 Vec3 tracks are lost, including six moving translations. Fixing only accessor metadata will not restore this motion.

Recommendation: emit one channel per supported `(node, target)` pair. Resolve duplicate or conflicting tracks explicitly, and honor the native `AF_UseQuaternionForRotation` flag rather than treating any quaternion track as authoritative. The native evaluator uses that flag to choose between Euler and quaternion rotation ([scene.cpp](../harfang/engine/scene.cpp), 2195–2203); the exporter ignores it.

### A2 — High: animated scale is reflected incorrectly

All exported Vec3 values undergo `vec.z = -vec.z` (319–323), regardless of the target. That conversion is suitable for translation under the exporter's Z-axis reflection, but scale should retain its components.

A selected scale key `(1, 1, 1)` becomes `(1, 1, -1)`. This introduces a reflection that is absent in the source and inconsistent with static scale export, which copies all three components unchanged (1048–1051).

This defect is currently masked in `car_engine` by A1: its scale tracks are never selected. Track enumeration must be fixed together with target-specific conversion.

### A3 — High: Euler rotation produces an invalid rotation channel

Native Vec3 tracks can target `Rotation`; the native evaluator treats them as Euler angles. The exporter maps that target to `rotation` but writes a `VEC3` accessor and merely negates Z (300–371).

A glTF rotation channel requires quaternion values. Selected Euler rotation tracks must be evaluated/converted to quaternions; simply changing the accessor type would not fix the data.

### A4 — High: quaternion maximum bounds are wrong

At line 256:

```cpp
output_max.w = quat.w > output_max.w ? quat.z : output_max.w;
```

The update compares W but stores Z. This does not directly change the serialized quaternion samples; it writes incorrect accessor metadata.

The existing Khronos validator reports for both `car_engine` formats contain **29 `ACCESSOR_MAX_MISMATCH` and 29 `ACCESSOR_ELEMENT_OUT_OF_MAX_BOUND` errors**, one pair per rotation output accessor. These account for 58 of the 61 errors previously reported; the other three concern light color values and are unrelated to animation.

Recommendation: correct the assignment and derive bounds from the final serialized data, or omit unnecessary output bounds. Continue supplying correct time bounds. File acceptance by a particular viewer must not be confused with specification conformance.

### A5 — High: interpolation does not preserve native curves

`AnimationSampler` defaults to `LINEAR` ([tiny_gltf.h](../tools/gltf_converter/tiny_gltf.h), 546–558); the exporter never assigns another interpolation mode or bakes the native evaluator.

Native Vec3 tracks use `AnimTrackHermiteT<Vec3>`, including tension and bias, and evaluate with Hermite interpolation ([animation.h](../harfang/engine/animation.h), 155–168, 184–185, 205–213). Copying only the stored keys into a linear glTF sampler changes intermediate positions and scales, even when tension and bias are zero.

Native quaternion evaluation uses component-wise linear interpolation followed by normalization in the node evaluator (`scene.cpp`, 2197–2199). That is not generally identical to spherical interpolation between glTF rotation keys. Matching key orientations therefore does not prove matching trajectories or angular speed.

Recommendation: establish an explicit fidelity policy. Evaluate native tracks and bake to glTF-compatible samples with an error tolerance, or implement a rigorously validated analytic conversion where possible. Test between original key times, not only at the keys. `frame_duration` is a useful initial sampling interval, but it is not by itself an error guarantee.

### A6 — High: a missing exported node can silently target node zero

Node association uses:

```cpp
channel.target_node = NodeRef_to_IdNode[node_anim.node];
```

`std::map::operator[]` inserts an absent key with integer value zero. A filtered or otherwise unexported animated node can consequently animate glTF node zero, or reference an invalid node if none exist.

Disabled-node filtering returns before registering the node or traversing its children (1023–1026), while animation export independently traverses all clip bindings. This makes the missing-map-entry path reachable without corrupting the source scene.

Recommendation: use `find`, omit or explicitly reject channels for nonexported nodes, and report why they were skipped. Rebuild or clear mappings per export if the tool later supports multiple exports in one process.

### A7 — Medium: clip timing and nested instance animation are not preserved

`SceneAnim` contains `t_start`, `t_end`, `frame_duration`, and a scene-level animation reference ([scene.h](../harfang/engine/scene.h), 109–118). `ExportMotions` reads the name and node bindings, but does not use those timing fields or the scene-level reference.

It serializes raw key timestamps without trimming to the clip range, shifting the clip origin, or creating boundary samples. Native playback uses the clip's start/end interval (`scene.cpp`, 2403–2405). Clips with nonzero starts, keys outside their range, or a deliberate hold after the last key can therefore have different effective timing in consumers. The 0–10 second `car_engine` clip does not expose these cases.

`Scene::GetSceneAnims()` excludes clips flagged `SAF_Instantiated` (`scene.cpp`, 2331–2337). Native animation also supports `instance_anim_track` with clip names, loop modes, and time scaling (`animation.h`, 190–197, 215), but the exporter does not evaluate or flatten that track. A top-level clip driving instance playback is not equivalent to an exported list of raw node tracks.

Recommendation: define clip-relative timing, include evaluated boundaries, and either bake instance playback or report it as unsupported. Runtime looping and automatic playback require a documented consumer policy; they should not be assumed to follow from exporting a clip.

### A8 — Medium: incomplete validation can create invalid output or unsafe access

The code dereferences `scene.GetAnim(...)` without checking for a null result. It tests track-vector length, not whether the selected track contains keys. Empty tracks can reach `serialize`, which uses `&from[0]` on an empty vector (173–177).

Other unchecked cases include unknown target names (mapped to an empty string), target/value-type mismatches, non-finite samples, non-increasing timestamps, and distinct nanosecond timestamps collapsing to the same float time. The exporter also does not check duplicate channels targeting the same node/property.

Recommendation: validate the complete channel before allocating output data; fail or warn explicitly, without silently substituting a target. Check float timestamps after conversion, because native-time ordering alone is insufficient.

## Support boundaries

| Feature | Current behavior |
| --- | --- |
| Named top-level clips | Exported if at least one eligible channel exists. |
| Node quaternion rotation | First quaternion track only; bounds bug and interpolation fidelity limitations. |
| Node translation | First Vec3 track only, and only when no quaternion track exists; curves become linear. |
| Node scale | Same selection restriction; Z component incorrectly negated. |
| Node Euler rotation | May be selected, but produces invalid VEC3 rotation output. |
| Several transform properties on one node | Not preserved by normal multi-track `Anim` entries. |
| Skin deformation | Not implemented: no exported skins, inverse bind matrices, joint indices, or weights. Animating ordinary/bone nodes alone does not deform a mesh. |
| Morph animation | Not implemented. The `Weights` target string mapping is not evidence of support: no morph target export or correct weight-data path exists. |
| Material values, camera FOV, light properties, visibility | Native tracks exist, but this exporter ignores their track types. Static material/light/camera export does not imply animation support. |
| Scene environment animation | Scene-level animation reference is ignored. |
| Instance clip playback / time scaling | Not evaluated or baked. |

Core glTF animation channels target translation, quaternion rotation, scale, or morph weights. Rotation uses VEC4 values; translation and scale use VEC3. Input time samples must be ordered, and interpolation semantics matter. Properties such as arbitrary material values require an explicit extension strategy or a documented unsupported-feature policy. These format constraints are described in the [Khronos glTF animation specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#animations).

## Recommended repair sequence

1. Correct quaternion output bounds and validate node mappings, targets, keys, and times. These changes make failures diagnosable and remove known invalid metadata.
2. Enumerate all supported transform targets; implement separate translation, scale, Euler rotation, and quaternion rotation handling. Respect the native rotation-mode flag and prevent duplicate channels.
3. Preserve motion through native-evaluator sampling with a documented tolerance, clip-relative time, and boundary samples. Keep quaternion normalization and sign continuity explicit; avoid unnecessary quaternion-to-Euler round trips when a tested basis conversion suffices.
4. Validate `car_engine` again: a lossless unpruned export has 29 translation, 29 rotation, and 29 scale channels. If constants are optimized away, prove pose equivalence and retain all six moving translations.
5. Treat skinning, morph targets, instance playback, and non-transform property animation as separate features with explicit support declarations. They should not be implied by the presence of `ExportMotions`.

## Acceptance tests for a future fix

- One node with simultaneous translation, rotation, and scale: all three motions survive.
- Positive and negative animated scales: no spurious Z reflection.
- Euler-only rotation and conflicting Euler/quaternion tracks: valid quaternion output matching the native rotation mode.
- Hermite translation with nonzero tension/bias: compare evaluated positions at intermediate times against the native evaluator.
- Sparse quaternion keys and rotations crossing Euler branch cuts: compare orientation and angular progression, including intermediate times.
- Nonzero clip starts, trimmed ranges, one-key tracks, and end holds: preserve the agreed playback interval.
- Disabled/filtered targets, invalid references, unknown targets, empty tracks, non-finite values, and colliding float timestamps: defined diagnostics without node-zero fallback or unsafe serialization.
- Multiple clips and animated instances: preserve clip separation and either correctly bake nested playback or explicitly report unsupported behavior.
- Both `.gltf` and `.glb`: no animation-related validator errors; do not hide remaining unrelated scene errors.
- `car_engine`: verify the six dummy-node translations, all rotation targets, and resulting local/world transforms against native evaluation. A viewer playback test should supplement numerical comparison.

## Reproducing the principal measurements

Run from the workspace root. This uses the existing accessor decoder from the geometry validation tooling and requires the previously generated exports.

```python
import collections
import json
from pathlib import Path
import sys

sys.path.insert(0, 'harfang3d/tools/gltf_converter/tests')
from compare_exports import load, accessor

source = json.loads(Path(
    'harfang3d/tutorials/resources/car_engine/engine.scn').read_text())
for target in ('Position', 'Scale'):
    tracks = [t for a in source['anims'] for t in a['anim']['vec3']
              if t['target'] == target]
    varying = sum(len({tuple(k['v']) for k in t['keys']}) > 1 for t in tracks)
    print(target, 'tracks:', len(tracks), 'varying:', varying)

for extension in ('gltf', 'glb'):
    model, buffers = load(Path(
        'build/gltf-vertex-audit/after/engine.' + extension))
    for clip in model.get('animations', []):
        paths = collections.Counter(c['target']['path'] for c in clip['channels'])
        times = [accessor(model, buffers, s['input']) for s in clip['samplers']]
        print(extension, clip.get('name'), paths,
              'keys:', collections.Counter(map(len, times)),
              'range:', min(t[0][0] for t in times), max(t[-1][0] for t in times))
```

Expected result: 29 position tracks with six varying, 29 constant scale tracks, and one clip per output format containing only 29 rotation channels, each with 81 timestamps spanning 0–10 seconds.
