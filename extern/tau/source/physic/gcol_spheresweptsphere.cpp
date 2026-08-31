/*

GCollide	Realtime collision library.
			Written by Emmanuel Julien.
			All rights reserved 2000~2005.

			Emmanuel Julien
			http://www.nengine.fr
			mailto:ejulien@nengine.fr

*/


		#include	"physic.h"
 

//-------------------------------------------------------------
int					GCollide::SphereSweptSphere
										(
											nVector &prv_pos_a,
											nVector &pos_a,
											float radius_a,
											nVector &prv_pos_b,
											nVector &pos_b,
											float radius_b,
											GColCtcInfo *cinfo
										)
//-------------------------------------------------------------
{
#if		1	// Discrete
	float		r = radius_a + radius_b,
				d2 = nVector::Dist2(pos_a, pos_b);

	if	(nMath::EqualZero(d2))
		return 0;

	if	(d2 >= r * r)
		return 0;

	if	(!cinfo || !cinfo->n || !cinfo->d)
		return 1;

	float	d = sqrt(d2);
	cinfo->n[0] = (pos_b - pos_a) / d;
	cinfo->d[0] = r - d;

	if	(!cinfo->p || !cinfo->t || !cinfo->cmax)
		return 1;

	cinfo->p[0] = pos_b - cinfo->n[0] * radius_b;
	cinfo->t[0] = r - d;
	return 1;
#else
	nVector		s = prv_pos_a - prv_pos_b,
				e = pos_a - pos_b,
				dt = e - s;
	float		r = radius_a + radius_b;

	//
	float		sl2 = s.Len2(),
				dl2 = dt.Len2();
	if	(nMath::EqualZero(dl2))
		return false;

	float		depth = 0;
	nVector		n;

	if	(sl2 < (r * r))		// Embedded...
	{
		if	(nMath::EqualZero(sl2))
		{
			if	(nMath::EqualZero(e.Len2()))
					n.Set(0, 1, 0);
			else	n = e.Reverse().Normalize();
		}
		else	n = s.Reverse().Normalize();
		depth = -(r - s.Len());
	}
	else
	{
		float		t[2];
		nVector		o(0, 0, 0);
		if	(!nTools::LineIntersectSphere(s, dt.Normalize(), o, r, t))
			return 0;
		if	((t[0] < 0.f) || (t[0] > 1.f))
			return 0;

		float		dl = dt.Len();
		n = dt / dl;
		depth = -dl * (1 - t[0]);		// Length from hit to end of line.
	}

	if	(!cinfo || !cinfo->n || !cinfo->d)
		return 1;

	cinfo->d[0] = depth;
	cinfo->n[0] = n.Reverse();

	if	(!cinfo->p || !cinfo->t || !cinfo->cmax)
		return 1;

	cinfo->p[0] = pos_b - n * radius_b;
	if	(cinfo->t[0])
		cinfo->t[0] = depth;
	return 1;
#endif
}
