// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "engine/physics.h"
#include "engine/scene_tau_physics_contact.h"

#include "foundation/dynamic_aabb_tree.h"
#include "foundation/quaternion.h"
#include "foundation/rw_interface.h"
#include "foundation/time.h"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hg {

class Scene;
class SceneTauPhysics;
struct CollisionGeometry;
struct TauStepScratch;

namespace tau_internal {
static constexpr size_t kTauIslandHistogramBucketCount = 7;

struct TauStepReuseStats {
	size_t proxy_reuses{0};
	size_t manifold_reuses{0};
	size_t manifold_points_reused{0};
	size_t manifold_reuse_misses{0};
	size_t cuboid_coherent_reuses{0};
	size_t cuboid_coherent_points_reused{0};
	size_t cuboid_coherent_fallbacks{0};
	size_t primitive_manifolds{0};
	size_t warm_start_hits{0};
	size_t warm_start_misses{0};
	size_t zero_restitution_constraints{0};
	size_t nonzero_restitution_constraints{0};
	size_t zero_friction_constraints{0};
	size_t nonzero_friction_constraints{0};
	size_t rolling_friction_pass_skips{0};
	size_t rolling_friction_contact_evaluations{0};
	size_t all_sleeping_substeps_skipped{0};
	size_t all_sleeping_body_checks{0};
	size_t scratch_growths{0};
	size_t body_proxy_capacity{0};
	size_t candidate_capacity{0};
	size_t contact_capacity{0};
	size_t position_constraint_capacity{0};
	size_t velocity_constraint_capacity{0};
	size_t island_body_capacity{0};
	size_t island_workload_capacity{0};
};

// Optional per-substep topology/workload capture used to decide whether
// independent-island scheduling can amortize its partitioning overhead. Count
// histograms use 0, 1, 2-4, 5-16, 17-64, 65-256, and 257+ buckets. Evaluation
// histograms use 0, 1-64, 65-256, 257-1024, 1025-4096, 4097-16384, and 16385+.
struct TauIslandWorkloadStats {
	bool collected{false};
	size_t total_islands{0};
	size_t active_islands{0};
	size_t sleeping_islands{0};
	size_t solver_islands{0};
	size_t bodies_in_active_islands{0};
	size_t active_bodies{0};
	size_t proxy_updates{0};
	size_t candidate_pairs{0};
	size_t narrowphase_calls{0};
	size_t contacts{0};
	size_t position_constraints{0};
	size_t velocity_constraints{0};
	size_t joint_constraints{0};
	size_t position_evaluations{0};
	size_t velocity_evaluations{0};
	size_t rolling_contact_evaluations{0};
	size_t solver_evaluations{0};
	size_t max_bodies{0};
	size_t max_active_bodies{0};
	size_t max_contacts{0};
	size_t max_position_constraints{0};
	size_t max_velocity_constraints{0};
	size_t max_joint_constraints{0};
	size_t max_solver_evaluations{0};
	float largest_body_share{0.f};
	float largest_active_body_share{0.f};
	float largest_contact_share{0.f};
	float largest_solver_evaluation_share{0.f};
	std::array<size_t, kTauIslandHistogramBucketCount> body_histogram{};
	std::array<size_t, kTauIslandHistogramBucketCount> active_body_histogram{};
	std::array<size_t, kTauIslandHistogramBucketCount> contact_histogram{};
	std::array<size_t, kTauIslandHistogramBucketCount> position_constraint_histogram{};
	std::array<size_t, kTauIslandHistogramBucketCount> velocity_constraint_histogram{};
	std::array<size_t, kTauIslandHistogramBucketCount> joint_constraint_histogram{};
	std::array<size_t, kTauIslandHistogramBucketCount> solver_evaluation_histogram{};
};

bool IsNodeSleeping(const SceneTauPhysics &physics, NodeRef ref);
uint32_t GetNodeSleepIslandId(const SceneTauPhysics &physics, NodeRef ref);
bool HasNodeSleepingSupportSnapshot(const SceneTauPhysics &physics, NodeRef ref);
TauStepReuseStats GetLastStepReuseStats(const SceneTauPhysics &physics);
TauIslandWorkloadStats GetLastIslandWorkloadStats(const SceneTauPhysics &physics);
void SetIslandWorkloadDiagnosticsForTest(SceneTauPhysics &physics, bool enable);
void TransformNodeSleepCohortForTest(
	SceneTauPhysics &physics, NodeRef ref, const Vec3 &displacement, const Quaternion &rotation);
}

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
	std::string collision_resource;
	std::shared_ptr<const CollisionGeometry> collision_geometry;
};

