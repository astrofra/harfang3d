/*

GCollide	[Generic collision]
			Id: OBB/Sphere swept test.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//----------------------------------------------------------------
void		ClosestPointToAABB(nVector &p, nVector &s, nVector &q)
//----------------------------------------------------------------
{
	q = p;
			if	(q.x < -s.x)
			q.x = -s.x;
	else	if	(q.x > s.x)
			q.x = s.x;
			if	(q.y < -s.y)
			q.y = -s.y;
	else	if	(q.y > s.y)
			q.y = s.y;
			if	(q.z < -s.z)
			q.z = -s.z;
	else	if	(q.z > s.z)
			q.z = s.z;
}

//----------------------------------------------------------------------
void		ComputeNearestExitNormal(nVector &p, nVector &s, nVector &n)
//----------------------------------------------------------------------
{
	nVector	nc[6];
	nc[0].Set(0, 0, 1);
	nc[1].Set(0, 1, 0);
	nc[2].Set(1, 0, 0);
	nc[3].Set(-1, 0, 0);
	nc[4].Set(0, -1, 0);
	nc[5].Set(0, 0, -1);

	if	(nMath::EqualZero(nVector::Dist2(p, s)))
		n.Set(0, 1, 0);
	else
	{
		nVector	pn = (p / s).Normalize();
		uint		bi = 0;
		float		bd = pn.Dot(nc[0]);

		for	(uint i = 1; i < 6; ++i)
		{
			float		d = pn.Dot(nc[i]);
			if	(d > bd)
			{
				bd = d;
				bi = i;
			}
		}
		n = nc[bi];
	}
}

//-------------------------------------------------------------
int			GCollide::OBBSweptSphere
										(
											nOBB &prv_obb,
											nOBB &obb,
											nVector &prv_pos,
											nVector &pos,
											float radius,
											GColCtcInfo *cinfo
										)
//-------------------------------------------------------------
{
	// Transform to box space.
	nMatrix4	prv_tmx	(
							nMatrix4::FromMatrix3(prv_obb.bb_rotation.Transpose()) *
							nMatrix4::TranslationMatrix(prv_obb.bb_position.Reverse())
						);
	nMatrix4	tmx		(
							nMatrix4::FromMatrix3(obb.bb_rotation.Transpose()) *
							nMatrix4::TranslationMatrix(obb.bb_position.Reverse())
						);
	nVector		s = prv_pos * prv_tmx,
				e = pos * tmx;
	if	(nMath::EqualZero(nVector::Dist2(s, e)))
		return 0;
	nVector		scl = obb.bb_scale * 0.5f;

	nVector		n, r;
	float		depth = 0.f;
/*
	// Catch initial embedded condition.
	// Inexact but fast and working enough.
	if	(	(s.x > -scl.x) && (s.x < scl.x) &&
			(s.y > -scl.y) && (s.y < scl.y) &&
			(s.z > -scl.z) && (s.z< scl.z)	)
	{
		ComputeNearestExitNormal(s, scl, n);
		r = s;
		depth = radius;
		goto face_hit;
	}
	else
*/	{
		// Perform collision detection.
		ClosestPointToAABB(e, scl, r);
		float		d2 = nVector::Dist2(e, r);

		if	(d2 < (radius * radius))
		{
			if	(nMath::EqualZero(d2))
					n = (e - s).Normalize();
			else	n = (r - e).Normalize();
			depth = radius - nMath::Sqrt(d2);
			goto hit;
		}
		else
		{
/*
			// Approximate swept collision.
			nVector	rscl(scl.Reverse());
			nVector	_s, _e, _n, _h;
			nMinMax	mm(rscl, scl);

			depth = Mm(10);

			n.Set(s.x > 0 ? 1 : -1, 1, 0);
			_s = s - n * radius;
			_e = e - n * radius;
			if	(mm.ClassifySegment(_s, _e, r, &_n) && (n == _n))
				goto face_hit;
			n.Set(0, s.y > 0 ? 1 : -1, 0);
			_s = s - n * radius;
			_e = e - n * radius;
			if	(mm.ClassifySegment(_s, _e, r, &_n) && (n == _n))
				goto face_hit;
			n.Set(0, 0, s.z > 0 ? 1 : -1);
			_s = s - n * radius;
			_e = e - n * radius;
			if	(mm.ClassifySegment(_s, _e, r, &_n) && (n == _n))
				goto face_hit;

			// Check end position.
			if	(	(e.x > -scl.x) && (e.x < scl.x) &&
					(e.y > -scl.y) && (e.y < scl.y) &&
					(e.z > -scl.z) && (e.z< scl.z)	)
			{
				ComputeNearestExitNormal(e, scl, n);
				r = e;
				depth = radius;
				goto face_hit;
			}

			ClosestPointToAABB(e, scl, r);
			d2 = nVector::Dist2(e, r);
			if	(d2 < (radius * radius))
			{
				if	(nMath::EqualZero(d2))
						n = (e - s).Normalize();
				else	n = (r - e).Normalize();
				depth = radius - nMath::Sqrt(d2);
				goto hit;
			}
*/
		}

	}
	return 0;

//face_hit:;

	n = n.Reverse();

hit:;
	r = (n * obb.bb_rotation) * radius + prv_pos;

	// Store contact infos.
	if	(!cinfo || !cinfo->n || !cinfo->d)
		return 1;

	cinfo->d[0] = -depth;
	cinfo->n[0] = n * obb.bb_rotation;

	if	(!cinfo->p || !cinfo->t || !cinfo->cmax)
		return 1;

	if	(cinfo->cmax > 0)
		cinfo->p[0] = r;// * prv_obb.bb_rotation + prv_obb.bb_position;
	if	(cinfo->t[0])
		cinfo->t[0] = -depth;

	return 1;
}
