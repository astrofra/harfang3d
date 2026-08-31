/*

GCollide	[Generic collision]
			Id: OBB/Polygon overlap.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"

		#define		OBBPOLY_MAXVTX		64


//-----------------------------------------------------------------
int			GCollide::OBBOverlapPolygon
										(
											nOBB &obb,
											nMatrix4 &mtx,
											nPolygon &p,
											nGeometry &g,
											GColCtcInfo *cinfo
										)
//-----------------------------------------------------------------
{
	if	(p.vtx_count > OBBPOLY_MAXVTX)
		__ERR__(__LOG_W__ << "polygon vertex count exceeds internal OBB SAT limit (n = " << p.vtx_count << ").\n", 0)

	float		pmn, pmx, omn, omx;
	nVector		axis;

	//-----------------------------------------------------------
	#define		PROJECT_ARRAY(ARRAY, N, MN, MX)\
	{\
		MN = MX = ARRAY[0].Dot(axis);\
		for	(n = 1; n < N; ++n)\
		{\
			const float	t = ARRAY[n].Dot(axis);\
					if	(t < MN)	MN = t;\
			else	if	(t > MX)	MX = t;\
		}\
	}
	#define		RECORD_OVERLAP(IDX)\
	{\
		if	((pmx < omn) || (pmn > omx))\
			return 0;\
		t[IDX] = (omn - pmx) > (pmn - omx) ? (omn - pmx) : (pmn - omx);\
		a[IDX] = axis;\
	}
	//-----------------------------------------------------------

	// Transform polygon in OBB space.
	nMatrix4	tmx((
						nMatrix4::FromMatrix3(obb.bb_rotation.Transpose()) *
						nMatrix4::TranslationMatrix(obb.bb_position.Reverse())
					) * mtx);

	nVector		tvt[OBBPOLY_MAXVTX];
	int			n;
	for	(n = 0; n < p.vtx_count; ++n)
		tvt[n] = g.vtx[p.bind[n]] * tmx;

	float		t[3 + 1 + 3 * OBBPOLY_MAXVTX];
	nVector		a[3 + 1 + 3 * OBBPOLY_MAXVTX];

	// OBB axis.
	//----------------------------------
	#define	PROJECT_POLYANDOBB_AXIS(AXS)\
	{\
		pmn = pmx = tvt[0].AXS;\
		for	(n = 1; n < p.vtx_count; ++n)\
		{\
			if	(tvt[n].AXS < pmn) pmn = tvt[n].AXS;\
			if	(tvt[n].AXS > pmx) pmx = tvt[n].AXS;\
		}\
		omn = -obb.bb_scale.AXS; omx = obb.bb_scale.AXS;\
	}
	//----------------------------------

	PROJECT_POLYANDOBB_AXIS(x)
	axis.Set(pmn < 0.f ? -1.f : 1.f, 0.f, 0.f);
	RECORD_OVERLAP(0)
	PROJECT_POLYANDOBB_AXIS(y)
	axis.Set(0.f, pmn < 0.f ? -1.f : 1.f, 0.f);
	RECORD_OVERLAP(1)
	PROJECT_POLYANDOBB_AXIS(z)
	axis.Set(0.f, 0.f, pmn < 0.f ? -1.f : 1.f);
	RECORD_OVERLAP(2)

	// Polygon axis.
	//-----------------------
	#define		ORIENT_AXIS\
	{\
		if	(pmn < 0)\
		{\
			axis = axis.Reverse();\
			pmn = -pmx; pmx = -pmn;\
		}\
	}
	#define		PROJECT_AABB\
	{\
		omx = scl.Dot(axis.Abs());\
		omn = -omx;\
	}
	//-----------------------
	nVector	scl = obb.bb_scale * 0.5f;

	axis = (tvt[2] - tvt[0]).Cross(tvt[1] - tvt[0]).Normalize();
	pmn = pmx = tvt[0].Dot(axis);
	ORIENT_AXIS
	PROJECT_AABB
	RECORD_OVERLAP(3)

	// Edge/edge.
	int			_id = 4, m;
	for	(m = 0; m < p.vtx_count; ++m)
	{
		int			mx = m + 1;
		if	(mx == p.vtx_count)
			mx = 0;
		nVector	pedg = (tvt[mx] - tvt[m]).Normalize();

		axis.Set(0, pedg.z, -pedg.y);	// X x edge
		float		l = axis.Len();
		if	(l > 0.000001f)
		{
			axis /= l;
			PROJECT_ARRAY(tvt, p.vtx_count, pmn, pmx)
			ORIENT_AXIS
			PROJECT_AABB
			RECORD_OVERLAP(_id + 0);
		}
		else	t[_id + 0] = -99999999.f;

		axis.Set(-pedg.z, 0, pedg.x);	// Y x edge
		l = axis.Len();
		if	(l > 0.000001f)
		{
			axis /= l;
			PROJECT_ARRAY(tvt, p.vtx_count, pmn, pmx)
			ORIENT_AXIS
			PROJECT_AABB
			RECORD_OVERLAP(_id + 1);
		}
		else	t[_id + 1] = -99999999.f;

		axis.Set(pedg.y, -pedg.x, 0);	// Z x edge
		l = axis.Len();
		if	(l > 0.000001f)
		{
			axis /= l;
			PROJECT_ARRAY(tvt, p.vtx_count, pmn, pmx)
			ORIENT_AXIS
			PROJECT_AABB
			RECORD_OVERLAP(_id + 2);
		}
		else	t[_id + 2] = -99999999.f;

		_id += 3;
	}

	// We are colliding and that's all we care about.
	if	(!cinfo || !cinfo->n || !cinfo->d)
		return 1;

	// Polygon normal as separation axis.
	uint		iaxs = 0;
	float		taxs = t[0];
	for	(n = 1; n < (4 + 3 * p.vtx_count); ++n)
		if	(t[n] > taxs)
		{
			iaxs = n;
			taxs = t[n];
		}

	cinfo->d[0] = t[iaxs] * -1.f;
	cinfo->n[0] = a[iaxs] * obb.bb_rotation;

	// We have a collision, we know the collision normal
	// and depth but don't care about contacts.
	if	(!cinfo->cmax || !cinfo->p)
		return 1;

	// Determine contact set.
	//--------------------------------------------------
	#define	SUTH_CLIP_AXIS(AXS, C, IN_OP, IN, INC, OUT)\
	{\
		uint	outc = 0;\
		for	(m = 0; m < INC; ++m)\
		{\
			if	(outc == cinfo->cmax - 1)\
			{\
				__LOG_W__ << "OBB/Poly contact overflow.\n";\
				break;\
			}\
			int			mx = m + 1;\
			if	(mx == INC)	mx = 0;\
			bool		bin = IN[m].AXS IN_OP C;\
			if	(bin)\
				OUT[outc++] = IN[m];\
			if	((IN[mx].AXS IN_OP C) == !bin)\
				OUT[outc++] = (IN[mx] - IN[m]) * ((C - IN[m].AXS) / (IN[mx].AXS - IN[m].AXS)) + IN[m];\
		}\
		INC = outc;\
	}
	//--------------------------------------------------

	nVector		out[OBBPOLY_MAXVTX];
	int			nctc = (int)p.vtx_count;

	SUTH_CLIP_AXIS(x, -scl.x, >, tvt, nctc, out)
	SUTH_CLIP_AXIS(x, scl.x, <, out, nctc, tvt)
	SUTH_CLIP_AXIS(y, -scl.y, >, tvt, nctc, out)
	SUTH_CLIP_AXIS(y, scl.y, <, out, nctc, tvt)
	SUTH_CLIP_AXIS(z, -scl.z, >, tvt, nctc, out)
	SUTH_CLIP_AXIS(z, scl.z, <, out, nctc, tvt)

	// Back transform contacts in world space.
	for	(n = 0; n < nctc; ++n)
	{
		cinfo->p[n] = tvt[n] * obb.bb_rotation + obb.bb_position;
		cinfo->t[n] = cinfo->d[0];	// TODO
	}
	return nctc;
}
