// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/dynamic_aabb_tree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace hg {

namespace {

static constexpr uint32_t InvalidNode = 0xffffffff;

bool IsValidBounds(const MinMax &bounds) {
	return std::isfinite(bounds.mn.x) && std::isfinite(bounds.mn.y) && std::isfinite(bounds.mn.z) && std::isfinite(bounds.mx.x) &&
		std::isfinite(bounds.mx.y) && std::isfinite(bounds.mx.z) && bounds.mn.x <= bounds.mx.x && bounds.mn.y <= bounds.mx.y &&
		bounds.mn.z <= bounds.mx.z;
}

bool IsFiniteVector(const Vec3 &value) { return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z); }

bool ContainsBounds(const MinMax &outer, const MinMax &inner) {
	return inner.mn.x >= outer.mn.x && inner.mn.y >= outer.mn.y && inner.mn.z >= outer.mn.z && inner.mx.x <= outer.mx.x &&
		inner.mx.y <= outer.mx.y && inner.mx.z <= outer.mx.z;
}

float SurfaceArea(const MinMax &bounds) {
	const Vec3 size = GetSize(bounds);
	return 2.f * (size.x * size.y + size.y * size.z + size.z * size.x);
}

template <typename T, size_t InlineCapacity = 64> class TraversalStack {
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

DynamicAABBTree::DynamicAABBTree(const DynamicAABBTreeConfig &config) : config(config) {
	if (!std::isfinite(this->config.fat_margin) || this->config.fat_margin < 0.f)
		this->config.fat_margin = 0.1f;
	if (!std::isfinite(this->config.displacement_multiplier) || this->config.displacement_multiplier < 0.f)
		this->config.displacement_multiplier = 2.f;
}

uint32_t DynamicAABBTree::AllocateNode() {
	uint32_t node_index;
	bool new_node = false;
	if (free_list != InvalidNode) {
		node_index = free_list;
		free_list = nodes[node_index].next;
	} else {
		if (nodes.size() >= std::numeric_limits<uint32_t>::max())
			return InvalidNode;
		node_index = uint32_t(nodes.size());
		nodes.emplace_back();
		new_node = true;
	}

	Node &node = nodes[node_index];
	if (new_node)
		node.generation = generation_seed;
	node.bounds = {};
	node.parent = InvalidNode;
	node.child_a = InvalidNode;
	node.child_b = InvalidNode;
	node.next = InvalidNode;
	node.user_data = 0;
	node.height = 0;
	node.moved = false;
	++node_count;
	return node_index;
}

void DynamicAABBTree::FreeNode(uint32_t node_index) {
	Node &node = nodes[node_index];
	node.parent = InvalidNode;
	node.child_a = InvalidNode;
	node.child_b = InvalidNode;
	node.next = free_list;
	node.height = -1;
	node.moved = false;
	++node.generation;
	free_list = node_index;
	--node_count;
}

MinMax DynamicAABBTree::MakeFatBounds(const MinMax &bounds, const Vec3 &displacement) const {
	const Vec3 margin(config.fat_margin);
	MinMax fat(bounds.mn - margin, bounds.mx + margin);
	if (!IsFiniteVector(displacement))
		return fat;

	const Vec3 extension = displacement * config.displacement_multiplier;
	for (int axis = 0; axis < 3; ++axis) {
		if (extension[axis] < 0.f)
			fat.mn[axis] += extension[axis];
		else
			fat.mx[axis] += extension[axis];
	}
	return fat;
}

DynamicAABBTreeProxy DynamicAABBTree::MakeProxy(uint32_t node_index) const {
	return (DynamicAABBTreeProxy(nodes[node_index].generation) << 32) | node_index;
}

uint32_t DynamicAABBTree::DecodeProxy(DynamicAABBTreeProxy proxy) const {
	if (proxy == InvalidDynamicAABBTreeProxy)
		return InvalidNode;
	const uint32_t node_index = uint32_t(proxy);
	const uint32_t generation = uint32_t(proxy >> 32);
	if (node_index >= nodes.size() || !nodes[node_index].IsLeaf() || nodes[node_index].generation != generation)
		return InvalidNode;
	return node_index;
}

DynamicAABBTreeProxy DynamicAABBTree::Insert(const MinMax &bounds, uint32_t user_data, const Vec3 &displacement) {
	if (!IsValidBounds(bounds))
		return InvalidDynamicAABBTreeProxy;

	const uint32_t leaf_index = AllocateNode();
	if (leaf_index == InvalidNode)
		return InvalidDynamicAABBTreeProxy;
	Node &leaf = nodes[leaf_index];
	leaf.bounds = MakeFatBounds(bounds, displacement);
	leaf.user_data = user_data;
	leaf.moved = true;
	InsertLeaf(leaf_index);
	++proxy_count;
	++moved_proxy_count;
	return MakeProxy(leaf_index);
}

bool DynamicAABBTree::Remove(DynamicAABBTreeProxy proxy) {
	const uint32_t leaf_index = DecodeProxy(proxy);
	if (leaf_index == InvalidNode)
		return false;
	if (nodes[leaf_index].moved)
		--moved_proxy_count;
	RemoveLeaf(leaf_index);
	FreeNode(leaf_index);
	--proxy_count;
	return true;
}

bool DynamicAABBTree::Update(DynamicAABBTreeProxy proxy, const MinMax &bounds, const Vec3 &displacement) {
	const uint32_t leaf_index = DecodeProxy(proxy);
	if (leaf_index == InvalidNode || !IsValidBounds(bounds))
		return false;

	Node &leaf = nodes[leaf_index];
	if (ContainsBounds(leaf.bounds, bounds)) {
		const Vec3 large_margin(config.fat_margin * 4.f);
		const MinMax shrink_limit(bounds.mn - large_margin, bounds.mx + large_margin);
		if (ContainsBounds(shrink_limit, leaf.bounds))
			return false;
	}

	RemoveLeaf(leaf_index);
	leaf.bounds = MakeFatBounds(bounds, displacement);
	InsertLeaf(leaf_index);
	if (!nodes[leaf_index].moved) {
		nodes[leaf_index].moved = true;
		++moved_proxy_count;
	}
	return true;
}

bool DynamicAABBTree::IsValidProxy(DynamicAABBTreeProxy proxy) const { return DecodeProxy(proxy) != InvalidNode; }

bool DynamicAABBTree::GetFatBounds(DynamicAABBTreeProxy proxy, MinMax &bounds) const {
	const uint32_t leaf_index = DecodeProxy(proxy);
	if (leaf_index == InvalidNode)
		return false;
	bounds = nodes[leaf_index].bounds;
	return true;
}

bool DynamicAABBTree::GetUserData(DynamicAABBTreeProxy proxy, uint32_t &user_data) const {
	const uint32_t leaf_index = DecodeProxy(proxy);
	if (leaf_index == InvalidNode)
		return false;
	user_data = nodes[leaf_index].user_data;
	return true;
}

bool DynamicAABBTree::SetUserData(DynamicAABBTreeProxy proxy, uint32_t user_data) {
	const uint32_t leaf_index = DecodeProxy(proxy);
	if (leaf_index == InvalidNode)
		return false;
	nodes[leaf_index].user_data = user_data;
	return true;
}

bool DynamicAABBTree::IsMoved(DynamicAABBTreeProxy proxy) const {
	const uint32_t leaf_index = DecodeProxy(proxy);
	return leaf_index != InvalidNode && nodes[leaf_index].moved;
}

bool DynamicAABBTree::ClearMoved(DynamicAABBTreeProxy proxy) {
	const uint32_t leaf_index = DecodeProxy(proxy);
	if (leaf_index == InvalidNode)
		return false;
	if (nodes[leaf_index].moved) {
		nodes[leaf_index].moved = false;
		--moved_proxy_count;
	}
	return true;
}

void DynamicAABBTree::ClearMoved() {
	for (Node &node : nodes)
		if (node.IsLeaf())
			node.moved = false;
	moved_proxy_count = 0;
}

bool DynamicAABBTree::VisitMoved(const DynamicAABBTreeVisitor &visitor) const {
	if (!visitor)
		return true;
	for (uint32_t node_index = 0; node_index < nodes.size(); ++node_index) {
		const Node &node = nodes[node_index];
		if (node.IsLeaf() && node.moved && !visitor(MakeProxy(node_index), node.user_data))
			return false;
	}
	return true;
}

bool DynamicAABBTree::Query(const MinMax &bounds, const DynamicAABBTreeVisitor &visitor) const {
	if (root == InvalidNode || !IsValidBounds(bounds) || !visitor || !Overlap(nodes[root].bounds, bounds))
		return true;

	TraversalStack<uint32_t> stack;
	stack.Push(root);
	while (!stack.Empty()) {
		const uint32_t node_index = stack.Pop();
		const Node &node = nodes[node_index];
		if (!Overlap(node.bounds, bounds))
			continue;
		if (node.IsLeaf()) {
			if (!visitor(MakeProxy(node_index), node.user_data))
				return false;
		} else {
			stack.Push(node.child_b);
			stack.Push(node.child_a);
		}
	}
	return true;
}

bool DynamicAABBTree::QueryOverlaps(const DynamicAABBTreePairVisitor &visitor) const {
	if (root == InvalidNode || !visitor)
		return true;

	struct NodePair {
		uint32_t a, b;
	};
	TraversalStack<NodePair> stack;
	stack.Push({root, root});
	while (!stack.Empty()) {
		const NodePair pair = stack.Pop();
		const Node &a = nodes[pair.a], &b = nodes[pair.b];
		if (pair.a == pair.b) {
			if (a.IsLeaf())
				continue;
			// Partition all pairs below a branch into left, right, and cross
			// subsets. Every unordered leaf pair therefore has one path.
			stack.Push({a.child_b, a.child_b});
			stack.Push({a.child_a, a.child_b});
			stack.Push({a.child_a, a.child_a});
			continue;
		}
		if (!Overlap(a.bounds, b.bounds))
			continue;
		if (a.IsLeaf() && b.IsLeaf()) {
			if (!visitor(MakeProxy(pair.a), a.user_data, MakeProxy(pair.b), b.user_data))
				return false;
			continue;
		}

		bool split_a = !a.IsLeaf() && b.IsLeaf();
		if (!a.IsLeaf() && !b.IsLeaf()) {
			const float area_a = SurfaceArea(a.bounds), area_b = SurfaceArea(b.bounds);
			split_a = a.height != b.height ? a.height > b.height : (area_a != area_b ? area_a > area_b : pair.a < pair.b);
		}
		if (split_a) {
			stack.Push({a.child_b, pair.b});
			stack.Push({a.child_a, pair.b});
		} else {
			stack.Push({pair.a, b.child_b});
			stack.Push({pair.a, b.child_a});
		}
	}
	return true;
}

void DynamicAABBTree::InsertLeaf(uint32_t leaf_index) {
	if (root == InvalidNode) {
		root = leaf_index;
		nodes[root].parent = InvalidNode;
		return;
	}

	const MinMax leaf_bounds = nodes[leaf_index].bounds;
	uint32_t sibling_index = root;
	while (!nodes[sibling_index].IsLeaf()) {
		const Node &sibling = nodes[sibling_index];
		const uint32_t child_a = sibling.child_a, child_b = sibling.child_b;
		const float area = SurfaceArea(sibling.bounds);
		const float combined_area = SurfaceArea(Union(sibling.bounds, leaf_bounds));
		const float branch_cost = 2.f * combined_area;
		const float inheritance_cost = 2.f * (combined_area - area);

		auto child_cost = [&](uint32_t child_index) {
			const Node &child = nodes[child_index];
			const float new_area = SurfaceArea(Union(leaf_bounds, child.bounds));
			return (child.IsLeaf() ? new_area : new_area - SurfaceArea(child.bounds)) + inheritance_cost;
		};
		const float cost_a = child_cost(child_a), cost_b = child_cost(child_b);
		if (branch_cost < cost_a && branch_cost < cost_b)
			break;
		sibling_index = cost_a != cost_b ? (cost_a < cost_b ? child_a : child_b) : std::min(child_a, child_b);
	}

	const uint32_t old_parent = nodes[sibling_index].parent;
	const uint32_t new_parent = AllocateNode();
	Node &parent = nodes[new_parent];
	parent.parent = old_parent;
	parent.bounds = Union(leaf_bounds, nodes[sibling_index].bounds);
	parent.height = nodes[sibling_index].height + 1;
	parent.child_a = sibling_index;
	parent.child_b = leaf_index;
	nodes[sibling_index].parent = new_parent;
	nodes[leaf_index].parent = new_parent;

	if (old_parent != InvalidNode) {
		Node &old = nodes[old_parent];
		if (old.child_a == sibling_index)
			old.child_a = new_parent;
		else
			old.child_b = new_parent;
	} else {
		root = new_parent;
	}

	uint32_t node_index = nodes[leaf_index].parent;
	while (node_index != InvalidNode) {
		node_index = Balance(node_index);
		Node &node = nodes[node_index];
		const Node &child_a = nodes[node.child_a], &child_b = nodes[node.child_b];
		node.height = 1 + std::max(child_a.height, child_b.height);
		node.bounds = Union(child_a.bounds, child_b.bounds);
		node_index = node.parent;
	}
}

void DynamicAABBTree::RemoveLeaf(uint32_t leaf_index) {
	if (leaf_index == root) {
		root = InvalidNode;
		nodes[leaf_index].parent = InvalidNode;
		return;
	}

	const uint32_t parent_index = nodes[leaf_index].parent;
	const uint32_t grand_parent = nodes[parent_index].parent;
	const uint32_t sibling_index = nodes[parent_index].child_a == leaf_index ? nodes[parent_index].child_b : nodes[parent_index].child_a;
	if (grand_parent != InvalidNode) {
		Node &grand = nodes[grand_parent];
		if (grand.child_a == parent_index)
			grand.child_a = sibling_index;
		else
			grand.child_b = sibling_index;
		nodes[sibling_index].parent = grand_parent;
		FreeNode(parent_index);

		uint32_t node_index = grand_parent;
		while (node_index != InvalidNode) {
			node_index = Balance(node_index);
			Node &node = nodes[node_index];
			const Node &child_a = nodes[node.child_a], &child_b = nodes[node.child_b];
			node.bounds = Union(child_a.bounds, child_b.bounds);
			node.height = 1 + std::max(child_a.height, child_b.height);
			node_index = node.parent;
		}
	} else {
		root = sibling_index;
		nodes[sibling_index].parent = InvalidNode;
		FreeNode(parent_index);
	}
	nodes[leaf_index].parent = InvalidNode;
}

uint32_t DynamicAABBTree::Balance(uint32_t node_index) {
	Node &a = nodes[node_index];
	if (a.IsLeaf() || a.height < 2)
		return node_index;

	const uint32_t b_index = a.child_a, c_index = a.child_b;
	Node &b = nodes[b_index], &c = nodes[c_index];
	const int32_t balance = c.height - b.height;
	if (balance > 1) {
		const uint32_t f_index = c.child_a, g_index = c.child_b;
		Node &f = nodes[f_index], &g = nodes[g_index];
		c.child_a = node_index;
		c.parent = a.parent;
		a.parent = c_index;
		if (c.parent != InvalidNode) {
			Node &parent = nodes[c.parent];
			if (parent.child_a == node_index)
				parent.child_a = c_index;
			else
				parent.child_b = c_index;
		} else {
			root = c_index;
		}

		if (f.height != g.height ? f.height > g.height : f_index < g_index) {
			c.child_b = f_index;
			a.child_b = g_index;
			g.parent = node_index;
			a.bounds = Union(b.bounds, g.bounds);
			c.bounds = Union(a.bounds, f.bounds);
			a.height = 1 + std::max(b.height, g.height);
			c.height = 1 + std::max(a.height, f.height);
		} else {
			c.child_b = g_index;
			a.child_b = f_index;
			f.parent = node_index;
			a.bounds = Union(b.bounds, f.bounds);
			c.bounds = Union(a.bounds, g.bounds);
			a.height = 1 + std::max(b.height, f.height);
			c.height = 1 + std::max(a.height, g.height);
		}
		return c_index;
	}

	if (balance < -1) {
		const uint32_t d_index = b.child_a, e_index = b.child_b;
		Node &d = nodes[d_index], &e = nodes[e_index];
		b.child_a = node_index;
		b.parent = a.parent;
		a.parent = b_index;
		if (b.parent != InvalidNode) {
			Node &parent = nodes[b.parent];
			if (parent.child_a == node_index)
				parent.child_a = b_index;
			else
				parent.child_b = b_index;
		} else {
			root = b_index;
		}

		if (d.height != e.height ? d.height > e.height : d_index < e_index) {
			b.child_b = d_index;
			a.child_a = e_index;
			e.parent = node_index;
			a.bounds = Union(c.bounds, e.bounds);
			b.bounds = Union(a.bounds, d.bounds);
			a.height = 1 + std::max(c.height, e.height);
			b.height = 1 + std::max(a.height, d.height);
		} else {
			b.child_b = e_index;
			a.child_a = d_index;
			d.parent = node_index;
			a.bounds = Union(c.bounds, d.bounds);
			b.bounds = Union(a.bounds, e.bounds);
			a.height = 1 + std::max(c.height, d.height);
			b.height = 1 + std::max(a.height, e.height);
		}
		return b_index;
	}

	return node_index;
}

bool DynamicAABBTree::Validate() const {
	std::vector<uint8_t> state(nodes.size(), 0);
	size_t free_count = 0;
	for (uint32_t node_index = free_list; node_index != InvalidNode; node_index = nodes[node_index].next) {
		if (node_index >= nodes.size() || state[node_index] != 0 || nodes[node_index].IsAllocated())
			return false;
		state[node_index] = 1;
		++free_count;
	}

	size_t allocated_count = 0, leaf_count = 0, moved_leaf_count = 0;
	if (root != InvalidNode) {
		if (root >= nodes.size() || nodes[root].parent != InvalidNode)
			return false;
		TraversalStack<uint32_t> stack;
		stack.Push(root);
		while (!stack.Empty()) {
			const uint32_t node_index = stack.Pop();
			if (node_index >= nodes.size() || state[node_index] != 0)
				return false;
			const Node &node = nodes[node_index];
			if (!node.IsAllocated() || !IsValidBounds(node.bounds))
				return false;
			state[node_index] = 2;
			++allocated_count;
			if (node.IsLeaf()) {
				if (node.height != 0 || node.child_b != InvalidNode)
					return false;
				++leaf_count;
				if (node.moved)
					++moved_leaf_count;
			} else {
				if (node.child_a >= nodes.size() || node.child_b >= nodes.size() || node.child_a == node.child_b ||
					nodes[node.child_a].parent != node_index || nodes[node.child_b].parent != node_index)
					return false;
				const Node &child_a = nodes[node.child_a], &child_b = nodes[node.child_b];
				if (node.height != 1 + std::max(child_a.height, child_b.height) || node.bounds != Union(child_a.bounds, child_b.bounds))
					return false;
				stack.Push(node.child_b);
				stack.Push(node.child_a);
			}
		}
	}

	if (allocated_count != node_count || leaf_count != proxy_count || moved_leaf_count != moved_proxy_count ||
		allocated_count + free_count != nodes.size())
		return false;
	return std::find(std::begin(state), std::end(state), 0) == std::end(state);
}

DynamicAABBTreeStats DynamicAABBTree::GetStats() const {
	DynamicAABBTreeStats stats;
	stats.proxy_count = proxy_count;
	stats.node_count = node_count;
	stats.node_capacity = nodes.capacity();
	stats.moved_proxy_count = moved_proxy_count;
	stats.height = root != InvalidNode ? uint32_t(nodes[root].height) : 0;
	float total_area = 0.f;
	for (const Node &node : nodes) {
		if (!node.IsAllocated())
			continue;
		total_area += SurfaceArea(node.bounds);
		if (!node.IsLeaf()) {
			stats.max_balance = std::max(stats.max_balance, uint32_t(std::abs(nodes[node.child_b].height - nodes[node.child_a].height)));
		}
	}
	const float root_area = root != InvalidNode ? SurfaceArea(nodes[root].bounds) : 0.f;
	stats.area_ratio = root_area > 0.f ? total_area / root_area : 0.f;
	return stats;
}

void DynamicAABBTree::Clear() {
	nodes.clear();
	++generation_seed;
	root = InvalidNode;
	free_list = InvalidNode;
	node_count = 0;
	proxy_count = 0;
	moved_proxy_count = 0;
}

} // namespace hg
