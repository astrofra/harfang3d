/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//---------------------------------------------------------
void		TauDistanceConstraint::Transform(bool do_world)
//---------------------------------------------------------
{
	iK = 0.f;
	if	(!enable || !(item[0] || item[1]))
		return;
	if	(!(item[0] && item[0]->IsActive()) && !(item[1] && item[1]->IsActive()))
		return;

	int		n;
	if	(do_world)
		for	(n = 0; n < 2; ++n)
			world_hook[n] = item[n] ? hook[n] * item[n]->rotation_matrix + item[n]->position : hook[n];

	// Must be called once world hooks are computed.
	TauConstraint::Transform(do_world);
}

//-------------------------------------------
void		TauDistanceConstraint::Process()
//-------------------------------------------
{
	if	(!iK)
		return;

	nVector	v0(0, 0, 0);
	nVector	r0(0, 0, 0);
	if	(item[0])
	{
		Vec3Sub(r0, world_hook[0], item[0]->world_pivot);
		item[0]->PointVelocity(r0, v0);
	}

	nVector	v1(0, 0, 0);
	nVector	r1(0, 0, 0);
	if	(item[1])
	{
		Vec3Sub(r1, world_hook[1], item[1]->world_pivot);
		item[1]->PointVelocity(r1, v1);
	}

	nVector	U;
	Vec3Sub(U, v1, v0);
	if	(use_correction)
		U = correction_vector * U.Dot(correction_vector);
	Vec3DivConst(U, U, tau.fq);

	nVector	cur_rel_pos = world_hook[1] - world_hook[0];
	if	(use_correction)
		cur_rel_pos = correction_vector * cur_rel_pos.Dot(correction_vector);
	nVector	rel_pos = cur_rel_pos + U;

	float		rpl = Vec3Len2(rel_pos);
	if	(rpl > 0.000001f)
	{
		rpl = nMath::Sqrt(rpl);
		float		length = rpl;
		if	((min_dist >= 0.f) && (length < min_dist))
			length = min_dist;
		if	((max_dist >= 0.f) && (length > max_dist))
			length = max_dist;
		rel_pos *= length / rpl;

		float	merc = maximum_erc / tau.fq;
		nVector	erc = (rel_pos - cur_rel_pos);
		if	((merc > 0) && (erc.Len() > merc))
			erc = erc.ClampMagnitude(0, merc);

		nVector	Vr = U - erc;
		nVector	J = Vr * 0.5f * iK * strength;
		if	(use_correction)
			J = correction_vector * J.Dot(correction_vector);

		if	(item[0] && item[0]->GetInverseMass())
		{
			item[0]->vel_linear += J * item[0]->GetInverseMass();
			item[0]->vel_angular += r0.Cross(J) * item[0]->inv_world_tensor;
		}
		if	(item[1] && item[1]->GetInverseMass())
		{
			item[1]->vel_linear -= J * item[1]->GetInverseMass();
			item[1]->vel_angular -= r1.Cross(J) * item[1]->inv_world_tensor;
		}
	}
}

//-------------------------------------------------------
TauDistanceConstraint::TauDistanceConstraint
									(
										Tau &t,
										TauItem *a,
										TauItem *b,
										nVector *hook_a,
										nVector *hook_b,
										float min_d,
										float max_d
									) : TauConstraint(t)
//-------------------------------------------------------
{
	type = Type_DistanceConstraint;

	item[0] = a;
	hook[0] = hook_a ? *hook_a : nVector(0, 0, 0);
	item[1] = b;
	hook[1] = hook_b ? *hook_b : nVector(0, 0, 0);

	min_dist = min_d;
	max_dist = max_d;
}
