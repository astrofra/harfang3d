/*

GCollide	[Generic collision]
			Id: OBB/OBB overlap.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//----------------------------------------------------------------------
uint				WindowLine(nVector &a, nVector &b, nVector &extends)
//----------------------------------------------------------------------
{
	nVector		*wa = &a, *wb = &b, *swp;

	//--------------------------------
	#define		AXIS_WINDOW_CLIP(_AX)\
	if	(extends._AX > 0.f)\
	{\
		if	(wb->_AX < wa->_AX)\
		{	swp = wa; wa = wb; wb = swp;	}\
		if	((wb->_AX < -extends._AX) || (wa->_AX > extends._AX))\
			return 0;\
		\
		nVector	dt = *wb - *wa;\
		if	(dt._AX)\
		{\
			float		k = (extends._AX - wa->_AX) / dt._AX;\
			if	(k < 1.f)\
				*wb = dt * k + *wa;\
			k = (-extends._AX - wa->_AX) / dt._AX;\
			if	(k > 0.f)\
				*wa = dt * k + *wa;\
		}\
	}
	//--------------------------------

	AXIS_WINDOW_CLIP(x)
	AXIS_WINDOW_CLIP(y)
	AXIS_WINDOW_CLIP(z)
	return 2;
}

//-------------------------------------------------------------------------------------------
int					GCollide::OBBOverlapOBB(const nOBB &a, const nOBB &b, GColCtcInfo *cinfo)
//-------------------------------------------------------------------------------------------
{
	// 1st OBB faces as separating axis.
	const nVector	bb_xt(a.bb_scale * 0.5f),
					b_bb_xt(b.bb_scale * 0.5f),
					D(b.bb_position - a.bb_position);

	nVector			A0(a.bb_rotation.GetRow(0)),
					B0(b.bb_rotation.GetRow(0)),
					B1(b.bb_rotation.GetRow(1)),
					B2(b.bb_rotation.GetRow(2));

	float			cache_t[15];
	//---------------------------------------
	#define		BBAXIS_OVERLAP(N, __A, __B)	\
		cache_t[(N)] = fabs(__A) - (__B);	\
		if	(cache_t[(N)] > 0.f)			\
			return 0;						\
	//---------------------------------------

	// a.extend as separating axis.
	float		A0D = A0.Dot(D),
				c00 = A0.Dot(B0), ac00 = fabs(c00),
				c01 = A0.Dot(B1), ac01 = fabs(c01),
				c02 = A0.Dot(B2), ac02 = fabs(c02);
	BBAXIS_OVERLAP(0, A0D, bb_xt.x + b_bb_xt.x * ac00 + b_bb_xt.y * ac01 + b_bb_xt.z * ac02);
	nVector		A1(a.bb_rotation.GetRow(1));
	float		A1D = A1.Dot(D),
				c10 = A1.Dot(B0), ac10 = fabs(c10),
				c11 = A1.Dot(B1), ac11 = fabs(c11),
				c12 = A1.Dot(B2), ac12 = fabs(c12);
	BBAXIS_OVERLAP(1, A1D, bb_xt.y + b_bb_xt.x * ac10 + b_bb_xt.y * ac11 + b_bb_xt.z * ac12);
	nVector		A2(a.bb_rotation.GetRow(2));
	float		A2D = A2.Dot(D),
				c20 = A2.Dot(B0), ac20 = fabs(c20),
				c21 = A2.Dot(B1), ac21 = fabs(c21),
				c22 = A2.Dot(B2), ac22 = fabs(c22);
	BBAXIS_OVERLAP(2, A2D, bb_xt.z + b_bb_xt.x * ac20 + b_bb_xt.y * ac21 + b_bb_xt.z * ac22);

	// b.extend as separating axis.
	BBAXIS_OVERLAP(3, B0.Dot(D), b_bb_xt.x + bb_xt.x * ac00 + bb_xt.y * ac10 + bb_xt.z * ac20);
	BBAXIS_OVERLAP(4, B1.Dot(D), b_bb_xt.y + bb_xt.x * ac01 + bb_xt.y * ac11 + bb_xt.z * ac21);
	BBAXIS_OVERLAP(5, B2.Dot(D), b_bb_xt.z + bb_xt.x * ac02 + bb_xt.y * ac12 + bb_xt.z * ac22);

	// a.extend.Cross(b.extend) as separating axis.
	BBAXIS_OVERLAP(6, c10 * A2D - c20 * A1D, bb_xt.y * ac20 + bb_xt.z * ac10 + b_bb_xt.y * ac02 + b_bb_xt.z * ac01);
	BBAXIS_OVERLAP(7, c11 * A2D - c21 * A1D, bb_xt.y * ac21 + bb_xt.z * ac11 + b_bb_xt.x * ac02 + b_bb_xt.z * ac00);
	BBAXIS_OVERLAP(8, c12 * A2D - c22 * A1D, bb_xt.y * ac22 + bb_xt.z * ac12 + b_bb_xt.x * ac01 + b_bb_xt.y * ac00);
	BBAXIS_OVERLAP( 9, c20 * A0D - c00 * A2D, bb_xt.x * ac20 + bb_xt.z * ac00 + b_bb_xt.y * ac12 + b_bb_xt.z * ac11);
	BBAXIS_OVERLAP(10, c21 * A0D - c01 * A2D, bb_xt.x * ac21 + bb_xt.z * ac01 + b_bb_xt.x * ac12 + b_bb_xt.z * ac10);
	BBAXIS_OVERLAP(11, c22 * A0D - c02 * A2D, bb_xt.x * ac22 + bb_xt.z * ac02 + b_bb_xt.x * ac11 + b_bb_xt.y * ac10);
	BBAXIS_OVERLAP(12, c00 * A1D - c10 * A0D, bb_xt.x * ac10 + bb_xt.y * ac00 + b_bb_xt.y * ac22 + b_bb_xt.z * ac21);
	BBAXIS_OVERLAP(13, c01 * A1D - c11 * A0D, bb_xt.x * ac11 + bb_xt.y * ac01 + b_bb_xt.x * ac22 + b_bb_xt.z * ac20);
	BBAXIS_OVERLAP(14, c02 * A1D - c12 * A0D, bb_xt.x * ac12 + bb_xt.y * ac02 + b_bb_xt.x * ac21 + b_bb_xt.y * ac20);

	// We are colliding and that's all we care about.
	if	(!cinfo || !cinfo->n || !cinfo->d)
		return 1;

	// We have a penetration.
	nVector		axis[15];
	axis[0] = A0; axis[1] = A1; axis[2] = A2;
	axis[3] = B0; axis[4] = B1; axis[5] = B2;
	axis[6] = A0.Cross(B0); axis[7] = A0.Cross(B1); axis[8] = A0.Cross(B2);
	axis[9] = A1.Cross(B0); axis[10] = A1.Cross(B1); axis[11] = A1.Cross(B2);
	axis[12] = A2.Cross(B0); axis[13] = A2.Cross(B1); axis[14] = A2.Cross(B2);

	float		k, dInf = -100000000.f;
	k = axis[6].Len(); if (k) cache_t[6] /= k; else cache_t[6] = dInf;
	k = axis[7].Len(); if (k) cache_t[7] /= k; else cache_t[7] = dInf;
	k = axis[8].Len(); if (k) cache_t[8] /= k; else cache_t[8] = dInf;
	k = axis[9].Len(); if (k) cache_t[9] /= k; else cache_t[9] = dInf;
	k = axis[10].Len(); if (k) cache_t[10] /= k; else cache_t[10] = dInf;
	k = axis[11].Len(); if (k) cache_t[11] /= k; else cache_t[11] = dInf;
	k = axis[12].Len(); if (k) cache_t[12] /= k; else cache_t[12] = dInf;
	k = axis[13].Len(); if (k) cache_t[13] /= k; else cache_t[13] = dInf;
	k = axis[14].Len(); if (k) cache_t[14] /= k; else cache_t[14] = dInf;

	// Get the deepest penetration depth and contact normal.
	uint		deepest = 0, n;
	cinfo->d[0] = dInf;

	for	(n = 0; n < 15; n++)
		if	(cache_t[n] > cinfo->d[0])
		{
			cinfo->d[0] = cache_t[n];
			deepest = n;
		}
	if	(cinfo->d[0] == dInf)
		return 0;

	cinfo->n[0] = axis[deepest].Normalize();
	if	(cinfo->n->Dot(D) < 0.f)
		cinfo->n[0] = cinfo->n->Reverse();

	// We have a collision, we know the collision normal
	// and depth but don't care about contacts.
	if	(!cinfo->cmax)
		return 1;

	// Determine contacts.
	// TODO Rewrite this...
	nVector	vtx[8];
	vtx[0].Set(-0.5,  0.5,  0.5); vtx[1].Set( 0.5,  0.5,  0.5);
	vtx[2].Set( 0.5, -0.5,  0.5); vtx[3].Set(-0.5, -0.5,  0.5);
	vtx[4].Set(-0.5,  0.5, -0.5); vtx[5].Set( 0.5,  0.5, -0.5);
	vtx[6].Set( 0.5, -0.5, -0.5); vtx[7].Set(-0.5, -0.5, -0.5);

	float		vertex_t1[8], vertex_t2[8];
	nVector		tvtx1[8], tvtx2[8];

	//	@todo	Rewrite the whole feature determination code!
	
	// Get 1st bb extremities along collision normal.
	nMatrix4	mtx1 = nMatrix4::FromMatrix3(a.bb_rotation) * nMatrix4::ScaleMatrix(a.bb_scale);
	mtx1.SetRow(3, a.bb_position);
	float		hi_t = -99999999.f;

	for	(n = 0; n < 8; n++)
	{
		tvtx1[n] = vtx[n] * mtx1;
		vertex_t1[n] = (tvtx1[n] - b.bb_position).Dot(cinfo->n[0]);
		if	(vertex_t1[n] > hi_t)
			hi_t = vertex_t1[n];
	}

	// Get 2nd bb extremities along collision normal.
	nMatrix4	mtx2 = nMatrix4::FromMatrix3(b.bb_rotation) * nMatrix4::ScaleMatrix(b.bb_scale);
	mtx2.SetRow(3, b.bb_position);
	float		lo_t = 99999999.f;

	for	(n = 0; n < 8; n++)
	{
		tvtx2[n] = vtx[n] * mtx2;
		vertex_t2[n] = (tvtx2[n] - b.bb_position).Dot(cinfo->n[0]);
		if	(vertex_t2[n] < lo_t)
			lo_t = vertex_t2[n];
	}

	// Grab 1st BB's offending vertice.
	nVector	v_ext1[8];
	float		t_ext[8];
	uint		n_ext1 = 0;
	for	(n = 0; n < 8; n++)
		if	(vertex_t1[n] >= lo_t)
		{
			v_ext1[n_ext1] = tvtx1[n];
			t_ext[n_ext1++] = vertex_t1[n];
		}

	if	(!n_ext1)
		return 0;

#define	__LESS
#ifdef	__LESS
	if	(n_ext1 == 3)	// Need one less...
	{
		uint		ri = 0;
		float		lt = t_ext[0];

		for	(n = 1; n < 3; ++n)
			if	(t_ext[n] < lt)
			{
				ri = n;
				lt = t_ext[n];
			}

		if	(ri < 2)
			v_ext1[ri] = v_ext1[2];
		n_ext1 = 2;
	}
#else
	if	(n_ext1 == 3)	// Need one more...
	{
		float	hit = 0;
		int		vn = -1;
		for	(n = 0; n < 8; n++)
			if	(((vertex_t1[n] > hit) && (vertex_t1[n] < lo_t)) || (vn == -1))
			{
				hit = vertex_t1[n];
				vn = n;
			}
		v_ext1[n_ext1++] = tvtx1[vn];
	}
#endif

	// Grab 2nd BB's offending vertice.
	nVector	v_ext2[8];
	uint		n_ext2 = 0;
	for	(n = 0; n < 8; n++)
		if	(vertex_t2[n] <= hi_t)
		{
			v_ext2[n_ext2] = tvtx2[n];
			t_ext[n_ext2++] = vertex_t2[n];
		}

	if	(!n_ext2)
		return 0;

#ifdef	__LESS
	if	(n_ext2 == 3)	// Need one less...
	{
		uint		ri = 0;
		float		lt = t_ext[0];

		for	(n = 1; n < 3; ++n)
			if	(t_ext[n] > lt)
			{
				ri = n;
				lt = t_ext[n];
			}

		if	(ri < 2)
			v_ext2[ri] = v_ext2[2];
		n_ext2 = 2;
	}
#else
	if	(n_ext2 == 3)	// Need one more...
	{
		float	hit = 0;
		int		vn = -1;
		for	(n = 0; n < 8; n++)
			if	(((vertex_t2[n] < hit) && (vertex_t2[n] > hi_t)) || (vn == -1))
			{
				hit = vertex_t2[n];
				vn = n;
			}
		v_ext2[n_ext2++] = tvtx2[vn];
	}
#endif

	//__LOG__ << "OBB/OBB configuration {"<< n_ext1 <<";"<< n_ext2 <<"}, AXIS = " << deepest << ".\n";

	// Find contacts.
	uint	tctc = 0;

	if	(n_ext1 > 4)
		n_ext1 = 4;
	if	(n_ext2 > 4)
		n_ext2 = 4;

	switch	(n_ext1)
	{
		// Single contact.
		case	1:
			cinfo->p[0] = v_ext1[0];
			tctc = 1;
			goto done;

		// Edge.
		case	2:
		{
			switch	(n_ext2)
			{
				case	1:
					cinfo->p[0] = v_ext2[0];
					tctc = 1;
					goto done;

				case	2:
				{
					float		t[2];
					nVector	u = v_ext1[1] - v_ext1[0];
					nVector	v = v_ext2[1] - v_ext2[0];

					if	(!nTools::LineClosestPointToLine(v_ext1[0], v_ext1[1], v_ext2[0], v_ext2[1], t))
					{
						nVector	s = v_ext2[0] - v_ext1[0];
						nVector	t = v_ext2[1] - v_ext1[0];
						float		k = 1.f / u.Len2();

						cinfo->p[0] = u * nMath::Clamp(s.Dot(u) * k, 0, 1) + v_ext1[0];
						cinfo->p[1] = u * nMath::Clamp(t.Dot(u) * k, 0, 1) + v_ext1[0];
						tctc = 2;
					}
					else
					{
						cinfo->p[0] =	(
											(u * nMath::Clamp(t[0], 0, 1) + v_ext1[0]) +
											(v * nMath::Clamp(t[1], 0, 1) + v_ext2[0])
										) * 0.5f;
						tctc = 1;
					}
					goto done;
				}

				case	4:
				{
					nMatrix3	brt(b.bb_rotation.Transpose());
					v_ext1[0] = (v_ext1[0] - b.bb_position) * brt;
					v_ext1[1] = (v_ext1[1] - b.bb_position) * brt;
					nVector	nrm = cinfo->n[0] * brt;

					if	(fabs(nrm.x) > 0.9f)
					{
						nVector	extends(-1.f, b_bb_xt.y, b_bb_xt.z);
						WindowLine(v_ext1[0], v_ext1[1], extends);
					}
					else	if	(fabs(nrm.y) > 0.9f)
					{
						nVector	extends(b_bb_xt.x, -1.f, b_bb_xt.z);
						WindowLine(v_ext1[0], v_ext1[1], extends);
					}
					else
					{
			 			nVector	extends(b_bb_xt.x, b_bb_xt.y, -1.f);
						WindowLine(v_ext1[0], v_ext1[1], extends);
					}

					cinfo->p[0] = (v_ext1[0] * b.bb_rotation) + b.bb_position;
					tctc = 1;
					if	(cinfo->cmax > 1)
					{
						cinfo->p[1] = (v_ext1[1] * b.bb_rotation) + b.bb_position;
						tctc = 2;
					}
					goto done;
				}
			}
		}

		// 1st BB's face.
		case	4:
			switch	(n_ext2)
			{
				case	1:
					cinfo->p[0] = v_ext2[0];
					tctc = 1;
					goto done;

				case	2:
				{
					nMatrix3	brt(a.bb_rotation.Transpose());
					v_ext2[0] = (v_ext2[0] - a.bb_position) * brt;
					v_ext2[1] = (v_ext2[1] - a.bb_position) * brt;
					nVector	nrm = cinfo->n[0] * brt;

					if	(fabs(nrm.x) > 0.9f)
					{
						nVector	extends(-1.f, bb_xt.y, bb_xt.z);
						WindowLine(v_ext2[0], v_ext2[1], extends);
					}
					else	if	(fabs(nrm.y) > 0.9f)
					{
						nVector	extends(bb_xt.x, -1.f, bb_xt.z);
						WindowLine(v_ext2[0], v_ext2[1], extends);
					}
					else
					{
			 			nVector	extends(bb_xt.x, bb_xt.y, -1.f);
						WindowLine(v_ext2[0], v_ext2[1], extends);
					}

					cinfo->p[0] = (v_ext2[0] * a.bb_rotation) + a.bb_position;
					tctc = 1;
					if	(cinfo->cmax > 1)
					{
						cinfo->p[1] = (v_ext2[1] * a.bb_rotation) + a.bb_position;
						tctc = 2;
					}
					goto done;
				}

				case	4:
				{
					nMatrix3	art(a.bb_rotation.Transpose());
					nMatrix3	brt(b.bb_rotation.Transpose());

					/*
						If the 1st box face is completely containing
						the 2nd box face then we swap both face before clipping.
					*/
					nVector	clipping_region;
					nVector	v_in[4];
					bool		swapped = false;

					for	(n = 0; n < 4; n++)
					{
						nVector	tmp = (v_ext1[n] - b.bb_position) * brt;
						if	(	(tmp.x >= -b_bb_xt.x) && (tmp.x <=  b_bb_xt.x) &&
								(tmp.y >= -b_bb_xt.y) && (tmp.y <=  b_bb_xt.y) &&
								(tmp.z >= -b_bb_xt.z) && (tmp.z <=  b_bb_xt.z)	)
							break;
					}
					if	(n == 4)	// Clip 2nd upon 1st.
					{
						for	(n = 0; n < 4; n++)
							v_in[n] = (v_ext2[n] - a.bb_position) * art;
						clipping_region = bb_xt;
						swapped = true;
					}
					else			// Clip 1st upon 2nd.
					{
						for	(n = 0; n < 4; n++)
							v_in[n] = (v_ext1[n] - b.bb_position) * brt;
						clipping_region = b_bb_xt;
					}

					// Rebuild edges (and we should normalize those edge in a perfect world...).
					uint	idx[4];
							if	(fabs((v_in[1] - v_in[0]).Dot(v_in[2] - v_in[0])) < 0.1f)
							{	idx[0] = 0; idx[1] = 2; idx[2] = 3; idx[3] = 1;	}
					else	if	(fabs((v_in[2] - v_in[0]).Dot(v_in[3] - v_in[0])) < 0.1f)
							{	idx[0] = 0; idx[1] = 3; idx[2] = 1; idx[3] = 2;	}
					else	{	idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 3;	}

					/*
						Clip face/face in local space and
						filter out unique intersection points.
					*/
					nVector	it[2], ot[8];
					uint		nctc =  0, m;

					#define		FILTER_CONTACT(__a, __w)						\
					it[0] = v_in[(__a)]; it[1] = v_in[(__w)];					\
					if	(WindowLine(it[0], it[1], clipping_region))				\
					{															\
						for	(n = 0; (n < 2) && (nctc < 8); n++)					\
						{														\
							for	(m = 0; m < nctc; m++)							\
								if	(nVector::Dist2(it[n], ot[m]) < 0.001f)		\
									break;										\
							if	(m == nctc)										\
								ot[nctc++] = it[n];								\
						}														\
					}

					FILTER_CONTACT(idx[0], idx[1])
					FILTER_CONTACT(idx[1], idx[2])
					FILTER_CONTACT(idx[2], idx[3])
					FILTER_CONTACT(idx[3], idx[0])

					// Contacts in world space.
					if	(swapped)
						for	(n = 0; (n < nctc) && (n < cinfo->cmax); n++)
							cinfo->p[n] = (ot[n] * a.bb_rotation) + a.bb_position;
					else
						for	(n = 0; (n < nctc) && (n < cinfo->cmax); n++)
							cinfo->p[n] = (ot[n] * b.bb_rotation) + b.bb_position;

					tctc = n;
					goto done;
				}
			}
			break;
	}

done:

	// Compute contact depths.
	if	(tctc && cinfo->t)
	{
		float		ctc_t[8];
		for	(n = 0, lo_t = 0.f; n < tctc; n++)
		{
			ctc_t[n] = (cinfo->p[n] - b.bb_position).Dot(cinfo->n[0]);
			if	(ctc_t[n] < lo_t)
				lo_t = ctc_t[n];
		}

		const float	dt = lo_t - cinfo->d[0];
		for	(n = 0; n < tctc; n++)
			cinfo->t[n] = ctc_t[n] - dt;
	}
	return tctc;
}
