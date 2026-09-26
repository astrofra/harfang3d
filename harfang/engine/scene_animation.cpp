// HARFANG(R). Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene.h"
#include "foundation/log.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace hg {

enum TransformAnimChannel : uint8_t { TAC_Position = 1, TAC_Rotation = 2, TAC_Scale = 4 };

struct AnimPose {
	Vec3 pos, scale;
	Quaternion rot;
};

struct AnimPlayerNode {
	NodeRef node;
	ComponentRef transform;
	uint8_t channels{};
};

struct AnimPlayerClip {
	SceneAnimRef ref;
	std::string name;
	time_ns start{}, end{};
	std::vector<BoundToNodeAnim> bindings; // Dense, in player node order; invalid AnimRef means reference pose.
};

struct SceneAnimPlayerData {
	NodeRef instance;
	std::vector<AnimPlayerNode> nodes;
	std::vector<AnimPlayerClip> clips;
	std::vector<AnimPose> reference, output, from, to;
	bool owns_channels{true}, active{false}, has_output{false}, pending{false};
	int target{-1}, source{-1};
	ScenePlayAnimRef target_ref, source_ref;
	time_ns target_time{}, source_time{}, elapsed{}, duration{}, fade_duration{};
	AnimLoopMode target_mode{ALM_Once}, source_mode{ALM_Once};
	Easing fade_easing{E_Linear}, transition_easing{E_Linear};
};

static bool IsTransformAnim(const Anim &anim) {
	if (!anim.bool_tracks.empty() || !anim.int_tracks.empty() || !anim.float_tracks.empty() || !anim.vec2_tracks.empty() || !anim.vec4_tracks.empty() ||
		!anim.color_tracks.empty() || !anim.string_tracks.empty() || !anim.instance_anim_track.keys.empty() || anim.vec3_tracks.size() >= 128 ||
		anim.quat_tracks.size() >= 128)
		return false;
	std::set<std::string> targets;
	for (const auto &track : anim.vec3_tracks)
		if ((track.target != "Position" && track.target != "Rotation" && track.target != "Scale") || !targets.insert(track.target).second)
			return false;
	targets.clear();
	for (const auto &track : anim.quat_tracks)
		if (track.target != "Rotation" || !targets.insert(track.target).second)
			return false;
	return true;
}

static uint8_t GetTransformChannels(const Anim &anim) {
	uint8_t channels = 0;
	for (const auto &track : anim.vec3_tracks)
		if (!track.keys.empty()) {
			if (track.target == "Position")
				channels |= TAC_Position;
			if (track.target == "Scale")
				channels |= TAC_Scale;
			if (track.target == "Rotation" && !(anim.flags & AF_UseQuaternionForRotation))
				channels |= TAC_Rotation;
		}
	if (anim.flags & AF_UseQuaternionForRotation)
		for (const auto &track : anim.quat_tracks)
			if (track.target == "Rotation" && !track.keys.empty())
				channels |= TAC_Rotation;
	return channels;
}

static bool ValidAnimRange(time_ns start, time_ns end) {
	return end >= start && uint64_t(end) - uint64_t(start) <= uint64_t(std::numeric_limits<time_ns>::max());
}

static bool Overlaps(const SceneAnimPlayerData &a, const SceneAnimPlayerData &b) {
	for (const auto &na : a.nodes)
		for (const auto &nb : b.nodes)
			if (na.transform == nb.transform && (na.channels & nb.channels))
				return true;
	return false;
}

static bool WritesPlayer(const Scene &scene, const SceneBoundAnim &bound, const SceneAnimPlayerData &player) {
	for (const auto &binding : bound.bound_node_anims) {
		const auto *anim = scene.GetAnim(binding.anim);
		if (!anim)
			continue;
		const auto channels = GetTransformChannels(*anim);
		for (const auto &node : player.nodes) {
			if (scene.GetNodeTransformRef(binding.node) == node.transform && (channels & node.channels))
				return true;
			// Discrete/nested timelines may operate on an ancestor of the owned rig.
			if (!anim->instance_anim_track.keys.empty() || !anim->bool_tracks.empty()) {
				std::set<NodeRef> visited;
				for (auto ref = node.node; scene.IsValidNodeRef(ref) && visited.insert(ref).second; ref = scene.GetNodeTransform(ref).GetParent())
					if (ref == binding.node)
						return true;
			}
		}
	}
	return false;
}

