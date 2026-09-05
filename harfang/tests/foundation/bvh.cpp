// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/bvh.h"
#include "foundation/data.h"
#include "foundation/data_rw_interface.h"

#include <algorithm>
#include <vector>

using namespace hg;

void test_bvh() {
	BVH empty;
	TEST_CHECK(BuildBVH(nullptr, 0, empty));
	TEST_CHECK(ValidateBVH(empty, 0));

	const std::vector<MinMax> bounds = {
		{Vec3(1.f, -1.f, -1.f), Vec3(2.f, 1.f, 1.f)},
		{Vec3(3.f, -1.f, -1.f), Vec3(4.f, 1.f, 1.f)},
		{Vec3(5.f, -1.f, -1.f), Vec3(6.f, 1.f, 1.f)},
		{Vec3(1.f, 2.f, -1.f), Vec3(2.f, 3.f, 1.f)},
	};
	BVH bvh;
	TEST_ASSERT(BuildBVH(bounds, bvh, 1));
	TEST_CHECK(ValidateBVH(bvh, bounds.size()));
	TEST_CHECK(bvh.nodes.size() == bounds.size() * 2 - 1);
	TEST_CHECK(bvh.nodes[0].bounds == MinMax(Vec3(1.f, -1.f, -1.f), Vec3(6.f, 3.f, 1.f)));

	std::vector<uint32_t> overlaps;
	TEST_CHECK(TraverseBVH(bvh, MinMax(Vec3(3.25f, -0.5f, -0.5f), Vec3(3.75f, 0.5f, 0.5f)), [&](uint32_t primitive) {
		overlaps.push_back(primitive);
		return true;
	}));
	TEST_CHECK(overlaps.size() == 1);
	if (overlaps.size() == 1)
		TEST_CHECK(overlaps[0] == 1);

	std::vector<uint32_t> ray_candidates;
	float max_distance = 100.f;
	TEST_CHECK(TraverseBVHRay(bvh, Vec3::Zero, Vec3::Right, max_distance, [&](uint32_t primitive) {
		ray_candidates.push_back(primitive);
		if (primitive == 0)
			max_distance = 1.5f;
		return true;
	}));
	TEST_CHECK(ray_candidates.size() == 1);
	if (ray_candidates.size() == 1)
		TEST_CHECK(ray_candidates[0] == 0);

	uint32_t visit_count = 0;
	TEST_CHECK(!TraverseBVH(bvh, bvh.nodes[0].bounds, [&](uint32_t) {
		++visit_count;
		return false;
	}));
	TEST_CHECK(visit_count == 1);

	Data serialized;
	TEST_ASSERT(SaveBVH(g_data_writer, DataWriteHandle(serialized), bvh, bounds.size()));
	serialized.Rewind();
	BVH round_trip;
	TEST_ASSERT(LoadBVH(g_data_reader, DataReadHandle(serialized), round_trip, bounds.size()));
	TEST_CHECK(round_trip.nodes.size() == bvh.nodes.size());
	TEST_CHECK(round_trip.primitive_indices == bvh.primitive_indices);
	if (round_trip.nodes.size() == bvh.nodes.size())
		for (size_t i = 0; i < bvh.nodes.size(); ++i) {
			TEST_CHECK(round_trip.nodes[i].bounds == bvh.nodes[i].bounds);
			TEST_CHECK(round_trip.nodes[i].first == bvh.nodes[i].first);
			TEST_CHECK(round_trip.nodes[i].count == bvh.nodes[i].count);
		}

	BVH invalid = bvh;
	invalid.primitive_indices[0] = uint32_t(bounds.size());
	TEST_CHECK(!ValidateBVH(invalid, bounds.size()));
}
