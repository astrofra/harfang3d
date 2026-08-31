// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/physics.h"

#include "foundation/rw_interface.h"
#include "foundation/time.h"

#include "tau_harfang_runtime.h"

#include <map>

namespace hg {

class Scene;

struct TauNode {
	tau_compat::NodeMotionAdapter motion{};
	Vec3 size{Vec3::One};
	RigidBodyType body_type{RBT_Static};
};

class SceneTauPhysics {
public:
	SceneTauPhysics() = default;
	~SceneTauPhysics() = default;

	void SceneCreatePhysics(const Scene &scene, const Reader &ir, const ReadProvider &ip);
	void SceneCreatePhysicsFromFile(const Scene &scene);
	void SceneCreatePhysicsFromAssets(const Scene &scene);

	void NodeCreatePhysics(const Node &node, const Reader &ir, const ReadProvider &ip);
	void NodeCreatePhysicsFromFile(const Node &node);
	void NodeCreatePhysicsFromAssets(const Node &node);

	void NodeStartTrackingCollisionEvents(NodeRef ref, CollisionEventTrackingMode mode = CETM_EventOnly);
	void NodeStopTrackingCollisionEvents(NodeRef ref);

	void NodeStartTrackingCollisionEvents(const Node &node, CollisionEventTrackingMode mode = CETM_EventOnly) {
		NodeStartTrackingCollisionEvents(node.ref, mode);
	}
	void NodeStopTrackingCollisionEvents(const Node &node) { NodeStopTrackingCollisionEvents(node.ref); }

	void NodeDestroyPhysics(const Node &node);

	bool NodeHasBody(NodeRef ref) const { return nodes.find(ref) != std::end(nodes); }
	bool NodeHasBody(const Node &node) const { return NodeHasBody(node.ref); }

	void StepSimulation(time_ns dt, time_ns step = time_from_ms(16), int max_step = 8);
	void CollectCollisionEvents(const Scene &scene, NodePairContacts &contacts);

	void SyncTransformsFromScene(const Scene &scene);
	void SyncTransformsToScene(Scene &scene);

	size_t GarbageCollect(const Scene &scene);
	size_t GarbageCollectResources();

	void ClearNodes();
	void Clear();

	void NodeWake(NodeRef ref) const;
	void NodeWake(const Node &node) const { NodeWake(node.ref); }

private:
	std::map<NodeRef, TauNode> nodes;
	std::map<NodeRef, CollisionEventTrackingMode> node_collision_event_tracking_modes;
};

} // namespace hg
