// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "engine/physics.h"

#include "foundation/quaternion.h"
#include "foundation/rw_interface.h"
#include "foundation/time.h"

#include "tau/compat/tau_harfang_runtime.h"

#include <map>
#include <vector>

namespace hg {

class Scene;

struct TauCollisionShape {
	CollisionType type{CT_Cube};
	Mat4 local_transform{Mat4::Identity};
	Vec3 local_position{Vec3::Zero};
	Mat3 local_rotation{Mat3::Identity};
	Vec3 size{Vec3::One};
	float radius{0.5f};
	float mass{0.f};
	float friction{0.f};
	float restitution{0.f};
};

struct TauNode {
	tau_compat::NodeMotionAdapter motion{};
	std::vector<TauCollisionShape> shapes;
	RigidBodyType body_type{RBT_Static};
	float total_mass{0.f};
	float inverse_mass{0.f};
	Mat3 inverse_inertia_body{Mat3::Zero};
	Vec3 position{Vec3::Zero};
	Vec3 previous_position{Vec3::Zero};
	Quaternion orientation{Quaternion::Identity};
	Quaternion previous_orientation{Quaternion::Identity};
	Vec3 scale{Vec3::One};
	float linear_damping{0.f};
	float angular_damping{0.f};
	float friction{0.f};
	float restitution{0.f};
	float rolling_friction{0.f};
	Vec3 linear_velocity{Vec3::Zero};
	Vec3 angular_velocity{Vec3::Zero};
	Vec3 linear_factor{Vec3::One};
	Vec3 angular_factor{Vec3::One};
	Vec3 accumulated_force{Vec3::Zero};
	Vec3 accumulated_torque{Vec3::Zero};
	bool deactivation_enabled{true};
};

class SceneTauPhysics {
public:
	SceneTauPhysics(int thread_count = 1) { (void)thread_count; }
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

	void NodeSetDeactivation(NodeRef ref, bool enable);
	void NodeSetDeactivation(const Node &node, bool enable) { NodeSetDeactivation(node.ref, enable); }
	bool NodeGetDeactivation(NodeRef ref) const;
	bool NodeGetDeactivation(const Node &node) const { return NodeGetDeactivation(node.ref); }

	void NodeResetWorld(NodeRef ref, const Mat4 &world);
	void NodeResetWorld(const Node &node, const Mat4 &world) { NodeResetWorld(node.ref, world); }
	void NodeTeleport(NodeRef ref, const Mat4 &world);
	void NodeTeleport(const Node &node, const Mat4 &world) { NodeTeleport(node.ref, world); }

	void NodeAddForce(NodeRef ref, const Vec3 &F);
	void NodeAddForce(NodeRef ref, const Vec3 &F, const Vec3 &world_pos);
	void NodeAddImpulse(NodeRef ref, const Vec3 &dt_velocity);
	void NodeAddImpulse(NodeRef ref, const Vec3 &dt_velocity, const Vec3 &world_pos);
	void NodeAddTorque(NodeRef ref, const Vec3 &T);
	void NodeAddTorqueImpulse(NodeRef ref, const Vec3 &T);

	void NodeAddForce(const Node &node, const Vec3 &F) { NodeAddForce(node.ref, F); }
	void NodeAddForce(const Node &node, const Vec3 &F, const Vec3 &world_pos) { NodeAddForce(node.ref, F, world_pos); }
	void NodeAddImpulse(const Node &node, const Vec3 &dt_velocity) { NodeAddImpulse(node.ref, dt_velocity); }
	void NodeAddImpulse(const Node &node, const Vec3 &dt_velocity, const Vec3 &world_pos) { NodeAddImpulse(node.ref, dt_velocity, world_pos); }
	void NodeAddTorque(const Node &node, const Vec3 &T) { NodeAddTorque(node.ref, T); }
	void NodeAddTorqueImpulse(const Node &node, const Vec3 &T) { NodeAddTorqueImpulse(node.ref, T); }

	Vec3 NodeGetPointVelocity(const Node &node, const Vec3 &world_pos) const { return NodeGetPointVelocity(node.ref, world_pos); }
	Vec3 NodeGetPointVelocity(NodeRef ref, const Vec3 &world_pos) const;

	Vec3 NodeGetLinearVelocity(NodeRef ref) const;
	void NodeSetLinearVelocity(NodeRef ref, const Vec3 &V);
	Vec3 NodeGetAngularVelocity(NodeRef ref) const;
	void NodeSetAngularVelocity(NodeRef ref, const Vec3 &W);

	Vec3 NodeGetLinearVelocity(const Node &node) const { return NodeGetLinearVelocity(node.ref); }
	void NodeSetLinearVelocity(const Node &node, const Vec3 &V) { NodeSetLinearVelocity(node.ref, V); }
	Vec3 NodeGetAngularVelocity(const Node &node) const { return NodeGetAngularVelocity(node.ref); }
	void NodeSetAngularVelocity(const Node &node, const Vec3 &W) { NodeSetAngularVelocity(node.ref, W); }

	Vec3 NodeGetLinearFactor(NodeRef ref) const;
	void NodeSetLinearFactor(NodeRef ref, const Vec3 &k);
	Vec3 NodeGetAngularFactor(NodeRef ref) const;
	void NodeSetAngularFactor(NodeRef ref, const Vec3 &k);

	Vec3 NodeGetLinearFactor(const Node &node) const { return NodeGetLinearFactor(node.ref); }
	void NodeSetLinearFactor(const Node &node, const Vec3 &k) { NodeSetLinearFactor(node.ref, k); }
	Vec3 NodeGetAngularFactor(const Node &node) const { return NodeGetAngularFactor(node.ref); }
	void NodeSetAngularFactor(const Node &node, const Vec3 &k) { NodeSetAngularFactor(node.ref, k); }

	void RenderCollision(bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_decl, bgfx::ProgramHandle program, RenderState state, uint32_t depth);

private:
	std::map<NodeRef, TauNode> nodes;
	std::map<NodeRef, CollisionEventTrackingMode> node_collision_event_tracking_modes;
	NodePairContacts latest_contacts;
};

} // namespace hg
