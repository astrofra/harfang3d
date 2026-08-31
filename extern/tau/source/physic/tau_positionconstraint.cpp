/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//---------------------------------------------------------
void		TauPositionConstraint::Transform(bool do_world)
//---------------------------------------------------------
{
	iK = 0.f;
	if	(!enable || !(item[0] || item[1]))
		return;

	// World hooks.
	if	(do_world)
	{
		world_hook[0] = item[0] ? hook[0] * item[0]->rotation_matrix + item[0]->position : hook[0];
		world_hook[1] = item[1] ? hook[1] * item[1]->rotation_matrix + item[1]->position : hook[1];
	}

	// Must be called once world hooks are computed.
	TauConstraint::Transform(do_world);
}

//------------------------------------------
void		TauPositionConstraint::Process()
//------------------------------------------
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
	Vec3DivConst(U, U, tau.fq);

	// Joint position error correction term.
	nVector	cur_rel_pos = world_hook[1] - world_hook[0];
	nVector	Vr = U + cur_rel_pos;

	// Position constraints.
	nMatrix3	irmtx = item[0] ? item[0]->rotation_matrix.Transpose() : nMatrix3::IdentityMatrix();

	if	(item[0])
		Vr = Vr * irmtx;
	if	(dof & DofPositionX)	Vr.x = 0;
	if	(dof & DofPositionY)	Vr.y = 0;
	if	(dof & DofPositionZ)	Vr.z = 0;
	if	(item[0])
		Vr = Vr * item[0]->rotation_matrix;

	nVector	J = Vr * 0.5f * iK * strength;
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

//-----------------------------------------------
void		TauPositionConstraint::SetDof(uint d)
//-----------------------------------------------
{
	dof = d;
}

//----------------------------------------------------------------------------------------------------------------------------------------
TauPositionConstraint::TauPositionConstraint(Tau &t, TauItem *a, TauItem *b, nVector *hook_a, nVector *hook_b, uint df) : TauConstraint(t)
//----------------------------------------------------------------------------------------------------------------------------------------
{
	type = Type_PositionConstraint;

	SetAnchor(a);
	SetAnchorHook(hook_a);
	SetTarget(b);
	SetTargetHook(hook_b);

	dof = df;
}
