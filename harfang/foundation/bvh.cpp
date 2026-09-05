// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/bvh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace hg {

namespace {

bool IsValidBounds(const MinMax &bounds) {
	return std::isfinite(bounds.mn.x) && std::isfinite(bounds.mn.y) && std::isfinite(bounds.mn.z) && std::isfinite(bounds.mx.x) &&
		std::isfinite(bounds.mx.y) && std::isfinite(bounds.mx.z) && bounds.mn.x <= bounds.mx.x && bounds.mn.y <= bounds.mx.y &&
		bounds.mn.z <= bounds.mx.z;
}

bool ContainsBounds(const MinMax &outer, const MinMax &inner) {
	return inner.mn.x >= outer.mn.x && inner.mn.y >= outer.mn.y && inner.mn.z >= outer.mn.z && inner.mx.x <= outer.mx.x &&
		inner.mx.y <= outer.mx.y && inner.mx.z <= outer.mx.z;
}

uint32_t LongestAxis(const Vec3 &size) {
	if (size.y > size.x && size.y >= size.z)
		return 1;
	if (size.z > size.x && size.z > size.y)
		return 2;
	return 0;
}

struct BVHBuilder {
	const MinMax *primitive_bounds{nullptr};
	BVH *bvh{nullptr};
	uint32_t leaf_size{0};

	void BuildNode(uint32_t node_index, uint32_t begin, uint32_t count) {
		if (count <= leaf_size) {
			MinMax bounds;
			for (uint32_t i = 0; i < count; ++i)
				bounds = Union(bounds, primitive_bounds[bvh->primitive_indices[begin + i]]);
			bvh->nodes[node_index] = {bounds, begin, count};
			return;
		}

		MinMax centroid_bounds;
		for (uint32_t i = 0; i < count; ++i)
			centroid_bounds = Union(centroid_bounds, GetCenter(primitive_bounds[bvh->primitive_indices[begin + i]]));
		const uint32_t axis = LongestAxis(GetSize(centroid_bounds));

		auto first = std::begin(bvh->primitive_indices) + begin;
		auto last = first + count;
		std::stable_sort(first, last, [&](uint32_t a, uint32_t b) {
			const float center_a = GetCenter(primitive_bounds[a])[axis];
			const float center_b = GetCenter(primitive_bounds[b])[axis];
			return center_a != center_b ? center_a < center_b : a < b;
		});

		const uint32_t left_count = count / 2;
		const uint32_t right_count = count - left_count;
		const uint32_t left_child = uint32_t(bvh->nodes.size());
		bvh->nodes.resize(bvh->nodes.size() + 2);
		BuildNode(left_child, begin, left_count);
		BuildNode(left_child + 1, begin + left_count, right_count);
		bvh->nodes[node_index] = {Union(bvh->nodes[left_child].bounds, bvh->nodes[left_child + 1].bounds), left_child, 0};
	}
};

template <typename T> bool ReadRawArray(const Reader &ir, const Handle &h, std::vector<T> &values, uint32_t count) {
	const size_t cursor = Tell(ir, h), size = ir.size(h);
	if (cursor > size || count > (size - cursor) / sizeof(T))
		return false;
	values.resize(count);
	return count == 0 || ir.read(h, values.data(), size_t(count) * sizeof(T)) == size_t(count) * sizeof(T);
}

template <typename T> bool WriteRawArray(const Writer &iw, const Handle &h, const std::vector<T> &values) {
	return values.empty() || iw.write(h, values.data(), values.size() * sizeof(T)) == values.size() * sizeof(T);
}

// Hierarchies built by BuildBVH are balanced and fit in the inline stack even
// at the uint32 primitive limit. The overflow path keeps traversal safe for a
// valid hierarchy authored by another builder.
template <typename T, size_t InlineCapacity = 64> class BVHTraversalStack {
public:
	bool Empty() const { return count == 0 && overflow.empty(); }

	void Push(const T &value) {
		if (!overflow.empty() || count == InlineCapacity)
			overflow.push_back(value);
		else
			inline_values[count++] = value;
	}

	T Pop() {
		if (!overflow.empty()) {
			const T value = overflow.back();
			overflow.pop_back();
			return value;
		}
		return inline_values[--count];
	}

private:
	std::array<T, InlineCapacity> inline_values;
	size_t count{0};
	std::vector<T> overflow;
};

} // namespace

