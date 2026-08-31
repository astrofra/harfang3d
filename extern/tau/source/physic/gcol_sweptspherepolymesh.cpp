/*

GCollide	[Generic collision]

			Emmanuel Julien 2004.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//-------------------------------------------------------------------------------
float				ProjectPointOntoSegment(nVector *pip, nVector *a, nVector *b)
//-------------------------------------------------------------------------------
{
	nVector		apip(*pip - *a);
	nVector		segment(*b - *a);
	nVector		src(*pip);

	float	t = apip.Dot(segment) / segment.Dot(segment);

			if	(t <= 0.f)
			*pip = *a;
	else	if	(t >= 1.f)
			*pip = *b;
	else	*pip = segment * t + *a;

	return nVector::Dist2(src, *pip);
}

//------------------------------------------------------------------------------------
bool				GetClosestPointToPolygon(nGeometryBIH *tree, int ip, nVector *pip)
//------------------------------------------------------------------------------------
{
	nGeometry	*geo = tree->GetGeometry();
	nPolygon	&pol = geo->pol[ip];

	// Make sure point is in polygon.
	const nGeometryBIHAccel
				&ca = tree->GetRaytracingAccelerationStructure()[ip];

	// Initial gradient.
	float		pu = (*pip)[ca.cu] - geo->vtx[pol.bind[0]][ca.cu],
				pv = (*pip)[ca.cv] - geo->vtx[pol.bind[0]][ca.cv];

	#define		RAY_EPSILON		-0.000001f

	float		*k = ca.k;
	for	(int m = 1; m < (pol.vtx_count - 1); ++m)
	{
		float		u = pv * k[0] + pu * k[1], v, w;
		if	(u < RAY_EPSILON)
			goto next;
		v = pu * k[2] + pv * k[3];
		if	(v < RAY_EPSILON)
			goto next;
		w = 1 - u - v;
		if	(w < RAY_EPSILON)
			goto next;
		// Point in polygon.
		return true;
next:;
		k += 4;
	}

	// Find the closest location on polygon perimeter.
	nVector			pli = *pip, opip = *pip;
	float			bd = ProjectPointOntoSegment(&pli, &geo->vtx[pol.bind[0]], &geo->vtx[pol.bind[1]]);

	*pip = pli;
	for	(ushort itri = 1; itri < pol.vtx_count; itri++)
	{
		uint	nv = itri + 1;
		if	(nv == pol.vtx_count)
			nv = 0;

		pli = opip;
		float	d = ProjectPointOntoSegment(&pli, &geo->vtx[pol.bind[itri]], &geo->vtx[pol.bind[nv]]);
		if	(d < bd)
		{
			*pip = pli;
			bd = d;
		}
	}
	return false;
}

//-----------------------------------------------------------
bool				IntersectIntervalSpherePolygon
									(
										nVector *src,
										nVector *vel,
										float vel_len,
										float radius,
										nGeometryBIH *tree,
										uint ip,
										nVector *itr,
										float *depth
									)
//-----------------------------------------------------------
{
	float		dot = vel->Dot(tree->GetGeometry()->pol_normal[ip]);
	if	(dot > 0)
		return false;

#if		1


	nVector		s = *src + *vel;

	// Motion originate from back of polygon.
	float		t = tree->GetRaytracingAccelerationStructure()[ip].d + s.Dot(tree->GetGeometry()->pol_normal[ip]);
	if	(t < 0)
		return false;

	/*
		Sphere is embedded in plane and may be
		embedded in polygon.
	*/
	if	(t < radius)
	{
		*itr = tree->GetGeometry()->pol_normal[ip] * -t + s;

		if	(GetClosestPointToPolygon(tree, ip, itr))
		{
			*depth = radius - t;
			return true;
		}
		else
		{
			t = nVector::Dist(s, *itr);
			if	(t < radius)
			{
				*depth = radius - t;
				return true;
			}
		}
	}


#else

	nVector		&pn = tree->GetGeometry()->pol_normal[ip];
	float		d = tree->GetRaytracingAccelerationStructure()[ip].d;

	// Check motion origin.

	float		t = d + src->Dot(pn);
	if	(t < 0)
		return false;

	// Raytrace along velocity, from the closest sphere point to the plane.
	nVector		vn = *vel / vel_len;

	nVector		s = *src - pn * radius;

	t = d + s.Dot(pn);

	if	(t < 0.f)
	{
//		__LOG_N__ << "Embedded!\n";

		*itr = s + pn * t;
		if	(GetClosestPointToPolygon(tree, ip, itr))
		{
			*depth = -t;
			return true;
		}
/*
		float		st = (d + s.Dot(pn)) / vn.Dot(pn);

		nVector		i_plane = s + vn * st,
					i_poly = i_plane;

	GetClosestPointToPolygon(tree, ip, &i_poly);

				nVector		e = s + *vel;
				float		edp = d + e.Dot(pn);

				if	(edp < 0)
				{
					*itr = i_poly;
					*depth = -edp;
					return true;
				}
*/
	}
	else
	{

		float		st = (d + s.Dot(pn)) / vn.Dot(pn);

	if	(st > vel_len)
		return false;	// too far.

		// Plane intersection.
		nVector		i_plane = s + vn * st,
					i_poly = i_plane;

		if	(GetClosestPointToPolygon(tree, ip, &i_poly))
		{
	/*
			if	(st < 0)
			{
				*itr = i_poly;
				*depth = -st;
				return true;
			}
			else
	*/		{
				nVector		e = s + *vel;
				float		edp = d + e.Dot(pn);

				if	(edp < 0)
				{
					*itr = i_poly;
					*depth = -edp;
					return true;
				}
			}
		}
		else
		{
			float		t[2];
			if	(nTools::LineIntersectSphere(i_poly, vn.Inverse(), *src, radius, t))
			{
				*itr = i_poly;
				*depth = -(vel_len - t[0]);
				return true;
			}
		}
	}

#endif
	return false;
}

//----------------------------------------------------------
int					GCollide::SweptSpherePolygon
									(
										nVector &prv_pos,
										nVector &pos,
										float radius,
										uint ip,
										nGeometry &geometry,
										GColCtcInfo *cinfo
									)
//----------------------------------------------------------
{
	nVector			v = pos - prv_pos, i;
	float			d;

	nGeometryBIH	*tree = geometry.GetTree();
	if	(IntersectIntervalSpherePolygon(&prv_pos, &v, v.Len(), radius, tree, ip, &i, &d))
	{
		cinfo->n[0] = (i - prv_pos).Normalize();
		cinfo->p[0] = i;
		cinfo->d[0] = -d;
		cinfo->t[0] = -d;
		return 1;
	}
	return 0;
}