bool Scene::HasAnimPlayerConflict(const SceneBoundAnim &anim, const SceneAnimPlayerData *except) const {
	for (const auto &player : anim_players)
		if (player.get() != except && player->owns_channels && WritesPlayer(*this, anim, *player))
			return true;
	return false;
}

SceneAnimPlayer Scene::CreateAnimPlayer(const std::vector<SceneAnimRef> &clips) {
	auto data = std::make_shared<SceneAnimPlayerData>();
	std::map<NodeRef, uint8_t> channels;
	std::set<std::string> names;
	for (auto ref : clips) {
		const auto *clip = GetSceneAnim(ref);
		if (!clip || clip->scene_anim != InvalidAnimRef || !ValidAnimRange(clip->t_start, clip->t_end) || !names.insert(clip->name).second) {
			warn("Animation player requires valid, uniquely named transform clips");
			return {};
		}
		std::set<NodeRef> targets;
		for (const auto &binding : clip->node_anims) {
			const auto *anim = GetAnim(binding.anim);
			if (!anim || !IsTransformAnim(*anim) || !GetNodeTransform(binding.node).IsValid() || !targets.insert(binding.node).second) {
				warn("Animation player does not support invalid, duplicate, discrete or nested animation tracks");
				return {};
			}
			channels[binding.node] |= GetTransformChannels(*anim);
		}
	}
	std::set<ComponentRef> transforms;
	for (const auto &entry : channels) {
		if (!entry.second)
			continue;
		const auto transform = GetNodeTransform(entry.first);
		if (!transforms.insert(transform.ref).second) {
			warn("Animation player nodes share a transform component");
			return {};
		}
		const auto trs = transform.GetTRS();
		data->nodes.push_back({entry.first, transform.ref, entry.second});
		data->reference.push_back({trs.pos, trs.scl, QuaternionFromEuler(trs.rot)});
	}
	if (data->nodes.empty()) {
		warn("Animation player has no transform channels");
		return {};
	}
	for (auto ref : clips) {
		const auto &clip = *GetSceneAnim(ref);
		AnimPlayerClip out;
		out.ref = ref;
		out.name = clip.name;
		out.start = clip.t_start;
		out.end = clip.t_end;
		for (const auto &node : data->nodes) {
			BoundToNodeAnim bound{};
			for (const auto &binding : clip.node_anims)
				if (binding.node == node.node)
					bound = BindNodeAnim(binding.node, binding.anim);
			out.bindings.push_back(std::move(bound));
		}
		data->clips.push_back(std::move(out));
	}
	for (const auto &player : anim_players)
		if (player->owns_channels && Overlaps(*data, *player)) {
			warn("Animation player channels already owned");
			return {};
		}
	for (const auto &play : play_anims)
		if (!(play.flags & SPAF_Managed) && WritesPlayer(*this, play.bound_anim, *data)) {
			warn("Stop existing playback before creating this animation player");
			return {};
		}
	data->output = data->from = data->to = data->reference;
	return {scene_ref, anim_players.add_ref(std::move(data))};
}

SceneAnimPlayer Scene::CreateInstanceAnimPlayer(const Node &node) {
	if (node.scene_ref != scene_ref || !node.IsValid() || node_instance_view.find(node.ref) == node_instance_view.end()) {
		warn("Invalid animation player instance");
		return {};
	}
	const auto &view = GetNodeInstanceSceneView(node.ref);
	for (auto ref : view.scene_anims)
		if (const auto *clip = GetSceneAnim(ref))
			for (const auto &binding : clip->node_anims)
				if (std::find(view.nodes.begin(), view.nodes.end(), binding.node) == view.nodes.end()) {
					warn("Instance animation targets a node outside its view");
					return {};
				}
	auto player = CreateAnimPlayer(view.scene_anims);
	if (player.IsValid())
		anim_players[player.ref.idx]->instance = node.ref;
	return player;
}

SceneAnimPlayerData *SceneAnimPlayer::GetData() const {
	if (!scene_ref || !scene_ref->scene || !scene_ref->scene->anim_players.is_valid(ref))
		return nullptr;
	return scene_ref->scene->anim_players[ref.idx].get();
}

