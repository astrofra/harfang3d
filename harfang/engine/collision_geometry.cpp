// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/collision_geometry.h"

#include "engine/file_format.h"

#include "foundation/file_rw_interface.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace hg {

namespace {

static constexpr uint32_t k_collision_geometry_version = 1;

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

bool CollisionEdgesEqual(const CollisionEdge &a, const CollisionEdge &b) {
	const CollisionEdgeLess less;
	return !less(a, b) && !less(b, a);
}

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

} // namespace

uint32_t GetCollisionGeometryBinaryFormatVersion() { return k_collision_geometry_version; }

bool PrepareCollisionGeometry(CollisionGeometry &geometry, uint32_t max_primitives_per_leaf) {
	geometry.bounds = {};
	geometry.boundary_edges.clear();
	geometry.triangle_bvh = {};
	geometry.boundary_bvh = {};
	if (geometry.triangles.empty() || geometry.triangles.size() > std::numeric_limits<uint32_t>::max())
		return false;

	std::vector<MinMax> triangle_bounds;
	triangle_bounds.reserve(geometry.triangles.size());
	std::vector<CollisionEdge> edges;
	if (geometry.triangles.size() > std::numeric_limits<size_t>::max() / 3)
		return false;
	edges.reserve(geometry.triangles.size() * 3);
	for (const auto &triangle : geometry.triangles) {
		MinMax bounds;
		bounds = Union(bounds, triangle.a);
		bounds = Union(bounds, triangle.b);
		bounds = Union(bounds, triangle.c);
		triangle_bounds.push_back(bounds);
		edges.push_back(MakeCollisionEdge(triangle.a, triangle.b));
		edges.push_back(MakeCollisionEdge(triangle.b, triangle.c));
		edges.push_back(MakeCollisionEdge(triangle.c, triangle.a));
	}

	std::sort(std::begin(edges), std::end(edges), CollisionEdgeLess{});
	for (size_t begin = 0; begin < edges.size();) {
		size_t end = begin + 1;
		while (end < edges.size() && CollisionEdgesEqual(edges[begin], edges[end]))
			++end;
		if (end == begin + 1)
			geometry.boundary_edges.push_back(edges[begin]);
		begin = end;
	}

	if (!BuildBVH(triangle_bounds, geometry.triangle_bvh, max_primitives_per_leaf))
		return false;
	geometry.bounds = geometry.triangle_bvh.nodes[0].bounds;

	std::vector<MinMax> boundary_bounds;
	boundary_bounds.reserve(geometry.boundary_edges.size());
	for (const auto &edge : geometry.boundary_edges)
		boundary_bounds.emplace_back(Min(edge.a, edge.b), Max(edge.a, edge.b));
	return BuildBVH(boundary_bounds, geometry.boundary_bvh, max_primitives_per_leaf);
}

bool LoadCollisionGeometry(const Reader &ir, const Handle &h, CollisionGeometry &geometry) {
	geometry = {};
	if (!ir.is_valid(h))
		return false;

	uint32_t magic = 0, version = 0, triangle_count = 0;
	uint8_t marker = 0;
	if (!Read(ir, h, magic) || magic != HarfangMagic || !Read(ir, h, marker) || marker != CollisionGeometryMarker || !Read(ir, h, version) ||
		version > k_collision_geometry_version || !Read(ir, h, triangle_count))
		return false;

	if (triangle_count == 0 || !ReadRawArray(ir, h, geometry.triangles, triangle_count)) {
		geometry = {};
		return false;
	}

	if (version == 0) {
		if (!PrepareCollisionGeometry(geometry)) {
			geometry = {};
			return false;
		}
		return true;
	}

	uint32_t boundary_edge_count = 0;
	const uint64_t max_boundary_edge_count = uint64_t(triangle_count) * 3;
	if (!Read(ir, h, boundary_edge_count) || boundary_edge_count > max_boundary_edge_count ||
		!ReadRawArray(ir, h, geometry.boundary_edges, boundary_edge_count) ||
		!LoadBVH(ir, h, geometry.triangle_bvh, geometry.triangles.size()) ||
		!LoadBVH(ir, h, geometry.boundary_bvh, geometry.boundary_edges.size())) {
		geometry = {};
		return false;
	}
	geometry.bounds = geometry.triangle_bvh.nodes[0].bounds;
	return true;
}

bool SaveCollisionGeometry(const Writer &iw, const Handle &h, const CollisionGeometry &geometry) {
	if (!iw.is_valid(h) || geometry.triangles.empty() || geometry.triangles.size() > std::numeric_limits<uint32_t>::max())
		return false;

	CollisionGeometry prepared;
	const CollisionGeometry *cooked = &geometry;
	if (!ValidateBVH(geometry.triangle_bvh, geometry.triangles.size()) ||
		!ValidateBVH(geometry.boundary_bvh, geometry.boundary_edges.size()) || geometry.triangle_bvh.nodes[0].bounds != geometry.bounds) {
		prepared.triangles = geometry.triangles;
		if (!PrepareCollisionGeometry(prepared))
			return false;
		cooked = &prepared;
	}
	if (cooked->boundary_edges.size() > std::numeric_limits<uint32_t>::max())
		return false;

	const uint32_t triangle_count = uint32_t(cooked->triangles.size());
	return Write(iw, h, HarfangMagic) && Write(iw, h, CollisionGeometryMarker) && Write(iw, h, k_collision_geometry_version) &&
		Write(iw, h, triangle_count) && WriteRawArray(iw, h, cooked->triangles) && Write(iw, h, uint32_t(cooked->boundary_edges.size())) &&
		WriteRawArray(iw, h, cooked->boundary_edges) && SaveBVH(iw, h, cooked->triangle_bvh, cooked->triangles.size()) &&
		SaveBVH(iw, h, cooked->boundary_bvh, cooked->boundary_edges.size());
}

bool LoadCollisionGeometryFromFile(const char *path, CollisionGeometry &geometry) {
	return LoadCollisionGeometry(g_file_reader, ScopedReadHandle(g_file_read_provider, path, true), geometry);
}

bool SaveCollisionGeometryToFile(const char *path, const CollisionGeometry &geometry) {
	return SaveCollisionGeometry(g_file_writer, ScopedWriteHandle(g_file_write_provider, path), geometry);
}

} // namespace hg
