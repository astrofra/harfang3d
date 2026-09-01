// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene_tau_physics.h"

#include "engine/assets_rw_interface.h"
#include "engine/collision_geometry.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"

#include "foundation/bvh.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/matrix4.h"
#include "foundation/minmax.h"
#include "foundation/obb.h"
#include "foundation/profiler.h"
#include "foundation/string.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace hg {

namespace {

static const Vec3 k_tau_gravity(0.f, -9.81f, 0.f);
static constexpr float k_tau_collision_epsilon = 1e-5f;
static constexpr float k_tau_position_slop = 0.002f;
static constexpr float k_tau_position_correction = 0.75f;
static constexpr float k_tau_restitution_threshold = 1.0f;
static constexpr float k_tau_baumgarte = 0.15f;
static constexpr float k_tau_rolling_friction_impulse_scale = 0.015f;
static constexpr float k_tau_manifold_point_tolerance = 0.05f;
static constexpr float k_tau_manifold_normal_tolerance = 0.94f;
static constexpr float k_tau_manifold_clip_tolerance = 0.001f;
static constexpr float k_tau_sat_tie_tolerance = 0.0001f;
static constexpr float k_tau_bullet_convex_margin = 0.04f;
static constexpr uint32_t k_tau_manifold_lifetime = 3;
static constexpr size_t k_tau_max_manifolds = 4096;
static constexpr int k_tau_position_iterations = 3;
static constexpr int k_tau_velocity_iterations = 8;

bool TauProfilingEnabled() {
	static const bool enabled = [] {
		const char *value = std::getenv("HG_TAU_PROFILE");
		return value != nullptr && value[0] != '\0' && value[0] != '0';
	}();
	return enabled;
}

class TauProfileSection {
public:
	explicit TauProfileSection(const char *name) {
		if (TauProfilingEnabled())
			section_index = BeginProfilerSection(name);
	}
	~TauProfileSection() {
		if (section_index != std::numeric_limits<ProfilerSectionIndex>::max())
			EndProfilerSection(section_index);
	}