bool SceneAnimPlayer::IsValid() const { return GetData() != nullptr; }
bool SceneAnimPlayer::SetCrossFadeDuration(time_ns duration) {
	if (auto *data = GetData()) {
		if (duration < 0) {
			warn("Negative crossfade duration");
			return false;
		}
		data->fade_duration = duration;
		return true;
	}
	return false;
}
time_ns SceneAnimPlayer::GetCrossFadeDuration() const {
	const auto *data = GetData();
	return data ? data->fade_duration : 0;
}
bool SceneAnimPlayer::SetCrossFadeEasing(Easing easing) {
	if (auto *data = GetData()) {
		if (easing != E_Linear && easing != E_SmoothStep) {
			warn("Unsupported crossfade easing");
			return false;
		}
		data->fade_easing = easing;
		return true;
	}
	return false;
}
Easing SceneAnimPlayer::GetCrossFadeEasing() const {
	const auto *data = GetData();
	return data ? data->fade_easing : E_Linear;
}
bool SceneAnimPlayer::IsTransitioning() const {
	const auto *data = GetData();
	return data && data->active && data->elapsed < data->duration;
}

void SceneAnimPlayer::Stop() {
	if (auto *data = GetData()) {
		auto &plays = scene_ref->scene->play_anims;
		plays.remove_ref(data->target_ref);
		plays.remove_ref(data->source_ref);
		data->target_ref = data->source_ref = {};
		data->active = data->owns_channels = data->has_output = data->pending = false;
		data->target = data->source = -1;
		data->elapsed = data->duration = 0;
	}
}

static bool SamplePlayerClip(const Scene &scene, const SceneAnimPlayerData &data, int clip_index, time_ns t, std::vector<AnimPose> &pose) {
	pose = data.reference;
	const auto &clip = data.clips[clip_index];
	if (!scene.IsValidSceneAnim(clip.ref))
		return false;
	for (size_t i = 0; i < clip.bindings.size(); ++i) {
		const auto &bound = clip.bindings[i];
		if (bound.anim == InvalidAnimRef)
			continue;
		const auto *anim = scene.GetAnim(bound.anim);
		if (!anim)
			return false;
		auto &p = pose[i];
		for (int channel = 0; channel < NV3AT_Count; ++channel) {
			const int track = bound.vec3_track[channel];
			if (track < 0)
				continue;
			if (size_t(track) >= anim->vec3_tracks.size())
				return false;
			if (channel == NV3AT_TransformPosition)
				Evaluate(anim->vec3_tracks[track], t, p.pos);
			if (channel == NV3AT_TransformScale)
				Evaluate(anim->vec3_tracks[track], t, p.scale);
			if (channel == NV3AT_TransformRotation && !(anim->flags & AF_UseQuaternionForRotation)) {
				Vec3 euler;
				if (Evaluate(anim->vec3_tracks[track], t, euler))
					p.rot = QuaternionFromEuler(euler);
			}
		}
		const int track = bound.quat_track[NQAT_TransformRotation];
		if ((anim->flags & AF_UseQuaternionForRotation) && track >= 0) {
			if (size_t(track) >= anim->quat_tracks.size())
				return false;
			Quaternion q;
			if (Evaluate(anim->quat_tracks[track], t, q) && Len2(q) > 1e-12f)
				p.rot = Normalize(q);
		}
	}
	return true;
}

ScenePlayAnimRef SceneAnimPlayer::Play(const std::string &name, AnimLoopMode mode, bool restart) {
	auto *data = GetData();
	if (!data)
		return {};
	auto &scene = *scene_ref->scene;
	int target = -1;
	for (size_t i = 0; i < data->clips.size(); ++i)
		if (data->clips[i].name == name)
			target = int(i);
	if (target < 0 || (mode != ALM_Once && mode != ALM_Loop && mode != ALM_Infinite) ||
		(mode == ALM_Loop && data->clips[target].start == data->clips[target].end)) {
		warn("Invalid animation player request");
		return {};
	}
	if (!restart && data->active && target == data->target && mode == data->target_mode && scene.IsPlaying(data->target_ref))
		return data->target_ref;
	for (const auto &player : scene.anim_players)
		if (player.get() != data && player->owns_channels && Overlaps(*data, *player)) {
			warn("Animation player channels already owned");
			return {};
		}
	for (const auto &play : scene.play_anims)
		if (!(play.flags & Scene::SPAF_Managed) && WritesPlayer(scene, play.bound_anim, *data)) {
			warn("Animation player conflicts with existing playback");
			return {};
		}
	// Sample the incoming entry before retiring any accepted request.
	if (!SamplePlayerClip(scene, *data, target, data->clips[target].start, data->to)) {
		warn("Animation player clip was invalidated");
		return {};
	}
	const bool fade = data->has_output && data->fade_duration > 0;
	if (fade && !data->pending && data->elapsed >= data->duration) {
		data->source = data->target;
		data->source_time = data->target_time;
		data->source_mode = data->target_mode;
		data->source_ref = data->target_ref;
		data->from = data->output;
	} else {
		scene.play_anims.remove_ref(data->source_ref);
		scene.play_anims.remove_ref(data->target_ref);
		data->source_ref = {};
		data->source = -1;
		data->from = data->output;
	}
	data->target = target;
	data->target_time = data->clips[target].start;
	data->target_mode = mode;
	data->duration = fade ? data->fade_duration : 0;
	data->elapsed = 0;
	data->transition_easing = data->fade_easing;
	data->active = data->owns_channels = true;
	data->pending = true;
	Scene::ScenePlayAnim play{};
	play.name = name;
	play.flags = Scene::SPAF_Managed;
	data->target_ref = scene.play_anims.add_ref(std::move(play));
	return data->target_ref;
}