bool BuildBVH(const MinMax *primitive_bounds, size_t primitive_count, BVH &bvh, uint32_t max_primitives_per_leaf) {
	bvh = {};
	if (primitive_count == 0)
		return true;
	if (primitive_bounds == nullptr || primitive_count > std::numeric_limits<uint32_t>::max() || max_primitives_per_leaf == 0)
		return false;
	for (size_t i = 0; i < primitive_count; ++i)
		if (!IsValidBounds(primitive_bounds[i]))
			return false;

	bvh.primitive_indices.resize(primitive_count);
	std::iota(std::begin(bvh.primitive_indices), std::end(bvh.primitive_indices), 0u);
	const size_t leaf_count = (primitive_count + max_primitives_per_leaf - 1) / max_primitives_per_leaf;
	if (leaf_count > (size_t(std::numeric_limits<uint32_t>::max()) + 1) / 2)
		return false;
	bvh.nodes.reserve(leaf_count * 2 - 1);
	bvh.nodes.resize(1);
	BVHBuilder{primitive_bounds, &bvh, max_primitives_per_leaf}.BuildNode(0, 0, uint32_t(primitive_count));
	return ValidateBVH(bvh, primitive_count);
}

bool ValidateBVH(const BVH &bvh, size_t primitive_count) {
	if (primitive_count == 0)
		return bvh.nodes.empty() && bvh.primitive_indices.empty();
	if (primitive_count > std::numeric_limits<uint32_t>::max() || bvh.nodes.empty() || bvh.primitive_indices.size() != primitive_count)
		return false;

	std::vector<uint8_t> seen_primitives(primitive_count, 0);
	for (const uint32_t primitive_index : bvh.primitive_indices) {
		if (primitive_index >= primitive_count || seen_primitives[primitive_index] != 0)
			return false;
		seen_primitives[primitive_index] = 1;
	}

	std::vector<uint8_t> seen_nodes(bvh.nodes.size(), 0), covered_slots(primitive_count, 0);
	std::vector<uint32_t> stack = {0};
	while (!stack.empty()) {
		const uint32_t node_index = stack.back();
		stack.pop_back();
		if (node_index >= bvh.nodes.size() || seen_nodes[node_index] != 0)
			return false;
		seen_nodes[node_index] = 1;

		const BVHNode &node = bvh.nodes[node_index];
		if (!IsValidBounds(node.bounds))
			return false;
		if (node.IsLeaf()) {
			if (node.first > primitive_count || node.count > primitive_count - node.first)
				return false;
			for (uint32_t i = 0; i < node.count; ++i) {
				if (covered_slots[node.first + i] != 0)
					return false;
				covered_slots[node.first + i] = 1;
			}
		} else {
			if (node.first >= bvh.nodes.size() || node.first + 1 >= bvh.nodes.size())
				return false;
			const BVHNode &left = bvh.nodes[node.first], &right = bvh.nodes[node.first + 1];
			if (!ContainsBounds(node.bounds, left.bounds) || !ContainsBounds(node.bounds, right.bounds))
				return false;
			stack.push_back(node.first + 1);
			stack.push_back(node.first);
		}
	}

	return std::find(std::begin(seen_nodes), std::end(seen_nodes), 0) == std::end(seen_nodes) &&
		std::find(std::begin(covered_slots), std::end(covered_slots), 0) == std::end(covered_slots);
}