	TauProfileSection(const TauProfileSection &) = delete;
	TauProfileSection &operator=(const TauProfileSection &) = delete;

private:
	ProfilerSectionIndex section_index{std::numeric_limits<ProfilerSectionIndex>::max()};
};

bool TauContactDiagnosticsEnabled();

bool HasTauResourceSuffix(const std::string &resource, const std::string &suffix) {
	return resource.size() >= suffix.size() && resource.compare(resource.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string ResolveTauCollisionResource(const std::string &resource) {
	static const std::string logical_suffix = ".physics";
	static const std::string bullet_suffix = "_bullet";
	if (HasTauResourceSuffix(resource, logical_suffix))
		return resource + "_triangles";
	if (HasTauResourceSuffix(resource, logical_suffix + bullet_suffix))
		return resource.substr(0, resource.size() - bullet_suffix.size()) + "_triangles";
	return resource;
}

enum class TauWorldWriteMode { Reset, CaptureSource, Solved };

struct TauWorldShape {
	NodeRef ref{};
	TauNode *node{nullptr};
	const TauCollisionShape *shape{nullptr};
	uint32_t shape_index{0};
	Vec3 position{Vec3::Zero};
	Vec3 previous_position{Vec3::Zero};
	float radius{0.f};
	TauCapsuleGeometry capsule;
	TauCapsuleGeometry previous_capsule;
	OBB obb;
	OBB previous_obb;
	MinMax bounds;
};

struct TauBodyProxy {
	NodeRef ref{};
	TauNode *node{nullptr};
	std::vector<TauWorldShape> shapes;
	MinMax bounds;
};

struct TauBodyPair {
	uint32_t a{0}, b{0};
};

struct TauContactConstraint {
	NodeRef ref_a{};
	NodeRef ref_b{};
	TauNode *node_a{nullptr};
	TauNode *node_b{nullptr};
	uint32_t shape_a{0}, shape_b{0};
	Vec3 local_point_a{Vec3::Zero}, local_point_b{Vec3::Zero};
	Vec3 point{Vec3::Zero};
	Vec3 normal{Vec3::Up}; // points from A toward B
	float penetration{0.f};
	float friction{0.5f};
	float restitution{0.f};
	float restitution_velocity{0.f};
	float accumulated_normal_impulse{0.f};
	Vec3 accumulated_tangent_impulse{Vec3::Zero};
	size_t manifold_index{std::numeric_limits<size_t>::max()};
	uint8_t manifold_point_index{0};
	uint8_t manifold_point_count{1};
	bool persistent{false};
};

struct TauContactDiagnostics {
	bool enabled{false};
	size_t total_bodies{0}, dynamic_bodies{0};
	size_t proxy_bodies{0}, proxy_shapes{0}, proxy_shape_vector_reserves{0};
	size_t body_pair_tests{0}, static_pair_rejects{0}, body_bounds_rejects{0}, body_pair_candidates{0};
	size_t shape_body_bounds_tests{0}, shape_body_bounds_rejects{0};
	size_t shape_pair_bounds_tests{0}, shape_pair_bounds_rejects{0}, narrowphase_calls{0};
	size_t shape_pairs{0};
	size_t face_a_manifolds{0}, face_b_manifolds{0}, edge_edge_manifolds{0};
	size_t manifold_points{0};
	size_t warm_start_hits{0}, warm_start_misses{0};
	size_t manifold_cache_comparisons{0}, manifold_cache_evictions{0}, manifold_cache_overflows{0};
	size_t stale_discards{0}, candidate_reallocations{0}, contact_reallocations{0};
	size_t position_constraint_evaluations{0}, velocity_constraint_evaluations{0}, rolling_contact_evaluations{0};
	size_t tracked_contact_evaluations{0}, motion_updates{0};
	size_t friction_clamps{0};
	float normal_impulse_total{0.f}, tangent_impulse_total{0.f};
	float max_penetration{0.f}, max_post_solve_penetration{0.f};
};

bool IsTauPhase1RigidBodyType(RigidBodyType type) {
	return type == RBT_Dynamic || type == RBT_Static || type == RBT_Kinematic;
}

bool IsDynamicTauNode(const TauNode &node) {
	return node.body_type == RBT_Dynamic && node.total_mass > 0.f && node.inverse_mass > 0.f;
}

bool IsTauSolverShape(CollisionType type) { return type == CT_Cube || type == CT_Sphere || type == CT_Capsule; }

bool IsTauAnalyticShape(CollisionType type) {
	return type == CT_Cube || type == CT_Sphere || type == CT_Capsule || type == CT_Cone || type == CT_Cylinder;
}

bool IsTauRaycastShape(CollisionType type) { return IsTauAnalyticShape(type) || type == CT_Mesh; }

std::vector<TauCollisionShape> CollectTauPhase1Collisions(const Node &node, float &total_mass) {
	std::vector<TauCollisionShape> shapes;
	total_mass = 0.f;

	for (size_t i = 0; i < node.GetCollisionCount(); ++i) {
		const auto collision = node.GetCollision(i);
		if (!collision.IsValid())
			continue;
		if (!IsTauRaycastShape(collision.GetType()))
			return {};

		TauCollisionShape shape;
		shape.type = collision.GetType();
		shape.local_transform = collision.GetLocalTransform();
		Decompose(shape.local_transform, &shape.local_position, &shape.local_rotation, nullptr);
		shape.size = collision.GetSize();
		shape.radius = collision.GetRadius();
		shape.mass = collision.GetMass();
		shape.collision_resource = collision.GetCollisionResource();
		// Collision components currently do not store contact material properties in Scene.
		// Keep them on the rigid body side for the Tau phase-1 backend, like Bullet does.
		shape.friction = 0.f;
		shape.restitution = 0.f;

		// Cone and cylinder are raycast/debug shapes for now. Do not turn them into
		// dynamic bodies until their narrow phase is implemented.
		if (IsTauSolverShape(shape.type))
			total_mass += shape.mass;
		shapes.push_back(shape);
	}

	return shapes;
}

Mat4 GetNodeWorld(const Node &node) {
	// Match Bullet and fetch an up-to-date world matrix, especially for freshly instantiated nodes.
	return node.ComputeWorld();
}

TauNode *FindTauNode(std::map<NodeRef, TauNode> &nodes, NodeRef ref) {
	const auto it = nodes.find(ref);
	return it != std::end(nodes) ? &it->second : nullptr;
}

const TauNode *FindTauNode(const std::map<NodeRef, TauNode> &nodes, NodeRef ref) {
	const auto it = nodes.find(ref);
	return it != std::end(nodes) ? &it->second : nullptr;
}

void ResetDynamicState(TauNode &node) {
	node.linear_velocity = Vec3::Zero;
	node.angular_velocity = Vec3::Zero;
	node.accumulated_force = Vec3::Zero;
	node.accumulated_torque = Vec3::Zero;
}

Mat3 DiagonalMat3(const Vec3 &v) {
	Mat3 out = Mat3::Zero;
	out.m[0][0] = v.x;
	out.m[1][1] = v.y;
	out.m[2][2] = v.z;
	return out;
}

Mat3 OuterProductMat3(const Vec3 &a, const Vec3 &b) {
	Mat3 out = Mat3::Zero;
	out.m[0][0] = a.x * b.x;
	out.m[0][1] = a.x * b.y;
	out.m[0][2] = a.x * b.z;
	out.m[1][0] = a.y * b.x;
	out.m[1][1] = a.y * b.y;
	out.m[1][2] = a.y * b.z;
	out.m[2][0] = a.z * b.x;
	out.m[2][1] = a.z * b.y;
	out.m[2][2] = a.z * b.z;
	return out;
}

Mat3 ParallelAxisTensor(float mass, const Vec3 &offset) {
	const float rr = Dot(offset, offset);
	return (Mat3::Identity * rr - OuterProductMat3(offset, offset)) * mass;
}

Mat4 ComposeTauWorld(const TauNode &node) {
	return TransformationMat4(node.position, ToMatrix3(node.orientation), node.scale);
}

void SetTauNodeWorld(TauNode &node, const Mat4 &world, TauWorldWriteMode mode) {
	const Vec3 previous_position = node.position;
	const Quaternion previous_orientation = node.orientation;

	Mat3 rotation;
	Decompose(world, &node.position, &rotation, &node.scale);
	node.orientation = Normalize(QuaternionFromMatrix3(rotation));

	if (mode == TauWorldWriteMode::Reset) {
		node.previous_position = node.position;
		node.previous_orientation = node.orientation;
		node.motion.Reset(world);
	} else if (mode == TauWorldWriteMode::CaptureSource) {
		node.previous_position = previous_position;
		node.previous_orientation = previous_orientation;
		node.motion.CaptureSourceWorld(world);
	} else {
		node.previous_position = previous_position;
		node.previous_orientation = previous_orientation;
		node.motion.WriteSolvedWorld(world);
	}
}

void UpdateTauMotionFromState(TauNode &node) {
	node.motion.WriteSolvedWorld(ComposeTauWorld(node));
}

void IntegrateTauOrientation(TauNode &node, const Vec3 &angular_step) {
	const float angle = Len(angular_step);
	if (angle <= k_tau_collision_epsilon)
		return;

	node.orientation = Normalize(QuaternionFromAxisAngle(angle, angular_step / angle) * node.orientation);
}

Mat3 ComputeTauInverseInertiaWorld(const TauNode &node) {
	const Mat3 rotation = ToMatrix3(node.orientation);
	return rotation * node.inverse_inertia_body * Transpose(rotation);
}

void RefreshTauMassProperties(TauNode &node) {
	node.inverse_mass = IsDynamicTauNode(node) || (node.body_type == RBT_Dynamic && node.total_mass > 0.f) ? 1.f / node.total_mass : 0.f;
	node.inverse_inertia_body = Mat3::Zero;

	if (node.body_type != RBT_Dynamic || node.total_mass <= 0.f)
		return;

	const Vec3 abs_scale = Abs(node.scale);
	Mat3 inertia = Mat3::Zero;

	for (const auto &shape : node.shapes) {
		if (!IsTauSolverShape(shape.type) || shape.mass <= 0.f)
			continue;

		Mat3 centered_inertia;
		if (shape.type == CT_Sphere) {
			const float radius = shape.radius * std::max({abs_scale.x, abs_scale.y, abs_scale.z});
			centered_inertia = Mat3::Identity * (2.f * shape.mass * radius * radius / 5.f);
		} else if (shape.type == CT_Capsule) {
			// Match btCapsuleShape::calculateLocalInertia: approximate the capsule
			// with the box enclosing its cylindrical span and spherical caps.
			const float radius = shape.radius * std::max(abs_scale.x, abs_scale.z);
			const float height = Abs(shape.size.y) * abs_scale.y;
			const Vec3 bounding_size(radius * 2.f, height + radius * 2.f, radius * 2.f);
			const Vec3 size_sq = bounding_size * bounding_size;
			const Vec3 diagonal(shape.mass * (size_sq.y + size_sq.z) / 12.f, shape.mass * (size_sq.x + size_sq.z) / 12.f,
				shape.mass * (size_sq.x + size_sq.y) / 12.f);
			centered_inertia = shape.local_rotation * DiagonalMat3(diagonal) * Transpose(shape.local_rotation);
		} else {
			const Vec3 scaled_size = abs_scale * shape.size;
			const Vec3 size_sq = scaled_size * scaled_size;
			const Vec3 diagonal(shape.mass * (size_sq.y + size_sq.z) / 12.f, shape.mass * (size_sq.x + size_sq.z) / 12.f,
				shape.mass * (size_sq.x + size_sq.y) / 12.f);
			centered_inertia = shape.local_rotation * DiagonalMat3(diagonal) * Transpose(shape.local_rotation);
		}
		const Vec3 local_offset = node.scale * shape.local_position;

		inertia += centered_inertia + ParallelAxisTensor(shape.mass, local_offset);
	}

	Mat3 inverse_inertia;
	if (Inverse(inertia, inverse_inertia))
		node.inverse_inertia_body = inverse_inertia;
}

float GetTauBodyFriction(const TauNode &node, const TauCollisionShape &shape) {
	return std::max(node.friction, shape.friction);
}

float GetTauBodyRestitution(const TauNode &node, const TauCollisionShape &shape) {
	return std::max(node.restitution, shape.restitution);
}

float CombineTauFriction(const TauNode &a, const TauCollisionShape &shape_a, const TauNode &b, const TauCollisionShape &shape_b) {
	return Clamp(GetTauBodyFriction(a, shape_a) * GetTauBodyFriction(b, shape_b), -10.f, 10.f);
}

float CombineTauRestitution(const TauNode &a, const TauCollisionShape &shape_a, const TauNode &b, const TauCollisionShape &shape_b) {
	return GetTauBodyRestitution(a, shape_a) * GetTauBodyRestitution(b, shape_b);
}

float CombineTauRollingFriction(const TauNode &a, const TauNode &b) {
	// A static ground commonly uses zero rolling friction. Keep the dynamic body's
	// coefficient effective so a QA sweep from 0 to 1 remains observable.
	return Clamp(std::max(a.rolling_friction, b.rolling_friction), 0.f, 1.f);
}

OBB BuildTauWorldOBB(const Vec3 &position, const Quaternion &orientation, const Vec3 &scale, const TauCollisionShape &shape) {
	const Mat3 node_rotation = ToMatrix3(orientation);
	return {position + node_rotation * (scale * shape.local_position), Abs(scale) * shape.size, node_rotation * shape.local_rotation};
}

OBB BuildTauWorldOBB(const TauNode &node, const TauCollisionShape &shape) {
	return BuildTauWorldOBB(node.position, node.orientation, node.scale, shape);
}

OBB BuildTauPreviousWorldOBB(const TauNode &node, const TauCollisionShape &shape) {
	return BuildTauWorldOBB(node.previous_position, node.previous_orientation, node.scale, shape);
}

Vec3 BuildTauWorldSphereCenter(const TauNode &node, const TauCollisionShape &shape, bool previous = false) {
	const Quaternion orientation = previous ? node.previous_orientation : node.orientation;
	const Vec3 position = previous ? node.previous_position : node.position;
	return position + ToMatrix3(orientation) * (node.scale * shape.local_position);
}

float BuildTauWorldSphereRadius(const TauNode &node, const TauCollisionShape &shape) {
	const Vec3 scale = Abs(node.scale);
	return shape.radius * std::max({scale.x, scale.y, scale.z});
}

Mat3 BuildTauWorldSphereRotation(const TauNode &node, const TauCollisionShape &shape) {
	return ToMatrix3(node.orientation) * shape.local_rotation;
}

struct TauPrimitiveFrame {
	Vec3 center{Vec3::Zero};
	Vec3 axis_x{Vec3::Right}, axis_y{Vec3::Up}, axis_z{Vec3::Front};
};

struct TauRayShapeHit {
	float t{std::numeric_limits<float>::max()};
	Vec3 normal{Vec3::Zero};
};

TauPrimitiveFrame BuildTauPrimitiveFrame(const TauNode &node, const TauCollisionShape &shape) {
	const Mat3 rotation = ToMatrix3(node.orientation) * shape.local_rotation;
	return {BuildTauWorldSphereCenter(node, shape), Normalize(GetX(rotation)), Normalize(GetY(rotation)), Normalize(GetZ(rotation))};
}

TauCapsuleGeometry BuildTauWorldCapsule(const TauNode &node, const TauCollisionShape &shape, bool previous = false) {
	const Quaternion orientation = previous ? node.previous_orientation : node.orientation;
	const Mat3 rotation = ToMatrix3(orientation) * shape.local_rotation;
	const Vec3 center = BuildTauWorldSphereCenter(node, shape, previous);
	const Vec3 scale = Abs(node.scale);
	const float half_height = Abs(shape.size.y) * scale.y * 0.5f;
	const Vec3 half_axis = Normalize(GetY(rotation)) * half_height;
	return {center - half_axis, center + half_axis, shape.radius * std::max(scale.x, scale.z)};
}

Vec3 TauFrameWorldToLocal(const TauPrimitiveFrame &frame, const Vec3 &point) {
	const Vec3 delta = point - frame.center;
	return {Dot(delta, frame.axis_x), Dot(delta, frame.axis_y), Dot(delta, frame.axis_z)};
}

Vec3 TauFrameVectorToLocal(const TauPrimitiveFrame &frame, const Vec3 &vector) {
	return {Dot(vector, frame.axis_x), Dot(vector, frame.axis_y), Dot(vector, frame.axis_z)};
}

Vec3 TauFrameVectorToWorld(const TauPrimitiveFrame &frame, const Vec3 &vector) {
	return frame.axis_x * vector.x + frame.axis_y * vector.y + frame.axis_z * vector.z;
}

bool SolveTauRayQuadratic(float a, float b, float c, float &t0, float &t1) {
	if (Abs(a) <= k_tau_collision_epsilon) {
		if (Abs(b) <= k_tau_collision_epsilon)
			return false;
		t0 = t1 = -c / b;
		return true;
	}

	const float discriminant = b * b - 4.f * a * c;
	if (discriminant < 0.f)
		return false;

	const float root = Sqrt(std::max(0.f, discriminant));
	t0 = (-b - root) / (2.f * a);
	t1 = (-b + root) / (2.f * a);
	if (t0 > t1)
		std::swap(t0, t1);
	return true;
}

void ConsiderTauRayHit(float t, const Vec3 &normal, float max_distance, TauRayShapeHit &hit) {
	if (t < -k_tau_collision_epsilon || t > max_distance + k_tau_collision_epsilon || t >= hit.t)
		return;
	hit.t = Clamp(t, 0.f, max_distance);
	hit.normal = normal;
}

void IntersectTauRaySphere(const Vec3 &origin, const Vec3 &direction, const Vec3 &center, float radius, float max_distance,
	int hemisphere, TauRayShapeHit &hit) {
	if (radius <= k_tau_collision_epsilon)
		return;

	const Vec3 offset = origin - center;
	float t0, t1;
	if (!SolveTauRayQuadratic(Dot(direction, direction), 2.f * Dot(offset, direction), Dot(offset, offset) - radius * radius, t0, t1))
		return;

	const float roots[2] = {t0, t1};
	for (float t : roots) {
		const Vec3 point = origin + direction * t;
		if ((hemisphere > 0 && point.y < center.y - k_tau_collision_epsilon) ||
			(hemisphere < 0 && point.y > center.y + k_tau_collision_epsilon))
			continue;
		ConsiderTauRayHit(t, Normalize(point - center), max_distance, hit);
	}
}

bool IntersectTauRayBox(
	const Vec3 &origin, const Vec3 &direction, const Vec3 &half_extents, float max_distance, TauRayShapeHit &hit) {
	float t_near = -std::numeric_limits<float>::max();
	float t_far = std::numeric_limits<float>::max();
	Vec3 near_normal = Vec3::Zero, far_normal = Vec3::Zero;

	for (int axis = 0; axis < 3; ++axis) {
		if (Abs(direction[axis]) <= k_tau_collision_epsilon) {
			if (origin[axis] < -half_extents[axis] || origin[axis] > half_extents[axis])
				return false;
			continue;
		}

		float t0 = (-half_extents[axis] - origin[axis]) / direction[axis];
		float t1 = (half_extents[axis] - origin[axis]) / direction[axis];
		Vec3 n0 = Vec3::Zero, n1 = Vec3::Zero;
		n0[axis] = -1.f;
		n1[axis] = 1.f;
		if (t0 > t1) {
			std::swap(t0, t1);
			std::swap(n0, n1);
		}
		if (t0 > t_near) {
			t_near = t0;
			near_normal = n0;
		}
		if (t1 < t_far) {
			t_far = t1;
			far_normal = n1;
		}
		if (t_near > t_far)
			return false;
	}

	if (t_far < -k_tau_collision_epsilon || t_near > max_distance + k_tau_collision_epsilon)
		return false;
	if (t_near >= 0.f)
		ConsiderTauRayHit(t_near, near_normal, max_distance, hit);
	else
		ConsiderTauRayHit(t_far, far_normal, max_distance, hit);
	return hit.t != std::numeric_limits<float>::max();
}

void IntersectTauRayCylinderSide(const Vec3 &origin, const Vec3 &direction, float radius, float half_height, float max_distance,
	TauRayShapeHit &hit) {
	float t0, t1;
	if (!SolveTauRayQuadratic(direction.x * direction.x + direction.z * direction.z,
			2.f * (origin.x * direction.x + origin.z * direction.z), origin.x * origin.x + origin.z * origin.z - radius * radius, t0, t1))
		return;

	const float roots[2] = {t0, t1};
	for (float t : roots) {
		const Vec3 point = origin + direction * t;
		if (point.y < -half_height - k_tau_collision_epsilon || point.y > half_height + k_tau_collision_epsilon)
			continue;
		ConsiderTauRayHit(t, Normalize(Vec3(point.x, 0.f, point.z)), max_distance, hit);
	}
}

void IntersectTauRayDisc(const Vec3 &origin, const Vec3 &direction, float y, float radius, const Vec3 &normal, float max_distance,
	TauRayShapeHit &hit) {
	if (Abs(direction.y) <= k_tau_collision_epsilon)
		return;
	const float t = (y - origin.y) / direction.y;
	const Vec3 point = origin + direction * t;
	if (point.x * point.x + point.z * point.z <= radius * radius + k_tau_collision_epsilon)
		ConsiderTauRayHit(t, normal, max_distance, hit);
}

bool IntersectTauRayCylinder(
	const Vec3 &origin, const Vec3 &direction, float radius, float height, float max_distance, TauRayShapeHit &hit) {
	if (radius <= k_tau_collision_epsilon || height <= k_tau_collision_epsilon)
		return false;
	const float half_height = height * 0.5f;
	IntersectTauRayCylinderSide(origin, direction, radius, half_height, max_distance, hit);
	IntersectTauRayDisc(origin, direction, half_height, radius, Vec3::Up, max_distance, hit);
	IntersectTauRayDisc(origin, direction, -half_height, radius, -Vec3::Up, max_distance, hit);
	return hit.t != std::numeric_limits<float>::max();
}

bool IntersectTauRayCapsule(
	const Vec3 &origin, const Vec3 &direction, float radius, float height, float max_distance, TauRayShapeHit &hit) {
	if (radius <= k_tau_collision_epsilon)
		return false;
	const float half_height = std::max(0.f, height * 0.5f);
	if (half_height > k_tau_collision_epsilon)
		IntersectTauRayCylinderSide(origin, direction, radius, half_height, max_distance, hit);
	IntersectTauRaySphere(origin, direction, Vec3(0.f, half_height, 0.f), radius, max_distance, 1, hit);
	IntersectTauRaySphere(origin, direction, Vec3(0.f, -half_height, 0.f), radius, max_distance, -1, hit);
	return hit.t != std::numeric_limits<float>::max();
}

bool IntersectTauRayCone(
	const Vec3 &origin, const Vec3 &direction, float radius, float height, float max_distance, TauRayShapeHit &hit) {
	if (radius <= k_tau_collision_epsilon || height <= k_tau_collision_epsilon)
		return false;

	const float half_height = height * 0.5f;
	const float slope = radius / height;
	const float slope_sq = slope * slope;
	// btConeShape keeps Bullet's default 4 cm convex margin instead of
	// subtracting it from its implicit dimensions (as btBoxShape does). Match
	// the resulting parallel side surface so the QA ray fan has the same
	// silhouette. The base cap is left at its declared height; its rounded rim
	// is outside this phase-1 analytic contract.
	const float side_margin = k_tau_bullet_convex_margin * Sqrt(1.f + slope_sq);
	const float apex_delta = half_height - origin.y + side_margin / slope;
	float t0, t1;
	if (SolveTauRayQuadratic(direction.x * direction.x + direction.z * direction.z - slope_sq * direction.y * direction.y,
			2.f * (origin.x * direction.x + origin.z * direction.z + slope_sq * apex_delta * direction.y),
			origin.x * origin.x + origin.z * origin.z - slope_sq * apex_delta * apex_delta, t0, t1)) {
		const float roots[2] = {t0, t1};
		for (float t : roots) {
			const Vec3 point = origin + direction * t;
			if (point.y < -half_height - k_tau_collision_epsilon || point.y > half_height + k_tau_collision_epsilon)
				continue;
			Vec3 normal(point.x, slope_sq * (half_height + side_margin / slope - point.y), point.z);
			if (Len(normal) <= k_tau_collision_epsilon)
				normal = Vec3::Up;
			ConsiderTauRayHit(t, Normalize(normal), max_distance, hit);
		}
	}

	IntersectTauRayDisc(origin, direction, -half_height, radius, -Vec3::Up, max_distance, hit);
	return hit.t != std::numeric_limits<float>::max();
}

bool IntersectTauRayMesh(const TauNode &node, const TauCollisionShape &shape, const Vec3 &world_origin, const Vec3 &world_direction,
	float max_distance, TauRayShapeHit &world_hit) {
	if (!shape.collision_geometry || shape.collision_geometry->triangles.empty())
		return false;

	const Mat4 local_to_world = ComposeTauWorld(node) * shape.local_transform;
	Mat4 world_to_local;
	if (!Inverse(local_to_world, world_to_local))
		return false;

	const Vec3 origin = world_to_local * world_origin;
	const Vec3 direction = world_to_local * (world_origin + world_direction) - origin;

	TauRayShapeHit local_hit;
	auto is_on_open_boundary = [&](const Vec3 &point) {
		bool on_boundary = false;
		const Vec3 tolerance(k_tau_collision_epsilon, k_tau_collision_epsilon, k_tau_collision_epsilon);
		TraverseBVH(shape.collision_geometry->boundary_bvh, MinMax(point - tolerance, point + tolerance), [&](uint32_t edge_index) {
			const auto &edge = shape.collision_geometry->boundary_edges[edge_index];
			const Vec3 edge_direction = edge.b - edge.a;
			const float edge_length_squared = Len2(edge_direction);
			if (edge_length_squared <= k_tau_collision_epsilon * k_tau_collision_epsilon)
				return true;
			const float edge_t = Clamp(Dot(point - edge.a, edge_direction) / edge_length_squared, 0.f, 1.f);
			on_boundary = Len2(point - (edge.a + edge_direction * edge_t)) <= k_tau_collision_epsilon * k_tau_collision_epsilon;
			return !on_boundary;
		});
		return on_boundary;
	};

	float traversal_distance = max_distance + k_tau_collision_epsilon;
	TraverseBVHRay(shape.collision_geometry->triangle_bvh, origin, direction, traversal_distance, [&](uint32_t triangle_index) {
		const auto &triangle = shape.collision_geometry->triangles[triangle_index];
		const Vec3 edge_ab = triangle.b - triangle.a;
		const Vec3 edge_ac = triangle.c - triangle.a;
		const Vec3 p = Cross(direction, edge_ac);
		const float determinant = Dot(edge_ab, p);
		if (Abs(determinant) <= k_tau_collision_epsilon)
			return true;

		const float inverse_determinant = 1.f / determinant;
		const Vec3 from_a = origin - triangle.a;
		const float u = Dot(from_a, p) * inverse_determinant;
		if (u < -k_tau_collision_epsilon || u > 1.f + k_tau_collision_epsilon)
			return true;

		const Vec3 q = Cross(from_a, edge_ab);
		const float v = Dot(direction, q) * inverse_determinant;
		if (v < -k_tau_collision_epsilon || u + v > 1.f + k_tau_collision_epsilon)
			return true;

		const float triangle_t = Dot(edge_ac, q) * inverse_determinant;
		// Bullet's cooked triangle mesh treats its open boundary as an open
		// set while still accepting shared internal edges. Preserve that
		// behavior without relying on AABB quantization details. Test against
		// the complete boundary so a boundary vertex cannot be accepted through
		// one of its incident internal edges.
		if (std::min({u, v, 1.f - u - v}) <= k_tau_collision_epsilon &&
			is_on_open_boundary(origin + direction * triangle_t))
			return true;

		Vec3 normal = Cross(edge_ab, edge_ac);
		if (Len2(normal) <= k_tau_collision_epsilon * k_tau_collision_epsilon)
			return true;
		normal = Normalize(normal);
		if (Dot(normal, direction) > 0.f)
			normal = -normal;
		ConsiderTauRayHit(triangle_t, normal, max_distance, local_hit);
		if (local_hit.t != std::numeric_limits<float>::max())
			traversal_distance = local_hit.t;
		return true;
	});

	if (local_hit.t == std::numeric_limits<float>::max())
		return false;

	world_hit.t = local_hit.t;
	world_hit.normal = Normalize(Transpose(Mat3(world_to_local)) * local_hit.normal);
	return true;
}

bool IntersectTauRayShape(const TauNode &node, const TauCollisionShape &shape, const Vec3 &world_origin, const Vec3 &world_direction,
	float max_distance, TauRayShapeHit &world_hit) {
	if (shape.type == CT_Mesh)
		return IntersectTauRayMesh(node, shape, world_origin, world_direction, max_distance, world_hit);

	const TauPrimitiveFrame frame = BuildTauPrimitiveFrame(node, shape);
	const Vec3 origin = TauFrameWorldToLocal(frame, world_origin);
	const Vec3 direction = TauFrameVectorToLocal(frame, world_direction);
	const Vec3 scale = Abs(node.scale);
	TauRayShapeHit local_hit;
	bool intersected = false;

	if (shape.type == CT_Cube) {
		intersected = IntersectTauRayBox(origin, direction, scale * Abs(shape.size) * 0.5f, max_distance, local_hit);
	} else if (shape.type == CT_Sphere) {
		IntersectTauRaySphere(origin, direction, Vec3::Zero, shape.radius * std::max({scale.x, scale.y, scale.z}), max_distance, 0, local_hit);
		intersected = local_hit.t != std::numeric_limits<float>::max();
	} else {
		const float radius = shape.radius * std::max(scale.x, scale.z);
		const float height = Abs(shape.size.y) * scale.y;
		if (shape.type == CT_Capsule)
			intersected = IntersectTauRayCapsule(origin, direction, radius, height, max_distance, local_hit);
		else if (shape.type == CT_Cylinder)
			intersected = IntersectTauRayCylinder(origin, direction, radius, height, max_distance, local_hit);
		else if (shape.type == CT_Cone)
			intersected = IntersectTauRayCone(origin, direction, radius, height, max_distance, local_hit);
	}

	if (!intersected)
		return false;
	world_hit.t = local_hit.t;
	world_hit.normal = Normalize(TauFrameVectorToWorld(frame, local_hit.normal));
	return true;
}

TauBodyProxy BuildTauBodyProxy(NodeRef ref, TauNode &node) {
	TauBodyProxy proxy;
	proxy.ref = ref;
	proxy.node = &node;
	proxy.shapes.reserve(node.shapes.size());

	bool has_bounds = false;

	for (uint32_t shape_index = 0; shape_index < node.shapes.size(); ++shape_index) {
		const auto &shape = node.shapes[shape_index];
		if (!IsTauSolverShape(shape.type))
			continue;
		TauWorldShape world_shape;
		world_shape.ref = ref;
		world_shape.node = &node;
		world_shape.shape = &shape;
		world_shape.shape_index = shape_index;
		if (shape.type == CT_Sphere) {
			world_shape.position = BuildTauWorldSphereCenter(node, shape);
			world_shape.previous_position = BuildTauWorldSphereCenter(node, shape, true);
			world_shape.radius = BuildTauWorldSphereRadius(node, shape);
			world_shape.bounds = {world_shape.position - Vec3(world_shape.radius), world_shape.position + Vec3(world_shape.radius)};
		} else if (shape.type == CT_Capsule) {
			world_shape.capsule = BuildTauWorldCapsule(node, shape);
			world_shape.previous_capsule = BuildTauWorldCapsule(node, shape, true);
			world_shape.position = (world_shape.capsule.a + world_shape.capsule.b) * 0.5f;
			world_shape.previous_position = (world_shape.previous_capsule.a + world_shape.previous_capsule.b) * 0.5f;
			const Vec3 radius(world_shape.capsule.radius);
			world_shape.bounds = {Min(world_shape.capsule.a, world_shape.capsule.b) - radius,
				Max(world_shape.capsule.a, world_shape.capsule.b) + radius};
		} else {
			world_shape.obb = BuildTauWorldOBB(node, shape);
			world_shape.previous_obb = BuildTauPreviousWorldOBB(node, shape);
			world_shape.position = world_shape.obb.pos;
			world_shape.previous_position = world_shape.previous_obb.pos;
			world_shape.bounds = MinMaxFromOBB(world_shape.obb);
		}

		proxy.shapes.push_back(world_shape);
		proxy.bounds = has_bounds ? Union(proxy.bounds, world_shape.bounds) : world_shape.bounds;
		has_bounds = true;
	}

	return proxy;
}

Color GetTauDebugColor(const TauNode &node) {
	if (node.body_type == RBT_Dynamic)
		return Color::Green;
	if (node.body_type == RBT_Kinematic)
		return Color::Yellow;
	return Color::Orange;
}

Vec3 GetTauObbAxis(const OBB &obb, int axis);

void AppendTauObbWireframe(Vertices &vtx, size_t &vtx_count, const OBB &obb, const Color &color) {
	const Vec3 half_extents = Abs(obb.scl) * 0.5f;
	const Vec3 axis_x = GetTauObbAxis(obb, 0) * half_extents.x;
	const Vec3 axis_y = GetTauObbAxis(obb, 1) * half_extents.y;
	const Vec3 axis_z = GetTauObbAxis(obb, 2) * half_extents.z;

	const Vec3 corners[8] = {
		obb.pos - axis_x - axis_y - axis_z,
		obb.pos + axis_x - axis_y - axis_z,
		obb.pos + axis_x + axis_y - axis_z,
		obb.pos - axis_x + axis_y - axis_z,
		obb.pos - axis_x - axis_y + axis_z,
		obb.pos + axis_x - axis_y + axis_z,
		obb.pos + axis_x + axis_y + axis_z,
		obb.pos - axis_x + axis_y + axis_z,
	};

	static const uint8_t edges[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7},
	};

	for (const auto &edge : edges) {
		vtx.Begin(vtx_count++).SetPos(corners[edge[0]]).SetColor0(color).End();
		vtx.Begin(vtx_count++).SetPos(corners[edge[1]]).SetColor0(color).End();
	}
}

void AppendTauSphereWireframe(Vertices &vtx, size_t &vtx_count, const Vec3 &center, float radius, const Mat3 &rotation, const Color &color) {
	static constexpr int segment_count = 16;
	const Vec3 axis_x = Normalize(GetX(rotation));
	const Vec3 axis_y = Normalize(GetY(rotation));
	const Vec3 axis_z = Normalize(GetZ(rotation));
	const Vec3 axes[3][2] = {{axis_x, axis_y}, {axis_x, axis_z}, {axis_y, axis_z}};
	for (const auto &plane : axes) {
		for (int i = 0; i < segment_count; ++i) {
			const float angle0 = 2.f * Pi * float(i) / float(segment_count);
			const float angle1 = 2.f * Pi * float(i + 1) / float(segment_count);
			vtx.Begin(vtx_count++).SetPos(center + (plane[0] * Cos(angle0) + plane[1] * Sin(angle0)) * radius).SetColor0(color).End();
			vtx.Begin(vtx_count++).SetPos(center + (plane[0] * Cos(angle1) + plane[1] * Sin(angle1)) * radius).SetColor0(color).End();
		}
	}
}

void AppendTauLine(Vertices &vtx, size_t &vtx_count, const Vec3 &a, const Vec3 &b, const Color &color) {
	vtx.Begin(vtx_count++).SetPos(a).SetColor0(color).End();
	vtx.Begin(vtx_count++).SetPos(b).SetColor0(color).End();
}

void AppendTauMeshWireframe(
	Vertices &vtx, size_t &vtx_count, const Mat4 &world, const CollisionGeometry &geometry, const Color &color) {
	for (const auto &triangle : geometry.triangles) {
		const Vec3 a = world * triangle.a;
		const Vec3 b = world * triangle.b;
		const Vec3 c = world * triangle.c;
		AppendTauLine(vtx, vtx_count, a, b, color);
		AppendTauLine(vtx, vtx_count, b, c, color);
		AppendTauLine(vtx, vtx_count, c, a, color);
	}
}

Vec3 TauFrameLocalToWorld(const TauPrimitiveFrame &frame, const Vec3 &point) {
	return frame.center + TauFrameVectorToWorld(frame, point);
}

void AppendTauHorizontalCircle(
	Vertices &vtx, size_t &vtx_count, const TauPrimitiveFrame &frame, float y, float radius, const Color &color) {
	static constexpr int segment_count = 16;
	for (int i = 0; i < segment_count; ++i) {
		const float angle0 = 2.f * Pi * float(i) / float(segment_count);
		const float angle1 = 2.f * Pi * float(i + 1) / float(segment_count);
		AppendTauLine(vtx, vtx_count, TauFrameLocalToWorld(frame, Vec3(Cos(angle0) * radius, y, Sin(angle0) * radius)),
			TauFrameLocalToWorld(frame, Vec3(Cos(angle1) * radius, y, Sin(angle1) * radius)), color);
	}
}

void AppendTauCylinderWireframe(Vertices &vtx, size_t &vtx_count, const TauPrimitiveFrame &frame, float radius, float height, const Color &color) {
	const float half_height = height * 0.5f;
	AppendTauHorizontalCircle(vtx, vtx_count, frame, half_height, radius, color);
	AppendTauHorizontalCircle(vtx, vtx_count, frame, -half_height, radius, color);
	const Vec3 radial[4] = {Vec3(radius, 0.f, 0.f), Vec3(-radius, 0.f, 0.f), Vec3(0.f, 0.f, radius), Vec3(0.f, 0.f, -radius)};
	for (const auto &point : radial)
		AppendTauLine(vtx, vtx_count, TauFrameLocalToWorld(frame, point + Vec3(0.f, -half_height, 0.f)),
			TauFrameLocalToWorld(frame, point + Vec3(0.f, half_height, 0.f)), color);
}

void AppendTauConeWireframe(Vertices &vtx, size_t &vtx_count, const TauPrimitiveFrame &frame, float radius, float height, const Color &color) {
	const float half_height = height * 0.5f;
	AppendTauHorizontalCircle(vtx, vtx_count, frame, -half_height, radius, color);
	const Vec3 apex = TauFrameLocalToWorld(frame, Vec3(0.f, half_height, 0.f));
	const Vec3 base[4] = {Vec3(radius, -half_height, 0.f), Vec3(-radius, -half_height, 0.f), Vec3(0.f, -half_height, radius),
		Vec3(0.f, -half_height, -radius)};
	for (const auto &point : base)
		AppendTauLine(vtx, vtx_count, TauFrameLocalToWorld(frame, point), apex, color);
}

void AppendTauCapsuleHemisphere(Vertices &vtx, size_t &vtx_count, const TauPrimitiveFrame &frame, float center_y, float radius,
	bool top, bool x_plane, const Color &color) {
	static constexpr int segment_count = 8;
	const float angle_offset = top ? 0.f : Pi;
	for (int i = 0; i < segment_count; ++i) {
		const float angle0 = angle_offset + Pi * float(i) / float(segment_count);
		const float angle1 = angle_offset + Pi * float(i + 1) / float(segment_count);
		const float radial0 = Cos(angle0) * radius, radial1 = Cos(angle1) * radius;
		const Vec3 p0 = x_plane ? Vec3(radial0, center_y + Sin(angle0) * radius, 0.f) : Vec3(0.f, center_y + Sin(angle0) * radius, radial0);
		const Vec3 p1 = x_plane ? Vec3(radial1, center_y + Sin(angle1) * radius, 0.f) : Vec3(0.f, center_y + Sin(angle1) * radius, radial1);
		AppendTauLine(vtx, vtx_count, TauFrameLocalToWorld(frame, p0), TauFrameLocalToWorld(frame, p1), color);
	}
}

void AppendTauCapsuleWireframe(Vertices &vtx, size_t &vtx_count, const TauPrimitiveFrame &frame, float radius, float height, const Color &color) {
	const float half_height = height * 0.5f;
	AppendTauCylinderWireframe(vtx, vtx_count, frame, radius, height, color);
	AppendTauCapsuleHemisphere(vtx, vtx_count, frame, half_height, radius, true, true, color);
	AppendTauCapsuleHemisphere(vtx, vtx_count, frame, half_height, radius, true, false, color);
	AppendTauCapsuleHemisphere(vtx, vtx_count, frame, -half_height, radius, false, true, color);
	AppendTauCapsuleHemisphere(vtx, vtx_count, frame, -half_height, radius, false, false, color);
}

void AppendTauManifoldPoint(Vertices &vtx, size_t &vtx_count, const Vec3 &point, const Vec3 &normal, float penetration) {
	static const Vec3 axes[3] = {Vec3::Right, Vec3::Up, Vec3::Front};
	for (const Vec3 &axis : axes) {
		vtx.Begin(vtx_count++).SetPos(point - axis * 0.05f).SetColor0(Color::Purple).End();
		vtx.Begin(vtx_count++).SetPos(point + axis * 0.05f).SetColor0(Color::Purple).End();
	}
	vtx.Begin(vtx_count++).SetPos(point).SetColor0(Color::Blue).End();
	vtx.Begin(vtx_count++).SetPos(point + normal * (0.25f + penetration)).SetColor0(Color::Blue).End();
}

Vec3 GetTauObbAxis(const OBB &obb, int axis) {
	if (axis == 0)
		return Normalize(GetX(obb.rot));
	if (axis == 1)
		return Normalize(GetY(obb.rot));
	return Normalize(GetZ(obb.rot));
}

Vec3 GetTauFaceCenter(const OBB &obb, const Vec3 &direction) {
	const Vec3 half_extents = Abs(obb.scl) * 0.5f;
	const Vec3 axis_x = GetTauObbAxis(obb, 0);
	const Vec3 axis_y = GetTauObbAxis(obb, 1);
	const Vec3 axis_z = GetTauObbAxis(obb, 2);
	const auto face_offset = [](float projection, float extent) {
		if (projection > k_tau_collision_epsilon)
			return extent;
		if (projection < -k_tau_collision_epsilon)
			return -extent;
		return 0.f;
	};

	return obb.pos + axis_x * face_offset(Dot(direction, axis_x), half_extents.x) + axis_y * face_offset(Dot(direction, axis_y), half_extents.y) +
		axis_z * face_offset(Dot(direction, axis_z), half_extents.z);
}

Vec3 WorldToTauObbLocal(const OBB &obb, const Vec3 &point) {
	const Vec3 delta = point - obb.pos;
	return Vec3(Dot(delta, GetTauObbAxis(obb, 0)), Dot(delta, GetTauObbAxis(obb, 1)), Dot(delta, GetTauObbAxis(obb, 2)));
}

Vec3 TauObbLocalToWorld(const OBB &obb, const Vec3 &point) {
	return obb.pos + GetTauObbAxis(obb, 0) * point.x + GetTauObbAxis(obb, 1) * point.y + GetTauObbAxis(obb, 2) * point.z;
}

Vec3 OrientTauAxisFromAToB(Vec3 normal, const Vec3 &delta) {
	const float projection = Dot(normal, delta);
	if (projection < -k_tau_collision_epsilon)
		return -normal;
	if (projection > k_tau_collision_epsilon)
		return normal;

	// Coincident centers still need a repeatable orientation independent of velocity.
	int dominant_axis = 0;
	if (Abs(normal.y) > Abs(normal[dominant_axis]))
		dominant_axis = 1;
	if (Abs(normal.z) > Abs(normal[dominant_axis]))
		dominant_axis = 2;
	return normal[dominant_axis] < 0.f ? -normal : normal;
}

int TauFeaturePriority(TauContactFeatureType type) {
	return type == TauContactFeatureType::EdgeEdge ? 1 : 0;
}

struct TauClipVertex {
	Vec3 point{Vec3::Zero};
	uint16_t provenance{0};
};

size_t ClipTauPolygonAgainstPlane(const std::array<TauClipVertex, 12> &input, size_t input_count, std::array<TauClipVertex, 12> &output,
	const Vec3 &plane_axis, const Vec3 &plane_center, float extent, uint16_t plane_bit) {
	if (input_count == 0)
		return 0;

	size_t output_count = 0;
	TauClipVertex previous = input[input_count - 1];
	float previous_distance = Dot(previous.point - plane_center, plane_axis) - extent;
	bool previous_inside = previous_distance <= k_tau_manifold_clip_tolerance;

	for (size_t i = 0; i < input_count; ++i) {
		const TauClipVertex current = input[i];
		const float current_distance = Dot(current.point - plane_center, plane_axis) - extent;
		const bool current_inside = current_distance <= k_tau_manifold_clip_tolerance;

		if (previous_inside != current_inside) {
			const float denominator = previous_distance - current_distance;
			const float t = Abs(denominator) > k_tau_collision_epsilon ? previous_distance / denominator : 0.f;
			if (output_count < output.size())
				output[output_count++] = {previous.point + (current.point - previous.point) * Clamp(t, 0.f, 1.f),
					uint16_t(previous.provenance | current.provenance | plane_bit)};
		}
		if (current_inside && output_count < output.size())
			output[output_count++] = current;

		previous = current;
		previous_distance = current_distance;
		previous_inside = current_inside;
	}

	return output_count;
}

void SelectTauIncidentFace(const OBB &obb, const Vec3 &reference_normal, uint8_t &axis, int8_t &sign) {
	axis = 0;
	float best_alignment = Abs(Dot(GetTauObbAxis(obb, 0), reference_normal));
	for (uint8_t i = 1; i < 3; ++i) {
		const float alignment = Abs(Dot(GetTauObbAxis(obb, i), reference_normal));
		if (alignment > best_alignment + k_tau_sat_tie_tolerance) {
			axis = i;
			best_alignment = alignment;
		}
	}
	sign = Dot(GetTauObbAxis(obb, axis), reference_normal) > 0.f ? -1 : 1;
}

void BuildTauFaceQuad(const OBB &obb, uint8_t face_axis, int8_t face_sign, std::array<TauClipVertex, 12> &vertices, size_t &vertex_count) {
	const Vec3 half = Abs(obb.scl) * 0.5f;
	const uint8_t axis_u = (face_axis + 1) % 3;
	const uint8_t axis_v = (face_axis + 2) % 3;
	const Vec3 center = obb.pos + GetTauObbAxis(obb, face_axis) * (half[face_axis] * float(face_sign));
	const Vec3 u = GetTauObbAxis(obb, axis_u) * half[axis_u];
	const Vec3 v = GetTauObbAxis(obb, axis_v) * half[axis_v];

	vertices[0] = {center - u - v, 1u << 0};
	vertices[1] = {center + u - v, 1u << 1};
	vertices[2] = {center + u + v, 1u << 2};
	vertices[3] = {center - u + v, 1u << 3};
	vertex_count = 4;
}

uint32_t ClassifyTauSurfacePoint(const Vec3 &point, const Vec3 &half_extents) {
	uint32_t feature = 0;
	for (int axis = 0; axis < 3; ++axis) {
		uint32_t code = 0;
		if (Abs(point[axis] + half_extents[axis]) <= k_tau_manifold_clip_tolerance * 2.f)
			code = 1;
		else if (Abs(point[axis] - half_extents[axis]) <= k_tau_manifold_clip_tolerance * 2.f)
			code = 2;
		feature |= code << (axis * 2);
	}
	return feature;
}

bool TauManifoldPointLess(const TauManifoldPoint &a, const TauManifoldPoint &b) {
	if (a.feature_id != b.feature_id)
		return a.feature_id < b.feature_id;
	if (a.local_point_a.x != b.local_point_a.x)
		return a.local_point_a.x < b.local_point_a.x;
	if (a.local_point_a.y != b.local_point_a.y)
		return a.local_point_a.y < b.local_point_a.y;
	return a.local_point_a.z < b.local_point_a.z;
}

void ReduceTauManifoldPoints(const std::array<TauManifoldPoint, 12> &candidates, size_t candidate_count, uint8_t reference_axis,
	const OBB &reference, bool reference_is_a, std::array<TauManifoldPoint, 4> &points, uint8_t &point_count) {
	if (candidate_count <= points.size()) {
		point_count = uint8_t(candidate_count);
		for (size_t i = 0; i < candidate_count; ++i)
			points[i] = candidates[i];
		std::sort(points.begin(), points.begin() + point_count, TauManifoldPointLess);
		return;
	}

	const uint8_t axis_u = (reference_axis + 1) % 3;
	const uint8_t axis_v = (reference_axis + 2) % 3;
	const Vec3 world_u = GetTauObbAxis(reference, axis_u);
	const Vec3 world_v = GetTauObbAxis(reference, axis_v);
	std::array<size_t, 4> selected{};
	size_t selected_count = 0;
	const auto candidate_world_point = [&](size_t index) {
		return TauObbLocalToWorld(reference, reference_is_a ? candidates[index].local_point_a : candidates[index].local_point_b);
	};

	auto add_extreme = [&](const Vec3 &direction) {
		size_t best = 0;
		float best_projection = Dot(candidate_world_point(0), direction);
		for (size_t i = 1; i < candidate_count; ++i) {
			const float projection = Dot(candidate_world_point(i), direction);
			if (projection > best_projection + k_tau_collision_epsilon ||
				(Abs(projection - best_projection) <= k_tau_collision_epsilon && TauManifoldPointLess(candidates[i], candidates[best]))) {
				best = i;
				best_projection = projection;
			}
		}
		if (std::find(selected.begin(), selected.begin() + selected_count, best) == selected.begin() + selected_count)
			selected[selected_count++] = best;
	};

	add_extreme(world_u);
	add_extreme(-world_u);
	add_extreme(world_v);
	add_extreme(-world_v);

	while (selected_count < points.size()) {
		size_t best = candidate_count;
		float best_min_distance = -1.f;
		for (size_t i = 0; i < candidate_count; ++i) {
			if (std::find(selected.begin(), selected.begin() + selected_count, i) != selected.begin() + selected_count)
				continue;
			const Vec3 candidate_world = candidate_world_point(i);
			float min_distance = std::numeric_limits<float>::max();
			for (size_t j = 0; j < selected_count; ++j) {
				const Vec3 selected_world = candidate_world_point(selected[j]);
				min_distance = std::min(min_distance, Len2(candidate_world - selected_world));
			}
			if (min_distance > best_min_distance + k_tau_collision_epsilon ||
				(Abs(min_distance - best_min_distance) <= k_tau_collision_epsilon &&
					(best == candidate_count || TauManifoldPointLess(candidates[i], candidates[best])))) {
				best = i;
				best_min_distance = min_distance;
			}
		}
		if (best == candidate_count)
			break;
		selected[selected_count++] = best;
	}

	point_count = uint8_t(selected_count);
	for (size_t i = 0; i < selected_count; ++i)
		points[i] = candidates[selected[i]];
	std::sort(points.begin(), points.begin() + point_count, TauManifoldPointLess);
}

bool GenerateTauFaceManifold(const OBB &a, const OBB &b, TauContactManifold &manifold, float sat_penetration) {
	const bool reference_is_a = manifold.feature.type == TauContactFeatureType::FaceA;
	const OBB &reference = reference_is_a ? a : b;
	const OBB &incident = reference_is_a ? b : a;
	const uint8_t reference_axis = reference_is_a ? manifold.feature.axis_a : manifold.feature.axis_b;
	const Vec3 reference_normal = reference_is_a ? manifold.normal : -manifold.normal;
	const Vec3 reference_half = Abs(reference.scl) * 0.5f;
	const int8_t reference_sign = Dot(GetTauObbAxis(reference, reference_axis), reference_normal) >= 0.f ? 1 : -1;
	uint8_t incident_axis = 0;
	int8_t incident_sign = 0;
	SelectTauIncidentFace(incident, reference_normal, incident_axis, incident_sign);

	if (reference_is_a) {
		manifold.feature.sign_a = reference_sign;
		manifold.feature.axis_b = incident_axis;
		manifold.feature.sign_b = incident_sign;
	} else {
		manifold.feature.sign_b = reference_sign;
		manifold.feature.axis_a = incident_axis;
		manifold.feature.sign_a = incident_sign;
	}

	std::array<TauClipVertex, 12> polygon_a{}, polygon_b{};
	size_t polygon_count = 0;
	BuildTauFaceQuad(incident, incident_axis, incident_sign, polygon_a, polygon_count);

	const Vec3 reference_center = reference.pos + GetTauObbAxis(reference, reference_axis) * (reference_half[reference_axis] * float(reference_sign));
	const uint8_t side_axis_u = (reference_axis + 1) % 3;
	const uint8_t side_axis_v = (reference_axis + 2) % 3;
	const Vec3 side_u = GetTauObbAxis(reference, side_axis_u);
	const Vec3 side_v = GetTauObbAxis(reference, side_axis_v);

	polygon_count = ClipTauPolygonAgainstPlane(polygon_a, polygon_count, polygon_b, side_u, reference_center, reference_half[side_axis_u], 1u << 4);
	polygon_count = ClipTauPolygonAgainstPlane(polygon_b, polygon_count, polygon_a, -side_u, reference_center, reference_half[side_axis_u], 1u << 5);
	polygon_count = ClipTauPolygonAgainstPlane(polygon_a, polygon_count, polygon_b, side_v, reference_center, reference_half[side_axis_v], 1u << 6);
	polygon_count = ClipTauPolygonAgainstPlane(polygon_b, polygon_count, polygon_a, -side_v, reference_center, reference_half[side_axis_v], 1u << 7);

	std::array<TauManifoldPoint, 12> candidates{};
	size_t candidate_count = 0;
	const Vec3 half_a = Abs(a.scl) * 0.5f;
	const Vec3 half_b = Abs(b.scl) * 0.5f;
	for (size_t i = 0; i < polygon_count; ++i) {
		const float separation = Dot(polygon_a[i].point - reference_center, reference_normal);
		if (separation > k_tau_manifold_clip_tolerance)
			continue;

		const Vec3 reference_point = polygon_a[i].point - reference_normal * separation;
		const Vec3 point_a = reference_is_a ? reference_point : polygon_a[i].point;
		const Vec3 point_b = reference_is_a ? polygon_a[i].point : reference_point;
		TauManifoldPoint point;
		point.local_point_a = WorldToTauObbLocal(a, point_a);
		point.local_point_b = WorldToTauObbLocal(b, point_b);
		point.penetration = std::max(-separation, 0.f);
		point.feature_id = uint32_t(polygon_a[i].provenance) | (ClassifyTauSurfacePoint(point.local_point_a, half_a) << 8) |
			(ClassifyTauSurfacePoint(point.local_point_b, half_b) << 14);

		bool duplicate = false;
		for (size_t j = 0; j < candidate_count; ++j) {
			if (Len2(point.local_point_a - candidates[j].local_point_a) <= k_tau_collision_epsilon * k_tau_collision_epsilon &&
				Len2(point.local_point_b - candidates[j].local_point_b) <= k_tau_collision_epsilon * k_tau_collision_epsilon) {
				duplicate = true;
				break;
			}
		}
		if (!duplicate)
			candidates[candidate_count++] = point;
	}

	if (candidate_count == 0) {
		TauManifoldPoint fallback;
		fallback.local_point_a = WorldToTauObbLocal(a, GetTauFaceCenter(a, manifold.normal));
		fallback.local_point_b = WorldToTauObbLocal(b, GetTauFaceCenter(b, -manifold.normal));
		fallback.penetration = sat_penetration;
		fallback.feature_id = 0xffffffffu;
		manifold.points[0] = fallback;
		manifold.point_count = 1;
		return true;
	}

	ReduceTauManifoldPoints(candidates, candidate_count, reference_axis, reference, reference_is_a, manifold.points, manifold.point_count);
	return manifold.point_count > 0;
}

void BuildTauSupportEdge(const OBB &obb, uint8_t edge_axis, const Vec3 &support_direction, Vec3 &start, Vec3 &end, uint8_t &sign_mask) {
	const Vec3 half = Abs(obb.scl) * 0.5f;
	Vec3 center = obb.pos;
	sign_mask = 0;
	for (uint8_t axis = 0; axis < 3; ++axis) {
		if (axis == edge_axis)
			continue;
		const bool positive = Dot(GetTauObbAxis(obb, axis), support_direction) >= 0.f;
		if (positive)
			sign_mask |= uint8_t(1u << axis);
		center += GetTauObbAxis(obb, axis) * (positive ? half[axis] : -half[axis]);
	}
	const Vec3 edge = GetTauObbAxis(obb, edge_axis) * half[edge_axis];
	start = center - edge;
	end = center + edge;
}

void ClosestTauSegmentPoints(const Vec3 &p0, const Vec3 &p1, const Vec3 &q0, const Vec3 &q1, Vec3 &point_p, Vec3 &point_q) {
	const Vec3 d1 = p1 - p0;
	const Vec3 d2 = q1 - q0;
	const Vec3 r = p0 - q0;
	const float a = Dot(d1, d1);
	const float e = Dot(d2, d2);
	const float f = Dot(d2, r);
	float s = 0.f, t = 0.f;

	if (a <= k_tau_collision_epsilon && e <= k_tau_collision_epsilon) {
		point_p = p0;
		point_q = q0;
		return;
	}
	if (a <= k_tau_collision_epsilon) {
		t = Clamp(f / e, 0.f, 1.f);
	} else {
		const float c = Dot(d1, r);
		if (e <= k_tau_collision_epsilon) {
			s = Clamp(-c / a, 0.f, 1.f);
		} else {
			const float b = Dot(d1, d2);
			const float denominator = a * e - b * b;
			if (Abs(denominator) > k_tau_collision_epsilon)
				s = Clamp((b * f - c * e) / denominator, 0.f, 1.f);
			t = (b * s + f) / e;
			if (t < 0.f) {
				t = 0.f;
				s = Clamp(-c / a, 0.f, 1.f);
			} else if (t > 1.f) {
				t = 1.f;
				s = Clamp((b - c) / a, 0.f, 1.f);
			}
		}
	}

	point_p = p0 + d1 * s;
	point_q = q0 + d2 * t;
}

Vec3 ClosestTauPointOnSegment(const Vec3 &a, const Vec3 &b, const Vec3 &point) {
	const Vec3 segment = b - a;
	const float length_squared = Len2(segment);
	if (length_squared <= k_tau_collision_epsilon)
		return a;
	return a + segment * Clamp(Dot(point - a, segment) / length_squared, 0.f, 1.f);
}

Vec3 ClosestTauPointOnObb(const OBB &obb, const Vec3 &point) {
	const Vec3 half = Abs(obb.scl) * 0.5f;
	Vec3 local = WorldToTauObbLocal(obb, point);
	for (int axis = 0; axis < 3; ++axis)
		local[axis] = Clamp(local[axis], -half[axis], half[axis]);
	return TauObbLocalToWorld(obb, local);
}

Vec3 TauPerpendicularDirection(const Vec3 &direction) {
	const float length_squared = Len2(direction);
	if (length_squared <= k_tau_collision_epsilon)
		return Vec3::Up;
	const Vec3 axis = direction / Sqrt(length_squared);
	const Vec3 reference = Abs(axis.x) < 0.577f ? Vec3::Right : (Abs(axis.y) < 0.577f ? Vec3::Up : Vec3::Front);
	return Normalize(Cross(axis, reference));
}

Vec3 TauCapsuleFallbackNormal(const TauCapsuleGeometry &capsule, const Vec3 &target) {
	const Vec3 axis = capsule.b - capsule.a;
	const Vec3 center = (capsule.a + capsule.b) * 0.5f;
	Vec3 radial = target - center;
	const float axis_length_squared = Len2(axis);
	if (axis_length_squared > k_tau_collision_epsilon)
		radial -= axis * (Dot(radial, axis) / axis_length_squared);
	if (Len2(radial) > k_tau_collision_epsilon)
		return Normalize(radial);
	return TauPerpendicularDirection(axis);
}

void SetTauRoundedContactPoint(
	const Vec3 &axis_point_a, float radius_a, const Vec3 &axis_point_b, float radius_b, const Vec3 &normal, Vec3 &point) {
	const Vec3 surface_a = axis_point_a + normal * radius_a;
	const Vec3 surface_b = axis_point_b - normal * radius_b;
	point = (surface_a + surface_b) * 0.5f;
}

bool ComputeTauCapsuleSphereContactImpl(const TauCapsuleGeometry &capsule, const Vec3 &sphere_center, float sphere_radius, Vec3 &normal,
	float &penetration, Vec3 &point) {
	const float capsule_radius = std::max(0.f, capsule.radius);
	sphere_radius = std::max(0.f, sphere_radius);
	const Vec3 capsule_point = ClosestTauPointOnSegment(capsule.a, capsule.b, sphere_center);
	const Vec3 delta = sphere_center - capsule_point;
	const float distance = Len(delta);
	const float radius_sum = capsule_radius + sphere_radius;
	if (distance > radius_sum)
		return false;

	normal = distance > k_tau_collision_epsilon ? delta / distance : TauCapsuleFallbackNormal(capsule, sphere_center);
	penetration = radius_sum - distance;
	SetTauRoundedContactPoint(capsule_point, capsule_radius, sphere_center, sphere_radius, normal, point);
	return true;
}

bool ComputeTauCapsuleCapsuleContactImpl(
	const TauCapsuleGeometry &a, const TauCapsuleGeometry &b, Vec3 &normal, float &penetration, Vec3 &point) {
	Vec3 point_a, point_b;
	ClosestTauSegmentPoints(a.a, a.b, b.a, b.b, point_a, point_b);
	const Vec3 delta = point_b - point_a;
	const float distance = Len(delta);
	const float radius_a = std::max(0.f, a.radius), radius_b = std::max(0.f, b.radius);
	const float radius_sum = radius_a + radius_b;
	if (distance > radius_sum)
		return false;

	if (distance > k_tau_collision_epsilon) {
		normal = delta / distance;
	} else {
		const Vec3 center_delta = (b.a + b.b - a.a - a.b) * 0.5f;
		const Vec3 crossed_axes = Cross(a.b - a.a, b.b - b.a);
		if (Len2(crossed_axes) > k_tau_collision_epsilon) {
			normal = Normalize(crossed_axes);
			if (Dot(normal, center_delta) < 0.f)
				normal = -normal;
		} else {
			normal = TauCapsuleFallbackNormal(a, (b.a + b.b) * 0.5f);
			if (Dot(normal, center_delta) < 0.f)
				normal = -normal;
		}
	}

	penetration = radius_sum - distance;
	SetTauRoundedContactPoint(point_a, radius_a, point_b, radius_b, normal, point);
	return true;
}

bool TauSegmentIntersectsObb(const TauCapsuleGeometry &capsule, const OBB &obb) {
	const Vec3 a = WorldToTauObbLocal(obb, capsule.a);
	const Vec3 b = WorldToTauObbLocal(obb, capsule.b);
	const Vec3 direction = b - a;
	const Vec3 half = Abs(obb.scl) * 0.5f;
	float t_enter = 0.f, t_exit = 1.f;

	for (int axis = 0; axis < 3; ++axis) {
		if (Abs(direction[axis]) <= k_tau_collision_epsilon) {
			if (a[axis] < -half[axis] || a[axis] > half[axis])
				return false;
			continue;
		}
		float t0 = (-half[axis] - a[axis]) / direction[axis];
		float t1 = (half[axis] - a[axis]) / direction[axis];
		if (t0 > t1)
			std::swap(t0, t1);
		t_enter = std::max(t_enter, t0);
		t_exit = std::min(t_exit, t1);
		if (t_enter > t_exit)
			return false;
	}
	return true;
}

void ConsiderTauClosestPair(const Vec3 &capsule_point, const Vec3 &obb_point, float &best_distance_squared, Vec3 &best_capsule_point,
	Vec3 &best_obb_point) {
	const float distance_squared = Len2(obb_point - capsule_point);
	if (distance_squared < best_distance_squared) {
		best_distance_squared = distance_squared;
		best_capsule_point = capsule_point;
		best_obb_point = obb_point;
	}
}

bool ComputeTauCapsuleObbContactImpl(
	const TauCapsuleGeometry &capsule, const OBB &obb, Vec3 &normal, float &penetration, Vec3 &point) {
	const float radius = std::max(0.f, capsule.radius);
	const Vec3 half = Abs(obb.scl) * 0.5f;

	if (TauSegmentIntersectsObb(capsule, obb)) {
		const Vec3 local_a = WorldToTauObbLocal(obb, capsule.a);
		const Vec3 local_b = WorldToTauObbLocal(obb, capsule.b);
		float best_depth = std::numeric_limits<float>::max();
		int best_axis = 0;
		bool move_positive = true;
		for (int axis = 0; axis < 3; ++axis) {
			const float positive_depth = half[axis] + radius - std::min(local_a[axis], local_b[axis]);
			if (positive_depth < best_depth) {
				best_depth = positive_depth;
				best_axis = axis;
				move_positive = true;
			}
			const float negative_depth = half[axis] + radius + std::max(local_a[axis], local_b[axis]);
			if (negative_depth < best_depth) {
				best_depth = negative_depth;
				best_axis = axis;
				move_positive = false;
			}
		}

		const bool use_a = move_positive ? local_a[best_axis] <= local_b[best_axis] : local_a[best_axis] >= local_b[best_axis];
		Vec3 local_point = use_a ? local_a : local_b;
		for (int axis = 0; axis < 3; ++axis)
			local_point[axis] = Clamp(local_point[axis], -half[axis], half[axis]);
		local_point[best_axis] = move_positive ? half[best_axis] : -half[best_axis];
		normal = GetTauObbAxis(obb, best_axis) * (move_positive ? -1.f : 1.f);
		penetration = std::max(0.f, best_depth);
		point = TauObbLocalToWorld(obb, local_point);
		return true;
	}

	float best_distance_squared = std::numeric_limits<float>::max();
	Vec3 capsule_point, obb_point;
	ConsiderTauClosestPair(capsule.a, ClosestTauPointOnObb(obb, capsule.a), best_distance_squared, capsule_point, obb_point);
	ConsiderTauClosestPair(capsule.b, ClosestTauPointOnObb(obb, capsule.b), best_distance_squared, capsule_point, obb_point);

	std::array<Vec3, 8> vertices;
	for (uint8_t vertex = 0; vertex < vertices.size(); ++vertex) {
		const Vec3 local((vertex & 1) ? half.x : -half.x, (vertex & 2) ? half.y : -half.y, (vertex & 4) ? half.z : -half.z);
		vertices[vertex] = TauObbLocalToWorld(obb, local);
		ConsiderTauClosestPair(ClosestTauPointOnSegment(capsule.a, capsule.b, vertices[vertex]), vertices[vertex], best_distance_squared,
			capsule_point, obb_point);
	}

	static constexpr uint8_t edges[12][2] = {
		{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
	for (const auto &edge : edges) {
		Vec3 point_on_capsule, point_on_edge;
		ClosestTauSegmentPoints(capsule.a, capsule.b, vertices[edge[0]], vertices[edge[1]], point_on_capsule, point_on_edge);
		ConsiderTauClosestPair(point_on_capsule, point_on_edge, best_distance_squared, capsule_point, obb_point);
	}

	const float distance = Sqrt(best_distance_squared);
	if (distance > radius)
		return false;
	normal = distance > k_tau_collision_epsilon ? (obb_point - capsule_point) / distance : TauCapsuleFallbackNormal(capsule, obb.pos);
	penetration = radius - distance;
	SetTauRoundedContactPoint(capsule_point, radius, obb_point, 0.f, normal, point);
	return true;
}

bool ComputeTauObbContactManifold(const OBB &a, const OBB &b, TauContactManifold &manifold) {
	const Vec3 half_a = Abs(a.scl) * 0.5f;
	const Vec3 half_b = Abs(b.scl) * 0.5f;
	const Vec3 axis_a[3] = {GetTauObbAxis(a, 0), GetTauObbAxis(a, 1), GetTauObbAxis(a, 2)};
	const Vec3 axis_b[3] = {GetTauObbAxis(b, 0), GetTauObbAxis(b, 1), GetTauObbAxis(b, 2)};
	const Vec3 delta = b.pos - a.pos;
	const float t[3] = {Dot(delta, axis_a[0]), Dot(delta, axis_a[1]), Dot(delta, axis_a[2])};

	float r[3][3];
	float abs_r[3][3];

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			r[i][j] = Dot(axis_a[i], axis_b[j]);
			abs_r[i][j] = Abs(r[i][j]) + k_tau_collision_epsilon;
		}
	}

	float best_penetration = std::numeric_limits<float>::max();
	Vec3 best_normal = Vec3::Up;
	TauContactFeature best_feature;

	auto register_axis = [&](const Vec3 &axis, float axis_length, float separation, float radius_a, float radius_b, const TauContactFeature &feature) {
		if (axis_length <= k_tau_collision_epsilon)
			return true;

		const float total_radius = radius_a + radius_b;
		if (separation > total_radius)
			return false;

		const Vec3 candidate_normal = OrientTauAxisFromAToB(axis / axis_length, delta);
		const float axis_penetration = (total_radius - separation) / axis_length;
		const bool is_better = axis_penetration < best_penetration - k_tau_sat_tie_tolerance;
		const bool wins_tie = Abs(axis_penetration - best_penetration) <= k_tau_sat_tie_tolerance &&
			TauFeaturePriority(feature.type) < TauFeaturePriority(best_feature.type);
		if (is_better || wins_tie) {
			best_penetration = axis_penetration;
			best_normal = candidate_normal;
			best_feature = feature;
		}

		return true;
	};

	for (int i = 0; i < 3; ++i) {
		const float radius_a = half_a[i];
		const float radius_b = half_b.x * abs_r[i][0] + half_b.y * abs_r[i][1] + half_b.z * abs_r[i][2];
		TauContactFeature feature;
		feature.type = TauContactFeatureType::FaceA;
		feature.axis_a = uint8_t(i);
		if (!register_axis(axis_a[i], 1.f, Abs(t[i]), radius_a, radius_b, feature))
			return false;
	}

	for (int j = 0; j < 3; ++j) {
		const float radius_a = half_a.x * abs_r[0][j] + half_a.y * abs_r[1][j] + half_a.z * abs_r[2][j];
		const float radius_b = half_b[j];
		TauContactFeature feature;
		feature.type = TauContactFeatureType::FaceB;
		feature.axis_b = uint8_t(j);
		if (!register_axis(axis_b[j], 1.f, Abs(Dot(delta, axis_b[j])), radius_a, radius_b, feature))
			return false;
	}

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			const Vec3 axis = Cross(axis_a[i], axis_b[j]);
			const float axis_length = Len(axis);
			if (axis_length <= k_tau_collision_epsilon)
				continue;

			const float separation = Abs(t[(i + 2) % 3] * r[(i + 1) % 3][j] - t[(i + 1) % 3] * r[(i + 2) % 3][j]);
			const float radius_a = half_a[(i + 1) % 3] * abs_r[(i + 2) % 3][j] + half_a[(i + 2) % 3] * abs_r[(i + 1) % 3][j];
			const float radius_b = half_b[(j + 1) % 3] * abs_r[i][(j + 2) % 3] + half_b[(j + 2) % 3] * abs_r[i][(j + 1) % 3];
			TauContactFeature feature;
			feature.type = TauContactFeatureType::EdgeEdge;
			feature.axis_a = uint8_t(i);
			feature.axis_b = uint8_t(j);
			if (!register_axis(axis, axis_length, separation, radius_a, radius_b, feature))
				return false;
		}
	}