// Returns whether a once-only request has reached its endpoint. Avoid overflow and repeated wrapping.
static bool AdvancePlayerTime(const AnimPlayerClip &clip, AnimLoopMode mode, time_ns dt, time_ns &t) {
	if (mode == ALM_Loop) {
		const auto length = clip.end - clip.start;
		const auto step = dt % length;
		const auto offset = t - clip.start;
		t = clip.start + (offset >= length - step ? offset - (length - step) : offset + step);
		return false;
	}
	if (mode == ALM_Once) {
		t = dt >= clip.end - t ? clip.end : t + dt;
		return t == clip.end;
	}
	t = t > std::numeric_limits<time_ns>::max() - dt ? std::numeric_limits<time_ns>::max() : t + dt;
	return false;
}

void Scene::UpdateAnimPlayers(time_ns dt) {
	dt = std::max<time_ns>(dt, 0);
	for (auto ref = anim_players.first_ref(); ref != invalid_gen_ref;) {
		const auto next = anim_players.next_ref(ref);
		auto &data = *anim_players[ref.idx];
		bool valid = true;
		for (const auto &node : data.nodes)
			if (!IsValidNodeRef(node.node) || GetNodeTransformRef(node.node) != node.transform || !GetNodeTransform(node.node).IsValid())
				valid = false;
		for (const auto &clip : data.clips)
			if (!IsValidSceneAnim(clip.ref))
				valid = false;
		if (!valid) {
			DestroyAnimPlayer({scene_ref, ref});
			ref = next;
			continue;
		}
		if (data.active) {
			bool target_end = false, source_end = false;
			if (IsPlaying(data.target_ref))
				target_end = AdvancePlayerTime(data.clips[data.target], data.target_mode, dt, data.target_time);
			if (data.source >= 0 && IsPlaying(data.source_ref))
				source_end = AdvancePlayerTime(data.clips[data.source], data.source_mode, dt, data.source_time);
			valid = SamplePlayerClip(*this, data, data.target, data.target_time, data.to);
			if (data.source >= 0)
				valid = SamplePlayerClip(*this, data, data.source, data.source_time, data.from) && valid;
			if (!valid) {
				DestroyAnimPlayer({scene_ref, ref});
				ref = next;
				continue;
			}
			data.elapsed += std::min(dt, data.duration - data.elapsed);
			float weight = data.duration ? float(double(data.elapsed) / double(data.duration)) : 1.f;
			if (data.transition_easing == E_SmoothStep)
				weight = weight * weight * (3.f - 2.f * weight);
			for (size_t i = 0; i < data.nodes.size(); ++i) {
				auto &out = data.output[i];
				out.pos = Lerp(data.from[i].pos, data.to[i].pos, weight);
				out.scale = Lerp(data.from[i].scale, data.to[i].scale, weight);
				out.rot = Normalize(Slerp(data.from[i].rot, data.to[i].rot, weight));
				auto transform = GetNodeTransform(data.nodes[i].node);
				const auto channels = data.nodes[i].channels;
				if (channels & TAC_Position)
					transform.SetPos(out.pos);
				if (channels & TAC_Rotation)
					transform.SetRot(ToEuler(out.rot));
				if (channels & TAC_Scale)
					transform.SetScale(out.scale);
			}
			if (target_end)
				play_anims.remove_ref(data.target_ref);
			data.has_output = true;
			data.pending = false;
			if (source_end)
				play_anims.remove_ref(data.source_ref);
			if (data.elapsed == data.duration) {
				play_anims.remove_ref(data.source_ref);
				data.source_ref = {};
				data.source = -1;
			}
		}
		ref = next;
	}
}

