/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//-------------------------------------------------------------
void		Tau::DBG_Constraint(nRenderer &render, float scale)
//-------------------------------------------------------------
{
/*
	for	(TauConstraint *cst = NULL; cst = (TauConstraint *)constraint_list.Pool(cst);)
	{
		nVector	dt = cst->world_hook_b - cst->world_hook_a;
		if	(!dt.Len())
			continue;
		float		kc = cst->length / dt.Len();

		if	(kc > 1.f)
			kc = 1.f;
		bfmk_Color	color(1.f - kc, 0, kc);
		nVector	v[2];
		v[0] = cst->world_hook_a; v[1] = cst->world_hook_b;
		render.Line3D(v, &color);
		render.DrawCross(cst->world_hook_a, Cm(25) * scale, &color);
		render.DrawCross(cst->world_hook_b, Cm(25) * scale, &color);
	}
*/
}

//----------------------------------------------------------
void		Tau::DBG_Contact(nRenderer &render, float scale)
//----------------------------------------------------------
{
	nColor	nrms(1, 0, 0);

	for	(uint n = 0; n < colnode_count; n++)
	{
		// Prefer a straight up normal.
		nVector	nrm = colnode[n].n;
		if	(nrm.y < 0.f)
			nrm *= -1.f;

		uint		idx = colnode[n].ctc_start;
		for	(uint c = 0; c < colnode[n].ctc_count; c++)
		{
			nVector		v[2];
			v[0] = contact[idx + c].p;
			v[1] = v[0] + nrm * scale * 0.1f;// * contact[idx + c].d * 10;

			render.DrawCross(v[0], scale * Cm(2.5f));
			render.Line3D(v[0], v[1], &nrms);
		}
	}
}

//-------------------------------------------------------
void		Tau::DBG_Body(nRenderer &render, float scale)
//-------------------------------------------------------
{
	const float	delay = fq * K_DEAK;

	nLinkedListPool	pool;
	for	(TauItem *ci; (ci = (TauItem *)item_list.Pool(pool));)
	{
		float	k = 1.f - (ci->sleep / delay);
		nColor	color(k, k, k);

		nVector	com = ci->pivot * ci->rotation.AsMatrix3() + ci->position;
		render.DrawCross(com, scale * Cm(5), &color);
	}
}