	manifold = {};
	manifold.normal = best_normal;
	manifold.feature = best_feature;

	if (best_feature.type != TauContactFeatureType::EdgeEdge)
		return GenerateTauFaceManifold(a, b, manifold, best_penetration);

	Vec3 edge_a_start, edge_a_end, edge_b_start, edge_b_end;
	BuildTauSupportEdge(a, best_feature.axis_a, manifold.normal, edge_a_start, edge_a_end, manifold.feature.edge_signs_a);
	BuildTauSupportEdge(b, best_feature.axis_b, -manifold.normal, edge_b_start, edge_b_end, manifold.feature.edge_signs_b);
	Vec3 point_a, point_b;
	ClosestTauSegmentPoints(edge_a_start, edge_a_end, edge_b_start, edge_b_end, point_a, point_b);
	manifold.feature.sign_a = Dot(Cross(axis_a[best_feature.axis_a], axis_b[best_feature.axis_b]), manifold.normal) >= 0.f ? 1 : -1;
	manifold.feature.sign_b = manifold.feature.sign_a;
	manifold.points[0].local_point_a = WorldToTauObbLocal(a, point_a);
	manifold.points[0].local_point_b = WorldToTauObbLocal(b, point_b);
	manifold.points[0].penetration = best_penetration;
	manifold.points[0].feature_id = uint32_t(best_feature.axis_a) | (uint32_t(best_feature.axis_b) << 2) |
		(uint32_t(manifold.feature.edge_signs_a) << 4) | (uint32_t(manifold.feature.edge_signs_b) << 7);
	manifold.point_count = 1;
	return true;
}