struct TauWorldShape {
	const TauCollisionShape *shape{nullptr};
	uint32_t shape_index{0};
	Vec3 position{Vec3::Zero};
	float radius{0.f};
	TauCapsuleGeometry capsule;
	OBB obb;
	MinMax bounds;
};

enum class TauActivationState : uint8_t { Awake, SleepCandidate, Sleeping };

struct TauSleepSupportSnapshot {
	NodeRef ref{};
	Vec3 position{Vec3::Zero};
	Quaternion orientation{Quaternion::Identity};
};

struct TauNodeCold {
	std::vector<TauCollisionShape> shapes;
	std::vector<TauWorldShape> world_shapes;
	MinMax world_bounds;
	DynamicAABBTreeProxy broadphase_proxy{InvalidDynamicAABBTreeProxy};
	std::vector<NodeRef> sleep_supports;
	std::vector<NodeRef> sleep_neighbors;
	std::array<NodeRef, 8> sleep_dynamic_supports{};
	std::array<TauSleepSupportSnapshot, 8> sleeping_support_snapshots{};
	Mat3 inverse_inertia_body{Mat3::Zero};
	Vec3 previous_position{Vec3::Zero};
	Quaternion previous_orientation{Quaternion::Identity};
	Vec3 scale{Vec3::One};
	Vec3 accumulated_force{Vec3::Zero};
	Vec3 accumulated_torque{Vec3::Zero};
	float linear_damping{0.f};
	float angular_damping{0.f};
	float friction{0.f};
	float restitution{0.f};
	float rolling_friction{0.f};
	float sleep_timer{0.f};
	uint32_t sleep_island_id{0};
	uint32_t island_index{0xffffffff};
	uint8_t sleep_dynamic_support_count{0};
	uint8_t sleeping_support_snapshot_count{0};
	uint8_t unsupported_sleep_steps{0};
	bool world_proxy_cache_valid{false};
	bool externally_moved{false};
	bool transform_dirty{true};
	bool deactivation_enabled{true};
};

// Solver-hot body state. TauNodeStore keeps these entries contiguous and keeps
// ownership, shapes, broad-phase caches, and sleep history in a parallel cold
// array reached only by phases that need it.
struct TauNode {
	TauNodeCold *cold{nullptr};
	RigidBodyType body_type{RBT_Static};
	float total_mass{0.f};
	float inverse_mass{0.f};
	Vec3 position{Vec3::Zero};
	Quaternion orientation{Quaternion::Identity};
	Mat3 world_rotation{Mat3::Identity};
	Mat3 inverse_inertia_world{Mat3::Zero};
	Vec3 linear_velocity{Vec3::Zero};
	Vec3 angular_velocity{Vec3::Zero};
	Vec3 linear_factor{Vec3::One};
	Vec3 angular_factor{Vec3::One};
	TauActivationState activation_state{TauActivationState::Awake};
};

namespace tau_internal {

// Tau traverses every hot body several times per substep. Keep those payloads
// contiguous while retaining deterministic NodeRef ordering and O(1) lookup
// for the public node-oriented API.
struct TauNodeSlot {
	NodeRef first{};
	TauNode second;
};

static_assert(sizeof(TauNodeSlot) < sizeof(TauNodeCold), "Tau hot body storage must remain smaller than its cold payload");

class TauNodeStore {
public:
	using Storage = std::vector<TauNodeSlot>;
	using iterator = Storage::iterator;
	using const_iterator = Storage::const_iterator;

	iterator begin() { return slots.begin(); }
	const_iterator begin() const { return slots.begin(); }
	iterator end() { return slots.end(); }
	const_iterator end() const { return slots.end(); }

	bool empty() const { return slots.empty(); }
	size_t size() const { return slots.size(); }
	size_t capacity() const { return slots.capacity(); }
	void reserve(size_t count);
	void clear();

	iterator find(NodeRef ref);
	const_iterator find(NodeRef ref) const;
	bool contains(NodeRef ref) const;
	TauNode &InsertOrAssign(NodeRef ref, TauNode &&node, TauNodeCold &&cold);
	iterator erase(iterator it);

	// Exposed only for focused storage invariant tests.
	bool IsOrderedAndIndexed() const;

private:
	void ReindexFrom(size_t first);
	void RebindColdStorage(size_t first = 0);

	Storage slots;
	std::vector<TauNodeCold> cold_slots;
	std::unordered_map<NodeRef, size_t> indices;
};

} // namespace tau_internal