bool Scene::StopManagedAnim(ScenePlayAnimRef ref) {
	for (auto i = anim_players.first_ref(); i != invalid_gen_ref; i = anim_players.next_ref(i)) {
		auto &data = *anim_players[i.idx];
		if (data.target_ref == ref) {
			SceneAnimPlayer{scene_ref, i}.Stop();
			return true;
		}
		if (data.source_ref == ref) {
			play_anims.remove_ref(ref);
			data.source_ref = {};
			data.source = -1; // Keep the last sampled source pose frozen.
			return true;
		}
	}
	return false;
}

void Scene::DestroyAnimPlayer(const SceneAnimPlayer &player) {
	if (player.scene_ref != scene_ref || !anim_players.is_valid(player.ref))
		return;
	SceneAnimPlayer{scene_ref, player.ref}.Stop();
	anim_players.remove_ref(player.ref);
}
void Scene::StopAllAnimPlayers() {
	for (auto ref = anim_players.first_ref(); ref != invalid_gen_ref; ref = anim_players.next_ref(ref))
		SceneAnimPlayer{scene_ref, ref}.Stop();
}
void Scene::ClearAnimPlayers() {
	while (anim_players.size())
		DestroyAnimPlayer({scene_ref, anim_players.first_ref()});
}
void Scene::InvalidateAnimPlayers(NodeRef node, AnimRef anim, SceneAnimRef clip) {
	for (auto ref = anim_players.first_ref(); ref != invalid_gen_ref;) {
		const auto next = anim_players.next_ref(ref);
		const auto &data = *anim_players[ref.idx];
		bool invalid = node != InvalidNodeRef && node == data.instance;
		for (const auto &n : data.nodes)
			if (node != InvalidNodeRef && n.node == node)
				invalid = true;
		for (const auto &c : data.clips) {
			if (clip != InvalidSceneAnimRef && clip == c.ref)
				invalid = true;
			for (const auto &b : c.bindings)
				if (anim != InvalidAnimRef && anim == b.anim)
					invalid = true;
		}
		if (invalid)
			DestroyAnimPlayer({scene_ref, ref});
		ref = next;
	}
}

SceneAnimInfo GetSceneAnimInfo(const Scene &scene, SceneAnimRef ref) {
	const auto *anim = scene.GetSceneAnim(ref);
	return anim ? SceneAnimInfo{true, anim->name, anim->t_start, anim->t_end, anim->frame_duration} : SceneAnimInfo{};
}

static bool NodePath(const Node &node, std::string &path) {
	std::set<NodeRef> visited;
	for (auto current = node; current.IsValid(); current = current.GetTransform().GetParentNode()) {
		if (!visited.insert(current.ref).second)
			return false;
		const auto name = current.GetName();
		// Length-delimited segments keep arbitrary node names unambiguous.
		path = std::to_string(name.size()) + ":" + name + "/" + path;
	}
	return true;
}

SceneAnimNodeMap BuildSceneAnimNodeMap(const Scene &source, const Scene &destination) {
	SceneAnimNodeMap result;
	std::map<std::string, Node> targets;
	for (const auto &node : destination.GetNodes()) {
		std::string path;
		if (!NodePath(node, path) || !targets.emplace(path, node).second) {
			result.message = "Ambiguous destination node paths";
			return result;
		}
	}
	std::set<std::string> paths;
	for (const auto &node : source.GetNodes()) {
		std::string path;
		if (!NodePath(node, path) || !paths.insert(path).second) {
			result.message = "Ambiguous source node paths";
			return result;
		}
		const auto target = targets.find(path);
		if (target == targets.end()) {
			result.message = "No matching destination node: " + node.GetName();
			return result;
		}
		result.source_nodes.push_back(node);
		result.destination_nodes.push_back(target->second);
	}
	result.success = !result.source_nodes.empty();
	if (!result.success)
		result.message = "Empty animation node mapping";
	return result;
}

