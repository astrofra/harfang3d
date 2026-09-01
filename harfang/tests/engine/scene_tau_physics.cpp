// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/collision_geometry.h"
#include "engine/file_format.h"
#include "engine/scene.h"
#include "engine/scene_physics.h"
#include "engine/scene_tau_physics.h"

#include "foundation/data.h"
#include "foundation/data_rw_interface.h"
#include "foundation/file.h"

#include "../utils.h"

using namespace hg;

namespace {

Node CreateTauRaycastPrimitive(Scene &scene, CollisionType type, const Vec3 &position, float radius = 0.5f, float height = 1.f) {
	Node node = scene.CreateNode();
	node.SetTransform(scene.CreateTransform(TranslationMat4(position)));
	auto rigid_body = scene.CreateRigidBody();
	rigid_body.SetType(RBT_Dynamic);
	node.SetRigidBody(rigid_body);
	auto collision = scene.CreateCollision();
	collision.SetType(type);
	collision.SetMass(0.f);
	if (type == CT_Cube)
		collision.SetSize(Vec3::One);
	else {
		collision.SetRadius(radius);
		collision.SetHeight(height);
	}
	node.SetCollision(0, collision);
	return node;
}

void TestPreTickCallback() {
	Scene scene;
	const Node cube = CreatePhysicCube(scene, Vec3::One, TranslationMat4(Vec3(0.f, 2.f, 0.f)), InvalidModelRef, {}, 1.f);

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	int callback_count = 0;
	time_ns callback_dt = 0;
	SceneTauPhysics *callback_physics = nullptr;
	physics.SetPreTickCallback([&](SceneTauPhysics &ph, time_ns dt) {
		++callback_count;
		callback_dt = dt;
		callback_physics = &ph;
		ph.NodeSetLinearVelocity(cube, Vec3(0.f, 10.f, 0.f));
	});

	physics.StepSimulation(time_from_ms(32), time_from_ms(16), 8);
	TEST_CHECK(callback_count == 2);
	TEST_CHECK(callback_physics == &physics);
	TEST_CHECK(callback_dt >= time_from_ms(15) && callback_dt <= time_from_ms(17));
	TEST_CHECK(physics.NodeGetLinearVelocity(cube).y > 9.f);

	physics.SetPreTickCallback({});
	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 8);
	TEST_CHECK(callback_count == 2);
}

void TestScenePhysicsPreTickAdapter() {
	ScenePhysics physics;
	int callback_count = 0;
	ScenePhysics *callback_physics = nullptr;
	physics.SetPreTickCallback([&](ScenePhysics &ph, time_ns) {
		++callback_count;
		callback_physics = &ph;
	});

	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 8);
	TEST_CHECK(callback_count == 1);
	TEST_CHECK(callback_physics == &physics);
}

void TestRaycastVariousCollisionShapes() {
	Scene scene;
	const Node sphere = CreateTauRaycastPrimitive(scene, CT_Sphere, Vec3(2.f, 1.f, 2.5f));
	const Node cube = CreateTauRaycastPrimitive(scene, CT_Cube, Vec3(0.f, 1.f, 2.5f));
	const Node capsule = CreateTauRaycastPrimitive(scene, CT_Capsule, Vec3(-2.f, 1.f, 2.5f));
	const Node cone = CreateTauRaycastPrimitive(scene, CT_Cone, Vec3(-4.f, 1.f, 2.5f));
	const Node cylinder = CreateTauRaycastPrimitive(scene, CT_Cylinder, Vec3(-6.f, 1.f, 2.5f));

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	TEST_CHECK(physics.NodeHasBody(sphere));
	TEST_CHECK(physics.NodeHasBody(cube));
	TEST_CHECK(physics.NodeHasBody(capsule));
	TEST_CHECK(physics.NodeHasBody(cone));
	TEST_CHECK(physics.NodeHasBody(cylinder));

	struct ExpectedHit {
		Node node;
		float x;
		float z;
	};
	const ExpectedHit expected[] = {
		{sphere, 2.f, 2.0669873f}, {cube, 0.f, 2.f}, {capsule, -2.f, 2.f}, {cone, -4.f, 2.0802786f}, {cylinder, -6.f, 2.f}};
	for (const auto &item : expected) {
		const RaycastOut hit = physics.RaycastFirstHit(scene, Vec3(item.x, 0.75f, -5.f), Vec3(item.x, 0.75f, 10.f));
		TEST_CHECK(hit.node == item.node);
		TEST_CHECK(Abs(hit.P.x - item.x) < 0.0001f);
		TEST_CHECK(Abs(hit.P.y - 0.75f) < 0.0001f);
		TEST_CHECK_(Abs(hit.P.z - item.z) < 0.001f, "shape x %.3f: expected z %.6f, got %.6f", item.x, item.z, hit.P.z);
		TEST_CHECK_(Abs(hit.t - (item.z + 5.f)) < 0.001f, "shape x %.3f: expected t %.6f, got %.6f", item.x, item.z + 5.f, hit.t);
		TEST_CHECK(hit.N.z < -0.8f);
	}

	const RaycastOut miss = physics.RaycastFirstHit(scene, Vec3(4.f, 0.75f, -5.f), Vec3(4.f, 0.75f, 10.f));
	TEST_CHECK(!miss.node.IsValid());
}

