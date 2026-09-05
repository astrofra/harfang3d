// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/minmax.h"
#include "foundation/rw_interface.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace hg {

// Generic bounding volume hierarchy over caller-owned primitives. Leaves refer
// to primitives by index, so the same structure can accelerate geometry,
// scene objects, audio emitters or any other collection with AABB bounds.
struct BVHNode {
	MinMax bounds;
	// Leaf: first primitive index in BVH::primitive_indices.
	// Branch: index of the left child; the right child immediately follows it.
	uint32_t first{0};
	// A zero count identifies a branch. Every leaf contains at least one item.
	uint32_t count{0};

	bool IsLeaf() const { return count != 0; }
};

struct BVH {
	std::vector<BVHNode> nodes;
	std::vector<uint32_t> primitive_indices;
};

// Build a deterministic, balanced BVH. Primitive data remains caller-owned.
bool BuildBVH(const MinMax *primitive_bounds, size_t primitive_count, BVH &bvh, uint32_t max_primitives_per_leaf = 8);
inline bool BuildBVH(const std::vector<MinMax> &primitive_bounds, BVH &bvh, uint32_t max_primitives_per_leaf = 8) {
	return BuildBVH(primitive_bounds.data(), primitive_bounds.size(), bvh, max_primitives_per_leaf);
}

// Validate the hierarchy, its child links and its one-to-one primitive index table.
bool ValidateBVH(const BVH &bvh, size_t primitive_count);

// Return false when the visitor requested an early exit, true when traversal completed.
// A ray visitor may reduce max_distance after a narrow-phase hit to prune farther nodes.
using BVHPrimitiveVisitor = std::function<bool(uint32_t primitive_index)>;
bool TraverseBVH(const BVH &bvh, const MinMax &bounds, const BVHPrimitiveVisitor &visitor);
bool TraverseBVHRay(
	const BVH &bvh, const Vec3 &origin, const Vec3 &direction, float &max_distance, const BVHPrimitiveVisitor &visitor);

// Serialize an embedded BVH block. The containing resource owns format versioning.
bool LoadBVH(const Reader &ir, const Handle &h, BVH &bvh, size_t primitive_count);
bool SaveBVH(const Writer &iw, const Handle &h, const BVH &bvh, size_t primitive_count);

} // namespace hg