bool ComputeTauSphereObbContact(const Vec3 &sphere_center, float sphere_radius, const OBB &obb, Vec3 &normal, float &penetration, Vec3 &point) {
	const Vec3 half_extents = Abs(obb.scl) * 0.5f;
	const Vec3 axes[3] = {GetTauObbAxis(obb, 0), GetTauObbAxis(obb, 1), GetTauObbAxis(obb, 2)};
	const Vec3 offset = sphere_center - obb.pos;
	float local[3] = {Dot(offset, axes[0]), Dot(offset, axes[1]), Dot(offset, axes[2])};
	const float extent[3] = {half_extents.x, half_extents.y, half_extents.z};
	Vec3 closest = obb.pos;
	for (int i = 0; i < 3; ++i)
		closest += axes[i] * Clamp(local[i], -extent[i], extent[i]);

	const Vec3 delta = sphere_center - closest;
	const float distance = Len(delta);
	if (distance > sphere_radius)
		return false;

	if (distance > k_tau_collision_epsilon) {
		normal = -delta / distance; // sphere toward OBB
		penetration = sphere_radius - distance;
		point = closest;
		return true;
	}

	int nearest_axis = 0;
	float nearest_distance = extent[0] - Abs(local[0]);
	for (int i = 1; i < 3; ++i) {
		const float distance_to_face = extent[i] - Abs(local[i]);
		if (distance_to_face < nearest_distance) {
			nearest_axis = i;
			nearest_distance = distance_to_face;
		}
	}
	const float sign = local[nearest_axis] >= 0.f ? 1.f : -1.f;
	point = sphere_center + axes[nearest_axis] * (nearest_distance * sign);
	normal = -axes[nearest_axis] * sign;
	penetration = sphere_radius + nearest_distance;
	return true;
}

