/*

GCollide	Realtime collision library.
			Written by Emmanuel Julien.
			All rights reserved 2000~2005.

			Emmanuel Julien
			http://www.nengine.fr
			mailto:ejulien@nengine.fr

*/


		#include	"physic.h"
 

// @TODO Normalize the calling convention of the low-level API.

//-------------------------------------------------------------------------------------------------------
bool				GColShape::Raytrace(nVector &s, nVector &d, GColTraceResult &result, uint shape_mask)
//-------------------------------------------------------------------------------------------------------
{
	// Transform ray into shape space.
	nVector		local_s = s * imatrix, local_d;
	imatrix.ApplyRotation(&local_d, &d);

	// Perform trace.
	result.d = -1;
	result.shape = this;

	switch	(type)
	{
		default:
			return false;

		case	GColMesh:
		{
			if	(!(shape_mask & GColTraceMesh))
				return false;

			nGeometryBIHTrace	trace;
			mesh_tree->RaytraceGeometry(trace, local_s, local_d);

			if	(trace.ip == -1)
				return false;

			nGeometry	*g = mesh_tree->GetGeometry();
			nPolygon	*p = &g->pol[trace.ip];

			result.n = g->pol_normal[trace.ip];
			result.p = trace.s + trace.d * trace.i_t;
			result.m = g->GetMaterialFromIndex(p->material);
		}
		break;

		case	GColSphere:
		{
			if	(!(shape_mask & GColTraceSphere))
				return false;

			float		t[2];
			local_d = local_d.Normalize();
			if	(!nTools::LineIntersectSphere(local_s, local_d, nVector(0, 0, 0), scale.x, t))
				return false;
			if	((t[0] < 0) && (t[1] < 0))
				return false;
			
			// Select closest hit on the positive side of the ray.
			if	(t[0] > 0)
					result.p = local_s + local_d * t[0];
			else	result.p = local_s + local_d * t[1];

			result.n = result.p / scale.x;
		}
		break;

		case	GColCuboid:
		{
			if	(!(shape_mask & GColTraceCuboid))
				return false;

			nAABB		aabb(NULL, &scale);
			if	(!aabb.AABB_ClassifyLine(local_s, local_d, result.p, &result.n))
				return false;
		}
		break;
	}

	// Transform hit point back into world space.
	result.p *= matrix;
	result.n *= nMatrix3::FromMatrix4(matrix);
	result.d = nVector::Dist(s, result.p);
	if	(d.Dot(result.p - s) < 0.f)
		result.d *= -1.f;
	return true;
}

//---------------------------------------------------------------------------------------------------------
bool				GColItem::Raytrace(nVector &from, nVector &d, GColTraceResult &result, uint shape_mask)
//---------------------------------------------------------------------------------------------------------
{
	result.d = -1;

	GColTraceResult		shape_result;
	nLinkedListForeach(GColShape, shape, shape_list)
		if	(shape->Raytrace(from, d, shape_result, shape_mask))
			if	((result.d < 0) || (shape_result.d < result.d))
				result = shape_result;

	return (result.d < 0.f) ? false : true;
}

//--------------------------------------------------------------------------------------------------------------------
bool				GCollide::Raytrace(nVector &from, nVector &d, GColTraceResult &result, uint mask, uint shape_mask)
//--------------------------------------------------------------------------------------------------------------------
{
	result.d = -1;

	GColTraceResult		item_result;
	nLinkedListForeach(GColItem, item, item_list)
		if	(item->self_mask & mask)
			if	(item->Raytrace(from, d, item_result, shape_mask))
				if	((result.d < 0) || (item_result.d < result.d))
					result = item_result;

	return (result.d < 0.f) ? false : true;
}

