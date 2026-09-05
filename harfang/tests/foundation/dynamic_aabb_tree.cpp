// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/dynamic_aabb_tree.h"

#include <algorithm>
#include <limits>
#include <vector>

using namespace hg;

namespace {

uint64_t PairKey(uint32_t a, uint32_t b) {
	if (a > b)
		std::swap(a, b);
	return (uint64_t(a) << 32) | b;
}

std::vector<uint64_t> CollectPairs(const DynamicAABBTree &tree) {
	std::vector<uint64_t> pairs;
	TEST_CHECK(tree.QueryOverlaps([&](DynamicAABBTreeProxy, uint32_t a, DynamicAABBTreeProxy, uint32_t b) {
		pairs.push_back(PairKey(a, b));
		return true;
	}));
	std::sort(std::begin(pairs), std::end(pairs));
	return pairs;
}

uint32_t NextRandom(uint32_t &state) {
	state = state * 1664525u + 1013904223u;
	return state;
}

float RandomRange(uint32_t &state, float mn, float mx) {
	return mn + float(NextRandom(state) & 0xffffffu) / float(0x1000000u) * (mx - mn);
}

void TestBasicDynamicAABBTreeLifecycle() {
	DynamicAABBTree tree({0.25f, 2.f});
	TEST_CHECK(tree.Validate());
	TEST_CHECK(tree.GetStats().proxy_count == 0);

	const MinMax initial(Vec3::Zero, Vec3::One);
	const DynamicAABBTreeProxy first = tree.Insert(initial, 10);
	TEST_ASSERT(first != InvalidDynamicAABBTreeProxy);
	TEST_CHECK(tree.IsValidProxy(first));
	TEST_CHECK(tree.IsMoved(first));
	TEST_CHECK(tree.GetProxyCount() == 1);
	TEST_CHECK(tree.GetMovedProxyCount() == 1);
	MinMax fat;
	TEST_ASSERT(tree.GetFatBounds(first, fat));
	TEST_CHECK(fat == MinMax(Vec3(-0.25f), Vec3(1.25f)));
	uint32_t user_data = 0;
	TEST_CHECK(tree.GetUserData(first, user_data));
	TEST_CHECK(user_data == 10);
	TEST_CHECK(tree.SetUserData(first, 11));
	TEST_CHECK(tree.GetUserData(first, user_data));
	TEST_CHECK(user_data == 11);

	size_t moved_count = 0;
	TEST_CHECK(tree.VisitMoved([&](DynamicAABBTreeProxy proxy, uint32_t data) {
		TEST_CHECK(proxy == first);
		TEST_CHECK(data == 11);
		++moved_count;
		return true;
	}));
	TEST_CHECK(moved_count == 1);
	tree.ClearMoved();
	TEST_CHECK(!tree.IsMoved(first));
	TEST_CHECK(tree.GetMovedProxyCount() == 0);

	// Coherent movement remains in the original fat bounds.
	TEST_CHECK(!tree.Update(first, MinMax(Vec3(0.05f), Vec3(1.05f)), Vec3(0.05f, 0.f, 0.f)));
	TEST_CHECK(!tree.IsMoved(first));
	TEST_CHECK(tree.GetFatBounds(first, fat));
	TEST_CHECK(fat == MinMax(Vec3(-0.25f), Vec3(1.25f)));

	// A large move reinserts and directionally extends the new fat bounds.
	TEST_CHECK(tree.Update(first, MinMax(Vec3(5.f), Vec3(6.f)), Vec3(2.f, 0.f, 0.f)));
	TEST_CHECK(tree.IsMoved(first));
	TEST_CHECK(tree.GetMovedProxyCount() == 1);
	TEST_CHECK(tree.GetFatBounds(first, fat));
	TEST_CHECK(fat == MinMax(Vec3(4.75f), Vec3(10.25f, 6.25f, 6.25f)));

	std::vector<uint32_t> hits;
	TEST_CHECK(tree.Query(MinMax(Vec3(5.25f), Vec3(5.75f)), [&](DynamicAABBTreeProxy, uint32_t data) {
		hits.push_back(data);
		return true;
	}));
	TEST_CHECK(hits.size() == 1 && hits[0] == 11);
	TEST_CHECK(tree.Query(MinMax(Vec3(-1.f), Vec3(1.f)), [&](DynamicAABBTreeProxy, uint32_t) {
		TEST_CHECK(false);
		return true;
	}));

	const DynamicAABBTreeProxy second = tree.Insert(MinMax(Vec3(5.5f), Vec3(6.5f)), 20);
	const DynamicAABBTreeProxy far_proxy = tree.Insert(MinMax(Vec3(20.f), Vec3(21.f)), 30);
	TEST_ASSERT(second != InvalidDynamicAABBTreeProxy && far_proxy != InvalidDynamicAABBTreeProxy);
	TEST_CHECK(tree.GetMovedProxyCount() == 3);
	const auto pairs = CollectPairs(tree);
	TEST_CHECK(pairs.size() == 1 && pairs[0] == PairKey(11, 20));
	TEST_CHECK(tree.Validate());

	TEST_CHECK(tree.Remove(second));
	TEST_CHECK(tree.GetMovedProxyCount() == 2);
	TEST_CHECK(!tree.IsValidProxy(second));
	TEST_CHECK(!tree.Remove(second));
	const DynamicAABBTreeProxy reused = tree.Insert(MinMax(Vec3(30.f), Vec3(31.f)), 40);
	TEST_CHECK(reused != InvalidDynamicAABBTreeProxy);
	TEST_CHECK(reused != second);
	TEST_CHECK(!tree.IsValidProxy(second));
	TEST_CHECK(tree.Validate());

	const float nan = std::numeric_limits<float>::quiet_NaN();
	TEST_CHECK(tree.Insert(MinMax(Vec3(nan, 0.f, 0.f), Vec3::One), 50) == InvalidDynamicAABBTreeProxy);
	TEST_CHECK(!tree.Update(first, MinMax(Vec3::One, Vec3::Zero)));
	TEST_CHECK(tree.Validate());

	const DynamicAABBTreeProxy before_clear = reused;
	tree.Clear();
	TEST_CHECK(tree.Validate());
	TEST_CHECK(tree.GetStats().proxy_count == 0);
	TEST_CHECK(tree.GetProxyCount() == 0);
	TEST_CHECK(tree.GetMovedProxyCount() == 0);
	const DynamicAABBTreeProxy after_clear = tree.Insert(initial, 60);
	TEST_CHECK(after_clear != InvalidDynamicAABBTreeProxy);
	TEST_CHECK(after_clear != before_clear);
	TEST_CHECK(!tree.IsValidProxy(before_clear));
	TEST_CHECK(tree.Validate());
}

void TestDynamicAABBTreeAgainstBruteForce() {
	static constexpr size_t Count = 128;
	DynamicAABBTree tree({0.05f, 1.5f});
	std::vector<DynamicAABBTreeProxy> proxies(Count, InvalidDynamicAABBTreeProxy);
	std::vector<MinMax> exact_bounds(Count);
	std::vector<bool> active(Count, true);
	uint32_t random = 0x544155u;

	for (uint32_t i = 0; i < Count; ++i) {
		const Vec3 center(RandomRange(random, -12.f, 12.f), RandomRange(random, -12.f, 12.f), RandomRange(random, -12.f, 12.f));
		const Vec3 half(RandomRange(random, 0.2f, 1.5f), RandomRange(random, 0.2f, 1.5f), RandomRange(random, 0.2f, 1.5f));
		exact_bounds[i] = {center - half, center + half};
		proxies[i] = tree.Insert(exact_bounds[i], i);
		TEST_ASSERT(proxies[i] != InvalidDynamicAABBTreeProxy);
	}
	TEST_CHECK(tree.Validate());

	for (uint32_t i = 0; i < Count; i += 3) {
		const Vec3 displacement(RandomRange(random, -1.5f, 1.5f), RandomRange(random, -1.5f, 1.5f), RandomRange(random, -1.5f, 1.5f));
		exact_bounds[i].mn += displacement;
		exact_bounds[i].mx += displacement;
		tree.Update(proxies[i], exact_bounds[i], displacement);
	}
	for (uint32_t i = 0; i < Count; i += 7) {
		TEST_CHECK(tree.Remove(proxies[i]));
		active[i] = false;
	}
	TEST_CHECK(tree.Validate());

	std::vector<MinMax> fat_bounds(Count);
	for (uint32_t i = 0; i < Count; ++i)
		if (active[i])
			TEST_CHECK(tree.GetFatBounds(proxies[i], fat_bounds[i]));

	std::vector<uint64_t> expected_pairs;
	for (uint32_t i = 0; i < Count; ++i)
		if (active[i])
			for (uint32_t j = i + 1; j < Count; ++j)
				if (active[j] && Overlap(fat_bounds[i], fat_bounds[j]))
					expected_pairs.push_back(PairKey(i, j));
	std::sort(std::begin(expected_pairs), std::end(expected_pairs));
	TEST_CHECK(CollectPairs(tree) == expected_pairs);

	for (uint32_t query_index = 0; query_index < Count; query_index += 11) {
		std::vector<uint32_t> expected_hits, tree_hits;
		for (uint32_t i = 0; i < Count; ++i)
			if (active[i] && Overlap(exact_bounds[query_index], fat_bounds[i]))
				expected_hits.push_back(i);
		TEST_CHECK(tree.Query(exact_bounds[query_index], [&](DynamicAABBTreeProxy, uint32_t data) {
			tree_hits.push_back(data);
			return true;
		}));
		std::sort(std::begin(tree_hits), std::end(tree_hits));
		TEST_CHECK(tree_hits == expected_hits);
	}

	const auto stats = tree.GetStats();
	TEST_CHECK(stats.proxy_count == Count - (Count + 6) / 7);
	TEST_CHECK(stats.node_count == stats.proxy_count * 2 - 1);
	TEST_CHECK(stats.height < 32);
	TEST_CHECK(stats.max_balance <= 1);
	TEST_CHECK(stats.area_ratio >= 1.f);

	// Rebuilding from the current fat bounds produces the same pair set.
	DynamicAABBTree replay({0.f, 0.f});
	for (uint32_t i = 0; i < Count; ++i)
		if (active[i])
			TEST_ASSERT(replay.Insert(fat_bounds[i], i) != InvalidDynamicAABBTreeProxy);
	TEST_CHECK(replay.Validate());
	TEST_CHECK(CollectPairs(replay) == expected_pairs);

	size_t visit_count = 0;
	TEST_CHECK(!tree.Query(MinMax(Vec3(-100.f), Vec3(100.f)), [&](DynamicAABBTreeProxy, uint32_t) {
		++visit_count;
		return false;
	}));
	TEST_CHECK(visit_count == 1);
}

} // namespace

void test_dynamic_aabb_tree() {
	TestBasicDynamicAABBTreeLifecycle();
	TestDynamicAABBTreeAgainstBruteForce();
}