SceneAnimImportResult ImportSceneAnim(
	const Scene &source, SceneAnimRef source_anim, Scene &destination, const SceneAnimNodeMap &node_map, const std::string &name, bool preserve_missing_trs) {
	SceneAnimImportResult result;
	const auto *clip = source.GetSceneAnim(source_anim);
	if (!clip || clip->scene_anim != InvalidAnimRef || !ValidAnimRange(clip->t_start, clip->t_end) || name.empty() || !node_map.success ||
		node_map.source_nodes.empty() || node_map.source_nodes.size() != node_map.destination_nodes.size()) {
		result.message = "Invalid transform clip or node mapping";
		return result;
	}
	if (destination.GetSceneAnim(name.c_str()) != InvalidSceneAnimRef) {
		result.message = "Destination animation name already exists: " + name;
		return result;
	}
	std::map<NodeRef, NodeRef> mapping;
	std::set<NodeRef> targets;
	for (size_t i = 0; i < node_map.source_nodes.size(); ++i) {
		const auto &a = node_map.source_nodes[i], &b = node_map.destination_nodes[i];
		if (!a.IsValid() || !b.IsValid() || a.scene_ref->scene != &source || b.scene_ref->scene != &destination || !a.GetTransform().IsValid() ||
			!b.GetTransform().IsValid() || !mapping.emplace(a.ref, b.ref).second || !targets.insert(b.ref).second) {
			result.message = "Invalid, duplicate or foreign node in animation mapping";
			return result;
		}
	}
	for (const auto &entry : mapping) {
		const auto parent = source.GetNodeTransform(entry.first).GetParent();
		const auto expected = mapping.find(parent);
		const auto actual = destination.GetNodeTransform(entry.second).GetParent();
		if ((parent != InvalidNodeRef && (expected == mapping.end() || expected->second != actual)) || (parent == InvalidNodeRef && actual != InvalidNodeRef)) {
			result.message = "Animation mapping does not preserve parent relationships";
			return result;
		}
	}
	std::map<NodeRef, Anim> prepared;
	for (const auto &binding : clip->node_anims) {
		const auto *anim = source.GetAnim(binding.anim);
		if (!anim || !IsTransformAnim(*anim) || mapping.find(binding.node) == mapping.end() || !prepared.emplace(binding.node, *anim).second) {
			result.message = "Unsupported tracks, unmapped or duplicate animation target";
			return result;
		}
		result.copied_channels += uint32_t(anim->vec3_tracks.size() + anim->quat_tracks.size());
	}
	if (preserve_missing_trs) {
		for (const auto &entry : mapping) {
			auto &anim = prepared[entry.first];
			const auto trs = source.GetNodeTransform(entry.first).GetTRS();
			for (const auto &channel : std::vector<std::pair<std::string, Vec3>>{{"Position", trs.pos}, {"Scale", trs.scl}, {"Rotation", trs.rot}}) {
				if (channel.first == "Rotation" && (anim.flags & AF_UseQuaternionForRotation)) {
					auto track =
						std::find_if(anim.quat_tracks.begin(), anim.quat_tracks.end(), [](const AnimTrackT<Quaternion> &t) { return t.target == "Rotation"; });
					if (track == anim.quat_tracks.end()) {
						anim.quat_tracks.push_back({"Rotation", {}});
						track = anim.quat_tracks.end() - 1;
					}
					if (track->keys.empty()) {
						SetKey(*track, clip->t_start, QuaternionFromEuler(trs.rot));
						++result.completed_channels;
					}
				} else {
					auto track = std::find_if(
						anim.vec3_tracks.begin(), anim.vec3_tracks.end(), [&](const AnimTrackHermiteT<Vec3> &t) { return t.target == channel.first; });
					if (track == anim.vec3_tracks.end()) {
						anim.vec3_tracks.push_back({channel.first, {}});
						track = anim.vec3_tracks.end() - 1;
					}
					if (track->keys.empty()) {
						SetKey(*track, clip->t_start, channel.second);
						++result.completed_channels;
					}
				}
			}
		}
	}
	// All validation and track copies complete before publishing destination data.
	SceneAnim out;
	out.name = name;
	out.t_start = clip->t_start;
	out.t_end = clip->t_end;
	out.frame_duration = clip->frame_duration;
	std::vector<AnimRef> inserted;
	try {
		inserted.reserve(prepared.size());
		out.node_anims.reserve(prepared.size());
		for (auto &entry : prepared) {
			entry.second.flags &= ~AF_Instantiated;
			entry.second.t_start = out.t_start;
			entry.second.t_end = out.t_end;
			const auto ref = destination.AddAnim(std::move(entry.second));
			inserted.push_back(ref);
			out.node_anims.push_back({mapping.at(entry.first), ref});
		}
		result.anim = destination.AddSceneAnim(std::move(out));
	} catch (...) {
		for (auto ref : inserted)
			destination.DestroyAnim(ref);
		throw;
	}
	result.success = true;
	return result;
}

} // namespace hg
