// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/node.h"

#include "foundation/obb.h"
#include "foundation/vector3.h"

#include <array>
#include <cstdint>

namespace hg {

enum class TauContactFeatureType : uint8_t { FaceA, FaceB, EdgeEdge };

struct TauContactFeature {
	TauContactFeatureType type{TauContactFeatureType::FaceA};
	uint8_t axis_a{0}, axis_b{0};
	int8_t sign_a{0}, sign_b{0};
	uint8_t edge_signs_a{0}, edge_signs_b{0};
};

inline bool operator==(const TauContactFeature &a, const TauContactFeature &b) {
	return a.type == b.type && a.axis_a == b.axis_a && a.axis_b == b.axis_b && a.sign_a == b.sign_a && a.sign_b == b.sign_b &&
		   a.edge_signs_a == b.edge_signs_a && a.edge_signs_b == b.edge_signs_b;
}

struct TauManifoldPoint {
	// Surface anchors are expressed in the local orthonormal frames of the two cuboids.
	Vec3 local_point_a{Vec3::Zero}, local_point_b{Vec3::Zero};
	float penetration{0.f};
	float accumulated_normal_impulse{0.f};
	Vec3 accumulated_tangent_impulse{Vec3::Zero};
	uint32_t feature_id{0};
};

struct TauContactManifold {
	NodeRef ref_a{}, ref_b{};
	uint32_t shape_a{0}, shape_b{0};
	Vec3 normal{Vec3::Up}; // Always points from shape A toward shape B.
	TauContactFeature feature{};
	std::array<TauManifoldPoint, 4> points{};
	uint8_t point_count{0};
	uint32_t last_seen_step{0};
};

namespace tau_internal {

// Internal narrowphase entry points kept visible for focused Tau unit tests.
bool ComputeObbContactManifold(const OBB &a, const OBB &b, TauContactManifold &manifold);
Vec3 ObbLocalPointToWorld(const OBB &obb, const Vec3 &point);

} // namespace tau_internal

} // namespace hg
