// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/minmax.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace hg {

// Opaque generation-checked proxy returned by DynamicAABBTree::Insert.
using DynamicAABBTreeProxy = uint64_t;
static constexpr DynamicAABBTreeProxy InvalidDynamicAABBTreeProxy = ~DynamicAABBTreeProxy(0);

struct DynamicAABBTreeConfig {
	// Padding retained around every exact AABB. Small coherent movements stay
	// inside this fat AABB and do not require tree reinsertion.
	float fat_margin{0.1f};
	// Extend the fat AABB in the movement direction to reduce reinsertion for
	// consistently moving objects.
	float displacement_multiplier{2.f};
};

struct DynamicAABBTreeStats {
	size_t proxy_count{0};
	size_t node_count{0};
	size_t node_capacity{0};
	size_t moved_proxy_count{0};
	uint32_t height{0};
	uint32_t max_balance{0};
	float area_ratio{0.f};
};

using DynamicAABBTreeVisitor = std::function<bool(DynamicAABBTreeProxy proxy, uint32_t user_data)>;
using DynamicAABBTreePairVisitor = std::function<bool(DynamicAABBTreeProxy proxy_a, uint32_t user_data_a,
	DynamicAABBTreeProxy proxy_b, uint32_t user_data_b)>;

// Runtime dynamic bounding-volume hierarchy over caller-owned objects. The
// tree stores only fat AABBs and opaque uint32 user data; it has no dependency
// on scenes, physics, rendering, cooked assets, or the represented objects.
class DynamicAABBTree {
public:
	explicit DynamicAABBTree(const DynamicAABBTreeConfig &config = {});
	DynamicAABBTree(const DynamicAABBTree &) = delete;
	DynamicAABBTree &operator=(const DynamicAABBTree &) = delete;
	DynamicAABBTree(DynamicAABBTree &&) = default;
	DynamicAABBTree &operator=(DynamicAABBTree &&) = default;

	DynamicAABBTreeProxy Insert(const MinMax &bounds, uint32_t user_data, const Vec3 &displacement = Vec3::Zero);
	bool Remove(DynamicAABBTreeProxy proxy);

	// Return true when the fat AABB was replaced and the proxy reinserted. A
	// contained exact AABB returns false and keeps the current tree topology.
	bool Update(DynamicAABBTreeProxy proxy, const MinMax &bounds, const Vec3 &displacement = Vec3::Zero);

	bool IsValidProxy(DynamicAABBTreeProxy proxy) const;
	bool GetFatBounds(DynamicAABBTreeProxy proxy, MinMax &bounds) const;
	bool GetUserData(DynamicAABBTreeProxy proxy, uint32_t &user_data) const;
	bool SetUserData(DynamicAABBTreeProxy proxy, uint32_t user_data);

	bool IsMoved(DynamicAABBTreeProxy proxy) const;
	bool ClearMoved(DynamicAABBTreeProxy proxy);
	void ClearMoved();
	bool VisitMoved(const DynamicAABBTreeVisitor &visitor) const;

	// Return false when the visitor requests an early exit. QueryOverlaps emits
	// every unordered fat-AABB pair exactly once. Tree traversal order is not a
	// stable application order; deterministic consumers should sort by their
	// stable user IDs before processing.
	bool Query(const MinMax &bounds, const DynamicAABBTreeVisitor &visitor) const;
	bool QueryOverlaps(const DynamicAABBTreePairVisitor &visitor) const;

	bool Validate() const;
	DynamicAABBTreeStats GetStats() const;
	size_t GetProxyCount() const { return proxy_count; }
	size_t GetMovedProxyCount() const { return moved_proxy_count; }

	void Clear();
	const DynamicAABBTreeConfig &GetConfig() const { return config; }

private:
	struct Node {
		MinMax bounds;
		uint32_t parent{0xffffffff};
		uint32_t child_a{0xffffffff}, child_b{0xffffffff};
		uint32_t next{0xffffffff};
		uint32_t user_data{0};
		uint32_t generation{0};
		int32_t height{-1};
		bool moved{false};

		bool IsAllocated() const { return height >= 0; }
		bool IsLeaf() const { return IsAllocated() && child_a == 0xffffffff; }
	};

	uint32_t AllocateNode();
	void FreeNode(uint32_t node_index);
	void InsertLeaf(uint32_t leaf_index);
	void RemoveLeaf(uint32_t leaf_index);
	uint32_t Balance(uint32_t node_index);
	MinMax MakeFatBounds(const MinMax &bounds, const Vec3 &displacement) const;
	DynamicAABBTreeProxy MakeProxy(uint32_t node_index) const;
	uint32_t DecodeProxy(DynamicAABBTreeProxy proxy) const;

	DynamicAABBTreeConfig config;
	std::vector<Node> nodes;
	uint32_t root{0xffffffff};
	uint32_t free_list{0xffffffff};
	uint32_t generation_seed{0};
	size_t node_count{0};
	size_t proxy_count{0};
	size_t moved_proxy_count{0};
};

} // namespace hg