bool ComputeTauSphereContact(const TauWorldShape &a, const TauWorldShape &b, Vec3 &normal, float &penetration, Vec3 &point) {
	const Vec3 delta = b.position - a.position;
	const float distance = Len(delta);
	const float radius_sum = a.radius + b.radius;
	if (distance > radius_sum)
		return false;

	normal = distance > k_tau_collision_epsilon ? delta / distance : Vec3::Up;
	penetration = radius_sum - distance;
	point = a.position + normal * a.radius;
	return true;
}

bool ComputeTauContact(const TauWorldShape &a, const TauWorldShape &b, Vec3 &normal, float &penetration, Vec3 &point) {
	if (a.shape->type == CT_Cube && b.shape->type == CT_Cube) {
		TauContactManifold manifold;
		if (!ComputeTauObbContactManifold(a.obb, b.obb, manifold))
			return false;
		normal = manifold.normal;
		penetration = manifold.points[0].penetration;
		point = (TauObbLocalToWorld(a.obb, manifold.points[0].local_point_a) + TauObbLocalToWorld(b.obb, manifold.points[0].local_point_b)) * 0.5f;
		return true;
	}
	if (a.shape->type == CT_Sphere && b.shape->type == CT_Cube)
		return ComputeTauSphereObbContact(a.position, a.radius, b.obb, normal, penetration, point);
	if (a.shape->type == CT_Cube && b.shape->type == CT_Sphere) {
		const bool contact = ComputeTauSphereObbContact(b.position, b.radius, a.obb, normal, penetration, point);
		if (contact)
			normal = -normal;
		return contact;
	}
	if (a.shape->type == CT_Capsule && b.shape->type == CT_Sphere)
		return ComputeTauCapsuleSphereContactImpl(a.capsule, b.position, b.radius, normal, penetration, point);
	if (a.shape->type == CT_Sphere && b.shape->type == CT_Capsule) {
		const bool contact = ComputeTauCapsuleSphereContactImpl(b.capsule, a.position, a.radius, normal, penetration, point);
		if (contact)
			normal = -normal;
		return contact;
	}
	if (a.shape->type == CT_Capsule && b.shape->type == CT_Cube)
		return ComputeTauCapsuleObbContactImpl(a.capsule, b.obb, normal, penetration, point);
	if (a.shape->type == CT_Cube && b.shape->type == CT_Capsule) {
		const bool contact = ComputeTauCapsuleObbContactImpl(b.capsule, a.obb, normal, penetration, point);
		if (contact)
			normal = -normal;
		return contact;
	}
	if (a.shape->type == CT_Capsule && b.shape->type == CT_Capsule)
		return ComputeTauCapsuleCapsuleContactImpl(a.capsule, b.capsule, normal, penetration, point);
	if (a.shape->type == CT_Sphere && b.shape->type == CT_Sphere)
		return ComputeTauSphereContact(a, b, normal, penetration, point);
	return false;
}

float GetTauInverseMass(const TauNode &node) {
	return IsDynamicTauNode(node) ? node.inverse_mass : 0.f;
}

Vec3 GetTauPointVelocity(const TauNode &node, const Vec3 &world_pos) {
	return node.linear_velocity + Cross(node.angular_velocity, world_pos - node.position);
}

void ApplyTauImpulse(TauNode &node, const Vec3 &impulse, const Vec3 &arm) {
	if (!IsDynamicTauNode(node))
		return;

	node.linear_velocity += (impulse * node.inverse_mass) * node.linear_factor;
	node.angular_velocity += (ComputeTauInverseInertiaWorld(node) * Cross(arm, impulse)) * node.angular_factor;
}

void ApplyTauPositionImpulse(TauNode &node, const Vec3 &impulse, const Vec3 &arm) {
	if (!IsDynamicTauNode(node))
		return;
	node.position += (impulse * node.inverse_mass) * node.linear_factor;
	Vec3 angular_step = (ComputeTauInverseInertiaWorld(node) * Cross(arm, impulse)) * node.angular_factor;
	const float angular_length = Len(angular_step);
	if (angular_length > 0.2f)
		angular_step *= 0.2f / angular_length;
	IntegrateTauOrientation(node, angular_step);
}

void ApplyTauAngularImpulse(TauNode &node, const Vec3 &impulse) {
	if (IsDynamicTauNode(node))
		node.angular_velocity += (ComputeTauInverseInertiaWorld(node) * impulse) * node.angular_factor;
}

float ComputeTauAngularMassTerm(const TauNode &node, const Vec3 &arm, const Vec3 &axis) {
	if (!IsDynamicTauNode(node))
		return 0.f;
	return Dot(axis, Cross(ComputeTauInverseInertiaWorld(node) * Cross(arm, axis), arm));
}

float ComputeTauConstraintMass(const TauContactConstraint &contact, const Vec3 &arm_a, const Vec3 &arm_b, const Vec3 &axis) {
	const float inverse_mass = GetTauInverseMass(*contact.node_a) + GetTauInverseMass(*contact.node_b);
	return inverse_mass + ComputeTauAngularMassTerm(*contact.node_a, arm_a, axis) + ComputeTauAngularMassTerm(*contact.node_b, arm_b, axis);
}

Vec3 GetTauConstraintAnchorA(const TauContactConstraint &contact) {
	if (!contact.persistent || contact.shape_a >= contact.node_a->shapes.size())
		return contact.point;
	return TauObbLocalToWorld(BuildTauWorldOBB(*contact.node_a, contact.node_a->shapes[contact.shape_a]), contact.local_point_a);
}

Vec3 GetTauConstraintAnchorB(const TauContactConstraint &contact) {
	if (!contact.persistent || contact.shape_b >= contact.node_b->shapes.size())
		return contact.point;
	return TauObbLocalToWorld(BuildTauWorldOBB(*contact.node_b, contact.node_b->shapes[contact.shape_b]), contact.local_point_b);
}

void RefreshTauConstraintPoint(TauContactConstraint &contact) {
	if (!contact.persistent)
		return;
	const Vec3 anchor_a = GetTauConstraintAnchorA(contact);
	const Vec3 anchor_b = GetTauConstraintAnchorB(contact);
	contact.point = (anchor_a + anchor_b) * 0.5f;
	contact.penetration = std::max(-Dot(anchor_b - anchor_a, contact.normal), 0.f);
}

void SolveTauPositionConstraints(std::vector<TauContactConstraint> &contacts, TauContactDiagnostics &diagnostics) {
	diagnostics.position_constraint_evaluations += contacts.size() * k_tau_position_iterations;
	for (int iteration = 0; iteration < k_tau_position_iterations; ++iteration) {
		for (auto &contact : contacts) {
			RefreshTauConstraintPoint(contact);
			const float depth = std::max(contact.penetration - k_tau_position_slop, 0.f);
			if (depth <= 0.f)
				continue;

			const Vec3 arm_a = contact.point - contact.node_a->position;
			const Vec3 arm_b = contact.point - contact.node_b->position;
			const float constraint_mass = ComputeTauConstraintMass(contact, arm_a, arm_b, contact.normal);
			if (constraint_mass <= k_tau_collision_epsilon)
				continue;

			const float share = 1.f / float(std::max<uint8_t>(contact.manifold_point_count, 1));
			const float correction_magnitude = k_tau_position_correction * depth * share / constraint_mass;
			const Vec3 correction_impulse = contact.normal * correction_magnitude;
			ApplyTauPositionImpulse(*contact.node_a, -correction_impulse, arm_a);
			ApplyTauPositionImpulse(*contact.node_b, correction_impulse, arm_b);
			if (!contact.persistent)
				contact.penetration = std::max(0.f, contact.penetration - depth * k_tau_position_correction * share);
		}
	}

	for (auto &contact : contacts) {
		RefreshTauConstraintPoint(contact);
		diagnostics.max_post_solve_penetration = std::max(diagnostics.max_post_solve_penetration, contact.penetration);
	}
}

void WarmStartTauVelocityConstraints(std::vector<TauContactConstraint> &contacts) {
	for (auto &contact : contacts) {
		RefreshTauConstraintPoint(contact);
		const Vec3 relative_velocity = GetTauPointVelocity(*contact.node_b, contact.point) - GetTauPointVelocity(*contact.node_a, contact.point);
		const float normal_speed = Dot(relative_velocity, contact.normal);
		contact.restitution_velocity = normal_speed < -k_tau_restitution_threshold ? -contact.restitution * normal_speed : 0.f;
		contact.accumulated_tangent_impulse -= contact.normal * Dot(contact.accumulated_tangent_impulse, contact.normal);
		const float max_friction_impulse = std::max(contact.friction, 0.f) * contact.accumulated_normal_impulse;
		const float tangent_length = Len(contact.accumulated_tangent_impulse);
		if (tangent_length > max_friction_impulse && tangent_length > k_tau_collision_epsilon)
			contact.accumulated_tangent_impulse *= max_friction_impulse / tangent_length;

		const Vec3 impulse = contact.normal * contact.accumulated_normal_impulse + contact.accumulated_tangent_impulse;
		if (Len2(impulse) <= k_tau_collision_epsilon * k_tau_collision_epsilon)
			continue;
		const Vec3 arm_a = contact.point - contact.node_a->position;
		const Vec3 arm_b = contact.point - contact.node_b->position;
		ApplyTauImpulse(*contact.node_a, -impulse, arm_a);
		ApplyTauImpulse(*contact.node_b, impulse, arm_b);
	}
}

