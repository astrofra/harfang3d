// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene_tau_physics.h"

#include "engine/assets_rw_interface.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"

#include "foundation/file_rw_interface.h"
#include "foundation/matrix4.h"
#include "foundation/minmax.h"
#include "foundation/obb.h"

#include <algorithm>
#include <limits>

namespace hg {

namespace {

static const Vec3 k_tau_gravity(0.f, -9.81f, 0.f);
static constexpr float k_tau_collision_epsilon = 1e-5f;
static constexpr float k_tau_position_slop = 0.002f;
static constexpr float k_tau_position_correction = 0.75f;
static constexpr float k_tau_restitution_threshold = 1.0f;
static constexpr float k_tau_baumgarte = 0.15f;
static constexpr float k_tau_rolling_friction_impulse_scale = 0.1f;
static constexpr int k_tau_position_iterations = 3;
static constexpr int k_tau_velocity_iterations = 8;

enum class TauWorldWriteMode { Reset, CaptureSource, Solved };

struct TauWorldShape {
	NodeRef ref{};
	TauNode *node{nullptr};
	const TauCollisionShape *shape{nullptr};
	Vec3 position{Vec3::Zero};
	Vec3 previous_position{Vec3::Zero};
	float radius{0.f};
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

struct TauContactConstraint {
	NodeRef ref_a{};
	NodeRef ref_b{};
	TauNode *node_a{nullptr};
	TauNode *node_b{nullptr};
	Vec3 point{Vec3::Zero};
	Vec3 normal{Vec3::Up}; // points from A toward B
	float penetration{0.f};
	float friction{0.5f};
	float restitution{0.f};
};

bool IsTauPhase1RigidBodyType(RigidBodyType type) {
	return type == RBT_Dynamic || type == RBT_Static;
}

bool IsDynamicTauNode(const TauNode &node) {
	return node.body_type == RBT_Dynamic && node.total_mass > 0.f && node.inverse_mass > 0.f;
}

std::vector<TauCollisionShape> CollectTauPhase1Collisions(const Node &node, float &total_mass) {
	std::vector<TauCollisionShape> shapes;
	total_mass = 0.f;

	for (size_t i = 0; i < node.GetCollisionCount(); ++i) {
		const auto collision = node.GetCollision(i);
		if (!collision.IsValid())
			continue;
		if (collision.GetType() != CT_Cube && collision.GetType() != CT_Sphere)
			return {};

		TauCollisionShape shape;
		shape.type = collision.GetType();
		shape.local_transform = collision.GetLocalTransform();
		Decompose(shape.local_transform, &shape.local_position, &shape.local_rotation, nullptr);
		shape.size = collision.GetSize();
		shape.radius = collision.GetRadius();
		shape.mass = collision.GetMass();
		// Collision components currently do not store contact material properties in Scene.
		// Keep them on the rigid body side for the Tau phase-1 backend, like Bullet does.
		shape.friction = 0.f;
		shape.restitution = 0.f;

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
		if (shape.mass <= 0.f)
			continue;

		Mat3 centered_inertia;
		if (shape.type == CT_Sphere) {
			const float radius = shape.radius * std::max({abs_scale.x, abs_scale.y, abs_scale.z});
			centered_inertia = Mat3::Identity * (2.f * shape.mass * radius * radius / 5.f);
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

TauBodyProxy BuildTauBodyProxy(NodeRef ref, TauNode &node) {
	TauBodyProxy proxy;
	proxy.ref = ref;
	proxy.node = &node;
	proxy.shapes.reserve(node.shapes.size());

	bool has_bounds = false;

	for (const auto &shape : node.shapes) {
		TauWorldShape world_shape;
		world_shape.ref = ref;
		world_shape.node = &node;
		world_shape.shape = &shape;
		if (shape.type == CT_Sphere) {
			world_shape.position = BuildTauWorldSphereCenter(node, shape);
			world_shape.previous_position = BuildTauWorldSphereCenter(node, shape, true);
			world_shape.radius = BuildTauWorldSphereRadius(node, shape);
			world_shape.bounds = {world_shape.position - Vec3(world_shape.radius), world_shape.position + Vec3(world_shape.radius)};
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

Vec3 ComputeTauContactOrientationHint(const TauWorldShape &a, const TauWorldShape &b) {
	const Vec3 current_delta = b.position - a.position;
	const Vec3 previous_delta = b.previous_position - a.previous_position;
	const Vec3 relative_motion = current_delta - previous_delta;
	const float epsilon_sq = k_tau_collision_epsilon * k_tau_collision_epsilon;

	if (Len2(relative_motion) > epsilon_sq)
		return -relative_motion;
	if (Len2(previous_delta) > epsilon_sq)
		return previous_delta;
	return current_delta;
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

bool ComputeTauObbContact(const OBB &a, const OBB &b, const Vec3 &orientation_hint, Vec3 &normal, float &penetration, Vec3 &point) {
	const Vec3 half_a = Abs(a.scl) * 0.5f;
	const Vec3 half_b = Abs(b.scl) * 0.5f;
	const Vec3 axis_a[3] = {GetTauObbAxis(a, 0), GetTauObbAxis(a, 1), GetTauObbAxis(a, 2)};
	const Vec3 axis_b[3] = {GetTauObbAxis(b, 0), GetTauObbAxis(b, 1), GetTauObbAxis(b, 2)};
	const Vec3 delta = b.pos - a.pos;
	const Vec3 hint = Len2(orientation_hint) > (k_tau_collision_epsilon * k_tau_collision_epsilon) ? orientation_hint : delta;
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

	auto register_axis = [&](const Vec3 &axis, float axis_length, float separation, float radius_a, float radius_b) {
		if (axis_length <= k_tau_collision_epsilon)
			return true;

		const float total_radius = radius_a + radius_b;
		if (separation > total_radius)
			return false;

		Vec3 candidate_normal = axis / axis_length;
		if (Dot(candidate_normal, hint) < 0.f)
			candidate_normal = -candidate_normal;

		const float axis_penetration = (total_radius - separation) / axis_length;
		if (axis_penetration < best_penetration) {
			best_penetration = axis_penetration;
			best_normal = candidate_normal;
		}

		return true;
	};

	for (int i = 0; i < 3; ++i) {
		const float radius_a = half_a[i];
		const float radius_b = half_b.x * abs_r[i][0] + half_b.y * abs_r[i][1] + half_b.z * abs_r[i][2];
		if (!register_axis(axis_a[i], 1.f, Abs(t[i]), radius_a, radius_b))
			return false;
	}

	for (int j = 0; j < 3; ++j) {
		const float radius_a = half_a.x * abs_r[0][j] + half_a.y * abs_r[1][j] + half_a.z * abs_r[2][j];
		const float radius_b = half_b[j];
		if (!register_axis(axis_b[j], 1.f, Abs(Dot(delta, axis_b[j])), radius_a, radius_b))
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
			if (!register_axis(axis, axis_length, separation, radius_a, radius_b))
				return false;
		}
	}

	normal = best_normal;
	penetration = best_penetration;
	// Use the smaller OBB's face center. Averaging support points makes a large floor select an arbitrary corner when the normal is vertical,
	// which applies the normal impulse far from the dynamic body's center and creates a spurious torque.
	const Vec3 size_a = Abs(a.scl);
	const Vec3 size_b = Abs(b.scl);
	const float volume_a = size_a.x * size_a.y * size_a.z;
	const float volume_b = size_b.x * size_b.y * size_b.z;
	point = volume_a <= volume_b ? GetTauFaceCenter(a, normal) : GetTauFaceCenter(b, -normal);
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
	if (a.shape->type == CT_Cube && b.shape->type == CT_Cube)
		return ComputeTauObbContact(a.obb, b.obb, ComputeTauContactOrientationHint(a, b), normal, penetration, point);
	if (a.shape->type == CT_Sphere && b.shape->type == CT_Cube)
		return ComputeTauSphereObbContact(a.position, a.radius, b.obb, normal, penetration, point);
	if (a.shape->type == CT_Cube && b.shape->type == CT_Sphere) {
		const bool contact = ComputeTauSphereObbContact(b.position, b.radius, a.obb, normal, penetration, point);
		if (contact)
			normal = -normal;
		return contact;
	}
	return ComputeTauSphereContact(a, b, normal, penetration, point);
}

float GetTauInverseMass(const TauNode &node) {
	return IsDynamicTauNode(node) ? node.inverse_mass : 0.f;
}

Vec3 GetTauPointVelocity(const TauNode &node, const Vec3 &world_pos) {
	return node.linear_velocity + Cross(node.angular_velocity, world_pos - node.position);
}

void ApplyTauLinearCorrection(TauNode &node, const Vec3 &delta_position) {
	if (!IsDynamicTauNode(node))
		return;
	node.position += delta_position * node.linear_factor;
}

void ApplyTauImpulse(TauNode &node, const Vec3 &impulse, const Vec3 &arm) {
	if (!IsDynamicTauNode(node))
		return;

	node.linear_velocity += (impulse * node.inverse_mass) * node.linear_factor;
	node.angular_velocity += (ComputeTauInverseInertiaWorld(node) * Cross(arm, impulse)) * node.angular_factor;
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

void SolveTauPositionConstraints(std::vector<TauContactConstraint> &contacts) {
	for (int iteration = 0; iteration < k_tau_position_iterations; ++iteration) {
		for (auto &contact : contacts) {
			const float inverse_mass_a = GetTauInverseMass(*contact.node_a);
			const float inverse_mass_b = GetTauInverseMass(*contact.node_b);
			const float inverse_mass_sum = inverse_mass_a + inverse_mass_b;
			const float depth = std::max(contact.penetration - k_tau_position_slop, 0.f);

			if (inverse_mass_sum <= k_tau_collision_epsilon || depth <= 0.f)
				continue;

			const Vec3 correction = contact.normal * (k_tau_position_correction * depth / inverse_mass_sum);
			ApplyTauLinearCorrection(*contact.node_a, -correction * inverse_mass_a);
			ApplyTauLinearCorrection(*contact.node_b, correction * inverse_mass_b);

			contact.penetration = std::max(0.f, contact.penetration - depth * k_tau_position_correction);
		}
	}
}

void SolveTauVelocityConstraints(std::vector<TauContactConstraint> &contacts, float dt_sec) {
	if (dt_sec <= 0.f)
		return;

	for (int iteration = 0; iteration < k_tau_velocity_iterations; ++iteration) {
		for (auto &contact : contacts) {
			const Vec3 arm_a = contact.point - contact.node_a->position;
			const Vec3 arm_b = contact.point - contact.node_b->position;
			const Vec3 relative_velocity = GetTauPointVelocity(*contact.node_b, contact.point) - GetTauPointVelocity(*contact.node_a, contact.point);
			const float normal_speed = Dot(relative_velocity, contact.normal);

			const float constraint_mass = ComputeTauConstraintMass(contact, arm_a, arm_b, contact.normal);
			if (constraint_mass <= k_tau_collision_epsilon)
				continue;

			const float restitution = normal_speed < -k_tau_restitution_threshold ? contact.restitution : 0.f;
			const float bias = k_tau_baumgarte * std::max(contact.penetration - k_tau_position_slop, 0.f) / dt_sec;
			const float normal_impulse_magnitude = std::max((-(1.f + restitution) * normal_speed + bias) / constraint_mass, 0.f);
			const Vec3 normal_impulse = contact.normal * normal_impulse_magnitude;

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

			float tangent_impulse_magnitude = -Dot(post_normal_velocity, tangent) / tangent_mass;
			const float max_friction_impulse = contact.friction * normal_impulse_magnitude;
			tangent_impulse_magnitude = Clamp(tangent_impulse_magnitude, -max_friction_impulse, max_friction_impulse);

			const Vec3 tangent_impulse = tangent * tangent_impulse_magnitude;
			ApplyTauImpulse(*contact.node_a, -tangent_impulse, arm_a);
			ApplyTauImpulse(*contact.node_b, tangent_impulse, arm_b);

		}
	}
}

void SolveTauRollingFriction(const std::vector<TauContactConstraint> &contacts, float dt_sec) {
	for (const auto &contact : contacts) {
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

std::vector<TauContactConstraint> BuildTauContacts(std::map<NodeRef, TauNode> &nodes) {
	std::vector<TauBodyProxy> bodies;
	bodies.reserve(nodes.size());

	for (auto &entry : nodes) {
		if (!entry.second.shapes.empty())
			bodies.push_back(BuildTauBodyProxy(entry.first, entry.second));
	}

	std::vector<TauContactConstraint> contacts;

	for (size_t i = 0; i < bodies.size(); ++i) {
		for (size_t j = i + 1; j < bodies.size(); ++j) {
			auto &body_a = bodies[i];
			auto &body_b = bodies[j];

			if ((!IsDynamicTauNode(*body_a.node) && !IsDynamicTauNode(*body_b.node)) || !Overlap(body_a.bounds, body_b.bounds))
				continue;

			for (const auto &shape_a : body_a.shapes) {
				if (!Overlap(shape_a.bounds, body_b.bounds))
					continue;

				for (const auto &shape_b : body_b.shapes) {
					if (!Overlap(shape_a.bounds, shape_b.bounds))
						continue;

					TauContactConstraint contact;
					contact.ref_a = body_a.ref;
					contact.ref_b = body_b.ref;
					contact.node_a = body_a.node;
					contact.node_b = body_b.node;
					contact.friction = CombineTauFriction(*body_a.node, *shape_a.shape, *body_b.node, *shape_b.shape);
					contact.restitution = CombineTauRestitution(*body_a.node, *shape_a.shape, *body_b.node, *shape_b.shape);
					if (ComputeTauContact(shape_a, shape_b, contact.normal, contact.penetration, contact.point))
						contacts.push_back(contact);
				}
			}
		}
	}

	return contacts;
}

void ClearTauContactsForNode(NodePairContacts &contacts, NodeRef ref) {
	contacts.erase(ref);
	for (auto &entry : contacts)
		entry.second.erase(ref);
}

void StoreTauContact(NodePairContacts &contacts, NodeRef ref_a, NodeRef ref_b, const Vec3 &point, const Vec3 &normal, float penetration) {
	contacts[ref_a][ref_b].push_back({point, normal, -penetration});
}

void CollectTauTrackedContacts(const std::vector<TauContactConstraint> &contacts, const std::map<NodeRef, CollisionEventTrackingMode> &tracking_modes,
	NodePairContacts &out_contacts) {
	out_contacts.clear();

	for (const auto &contact : contacts) {
		if (tracking_modes.find(contact.ref_a) != std::end(tracking_modes))
			StoreTauContact(out_contacts, contact.ref_a, contact.ref_b, contact.point, contact.normal, contact.penetration);
		if (tracking_modes.find(contact.ref_b) != std::end(tracking_modes))
			StoreTauContact(out_contacts, contact.ref_b, contact.ref_a, contact.point, -contact.normal, contact.penetration);
	}
}

void StepTauSubstep(std::map<NodeRef, TauNode> &nodes, float dt_sec, const std::map<NodeRef, CollisionEventTrackingMode> &tracking_modes,
	NodePairContacts &latest_contacts) {
	for (auto &entry : nodes) {
		auto &node = entry.second;
		if (!IsDynamicTauNode(node))
			continue;

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

	auto contacts = BuildTauContacts(nodes);
	SolveTauPositionConstraints(contacts);
	SolveTauVelocityConstraints(contacts, dt_sec);
	SolveTauRollingFriction(contacts, dt_sec);
	CollectTauTrackedContacts(contacts, tracking_modes, latest_contacts);

	for (auto &entry : nodes) {
		if (entry.second.body_type == RBT_Dynamic)
			UpdateTauMotionFromState(entry.second);
	}
}

} // namespace

void SceneTauPhysics::SceneCreatePhysics(const Scene &scene, const Reader &ir, const ReadProvider &ip) {
	(void)ir;
	(void)ip;

	ClearNodes();

	for (const auto &node : scene.GetAllNodes())
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

	float total_mass = 0.f;
	auto shapes = CollectTauPhase1Collisions(node, total_mass);
	if (shapes.empty())
		return;

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

	nodes[node.ref] = tau_node;
}

void SceneTauPhysics::NodeCreatePhysicsFromFile(const Node &node) { NodeCreatePhysics(node, g_file_reader, g_file_read_provider); }
void SceneTauPhysics::NodeCreatePhysicsFromAssets(const Node &node) { NodeCreatePhysics(node, g_assets_reader, g_assets_read_provider); }

void SceneTauPhysics::NodeStartTrackingCollisionEvents(NodeRef ref, CollisionEventTrackingMode mode) { node_collision_event_tracking_modes[ref] = mode; }
void SceneTauPhysics::NodeStopTrackingCollisionEvents(NodeRef ref) { node_collision_event_tracking_modes.erase(ref); }

void SceneTauPhysics::NodeDestroyPhysics(const Node &node) {
	nodes.erase(node.ref);
	node_collision_event_tracking_modes.erase(node.ref);
	ClearTauContactsForNode(latest_contacts, node.ref);
}

void SceneTauPhysics::StepSimulation(time_ns dt, time_ns step, int max_step) {
	const float dt_sec = time_to_sec_f(dt);
	if (dt_sec <= 0.f)
		return;

	const float step_sec = step > 0 ? time_to_sec_f(step) : dt_sec;
	const int requested_steps = step_sec > 0.f ? std::max(1, int(Ceil(dt_sec / step_sec))) : 1;
	const int substep_count = max_step > 0 ? std::min(requested_steps, max_step) : requested_steps;
	const float substep_dt = dt_sec / substep_count;

	latest_contacts.clear();

	for (int substep = 0; substep < substep_count; ++substep)
		StepTauSubstep(nodes, substep_dt, node_collision_event_tracking_modes, latest_contacts);

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
	for (auto &entry : nodes) {
		if (!scene.IsValidNodeRef(entry.first))
			continue;
		if (entry.second.body_type == RBT_Static)
			SetTauNodeWorld(entry.second, GetNodeWorld(scene.GetNode(entry.first)), TauWorldWriteMode::CaptureSource);
	}
}

void SceneTauPhysics::SyncTransformsToScene(Scene &scene) {
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

size_t SceneTauPhysics::GarbageCollectResources() { return 0; }

void SceneTauPhysics::ClearNodes() {
	nodes.clear();
	node_collision_event_tracking_modes.clear();
	latest_contacts.clear();
}

void SceneTauPhysics::Clear() { ClearNodes(); }

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
		SetTauNodeWorld(*node, world, TauWorldWriteMode::Reset);
		RefreshTauMassProperties(*node);
		ResetDynamicState(*node);
	}
}

void SceneTauPhysics::NodeTeleport(NodeRef ref, const Mat4 &world) {
	if (auto *node = FindTauNode(nodes, ref)) {
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

void SceneTauPhysics::RenderCollision(
	bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_decl, bgfx::ProgramHandle program, RenderState state, uint32_t depth) {
	size_t shape_count = 0;
	for (const auto &entry : nodes)
		shape_count += entry.second.shapes.size();

	if (shape_count == 0)
		return;

	Vertices vtx(vtx_decl, shape_count * 96);
	size_t vtx_count = 0;

	for (const auto &entry : nodes) {
		const auto &node = entry.second;
		const Color color = GetTauDebugColor(node);

		for (const auto &shape : node.shapes) {
			if (shape.type == CT_Sphere)
				AppendTauSphereWireframe(vtx, vtx_count, BuildTauWorldSphereCenter(node, shape), BuildTauWorldSphereRadius(node, shape),
					BuildTauWorldSphereRotation(node, shape), color);
			else
				AppendTauObbWireframe(vtx, vtx_count, BuildTauWorldOBB(node, shape), color);
		}
	}

	DrawLines(view_id, vtx, program, state, depth);
}

} // namespace hg