void TestRaycastAllHitsAreStableAndBounded() {
	Scene scene;
	const Node near_node = CreatePhysicCube(scene, Vec3::One, TranslationMat4(Vec3(0.f, 0.f, -2.f)), InvalidModelRef, {}, 0.f);
	const Node middle_node = CreatePhysicCube(scene, Vec3::One, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const Node far_node = CreatePhysicCube(scene, Vec3::One, TranslationMat4(Vec3(0.f, 0.f, 2.f)), InvalidModelRef, {}, 0.f);

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	const auto hits = physics.RaycastAllHits(scene, Vec3(0.f, 0.f, -5.f), Vec3(0.f, 0.f, 5.f));
	TEST_CHECK(hits.size() == 3);
	if (hits.size() == 3) {
		TEST_CHECK(hits[0].node == near_node);
		TEST_CHECK(hits[1].node == middle_node);
		TEST_CHECK(hits[2].node == far_node);
		TEST_CHECK(hits[0].t < hits[1].t && hits[1].t < hits[2].t);
	}

	const auto bounded_hits = physics.RaycastAllHits(scene, Vec3(0.f, 0.f, -5.f), Vec3(0.f, 0.f, -3.f));
	TEST_CHECK(bounded_hits.empty());
	const auto zero_length_hits = physics.RaycastAllHits(scene, Vec3::Zero, Vec3::Zero);
	TEST_CHECK(zero_length_hits.empty());
}

void TestMeshColliderRaycast() {
	CollisionGeometry source;
	source.triangles = {
		{Vec3(-2.f, 0.f, -1.f), Vec3(2.f, 0.f, 1.f), Vec3(2.f, 0.f, -1.f)},
		{Vec3(-2.f, 0.f, -1.f), Vec3(-2.f, 0.f, 1.f), Vec3(2.f, 0.f, 1.f)},
	};

	Data serialized;
	TEST_ASSERT(SaveCollisionGeometry(g_data_writer, DataWriteHandle(serialized), source));
	serialized.Rewind();
	CollisionGeometry round_trip;
	TEST_ASSERT(LoadCollisionGeometry(g_data_reader, DataReadHandle(serialized), round_trip));
	TEST_CHECK(round_trip.triangles.size() == source.triangles.size());
	TEST_CHECK(round_trip.bounds.mn == Vec3(-2.f, 0.f, -1.f));
	TEST_CHECK(round_trip.bounds.mx == Vec3(2.f, 0.f, 1.f));
	TEST_CHECK(ValidateBVH(round_trip.triangle_bvh, round_trip.triangles.size()));
	TEST_CHECK(ValidateBVH(round_trip.boundary_bvh, round_trip.boundary_edges.size()));
	TEST_CHECK(round_trip.boundary_edges.size() == 4);

	// Version 0 contained triangles only. Keep loading it by building the same
	// reusable acceleration structures at runtime for backward compatibility.
	Data legacy_serialized;
	const DataWriteHandle legacy_write(legacy_serialized);
	TEST_ASSERT(Write(g_data_writer, legacy_write, HarfangMagic));
	TEST_ASSERT(Write(g_data_writer, legacy_write, CollisionGeometryMarker));
	TEST_ASSERT(Write(g_data_writer, legacy_write, uint32_t(0)));
	TEST_ASSERT(Write(g_data_writer, legacy_write, uint32_t(source.triangles.size())));
	TEST_ASSERT(g_data_writer.write(legacy_write, source.triangles.data(), source.triangles.size() * sizeof(CollisionTriangle)) ==
		source.triangles.size() * sizeof(CollisionTriangle));
	legacy_serialized.Rewind();
	CollisionGeometry legacy_round_trip;
	TEST_ASSERT(LoadCollisionGeometry(g_data_reader, DataReadHandle(legacy_serialized), legacy_round_trip));
	TEST_CHECK(ValidateBVH(legacy_round_trip.triangle_bvh, legacy_round_trip.triangles.size()));

	const std::string temporary = test::CreateTempFilepath();
	Unlink(temporary.c_str());
	const std::string logical_resource = temporary + ".physics";
	const std::string cooked_resource = logical_resource + "_triangles";
	TEST_ASSERT(SaveCollisionGeometryToFile(cooked_resource.c_str(), source));

	Scene scene;
	Node mesh = scene.CreateNode();
	mesh.SetTransform(scene.CreateTransform(TranslationMat4(Vec3(2.f, 0.f, 3.f))));
	auto rigid_body = scene.CreateRigidBody();
	rigid_body.SetType(RBT_Kinematic);
	mesh.SetRigidBody(rigid_body);
	auto collision = scene.CreateCollision();
	collision.SetType(CT_Mesh);
	collision.SetCollisionResource(logical_resource);
	collision.SetMass(0.f);
	mesh.SetCollision(0, collision);

	SceneTauPhysics physics;
	physics.NodeCreatePhysicsFromFile(mesh);
	TEST_ASSERT(physics.NodeHasBody(mesh));

	const RaycastOut hit = physics.RaycastFirstHit(scene, Vec3(2.f, 1.f, 3.f), Vec3(2.f, -1.f, 3.f));
	TEST_CHECK(hit.node == mesh);
	TEST_CHECK(Abs(hit.P.y) < 0.0001f);
	TEST_CHECK(Abs(hit.t - 1.f) < 0.0001f);
	TEST_CHECK(hit.N.y > 0.99f);

	const RaycastOut miss = physics.RaycastFirstHit(scene, Vec3(5.f, 1.f, 3.f), Vec3(5.f, -1.f, 3.f));
	TEST_CHECK(!miss.node.IsValid());
	const RaycastOut open_boundary = physics.RaycastFirstHit(scene, Vec3(4.f, 1.f, 3.f), Vec3(4.f, -1.f, 3.f));
	TEST_CHECK(!open_boundary.node.IsValid());

	// A kinematic body's scene transform is authoritative. After synchronization,
	// the mesh's long X axis is rotated onto Z and the former miss becomes a hit.
	const Vec3 rotated_probe(2.f, 1.f, 4.5f);
	TEST_CHECK(!physics.RaycastFirstHit(scene, rotated_probe, Vec3(rotated_probe.x, -1.f, rotated_probe.z)).node.IsValid());
	mesh.GetTransform().SetRot(Vec3(0.f, Deg(90.f), 0.f));
	physics.SyncTransformsFromScene(scene);
	const RaycastOut rotated_hit = physics.RaycastFirstHit(scene, rotated_probe, Vec3(rotated_probe.x, -1.f, rotated_probe.z));
	TEST_CHECK(rotated_hit.node == mesh);
	TEST_CHECK(Abs(rotated_hit.P.y) < 0.0001f);
	TEST_CHECK(rotated_hit.N.y > 0.99f);

	physics.NodeDestroyPhysics(mesh);
	TEST_CHECK(physics.GarbageCollectResources() == 1);
	Unlink(cooked_resource.c_str());
}

} // namespace

void test_scene_tau_physics() {
	TestPreTickCallback();
	TestScenePhysicsPreTickAdapter();
	TestRaycastVariousCollisionShapes();
	TestRaycastAllHitsAreStableAndBounded();
	TestMeshColliderRaycast();
}
