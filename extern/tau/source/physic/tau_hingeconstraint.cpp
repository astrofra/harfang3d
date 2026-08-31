/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


/*
//------------------------------------------------------------------------------------------------
void		PinPointBody(TauItem &item, nVector &world_hook, nVector &world_target, float damping)
//------------------------------------------------------------------------------------------------
{
	if	(!item.GetInverseMass())
		return;

	nVector	dt = world_hook - world_target;
	nVector	nl = dt.Normalize();

	nVector	r = world_hook - item.world_pivot;
	float	k = item.GetInverseMass() + nl.Dot((r.Cross(nl) * item.inv_world_tensor).Cross(r));

	nVector	v;
	item.PointVelocity(r, v);
	nVector	Vr = v + dt;
	nVector	J = Vr * 0.5f / k * damping;

	item.vel_linear -= J * item.GetInverseMass();
	item.vel_angular -= r.Cross(J) * item.inv_world_tensor;
}
*/

//------------------------------------------------------
void		TauHingeConstraint::Transform(bool do_world)
//------------------------------------------------------
{
	if	(!enable || !item[1])
		return;

	nMatrix3	tm = item[0] ? item[0]->rotation_matrix * anchor_matrix : anchor_matrix;

	// World hooks.
	hinge0.world_hook[0] = item[0] ? (hook[1] + axis) * tm + item[1]->position : item[1]->position + hook[0] + axis;
	hinge0.world_hook[1] = item[1] ? (hook[1] + axis) * item[1]->rotation_matrix + item[1]->position : item[1]->position + hook[1] + axis;
	hinge1.world_hook[0] = item[0] ? (hook[1] - axis) * tm + item[1]->position : item[1]->position + hook[0] - axis;
	hinge1.world_hook[1] = item[1] ? (hook[1] - axis) * item[1]->rotation_matrix + item[1]->position : item[1]->position + hook[1] - axis;

	// Must be called once world hooks are computed.
	hinge0.Transform(false);
	hinge1.Transform(false);
}

//---------------------------------------
void		TauHingeConstraint::Process()
//---------------------------------------
{
	if	(!enable || !item[1])
		return;

	hinge0.Process();
	hinge1.Process();
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------
TauHingeConstraint::TauHingeConstraint(Tau &t, TauItem *a, TauItem *b, nVector *hook_a, nVector *hook_b, nVector *ax) : TauConstraint(t), hinge0(t), hinge1(t)
//------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	type = Type_HingeConstraint;

	SetAnchor(a);
	SetAnchorHook(hook_a);
	SetTarget(b);
	SetTargetHook(hook_b);

	if	(ax)
			axis = *ax;
	else	axis.Set(0, 0, 1);	// default to Z hinge.
}