// Generic 6-DoF constraints are unconstrained until limits or motors are set.
// Keep their frames so Tau can preserve Bullet's current creation semantics and
// grow the implementation without changing the public physics API again.
struct Tau6DofConstraint {
	NodeRef ref_a{};
	NodeRef ref_b{};
	Mat4 frame_in_a{Mat4::Identity};
	Mat4 frame_in_b{Mat4::Identity};
};

class SceneTauPhysics {
public:
	SceneTauPhysics(int thread_count = 1);
	~SceneTauPhysics();

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

	bool NodeHasBody(NodeRef ref) const { return nodes.contains(ref); }
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
	/// Return whether the body is currently sleeping.
	bool NodeIsSleeping(NodeRef ref) const;
	bool NodeIsSleeping(const Node &node) const { return NodeIsSleeping(node.ref); }

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

	void Add6DofConstraint(const NodeRef &node_a_ref, const NodeRef &node_b_ref, const Mat4 &anchor_a_local, const Mat4 &anchor_b_local);
	void Add6DofConstraint(const Node &node_a, const Node &node_b, const Mat4 &anchor_a_local, const Mat4 &anchor_b_local) {
		Add6DofConstraint(node_a.ref, node_b.ref, anchor_a_local, anchor_b_local);
	}

	RaycastOut RaycastFirstHit(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const;
	std::vector<RaycastOut> RaycastAllHits(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const;

	void RenderCollision(bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_decl, bgfx::ProgramHandle program, RenderState state, uint32_t depth);

	// Match Bullet's internal pre-tick contract: one call immediately before
	// each physics sub-step, with that sub-step's duration.
	void SetPreTickCallback(const std::function<void(SceneTauPhysics &, hg::time_ns t)> &cbk);
	void TriggerPreTickCallback(hg::time_ns dt);

private:
	friend bool tau_internal::IsNodeSleeping(const SceneTauPhysics &physics, NodeRef ref);
	friend uint32_t tau_internal::GetNodeSleepIslandId(const SceneTauPhysics &physics, NodeRef ref);
	friend bool tau_internal::HasNodeSleepingSupportSnapshot(const SceneTauPhysics &physics, NodeRef ref);
	friend tau_internal::TauStepReuseStats tau_internal::GetLastStepReuseStats(const SceneTauPhysics &physics);
	friend tau_internal::TauIslandWorkloadStats tau_internal::GetLastIslandWorkloadStats(const SceneTauPhysics &physics);
	friend void tau_internal::SetIslandWorkloadDiagnosticsForTest(SceneTauPhysics &physics, bool enable);
	friend void tau_internal::TransformNodeSleepCohortForTest(
		SceneTauPhysics &physics, NodeRef ref, const Vec3 &displacement, const Quaternion &rotation);

	std::shared_ptr<const CollisionGeometry> LoadCollisionGeometryResource(
		const Reader &ir, const ReadProvider &ip, const std::string &resource);
	void InvalidateAllSleepingFastPath() const {
		all_dynamic_bodies_sleeping = false;
		all_sleeping_fast_path_validated = false;
	}

	tau_internal::TauNodeStore nodes;
	std::map<std::string, std::shared_ptr<const CollisionGeometry>> collision_geometries;
	std::vector<Tau6DofConstraint> constraints;
	std::vector<TauContactManifold> contact_manifolds;
	std::unordered_multimap<size_t, size_t> contact_manifold_lookup;
	DynamicAABBTree broadphase_tree;
	std::unordered_map<DynamicAABBTreeProxy, std::unordered_set<DynamicAABBTreeProxy>> broadphase_pairs;
	uint32_t contact_step{0};
	uint32_t next_sleep_island_id{1};
	time_ns fixed_step_accumulator{0};
	bool requires_full_substep{false};
	// Updated after each complete substep. Public mutations invalidate both
	// fields so active worlds reject the all-sleeping path in O(1). Once all
	// dynamic bodies sleep, one defensive scan validates the steady state.
	mutable bool all_dynamic_bodies_sleeping{false};
	mutable bool all_sleeping_fast_path_validated{false};
	std::map<NodeRef, CollisionEventTrackingMode> node_collision_event_tracking_modes;
	NodePairContacts latest_contacts;
	std::unique_ptr<TauStepScratch> step_scratch;
	tau_internal::TauStepReuseStats last_step_reuse_stats;
	tau_internal::TauIslandWorkloadStats last_island_workload_stats;
	bool force_island_workload_diagnostics{false};
	std::function<void(SceneTauPhysics &, hg::time_ns t)> pre_tick_callback;
};

} // namespace hg
