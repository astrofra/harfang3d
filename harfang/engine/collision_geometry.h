// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/minmax.h"
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
	// Transient topology derived while loading, not serialized.
	std::vector<CollisionEdge> boundary_edges;
};

uint32_t GetCollisionGeometryBinaryFormatVersion();

bool LoadCollisionGeometry(const Reader &ir, const Handle &h, CollisionGeometry &geometry);
bool SaveCollisionGeometry(const Writer &iw, const Handle &h, const CollisionGeometry &geometry);

bool LoadCollisionGeometryFromFile(const char *path, CollisionGeometry &geometry);
bool SaveCollisionGeometryToFile(const char *path, const CollisionGeometry &geometry);

} // namespace hg