void SolveTauVelocityConstraints(std::vector<TauContactConstraint> &contacts, float dt_sec, TauContactDiagnostics &diagnostics) {
	if (dt_sec <= 0.f)
		return;

	diagnostics.velocity_constraint_evaluations += contacts.size() * k_tau_velocity_iterations;
	WarmStartTauVelocityConstraints(contacts);

	for (int iteration = 0; iteration < k_tau_velocity_iterations; ++iteration) {
		for (auto &contact : contacts) {
			RefreshTauConstraintPoint(contact);
			const Vec3 arm_a = contact.point - contact.node_a->position;
			const Vec3 arm_b = contact.point - contact.node_b->position;
			const Vec3 relative_velocity = GetTauPointVelocity(*contact.node_b, contact.point) - GetTauPointVelocity(*contact.node_a, contact.point);
			const float normal_speed = Dot(relative_velocity, contact.normal);

			const float constraint_mass = ComputeTauConstraintMass(contact, arm_a, arm_b, contact.normal);
			if (constraint_mass <= k_tau_collision_epsilon)
				continue;

			const float bias = k_tau_baumgarte * std::max(contact.penetration - k_tau_position_slop, 0.f) / dt_sec;
			const float normal_impulse_delta = (-normal_speed + contact.restitution_velocity + bias) / constraint_mass;
			const float old_normal_impulse = contact.accumulated_normal_impulse;
			contact.accumulated_normal_impulse = std::max(old_normal_impulse + normal_impulse_delta, 0.f);
			const float applied_normal_impulse = contact.accumulated_normal_impulse - old_normal_impulse;
			const Vec3 normal_impulse = contact.normal * applied_normal_impulse;

			ApplyTauImpulse(*contact.node_a, -normal_impulse, arm_a);
			ApplyTauImpulse(*contact.node_b, normal_impulse, arm_b);

			const Vec3 post_normal_velocity = GetTauPointVelocity(*contact.node_b, contact.point) - GetTauPointVelocity(*contact.node_a, contact.point);
			const Vec3 tangent_velocity = post_normal_velocity - contact.normal * Dot(post_normal_velocity, contact.normal);
			const float tangent_length = Len(tangent_velocity);
			if (tangent_length <= k_tau_collision_epsilon)
				continue;

			const Vec3 tangent = tangent_velocity / tangent_length;
			const float tangent_mass = ComputeTauConstraintMass(contact, arm_a, arm_b, tangent);
			if (tangent_mass <= k_tau_collision_epsilon)
				continue;

			const Vec3 tangent_impulse_delta = tangent * (-Dot(post_normal_velocity, tangent) / tangent_mass);
			const Vec3 old_tangent_impulse = contact.accumulated_tangent_impulse;
			Vec3 new_tangent_impulse = old_tangent_impulse + tangent_impulse_delta;
			new_tangent_impulse -= contact.normal * Dot(new_tangent_impulse, contact.normal);
			const float max_friction_impulse = std::max(contact.friction, 0.f) * contact.accumulated_normal_impulse;
			const float accumulated_tangent_length = Len(new_tangent_impulse);
			if (accumulated_tangent_length > max_friction_impulse && accumulated_tangent_length > k_tau_collision_epsilon) {
				new_tangent_impulse *= max_friction_impulse / accumulated_tangent_length;
				++diagnostics.friction_clamps;
			}
			contact.accumulated_tangent_impulse = new_tangent_impulse;
			const Vec3 applied_tangent_impulse = new_tangent_impulse - old_tangent_impulse;
			ApplyTauImpulse(*contact.node_a, -applied_tangent_impulse, arm_a);
			ApplyTauImpulse(*contact.node_b, applied_tangent_impulse, arm_b);
		}
	}

	for (const auto &contact : contacts) {
		diagnostics.normal_impulse_total += contact.accumulated_normal_impulse;
		diagnostics.tangent_impulse_total += Len(contact.accumulated_tangent_impulse);
	}
}

void SolveTauRollingFriction(const std::vector<TauContactConstraint> &contacts, float dt_sec, TauContactDiagnostics &diagnostics) {
	diagnostics.rolling_contact_evaluations += contacts.size();
	for (const auto &contact : contacts) {
		// Rolling resistance is a body-pair effect; applying it once for every face point over-damps four-point manifolds.
		if (contact.persistent && contact.manifold_point_index != 0)
			continue;
		const float rolling_friction = CombineTauRollingFriction(*contact.node_a, *contact.node_b);
		if (rolling_friction <= 0.f)
			continue;

		const Vec3 relative_angular_velocity = contact.node_b->angular_velocity - contact.node_a->angular_velocity;
		const Vec3 rolling_velocity = relative_angular_velocity - contact.normal * Dot(relative_angular_velocity, contact.normal);
		const float rolling_speed = Len(rolling_velocity);
		if (rolling_speed <= k_tau_collision_epsilon)
			continue;

		const Vec3 rolling_axis = rolling_velocity / rolling_speed;
		const float rolling_mass = Dot(rolling_axis, ComputeTauInverseInertiaWorld(*contact.node_a) * rolling_axis) +
			Dot(rolling_axis, ComputeTauInverseInertiaWorld(*contact.node_b) * rolling_axis);
		const float inverse_mass_sum = GetTauInverseMass(*contact.node_a) + GetTauInverseMass(*contact.node_b);
		if (rolling_mass <= k_tau_collision_epsilon || inverse_mass_sum <= k_tau_collision_epsilon)
			continue;

		// Apply rolling resistance once per sub-step. Reusing each solver iteration's
		// normal impulse was strong enough to lock every non-zero coefficient.
		const float supported_mass = 1.f / inverse_mass_sum;
		const float support_impulse = supported_mass * Abs(Dot(k_tau_gravity, contact.normal)) * dt_sec;
		const float contact_radius = std::max(Len(contact.point - contact.node_a->position), Len(contact.point - contact.node_b->position));
		const float max_rolling_impulse = k_tau_rolling_friction_impulse_scale * rolling_friction * support_impulse * contact_radius;
		const float rolling_impulse_magnitude = Clamp(-Dot(relative_angular_velocity, rolling_axis) / rolling_mass, -max_rolling_impulse, max_rolling_impulse);
		const Vec3 rolling_impulse = rolling_axis * rolling_impulse_magnitude;
		ApplyTauAngularImpulse(*contact.node_a, -rolling_impulse);
		ApplyTauAngularImpulse(*contact.node_b, rolling_impulse);
	}
}

bool TauManifoldCacheKeyMatches(const TauContactManifold &a, const TauContactManifold &b) {
	return a.ref_a == b.ref_a && a.ref_b == b.ref_b && a.shape_a == b.shape_a && a.shape_b == b.shape_b && a.feature == b.feature;
}

using TauManifoldLookup = std::unordered_multimap<size_t, size_t>;

