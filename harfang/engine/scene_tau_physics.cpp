// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene_tau_physics.h"

#include "engine/assets_rw_interface.h"
#include "engine/scene.h"

#include "foundation/file_rw_interface.h"

namespace hg {

namespace {

bool IsTauPhase1RigidBodyType(RigidBodyType type) {
	return type == RBT_Dynamic || type == RBT_Static;
}

Collision FindTauPhase1Collision(const Node &node) {
	for (size_t i = 0; i < node.GetCollisionCount(); ++i) {
		const auto collision = node.GetCollision(i);
		if (collision.IsValid() && collision.GetType() == CT_Cube)
			return collision;
	}
	return {};
}

Mat4 GetNodeWorld(const Node &node) {
	if (node.HasTransform())
		return node.GetTransform().GetWorld();
	return node.GetWorld();
}

} // namespace

void SceneTauPhysics::SceneCreatePhysics(const Scene &scene, const Reader &ir, const ReadProvider &ip) {
	(void)ir;
	(void)ip;

	ClearNodes();

	for (const auto &node : scene.GetNodes())
		NodeCreatePhysics(node, ir, ip);
}

void SceneTauPhysics::SceneCreatePhysicsFromFile(const Scene &scene) { SceneCreatePhysics(scene, g_file_reader, g_file_read_provider); }
void SceneTauPhysics::SceneCreatePhysicsFromAssets(const Scene &scene) { SceneCreatePhysics(scene, g_assets_reader, g_assets_read_provider); }

void SceneTauPhysics::NodeCreatePhysics(const Node &node, const Reader &ir, const ReadProvider &ip) {
	(void)ir;
	(void)ip;

	if (!node.IsValid() || !node.HasRigidBody())
		return;

	const auto rigid_body = node.GetRigidBody();
	const auto type = rigid_body.GetType();
	if (!IsTauPhase1RigidBodyType(type))
		return;

	const auto collision = FindTauPhase1Collision(node);
	if (!collision.IsValid())
		return;

	TauNode tau_node;
	tau_node.motion.Reset(node.GetWorld());
	tau_node.size = collision.GetSize();
	tau_node.body_type = type;

	nodes[node.ref] = tau_node;
}

void SceneTauPhysics::NodeCreatePhysicsFromFile(const Node &node) { NodeCreatePhysics(node, g_file_reader, g_file_read_provider); }
void SceneTauPhysics::NodeCreatePhysicsFromAssets(const Node &node) { NodeCreatePhysics(node, g_assets_reader, g_assets_read_provider); }

void SceneTauPhysics::NodeStartTrackingCollisionEvents(NodeRef ref, CollisionEventTrackingMode mode) { node_collision_event_tracking_modes[ref] = mode; }
void SceneTauPhysics::NodeStopTrackingCollisionEvents(NodeRef ref) { node_collision_event_tracking_modes.erase(ref); }

void SceneTauPhysics::NodeDestroyPhysics(const Node &node) {
	nodes.erase(node.ref);
	node_collision_event_tracking_modes.erase(node.ref);
}

void SceneTauPhysics::StepSimulation(time_ns dt, time_ns step, int max_step) {
	(void)dt;
	(void)step;
	(void)max_step;
}

void SceneTauPhysics::CollectCollisionEvents(const Scene &scene, NodePairContacts &contacts) {
	(void)scene;
	contacts.clear();
}

void SceneTauPhysics::SyncTransformsFromScene(const Scene &scene) {
	for (auto &it : nodes) {
		if (!scene.IsValidNodeRef(it.first))
			continue;
		it.second.motion.CaptureSourceWorld(GetNodeWorld(scene.GetNode(it.first)));
	}
}

void SceneTauPhysics::SyncTransformsToScene(Scene &scene) {
	for (auto &it : nodes) {
		if (!scene.IsValidNodeRef(it.first))
			continue;
		scene.SetNodeWorldMatrix(it.first, it.second.motion.GetWorld());
	}
}

size_t SceneTauPhysics::GarbageCollect(const Scene &scene) {
	size_t removed = 0;

	for (auto it = nodes.begin(); it != nodes.end();) {
		if (!scene.IsValidNodeRef(it->first)) {
			node_collision_event_tracking_modes.erase(it->first);
			it = nodes.erase(it);
			++removed;
		} else {
			++it;
		}
	}

	for (auto it = node_collision_event_tracking_modes.begin(); it != node_collision_event_tracking_modes.end();) {
		if (!scene.IsValidNodeRef(it->first)) {
			it = node_collision_event_tracking_modes.erase(it);
			++removed;
		} else {
			++it;
		}
	}

	return removed;
}

size_t SceneTauPhysics::GarbageCollectResources() { return 0; }

void SceneTauPhysics::ClearNodes() {
	nodes.clear();
	node_collision_event_tracking_modes.clear();
}

void SceneTauPhysics::Clear() { ClearNodes(); }

void SceneTauPhysics::NodeWake(NodeRef ref) const { (void)ref; }

} // namespace hg
