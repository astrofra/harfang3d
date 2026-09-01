// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/collision_geometry.h"

#include "engine/file_format.h"

#include "foundation/file_rw_interface.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>

namespace hg {

namespace {

static constexpr uint32_t k_collision_geometry_version = 0;

struct CollisionVertexLess {
	bool operator()(const Vec3 &a, const Vec3 &b) const {
		if (a.x != b.x)
			return a.x < b.x;
		if (a.y != b.y)
			return a.y < b.y;
		return a.z < b.z;
	}
};

struct CollisionEdgeLess {
	bool operator()(const CollisionEdge &a, const CollisionEdge &b) const {
		const CollisionVertexLess less;
		if (less(a.a, b.a))
			return true;
		if (less(b.a, a.a))
			return false;
		return less(a.b, b.b);
	}
};

CollisionEdge MakeCollisionEdge(Vec3 a, Vec3 b) {
	if (CollisionVertexLess{}(b, a))
		std::swap(a, b);
	return {a, b};
}

void PrepareCollisionGeometry(CollisionGeometry &geometry) {
	geometry.bounds = {};
	std::map<CollisionEdge, uint32_t, CollisionEdgeLess> edge_counts;
	for (const auto &triangle : geometry.triangles) {
		geometry.bounds = Union(geometry.bounds, triangle.a);
		geometry.bounds = Union(geometry.bounds, triangle.b);
		geometry.bounds = Union(geometry.bounds, triangle.c);
		++edge_counts[MakeCollisionEdge(triangle.a, triangle.b)];
		++edge_counts[MakeCollisionEdge(triangle.b, triangle.c)];
		++edge_counts[MakeCollisionEdge(triangle.c, triangle.a)];
	}

	geometry.boundary_edges.clear();
	for (const auto &edge : edge_counts)
		if (edge.second == 1)
			geometry.boundary_edges.push_back(edge.first);
}

} // namespace

uint32_t GetCollisionGeometryBinaryFormatVersion() { return k_collision_geometry_version; }

bool LoadCollisionGeometry(const Reader &ir, const Handle &h, CollisionGeometry &geometry) {
	geometry = {};
	if (!ir.is_valid(h))
		return false;

	uint32_t magic = 0, version = 0, triangle_count = 0;
	uint8_t marker = 0;
	if (!Read(ir, h, magic) || magic != HarfangMagic || !Read(ir, h, marker) || marker != CollisionGeometryMarker || !Read(ir, h, version) ||
		version > k_collision_geometry_version || !Read(ir, h, triangle_count))
		return false;

	const size_t cursor = Tell(ir, h), size = ir.size(h);
	if (cursor > size)
		return false;
	const size_t remaining = size - cursor;
	if (triangle_count > remaining / sizeof(CollisionTriangle))
		return false;

	geometry.triangles.resize(triangle_count);
	if (triangle_count != 0 && ir.read(h, geometry.triangles.data(), triangle_count * sizeof(CollisionTriangle)) !=
			triangle_count * sizeof(CollisionTriangle)) {
		geometry = {};
		return false;
	}

	PrepareCollisionGeometry(geometry);
	return !geometry.triangles.empty();
}

bool SaveCollisionGeometry(const Writer &iw, const Handle &h, const CollisionGeometry &geometry) {
	if (!iw.is_valid(h) || geometry.triangles.empty() || geometry.triangles.size() > std::numeric_limits<uint32_t>::max())
		return false;

	const uint32_t triangle_count = uint32_t(geometry.triangles.size());
	return Write(iw, h, HarfangMagic) && Write(iw, h, CollisionGeometryMarker) && Write(iw, h, k_collision_geometry_version) &&
		Write(iw, h, triangle_count) &&
		iw.write(h, geometry.triangles.data(), geometry.triangles.size() * sizeof(CollisionTriangle)) ==
			geometry.triangles.size() * sizeof(CollisionTriangle);
}

bool LoadCollisionGeometryFromFile(const char *path, CollisionGeometry &geometry) {
	return LoadCollisionGeometry(g_file_reader, ScopedReadHandle(g_file_read_provider, path, true), geometry);
}

bool SaveCollisionGeometryToFile(const char *path, const CollisionGeometry &geometry) {
	return SaveCollisionGeometry(g_file_writer, ScopedWriteHandle(g_file_write_provider, path), geometry);
}

} // namespace hg