void TauHashCombine(size_t &seed, size_t value) {
	seed ^= value + size_t(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

size_t GetTauManifoldCacheHash(const TauContactManifold &manifold) {
	size_t hash = std::hash<NodeRef>{}(manifold.ref_a);
	TauHashCombine(hash, std::hash<NodeRef>{}(manifold.ref_b));
	TauHashCombine(hash, manifold.shape_a);
	TauHashCombine(hash, manifold.shape_b);
	TauHashCombine(hash, size_t(manifold.feature.type));
	TauHashCombine(hash, manifold.feature.axis_a);
	TauHashCombine(hash, manifold.feature.axis_b);
	TauHashCombine(hash, uint8_t(manifold.feature.sign_a));
	TauHashCombine(hash, uint8_t(manifold.feature.sign_b));
	TauHashCombine(hash, manifold.feature.edge_signs_a);
	TauHashCombine(hash, manifold.feature.edge_signs_b);
	return hash;
}

TauManifoldLookup::iterator FindTauManifoldLookupEntry(
	TauManifoldLookup &lookup, const std::vector<TauContactManifold> &manifolds, const TauContactManifold &manifold, TauContactDiagnostics *diagnostics) {
	const auto range = lookup.equal_range(GetTauManifoldCacheHash(manifold));
	for (auto it = range.first; it != range.second; ++it) {
		if (diagnostics != nullptr && diagnostics->enabled)
			++diagnostics->manifold_cache_comparisons;
		if (it->second < manifolds.size() && TauManifoldCacheKeyMatches(manifolds[it->second], manifold))
			return it;
	}
	return lookup.end();
}

TauManifoldLookup::iterator FindTauManifoldLookupIndex(
	TauManifoldLookup &lookup, const std::vector<TauContactManifold> &manifolds, size_t manifold_index) {
	if (manifold_index >= manifolds.size())
		return lookup.end();
	const auto range = lookup.equal_range(GetTauManifoldCacheHash(manifolds[manifold_index]));
	for (auto it = range.first; it != range.second; ++it)
		if (it->second == manifold_index)
			return it;
	return lookup.end();
}

void RemoveTauManifoldAt(std::vector<TauContactManifold> &manifolds, TauManifoldLookup &lookup, size_t manifold_index) {
	const size_t last_index = manifolds.size() - 1;
	const auto removed_lookup = FindTauManifoldLookupIndex(lookup, manifolds, manifold_index);
	if (removed_lookup != lookup.end())
		lookup.erase(removed_lookup);

	if (manifold_index != last_index) {
		auto moved_lookup = FindTauManifoldLookupIndex(lookup, manifolds, last_index);
		manifolds[manifold_index] = std::move(manifolds[last_index]);
		if (moved_lookup != lookup.end())
			moved_lookup->second = manifold_index;
	}
	manifolds.pop_back();
}

void PruneTauManifoldCache(
	std::vector<TauContactManifold> &manifolds, TauManifoldLookup &lookup, uint32_t step, TauContactDiagnostics &diagnostics) {
	const auto old_size = manifolds.size();
	for (size_t i = 0; i < manifolds.size();) {
		if (step - manifolds[i].last_seen_step > k_tau_manifold_lifetime)
			RemoveTauManifoldAt(manifolds, lookup, i);
		else
			++i;
	}
	diagnostics.stale_discards += old_size - manifolds.size();
}

size_t UpdateTauManifoldCache(
	std::vector<TauContactManifold> &manifolds, TauManifoldLookup &lookup, TauContactManifold manifold, TauContactDiagnostics &diagnostics) {
	const auto cache_entry = FindTauManifoldLookupEntry(lookup, manifolds, manifold, &diagnostics);
	const size_t cache_index = cache_entry != lookup.end() ? cache_entry->second : std::numeric_limits<size_t>::max();

	if (cache_index != std::numeric_limits<size_t>::max()) {
		const TauContactManifold previous = manifolds[cache_index];
		const float normal_alignment = Dot(previous.normal, manifold.normal);
		std::array<bool, 4> previous_point_used{};
		const float tolerance_sq = k_tau_manifold_point_tolerance * k_tau_manifold_point_tolerance;

		for (uint8_t point_index = 0; point_index < manifold.point_count; ++point_index) {
			TauManifoldPoint &point = manifold.points[point_index];
			int best_match = -1;
			float best_distance = std::numeric_limits<float>::max();
			if (normal_alignment >= k_tau_manifold_normal_tolerance) {
				for (uint8_t previous_index = 0; previous_index < previous.point_count; ++previous_index) {
					if (previous_point_used[previous_index] || previous.points[previous_index].feature_id != point.feature_id)
						continue;
					const float distance_a = Len2(previous.points[previous_index].local_point_a - point.local_point_a);
					const float distance_b = Len2(previous.points[previous_index].local_point_b - point.local_point_b);
					const float distance = distance_a + distance_b;
					if (distance_a <= tolerance_sq && distance_b <= tolerance_sq && distance < best_distance) {
						best_match = previous_index;
						best_distance = distance;
					}
				}
			}

			if (best_match >= 0) {
				const auto &previous_point = previous.points[size_t(best_match)];
				point.accumulated_normal_impulse = previous_point.accumulated_normal_impulse * Clamp(normal_alignment, 0.f, 1.f);
				point.accumulated_tangent_impulse = previous_point.accumulated_tangent_impulse -
					manifold.normal * Dot(previous_point.accumulated_tangent_impulse, manifold.normal);
				previous_point_used[size_t(best_match)] = true;
				++diagnostics.warm_start_hits;
			} else {
				++diagnostics.warm_start_misses;
			}
		}

		manifolds[cache_index] = manifold;
		return cache_index;
	}

	diagnostics.warm_start_misses += manifold.point_count;
	if (manifolds.size() < k_tau_max_manifolds) {
		manifolds.push_back(manifold);
		const size_t new_index = manifolds.size() - 1;
		lookup.emplace(GetTauManifoldCacheHash(manifold), new_index);
		return new_index;
	}

	// Do not invalidate a constraint already emitted during this step. Replace only an inactive oldest entry.
	size_t oldest_index = std::numeric_limits<size_t>::max();
	uint32_t oldest_step = std::numeric_limits<uint32_t>::max();
	for (size_t i = 0; i < manifolds.size(); ++i) {
		if (manifolds[i].last_seen_step != manifold.last_seen_step && manifolds[i].last_seen_step < oldest_step) {
			oldest_step = manifolds[i].last_seen_step;
			oldest_index = i;
		}
	}
	if (oldest_index != std::numeric_limits<size_t>::max()) {
		const auto oldest_lookup = FindTauManifoldLookupIndex(lookup, manifolds, oldest_index);
		if (oldest_lookup != lookup.end())
			lookup.erase(oldest_lookup);
		manifolds[oldest_index] = manifold;
		lookup.emplace(GetTauManifoldCacheHash(manifold), oldest_index);
		++diagnostics.stale_discards;
		++diagnostics.manifold_cache_evictions;
		return oldest_index;
	}
	++diagnostics.manifold_cache_overflows;
	return std::numeric_limits<size_t>::max();
}

std::vector<TauContactConstraint> BuildTauContacts(
	std::map<NodeRef, TauNode> &nodes, std::vector<TauContactManifold> &manifolds, TauManifoldLookup &manifold_lookup, uint32_t step,
	TauContactDiagnostics &diagnostics) {
	TauProfileSection build_contacts_profile("Tau.BuildContacts");
	if (manifold_lookup.bucket_count() < k_tau_max_manifolds)
		manifold_lookup.reserve(k_tau_max_manifolds);
	PruneTauManifoldCache(manifolds, manifold_lookup, step, diagnostics);
	std::vector<TauBodyProxy> bodies;
	bodies.reserve(nodes.size());

	{
		TauProfileSection proxy_profile("Tau.ProxyUpdate");
		for (auto &entry : nodes) {
			if (!entry.second.shapes.empty()) {
				++diagnostics.proxy_shape_vector_reserves;
				auto proxy = BuildTauBodyProxy(entry.first, entry.second);
				diagnostics.proxy_shapes += proxy.shapes.size();
				if (!proxy.shapes.empty())
					bodies.push_back(std::move(proxy));
			}
		}
	}
	diagnostics.proxy_bodies = bodies.size();

	std::vector<TauContactConstraint> contacts;
	std::vector<TauBodyPair> body_pairs;
	body_pairs.reserve(bodies.size() * 8);
	std::vector<MinMax> body_bounds;
	body_bounds.reserve(bodies.size());
	size_t static_body_count = 0;
	for (const auto &body : bodies) {
		body_bounds.push_back(body.bounds);
		if (!IsDynamicTauNode(*body.node))
			++static_body_count;
	}

	{
		TauProfileSection broadphase_profile("Tau.BroadPhase");
		auto append_overlapping_pair = [&](uint32_t i, uint32_t j) {
			if ((!IsDynamicTauNode(*bodies[i].node) && !IsDynamicTauNode(*bodies[j].node)) || !Overlap(body_bounds[i], body_bounds[j]))
				return;
			if (diagnostics.enabled && body_pairs.size() == body_pairs.capacity())
				++diagnostics.candidate_reallocations;
			body_pairs.push_back({i, j});
		};

		BVH body_bvh;
		if (BuildBVH(body_bounds, body_bvh)) {
			for (uint32_t i = 0; i < bodies.size(); ++i) {
				TraverseBVH(body_bvh, body_bounds[i], [&](uint32_t j) {
					if (j > i)
						append_overlapping_pair(i, j);
					return true;
				});
			}
		} else {
			// Invalid bounds should not disable collision detection. Preserve the
			// former all-pairs path as a correctness fallback.
			for (uint32_t i = 0; i < bodies.size(); ++i)
				for (uint32_t j = i + 1; j < bodies.size(); ++j)
					append_overlapping_pair(i, j);
		}
		std::sort(std::begin(body_pairs), std::end(body_pairs), [](const TauBodyPair &a, const TauBodyPair &b) {
			return a.a == b.a ? a.b < b.b : a.a < b.a;
		});
	}
	const size_t body_count = bodies.size();
	diagnostics.body_pair_tests = body_count > 1 ? body_count * (body_count - 1) / 2 : 0;
	diagnostics.static_pair_rejects = static_body_count > 1 ? static_body_count * (static_body_count - 1) / 2 : 0;
	diagnostics.body_pair_candidates = body_pairs.size();
	diagnostics.body_bounds_rejects = diagnostics.body_pair_tests - diagnostics.static_pair_rejects - diagnostics.body_pair_candidates;

	{
		TauProfileSection narrowphase_profile("Tau.NarrowPhase");
		for (const auto &pair : body_pairs) {
			auto &body_a = bodies[pair.a];
			auto &body_b = bodies[pair.b];
			for (const auto &shape_a : body_a.shapes) {
				++diagnostics.shape_body_bounds_tests;
				if (!Overlap(shape_a.bounds, body_b.bounds)) {
					++diagnostics.shape_body_bounds_rejects;
					continue;
				}

				for (const auto &shape_b : body_b.shapes) {
					++diagnostics.shape_pair_bounds_tests;
					if (!Overlap(shape_a.bounds, shape_b.bounds)) {
						++diagnostics.shape_pair_bounds_rejects;
						continue;
					}
					++diagnostics.narrowphase_calls;

					const float friction = CombineTauFriction(*body_a.node, *shape_a.shape, *body_b.node, *shape_b.shape);
					const float restitution = CombineTauRestitution(*body_a.node, *shape_a.shape, *body_b.node, *shape_b.shape);
					if (shape_a.shape->type == CT_Cube && shape_b.shape->type == CT_Cube) {
						TauContactManifold manifold;
						if (!ComputeTauObbContactManifold(shape_a.obb, shape_b.obb, manifold))
							continue;

						++diagnostics.shape_pairs;
						manifold.ref_a = body_a.ref;
						manifold.ref_b = body_b.ref;
						manifold.shape_a = shape_a.shape_index;
						manifold.shape_b = shape_b.shape_index;
						manifold.last_seen_step = step;
						diagnostics.manifold_points += manifold.point_count;
						if (manifold.feature.type == TauContactFeatureType::FaceA)
							++diagnostics.face_a_manifolds;
						else if (manifold.feature.type == TauContactFeatureType::FaceB)
							++diagnostics.face_b_manifolds;
						else
							++diagnostics.edge_edge_manifolds;
						for (uint8_t point_index = 0; point_index < manifold.point_count; ++point_index)
							diagnostics.max_penetration = std::max(diagnostics.max_penetration, manifold.points[point_index].penetration);

						const size_t manifold_index = UpdateTauManifoldCache(manifolds, manifold_lookup, manifold, diagnostics);
						const TauContactManifold &active_manifold =
							manifold_index != std::numeric_limits<size_t>::max() ? manifolds[manifold_index] : manifold;
						for (uint8_t point_index = 0; point_index < active_manifold.point_count; ++point_index) {
							const auto &manifold_point = active_manifold.points[point_index];
							TauContactConstraint contact;
							contact.ref_a = body_a.ref;
							contact.ref_b = body_b.ref;
							contact.node_a = body_a.node;
							contact.node_b = body_b.node;
							contact.shape_a = shape_a.shape_index;
							contact.shape_b = shape_b.shape_index;
							contact.local_point_a = manifold_point.local_point_a;
							contact.local_point_b = manifold_point.local_point_b;
							contact.point = (TauObbLocalToWorld(shape_a.obb, manifold_point.local_point_a) +
								TauObbLocalToWorld(shape_b.obb, manifold_point.local_point_b)) *
								0.5f;
							contact.normal = active_manifold.normal;
							contact.penetration = manifold_point.penetration;
							contact.friction = friction;
							contact.restitution = restitution;
							contact.accumulated_normal_impulse = manifold_point.accumulated_normal_impulse;
							contact.accumulated_tangent_impulse = manifold_point.accumulated_tangent_impulse;
							contact.manifold_index = manifold_index;
							contact.manifold_point_index = point_index;
							contact.manifold_point_count = active_manifold.point_count;
							contact.persistent = manifold_index != std::numeric_limits<size_t>::max();
							if (diagnostics.enabled && contacts.size() == contacts.capacity())
								++diagnostics.contact_reallocations;
							contacts.push_back(contact);
						}
					} else {
						TauContactConstraint contact;
						contact.ref_a = body_a.ref;
						contact.ref_b = body_b.ref;
						contact.node_a = body_a.node;
						contact.node_b = body_b.node;
						contact.shape_a = shape_a.shape_index;
						contact.shape_b = shape_b.shape_index;
						contact.friction = friction;
						contact.restitution = restitution;
						if (ComputeTauContact(shape_a, shape_b, contact.normal, contact.penetration, contact.point)) {
							++diagnostics.shape_pairs;
							++diagnostics.manifold_points;
							diagnostics.max_penetration = std::max(diagnostics.max_penetration, contact.penetration);
							if (diagnostics.enabled && contacts.size() == contacts.capacity())
								++diagnostics.contact_reallocations;
							contacts.push_back(contact);
						}
					}
				}
			}
		}
	}

	return contacts;
}

void StoreTauSolvedManifoldImpulses(const std::vector<TauContactConstraint> &contacts, std::vector<TauContactManifold> &manifolds) {
	for (const auto &contact : contacts) {
		if (!contact.persistent || contact.manifold_index >= manifolds.size())
			continue;
		auto &manifold = manifolds[contact.manifold_index];
		if (contact.manifold_point_index >= manifold.point_count)
			continue;
		auto &point = manifold.points[contact.manifold_point_index];
		point.accumulated_normal_impulse = contact.accumulated_normal_impulse;
		point.accumulated_tangent_impulse = contact.accumulated_tangent_impulse;
		point.penetration = contact.penetration;
	}
}

void ClearTauContactsForNode(NodePairContacts &contacts, NodeRef ref) {
	contacts.erase(ref);
	for (auto &entry : contacts)
		entry.second.erase(ref);
}

void ClearTauManifoldsForNode(std::vector<TauContactManifold> &manifolds, TauManifoldLookup &lookup, NodeRef ref) {
	for (size_t i = 0; i < manifolds.size();) {
		if (manifolds[i].ref_a == ref || manifolds[i].ref_b == ref)
			RemoveTauManifoldAt(manifolds, lookup, i);
		else
			++i;
	}
}

void StoreTauContact(NodePairContacts &contacts, NodeRef ref_a, NodeRef ref_b, const Vec3 &point, const Vec3 &normal, float penetration) {
	contacts[ref_a][ref_b].push_back({point, normal, -penetration});
}

void CollectTauTrackedContacts(const std::vector<TauContactConstraint> &contacts, const std::map<NodeRef, CollisionEventTrackingMode> &tracking_modes,
	NodePairContacts &out_contacts, TauContactDiagnostics &diagnostics) {
	out_contacts.clear();
	if (tracking_modes.empty())
		return;

	diagnostics.tracked_contact_evaluations += contacts.size();
	for (const auto &contact : contacts) {
		if (tracking_modes.find(contact.ref_a) != std::end(tracking_modes))
			StoreTauContact(out_contacts, contact.ref_a, contact.ref_b, contact.point, contact.normal, contact.penetration);
		if (tracking_modes.find(contact.ref_b) != std::end(tracking_modes))
			StoreTauContact(out_contacts, contact.ref_b, contact.ref_a, contact.point, -contact.normal, contact.penetration);
	}
}

bool TauContactDiagnosticsEnabled() {
	static const bool enabled = [] {
		const char *value = std::getenv("HG_TAU_CONTACT_DIAGNOSTICS");
		return value != nullptr && value[0] != '\0' && value[0] != '0';
	}();
	return enabled;
}

void ReportTauContactDiagnostics(uint32_t step, const TauContactDiagnostics &diagnostics, size_t cache_size) {
	if (!TauContactDiagnosticsEnabled() || step % 60 != 0)
		return;
	std::string message = format("Tau contacts step %1: bodies=%2 dynamic=%3 proxies=%4/%5 proxy_allocs=%6 ")
			 .arg(step)
			 .arg(diagnostics.total_bodies)
			 .arg(diagnostics.dynamic_bodies)
			 .arg(diagnostics.proxy_bodies)
			 .arg(diagnostics.proxy_shapes)
			 .arg(diagnostics.proxy_shape_vector_reserves)
			 .str();
	message += format("broad(tests=%1 static=%2 aabb=%3 candidates=%4 reallocs=%5) ")
			 .arg(diagnostics.body_pair_tests)
			 .arg(diagnostics.static_pair_rejects)
			 .arg(diagnostics.body_bounds_rejects)
			 .arg(diagnostics.body_pair_candidates)
			 .arg(diagnostics.candidate_reallocations)
			 .str();
	message += format("narrow(shape_body=%1/%2 shape_aabb=%3/%4 calls=%5 contacts=%6) ")
			 .arg(diagnostics.shape_body_bounds_tests)
			 .arg(diagnostics.shape_body_bounds_rejects)
			 .arg(diagnostics.shape_pair_bounds_tests)
			 .arg(diagnostics.shape_pair_bounds_rejects)
			 .arg(diagnostics.narrowphase_calls)
			 .arg(diagnostics.shape_pairs)
			 .str();
	message += format("manifolds(faceA=%1 faceB=%2 edge=%3 points=%4 cache=%5 scan=%6 warm=%7/%8 evict=%9")
			 .arg(diagnostics.face_a_manifolds)
			 .arg(diagnostics.face_b_manifolds)
			 .arg(diagnostics.edge_edge_manifolds)
			 .arg(diagnostics.manifold_points)
			 .arg(cache_size)
			 .arg(diagnostics.manifold_cache_comparisons)
			 .arg(diagnostics.warm_start_hits)
			 .arg(diagnostics.warm_start_misses)
			 .arg(diagnostics.manifold_cache_evictions)
			 .str();
	message += format(" overflow=%1) ").arg(diagnostics.manifold_cache_overflows).str();
	message += format("solver(pos=%1 vel=%2 rolling=%3 contact_reallocs=%4) events=%5 motion=%6 stale=%7 ")
			 .arg(diagnostics.position_constraint_evaluations)
			 .arg(diagnostics.velocity_constraint_evaluations)
			 .arg(diagnostics.rolling_contact_evaluations)
			 .arg(diagnostics.contact_reallocations)
			 .arg(diagnostics.tracked_contact_evaluations)
			 .arg(diagnostics.motion_updates)
			 .arg(diagnostics.stale_discards)
			 .str();
	message += format("impulses(n=%1 t=%2 clamps=%3) penetration=%4->%5")
			 .arg(diagnostics.normal_impulse_total)
			 .arg(diagnostics.tangent_impulse_total)
			 .arg(diagnostics.friction_clamps)
			 .arg(diagnostics.max_penetration)
			 .arg(diagnostics.max_post_solve_penetration)
			 .str();
	log(message.c_str());
}

void StepTauSubstep(std::map<NodeRef, TauNode> &nodes, std::vector<TauContactManifold> &manifolds, TauManifoldLookup &manifold_lookup,
	uint32_t step, float dt_sec, const std::map<NodeRef, CollisionEventTrackingMode> &tracking_modes, NodePairContacts &latest_contacts) {
	TauProfileSection substep_profile("Tau.Substep");
	TauContactDiagnostics diagnostics;
	diagnostics.enabled = TauContactDiagnosticsEnabled();
	diagnostics.total_bodies = nodes.size();
	{
		TauProfileSection integration_profile("Tau.Integrate");
		for (auto &entry : nodes) {
			auto &node = entry.second;
			if (!IsDynamicTauNode(node))
				continue;
			++diagnostics.dynamic_bodies;

			node.previous_position = node.position;
			node.previous_orientation = node.orientation;
			node.linear_velocity += (k_tau_gravity + node.accumulated_force * node.inverse_mass) * dt_sec;
			node.linear_velocity *= std::max(0.f, 1.f - node.linear_damping * dt_sec);
			node.linear_velocity = node.linear_velocity * node.linear_factor;

			node.angular_velocity += (ComputeTauInverseInertiaWorld(node) * node.accumulated_torque) * dt_sec;
			node.angular_velocity *= std::max(0.f, 1.f - node.angular_damping * dt_sec);
			node.angular_velocity = node.angular_velocity * node.angular_factor;

			node.position += node.linear_velocity * dt_sec;
			IntegrateTauOrientation(node, node.angular_velocity * dt_sec);
		}
	}

	auto contacts = BuildTauContacts(nodes, manifolds, manifold_lookup, step, diagnostics);
	{
		TauProfileSection position_profile("Tau.PositionSolve");
		SolveTauPositionConstraints(contacts, diagnostics);
	}
	{
		TauProfileSection velocity_profile("Tau.VelocitySolve");
		SolveTauVelocityConstraints(contacts, dt_sec, diagnostics);
	}
	{
		TauProfileSection rolling_profile("Tau.RollingFriction");
		SolveTauRollingFriction(contacts, dt_sec, diagnostics);
	}
	{
		TauProfileSection manifold_store_profile("Tau.StoreManifoldImpulses");
		StoreTauSolvedManifoldImpulses(contacts, manifolds);
	}
	{
		TauProfileSection contact_events_profile("Tau.ContactEvents");
		CollectTauTrackedContacts(contacts, tracking_modes, latest_contacts, diagnostics);
	}

	{
		TauProfileSection motion_profile("Tau.MotionUpdate");
		for (auto &entry : nodes) {
			if (entry.second.body_type == RBT_Dynamic) {
				UpdateTauMotionFromState(entry.second);
				++diagnostics.motion_updates;
			}
		}
	}
	ReportTauContactDiagnostics(step, diagnostics, manifolds.size());
}

} // namespace

namespace tau_internal {

bool ComputeObbContactManifold(const OBB &a, const OBB &b, TauContactManifold &manifold) {
	return ComputeTauObbContactManifold(a, b, manifold);
}

Vec3 ObbLocalPointToWorld(const OBB &obb, const Vec3 &point) { return TauObbLocalToWorld(obb, point); }

bool ComputeCapsuleSphereContact(const TauCapsuleGeometry &capsule, const Vec3 &sphere_center, float sphere_radius, Vec3 &normal,
	float &penetration, Vec3 &point) {
	return ComputeTauCapsuleSphereContactImpl(capsule, sphere_center, sphere_radius, normal, penetration, point);
}

bool ComputeCapsuleObbContact(
	const TauCapsuleGeometry &capsule, const OBB &obb, Vec3 &normal, float &penetration, Vec3 &point) {
	return ComputeTauCapsuleObbContactImpl(capsule, obb, normal, penetration, point);
}

bool ComputeCapsuleCapsuleContact(
	const TauCapsuleGeometry &a, const TauCapsuleGeometry &b, Vec3 &normal, float &penetration, Vec3 &point) {
	return ComputeTauCapsuleCapsuleContactImpl(a, b, normal, penetration, point);
}

} // namespace tau_internal

void SceneTauPhysics::SceneCreatePhysics(const Scene &scene, const Reader &ir, const ReadProvider &ip) {
	ClearNodes();

	for (const auto &node : scene.GetAllNodes())
		NodeCreatePhysics(node, ir, ip);
}

void SceneTauPhysics::SceneCreatePhysicsFromFile(const Scene &scene) { SceneCreatePhysics(scene, g_file_reader, g_file_read_provider); }
void SceneTauPhysics::SceneCreatePhysicsFromAssets(const Scene &scene) { SceneCreatePhysics(scene, g_assets_reader, g_assets_read_provider); }

std::shared_ptr<const CollisionGeometry> SceneTauPhysics::LoadCollisionGeometryResource(
	const Reader &ir, const ReadProvider &ip, const std::string &resource) {
	const std::string resolved_resource = ResolveTauCollisionResource(resource);
	const auto cached = collision_geometries.find(resolved_resource);
	if (cached != std::end(collision_geometries))
		return cached->second;

	auto geometry = std::make_shared<CollisionGeometry>();
	const ScopedReadHandle handle(ip, resolved_resource.c_str(), true);
	if (!LoadCollisionGeometry(ir, handle, *geometry)) {
		warn(format("Failed to load cooked collision geometry '%1'").arg(resolved_resource));
		return {};
	}

	collision_geometries[resolved_resource] = geometry;
	return geometry;
}

void SceneTauPhysics::NodeCreatePhysics(const Node &node, const Reader &ir, const ReadProvider &ip) {
	if (!node.IsValid() || !node.HasRigidBody())
		return;

	const auto rigid_body = node.GetRigidBody();
	const auto type = rigid_body.GetType();
	if (!IsTauPhase1RigidBodyType(type))
		return;

	float total_mass = 0.f;
	auto shapes = CollectTauPhase1Collisions(node, total_mass);
	if (shapes.empty())
		return;
	for (auto &shape : shapes) {
		if (shape.type != CT_Mesh)
			continue;
		shape.collision_geometry = LoadCollisionGeometryResource(ir, ip, shape.collision_resource);
		if (!shape.collision_geometry)
			return;
	}

	TauNode tau_node;
	tau_node.shapes = std::move(shapes);
	tau_node.body_type = type;
	tau_node.total_mass = type == RBT_Dynamic ? total_mass : 0.f;
	tau_node.linear_damping = rigid_body.GetLinearDamping();
	tau_node.angular_damping = rigid_body.GetAngularDamping();
	tau_node.friction = rigid_body.GetFriction();
	tau_node.restitution = rigid_body.GetRestitution();
	tau_node.rolling_friction = rigid_body.GetRollingFriction();

	SetTauNodeWorld(tau_node, GetNodeWorld(node), TauWorldWriteMode::Reset);
	RefreshTauMassProperties(tau_node);

	ClearTauManifoldsForNode(contact_manifolds, contact_manifold_lookup, node.ref);
	nodes[node.ref] = tau_node;
}

void SceneTauPhysics::NodeCreatePhysicsFromFile(const Node &node) { NodeCreatePhysics(node, g_file_reader, g_file_read_provider); }
void SceneTauPhysics::NodeCreatePhysicsFromAssets(const Node &node) { NodeCreatePhysics(node, g_assets_reader, g_assets_read_provider); }

void SceneTauPhysics::NodeStartTrackingCollisionEvents(NodeRef ref, CollisionEventTrackingMode mode) { node_collision_event_tracking_modes[ref] = mode; }
void SceneTauPhysics::NodeStopTrackingCollisionEvents(NodeRef ref) { node_collision_event_tracking_modes.erase(ref); }

void SceneTauPhysics::NodeDestroyPhysics(const Node &node) {
	nodes.erase(node.ref);
	ClearTauManifoldsForNode(contact_manifolds, contact_manifold_lookup, node.ref);
	constraints.erase(std::remove_if(std::begin(constraints), std::end(constraints), [&node](const Tau6DofConstraint &constraint) {
		return constraint.ref_a == node.ref || constraint.ref_b == node.ref;
	}), std::end(constraints));
	node_collision_event_tracking_modes.erase(node.ref);
	ClearTauContactsForNode(latest_contacts, node.ref);
}

void SceneTauPhysics::Add6DofConstraint(
	const NodeRef &node_a_ref, const NodeRef &node_b_ref, const Mat4 &anchor_a_local, const Mat4 &anchor_b_local) {
	if (!NodeHasBody(node_a_ref) || !NodeHasBody(node_b_ref))
		return;

	// btGeneric6DofConstraint defaults every axis to free. The current Harfang
	// API exposes creation only, not limit or motor setup, so applying a fixed
	// joint here would diverge from Bullet and break existing scenes.
	constraints.push_back({node_a_ref, node_b_ref, anchor_a_local, anchor_b_local});
}

void SceneTauPhysics::StepSimulation(time_ns dt, time_ns step, int max_step) {
	TauProfileSection step_profile("Tau.StepSimulation");
	if (dt <= 0)
		return;

	int substep_count = 0;
	time_ns substep_time = dt;
	if (step > 0 && max_step > 0) {
		if (dt > std::numeric_limits<time_ns>::max() - fixed_step_accumulator)
			fixed_step_accumulator = std::numeric_limits<time_ns>::max();
		else
			fixed_step_accumulator += dt;

		const time_ns requested_steps = fixed_step_accumulator / step;
		fixed_step_accumulator %= step;
		substep_count = static_cast<int>(std::min<time_ns>(requested_steps, max_step));
		substep_time = step;
	} else {
		// Match Bullet's maxSubSteps == 0 mode: take one variable-duration step.
		fixed_step_accumulator = 0;
		substep_count = 1;
	}

	if (substep_count > 0)
		latest_contacts.clear();

	for (int substep = 0; substep < substep_count; ++substep) {
		if (++contact_step == 0) {
			contact_manifolds.clear();
			contact_manifold_lookup.clear();
			contact_step = 1;
		}
		TriggerPreTickCallback(substep_time);
		StepTauSubstep(nodes, contact_manifolds, contact_manifold_lookup, contact_step, time_to_sec_f(substep_time),
			node_collision_event_tracking_modes, latest_contacts);
	}

	for (auto &entry : nodes) {
		entry.second.accumulated_force = Vec3::Zero;
		entry.second.accumulated_torque = Vec3::Zero;
	}
}

void SceneTauPhysics::CollectCollisionEvents(const Scene &scene, NodePairContacts &contacts) {
	(void)scene;
	contacts = latest_contacts;
}

void SceneTauPhysics::SyncTransformsFromScene(const Scene &scene) {
	TauProfileSection sync_profile("Tau.SyncFromScene");
	for (auto &entry : nodes) {
		if (!scene.IsValidNodeRef(entry.first))
			continue;
		if (entry.second.body_type == RBT_Static || entry.second.body_type == RBT_Kinematic)
			SetTauNodeWorld(entry.second, GetNodeWorld(scene.GetNode(entry.first)), TauWorldWriteMode::CaptureSource);
	}
}

void SceneTauPhysics::SyncTransformsToScene(Scene &scene) {
	TauProfileSection sync_profile("Tau.SyncToScene");
	for (auto &entry : nodes) {
		if (!scene.IsValidNodeRef(entry.first))
			continue;
		if (entry.second.body_type == RBT_Dynamic)
			scene.SetNodeWorldMatrix(entry.first, entry.second.motion.GetWorld());
	}
}

size_t SceneTauPhysics::GarbageCollect(const Scene &scene) {
	size_t removed = 0;

	for (auto it = nodes.begin(); it != nodes.end();) {
		if (!scene.IsValidNodeRef(it->first)) {
			const NodeRef ref = it->first;
			ClearTauManifoldsForNode(contact_manifolds, contact_manifold_lookup, ref);
			constraints.erase(std::remove_if(std::begin(constraints), std::end(constraints), [ref](const Tau6DofConstraint &constraint) {
				return constraint.ref_a == ref || constraint.ref_b == ref;
			}), std::end(constraints));
			node_collision_event_tracking_modes.erase(it->first);
			ClearTauContactsForNode(latest_contacts, it->first);
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

size_t SceneTauPhysics::GarbageCollectResources() {
	size_t removed = 0;
	for (auto it = collision_geometries.begin(); it != collision_geometries.end();) {
		if (it->second.use_count() == 1) {
			it = collision_geometries.erase(it);
			++removed;
		} else {
			++it;
		}
	}
	return removed;
}

void SceneTauPhysics::ClearNodes() {
	nodes.clear();
	constraints.clear();
	contact_manifolds.clear();
	contact_manifold_lookup.clear();
	contact_step = 0;
	fixed_step_accumulator = 0;
	node_collision_event_tracking_modes.clear();
	latest_contacts.clear();
}

void SceneTauPhysics::Clear() {
	ClearNodes();
	collision_geometries.clear();
}

void SceneTauPhysics::NodeWake(NodeRef ref) const { (void)ref; }

void SceneTauPhysics::NodeSetDeactivation(NodeRef ref, bool enable) {
	if (auto *node = FindTauNode(nodes, ref))
		node->deactivation_enabled = enable;
}

bool SceneTauPhysics::NodeGetDeactivation(NodeRef ref) const {
	if (const auto *node = FindTauNode(nodes, ref))
		return node->deactivation_enabled;
	return true;
}

void SceneTauPhysics::NodeResetWorld(NodeRef ref, const Mat4 &world) {
	if (auto *node = FindTauNode(nodes, ref)) {
		ClearTauManifoldsForNode(contact_manifolds, contact_manifold_lookup, ref);
		SetTauNodeWorld(*node, world, TauWorldWriteMode::Reset);
		RefreshTauMassProperties(*node);
		ResetDynamicState(*node);
	}
}

void SceneTauPhysics::NodeTeleport(NodeRef ref, const Mat4 &world) {
	if (auto *node = FindTauNode(nodes, ref)) {
		ClearTauManifoldsForNode(contact_manifolds, contact_manifold_lookup, ref);
		SetTauNodeWorld(*node, world, TauWorldWriteMode::Solved);
		RefreshTauMassProperties(*node);
	}
}

void SceneTauPhysics::NodeAddForce(NodeRef ref, const Vec3 &F) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		node->accumulated_force += F;
	}
}

void SceneTauPhysics::NodeAddForce(NodeRef ref, const Vec3 &F, const Vec3 &world_pos) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		node->accumulated_force += F;
		node->accumulated_torque += Cross(world_pos - node->position, F);
	}
}

