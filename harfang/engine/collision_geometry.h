// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/bvh.h"
#include "foundation/rw_interface.h"

#include <cstdint>
#include <vector>

namespace hg {

// Backend-neutral, cooked collision geometry.  The source geometry is
// triangulated by assetc; runtime physics backends only consume this compact
// representation and never need access to editable .geo files.
struct CollisionTriangle {
	Vec3 a, b, c;
};

struct CollisionEdge {
	Vec3 a, b;
};

struct CollisionGeometry {
	std::vector<CollisionTriangle> triangles;
	MinMax bounds;
	std::vector<CollisionEdge> boundary_edges;
	BVH triangle_bvh;
	BVH boundary_bvh;
};

uint32_t GetCollisionGeometryBinaryFormatVersion();

// Derive bounds and open-boundary topology, then build the reusable spatial
// indices. assetc calls this offline; version-0 resources use it as a runtime
// compatibility path.
bool PrepareCollisionGeometry(CollisionGeometry &geometry, uint32_t max_primitives_per_leaf = 8);

bool LoadCollisionGeometry(const Reader &ir, const Handle &h, CollisionGeometry &geometry);
bool SaveCollisionGeometry(const Writer &iw, const Handle &h, const CollisionGeometry &geometry);

bool LoadCollisionGeometryFromFile(const char *path, CollisionGeometry &geometry);
bool SaveCollisionGeometryToFile(const char *path, const CollisionGeometry &geometry);

} // namespace hg