bool TraverseBVH(const BVH &bvh, const MinMax &bounds, const BVHPrimitiveVisitor &visitor) {
	if (bvh.nodes.empty() || !IsValidBounds(bounds) || !visitor || !Overlap(bvh.nodes[0].bounds, bounds))
		return true;

	BVHTraversalStack<uint32_t> stack;
	stack.Push(0);
	while (!stack.Empty()) {
		const uint32_t node_index = stack.Pop();
		const BVHNode &node = bvh.nodes[node_index];
		if (!Overlap(node.bounds, bounds))
			continue;
		if (node.IsLeaf()) {
			for (uint32_t i = 0; i < node.count; ++i)
				if (!visitor(bvh.primitive_indices[node.first + i]))
					return false;
		} else {
			stack.Push(node.first + 1);
			stack.Push(node.first);
		}
	}
	return true;
}

bool TraverseBVHRay(
	const BVH &bvh, const Vec3 &origin, const Vec3 &direction, float &max_distance, const BVHPrimitiveVisitor &visitor) {
	if (bvh.nodes.empty() || max_distance < 0.f || !visitor)
		return true;

	float root_near = 0.f, root_far = 0.f;
	if (!IntersectRay(bvh.nodes[0].bounds, origin, direction, root_near, root_far) || root_near > max_distance)
		return true;

	struct StackEntry {
		uint32_t node;
		float near_distance;
	};
	BVHTraversalStack<StackEntry> stack;
	stack.Push({0, root_near});
	while (!stack.Empty()) {
		const StackEntry entry = stack.Pop();
		if (entry.near_distance > max_distance)
			continue;

		const BVHNode &node = bvh.nodes[entry.node];
		if (node.IsLeaf()) {
			for (uint32_t i = 0; i < node.count; ++i)
				if (!visitor(bvh.primitive_indices[node.first + i]))
					return false;
			continue;
		}

		StackEntry hits[2];
		uint32_t hit_count = 0;
		for (uint32_t i = 0; i < 2; ++i) {
			float near_distance = 0.f, far_distance = 0.f;
			const uint32_t child = node.first + i;
			if (IntersectRay(bvh.nodes[child].bounds, origin, direction, near_distance, far_distance) && near_distance <= max_distance)
				hits[hit_count++] = {child, near_distance};
		}
		if (hit_count == 2 && hits[0].near_distance <= hits[1].near_distance)
			std::swap(hits[0], hits[1]);
		for (uint32_t i = 0; i < hit_count; ++i)
			stack.Push(hits[i]); // farther first, nearest is popped first
	}
	return true;
}

bool LoadBVH(const Reader &ir, const Handle &h, BVH &bvh, size_t primitive_count) {
	bvh = {};
	if (!ir.is_valid(h) || primitive_count > std::numeric_limits<uint32_t>::max())
		return false;

	uint32_t node_count = 0, index_count = 0;
	if (!Read(ir, h, node_count))
		return false;
	const size_t max_node_count = primitive_count == 0 ? 0 : primitive_count * 2 - 1;
	if (node_count > max_node_count || !ReadRawArray(ir, h, bvh.nodes, node_count) || !Read(ir, h, index_count) ||
		index_count != primitive_count || !ReadRawArray(ir, h, bvh.primitive_indices, index_count)) {
		bvh = {};
		return false;
	}
	if (!ValidateBVH(bvh, primitive_count)) {
		bvh = {};
		return false;
	}
	return true;
}

bool SaveBVH(const Writer &iw, const Handle &h, const BVH &bvh, size_t primitive_count) {
	if (!iw.is_valid(h) || !ValidateBVH(bvh, primitive_count) || bvh.nodes.size() > std::numeric_limits<uint32_t>::max() ||
		bvh.primitive_indices.size() > std::numeric_limits<uint32_t>::max())
		return false;
	return Write(iw, h, uint32_t(bvh.nodes.size())) && WriteRawArray(iw, h, bvh.nodes) &&
		Write(iw, h, uint32_t(bvh.primitive_indices.size())) && WriteRawArray(iw, h, bvh.primitive_indices);
}

} // namespace hg