void SceneTauPhysics::NodeAddImpulse(NodeRef ref, const Vec3 &impulse) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		node->linear_velocity += (impulse * node->inverse_mass) * node->linear_factor;
	}
}

void SceneTauPhysics::NodeAddImpulse(NodeRef ref, const Vec3 &impulse, const Vec3 &world_pos) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		const Vec3 arm = world_pos - node->position;
		node->linear_velocity += (impulse * node->inverse_mass) * node->linear_factor;
		node->angular_velocity += (ComputeTauInverseInertiaWorld(*node) * Cross(arm, impulse)) * node->angular_factor;
	}
}

void SceneTauPhysics::NodeAddTorque(NodeRef ref, const Vec3 &T) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		node->accumulated_torque += T;
	}
}

void SceneTauPhysics::NodeAddTorqueImpulse(NodeRef ref, const Vec3 &T) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		node->angular_velocity += (ComputeTauInverseInertiaWorld(*node) * T) * node->angular_factor;
	}
}

Vec3 SceneTauPhysics::NodeGetPointVelocity(NodeRef ref, const Vec3 &world_pos) const {
	if (const auto *node = FindTauNode(nodes, ref))
		return GetTauPointVelocity(*node, world_pos);
	return {};
}

Vec3 SceneTauPhysics::NodeGetLinearVelocity(NodeRef ref) const {
	if (const auto *node = FindTauNode(nodes, ref))
		return node->linear_velocity;
	return {};
}

void SceneTauPhysics::NodeSetLinearVelocity(NodeRef ref, const Vec3 &V) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		node->linear_velocity = V * node->linear_factor;
	}
}

Vec3 SceneTauPhysics::NodeGetAngularVelocity(NodeRef ref) const {
	if (const auto *node = FindTauNode(nodes, ref))
		return node->angular_velocity;
	return {};
}

void SceneTauPhysics::NodeSetAngularVelocity(NodeRef ref, const Vec3 &W) {
	if (auto *node = FindTauNode(nodes, ref)) {
		if (!IsDynamicTauNode(*node))
			return;
		node->angular_velocity = W * node->angular_factor;
	}
}

Vec3 SceneTauPhysics::NodeGetLinearFactor(NodeRef ref) const {
	if (const auto *node = FindTauNode(nodes, ref))
		return node->linear_factor;
	return {};
}

void SceneTauPhysics::NodeSetLinearFactor(NodeRef ref, const Vec3 &k) {
	if (auto *node = FindTauNode(nodes, ref))
		node->linear_factor = k;
}

Vec3 SceneTauPhysics::NodeGetAngularFactor(NodeRef ref) const {
	if (const auto *node = FindTauNode(nodes, ref))
		return node->angular_factor;
	return {};
}

void SceneTauPhysics::NodeSetAngularFactor(NodeRef ref, const Vec3 &k) {
	if (auto *node = FindTauNode(nodes, ref))
		node->angular_factor = k;
}

std::vector<RaycastOut> SceneTauPhysics::RaycastAllHits(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const {
	const Vec3 ray = world_p1 - world_p0;
	const float max_distance = Len(ray);
	if (max_distance <= k_tau_collision_epsilon)
		return {};
	const Vec3 direction = ray / max_distance;

	std::vector<RaycastOut> hits;
	for (const auto &entry : nodes) {
		if (!scene.IsValidNodeRef(entry.first))
			continue;
		for (const auto &shape : entry.second.shapes) {
			TauRayShapeHit shape_hit;
			if (!IntersectTauRayShape(entry.second, shape, world_p0, direction, max_distance, shape_hit))
				continue;
			RaycastOut out;
			out.P = world_p0 + direction * shape_hit.t;
			out.N = shape_hit.normal;
			out.node = scene.GetNode(entry.first);
			out.t = shape_hit.t;
			hits.push_back(out);
		}
	}

	// Bullet does not promise an order for all hits. Tau deliberately returns a
	// stable near-to-far order, which makes QA captures deterministic.
	std::stable_sort(std::begin(hits), std::end(hits), [](const RaycastOut &a, const RaycastOut &b) { return a.t < b.t; });
	return hits;
}

RaycastOut SceneTauPhysics::RaycastFirstHit(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const {
	const Vec3 ray = world_p1 - world_p0;
	const float max_distance = Len(ray);
	if (max_distance <= k_tau_collision_epsilon)
		return {};
	const Vec3 direction = ray / max_distance;

	RaycastOut closest;
	for (const auto &entry : nodes) {
		if (!scene.IsValidNodeRef(entry.first))
			continue;
		for (const auto &shape : entry.second.shapes) {
			TauRayShapeHit shape_hit;
			if (!IntersectTauRayShape(entry.second, shape, world_p0, direction, max_distance, shape_hit) || shape_hit.t >= closest.t)
				continue;
			closest.P = world_p0 + direction * shape_hit.t;
			closest.N = shape_hit.normal;
			closest.node = scene.GetNode(entry.first);
			closest.t = shape_hit.t;
		}
	}
	return closest;
}

void SceneTauPhysics::RenderCollision(
	bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_decl, bgfx::ProgramHandle program, RenderState state, uint32_t depth) {
	size_t shape_count = 0;
	for (const auto &entry : nodes)
		shape_count += entry.second.shapes.size();
	size_t manifold_point_count = 0;
	for (const auto &manifold : contact_manifolds)
		if (manifold.last_seen_step == contact_step)
			manifold_point_count += manifold.point_count;

	if (shape_count == 0 && manifold_point_count == 0)
		return;

	size_t vertex_capacity = manifold_point_count * 8;
	for (const auto &entry : nodes) {
		for (const auto &shape : entry.second.shapes) {
			if (shape.type == CT_Mesh && shape.collision_geometry)
				vertex_capacity += shape.collision_geometry->triangles.size() * 6;
			else
				vertex_capacity += 192;
		}
	}
	Vertices vtx(vtx_decl, vertex_capacity);
	size_t vtx_count = 0;

	for (const auto &entry : nodes) {
		const auto &node = entry.second;
		const Color color = GetTauDebugColor(node);

		for (const auto &shape : node.shapes) {
			if (shape.type == CT_Sphere)
				AppendTauSphereWireframe(vtx, vtx_count, BuildTauWorldSphereCenter(node, shape), BuildTauWorldSphereRadius(node, shape),
					BuildTauWorldSphereRotation(node, shape), color);
			else if (shape.type == CT_Cube)
				AppendTauObbWireframe(vtx, vtx_count, BuildTauWorldOBB(node, shape), color);
			else if (shape.type == CT_Mesh && shape.collision_geometry)
				AppendTauMeshWireframe(vtx, vtx_count, ComposeTauWorld(node) * shape.local_transform, *shape.collision_geometry, color);
			else {
				const TauPrimitiveFrame frame = BuildTauPrimitiveFrame(node, shape);
				const Vec3 scale = Abs(node.scale);
				const float radius = shape.radius * std::max(scale.x, scale.z);
				const float height = Abs(shape.size.y) * scale.y;
				if (shape.type == CT_Capsule)
					AppendTauCapsuleWireframe(vtx, vtx_count, frame, radius, height, color);
				else if (shape.type == CT_Cone)
					AppendTauConeWireframe(vtx, vtx_count, frame, radius, height, color);
				else if (shape.type == CT_Cylinder)
					AppendTauCylinderWireframe(vtx, vtx_count, frame, radius, height, color);
			}
		}
	}

	for (const auto &manifold : contact_manifolds) {
		if (manifold.last_seen_step != contact_step)
			continue;
		const TauNode *node_a = FindTauNode(nodes, manifold.ref_a);
		const TauNode *node_b = FindTauNode(nodes, manifold.ref_b);
		if (node_a == nullptr || node_b == nullptr || manifold.shape_a >= node_a->shapes.size() || manifold.shape_b >= node_b->shapes.size())
			continue;
		const OBB obb_a = BuildTauWorldOBB(*node_a, node_a->shapes[manifold.shape_a]);
		const OBB obb_b = BuildTauWorldOBB(*node_b, node_b->shapes[manifold.shape_b]);
		for (uint8_t point_index = 0; point_index < manifold.point_count; ++point_index) {
			const auto &manifold_point = manifold.points[point_index];
			const Vec3 point = (TauObbLocalToWorld(obb_a, manifold_point.local_point_a) + TauObbLocalToWorld(obb_b, manifold_point.local_point_b)) * 0.5f;
			AppendTauManifoldPoint(vtx, vtx_count, point, manifold.normal, manifold_point.penetration);
		}
	}

	DrawLines(view_id, vtx, program, state, depth);
}

void SceneTauPhysics::SetPreTickCallback(const std::function<void(SceneTauPhysics &, hg::time_ns t)> &cbk) { pre_tick_callback = cbk; }

void SceneTauPhysics::TriggerPreTickCallback(hg::time_ns dt) {
	if (pre_tick_callback)
		pre_tick_callback(*this, dt);
}

} // namespace hg
