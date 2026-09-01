// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/scene.h"
#include "engine/scene_tau_physics_contact.h"

#include "foundation/math.h"
#include "foundation/matrix3.h"

#include <array>

using namespace hg;

namespace {

void CheckManifoldOrientation(const OBB &a, const OBB &b, const TauContactManifold &manifold) {
	TEST_CHECK(manifold.point_count >= 1);
	TEST_CHECK(manifold.point_count <= 4);
	TEST_CHECK(Dot(manifold.normal, b.pos - a.pos) >= -0.00001f);
	TEST_CHECK(AlmostEqual(Len(manifold.normal), 1.f, 0.0001f));
	for (uint8_t i = 0; i < manifold.point_count; ++i) {
		const Vec3 anchor_a = tau_internal::ObbLocalPointToWorld(a, manifold.points[i].local_point_a);
		const Vec3 anchor_b = tau_internal::ObbLocalPointToWorld(b, manifold.points[i].local_point_b);
		TEST_CHECK(Dot(anchor_b - anchor_a, manifold.normal) <= 0.0011f);
		TEST_CHECK(manifold.points[i].penetration >= 0.f);
	}
}

} // namespace

void test_scene_tau_physics_contact() {
	{
		const OBB a(Vec3::Zero, Vec3(2.f));
		const OBB b(Vec3(1.5f, 0.f, 0.f), Vec3(2.f));
		TauContactManifold manifold;
		TEST_CHECK(tau_internal::ComputeObbContactManifold(a, b, manifold));
		TEST_CHECK(manifold.feature.type == TauContactFeatureType::FaceA);
		TEST_CHECK(manifold.feature.axis_a == 0);
		TEST_CHECK(manifold.point_count == 4);
		TEST_CHECK(Dot(manifold.normal, Vec3::Right) > 0.9999f);
		CheckManifoldOrientation(a, b, manifold);

		TauContactManifold moved;
		TEST_CHECK(tau_internal::ComputeObbContactManifold(a, OBB(Vec3(1.49f, 0.f, 0.f), Vec3(2.f)), moved));
		TEST_CHECK(moved.point_count == manifold.point_count);
		for (uint8_t i = 0; i < manifold.point_count; ++i)
			TEST_CHECK(moved.points[i].feature_id == manifold.points[i].feature_id);
	}
	{
		const OBB a(Vec3::Zero, Vec3(4.f, 2.8f, 2.2f));
		const Mat3 rotation(Vec3(0.7017353f, 0.4921312f, 0.5151451f), Vec3(-0.3864419f, 0.8703914f, -0.3050925f),
			Vec3(-0.5985234f, 0.0150205f, 0.8009645f));
		const OBB b(Vec3(-1.4057981f, -0.9909690f, -0.6104418f), Vec3(0.6f, 3.6f, 1.6f), rotation);
		TauContactManifold manifold;
		TEST_CHECK(tau_internal::ComputeObbContactManifold(a, b, manifold));
		TEST_CHECK(manifold.feature.type == TauContactFeatureType::FaceB);
		TEST_CHECK(manifold.feature.axis_b == 0);
		CheckManifoldOrientation(a, b, manifold);
	}
	{
		const OBB a(Vec3::Zero, Vec3(4.f, 0.6f, 1.f));
		const Mat3 rotation(Vec3(0.8996041f, -0.2106538f, 0.3825407f), Vec3(0.1625134f, 0.9745394f, 0.1544745f),
			Vec3(-0.4053416f, -0.0767979f, 0.9109337f));
		const OBB b(Vec3(0.8512964f, 0.3533990f, 0.7843592f), Vec3(3.4f, 0.5f, 0.8f), rotation);
		TauContactManifold manifold;
		TEST_CHECK(tau_internal::ComputeObbContactManifold(a, b, manifold));
		TEST_CHECK(manifold.feature.type == TauContactFeatureType::EdgeEdge);
		TEST_CHECK(manifold.feature.axis_a == 0);
		TEST_CHECK(manifold.feature.axis_b == 0);
		TEST_CHECK(manifold.point_count == 1);
		CheckManifoldOrientation(a, b, manifold);
	}
	{
		const OBB a(Vec3::Zero, Vec3(2.f));
		for (const float angle : std::array<float, 2>{-0.00005f, 0.00005f}) {
			const OBB b(Vec3(1.5f, 0.f, 0.f), Vec3(2.f), RotationMatZ(angle));
			TauContactManifold manifold;
			TEST_CHECK(tau_internal::ComputeObbContactManifold(a, b, manifold));
			TEST_CHECK(manifold.feature.type == TauContactFeatureType::FaceA);
			TEST_CHECK(manifold.feature.axis_a == 0);
			CheckManifoldOrientation(a, b, manifold);
		}
	}
}
