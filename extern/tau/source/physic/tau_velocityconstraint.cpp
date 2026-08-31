/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//------------------------------------------
void		TauVelocityConstraint::Process()
//------------------------------------------
{
	if	(!enable || !item[0])
		return;

	nMatrix3	tmtx(item[0]->rotation_matrix.Transpose());

	// Linear velocity constraint.
	nVector		local_linear_velocity = item[0]->vel_linear * tmtx;
	if	(mask & Velocity_LinearX)
		local_linear_velocity.x = local_linear_constraint.x;
	if	(mask & Velocity_LinearY)
		local_linear_velocity.y = local_linear_constraint.y;
	if	(mask & Velocity_LinearZ)
		local_linear_velocity.z = local_linear_constraint.z;
	item[0]->vel_linear = (local_linear_velocity * item[0]->rotation_matrix) * strength + item[0]->vel_linear * (1.f - strength);

	// Angular velocity constraint.
	nVector	local_angular_velocity = item[0]->vel_angular * tmtx;
	if	(mask & Velocity_AngularX)
		local_angular_velocity.x = local_angular_constraint.x;
	if	(mask & Velocity_AngularY)
		local_angular_velocity.y = local_angular_constraint.y;
	if	(mask & Velocity_AngularZ)
		local_angular_velocity.z = local_angular_constraint.z;
	item[0]->vel_angular = (local_angular_velocity * item[0]->rotation_matrix) * strength + item[0]->vel_angular * (1.f - strength);
}

//----------------------------------------------------------------------------------------------------------------------------------
TauVelocityConstraint::TauVelocityConstraint(Tau &t, TauItem *item, nVector *linear, nVector *angular, uint mask) : TauConstraint(t)
//----------------------------------------------------------------------------------------------------------------------------------
{
	type = Type_VelocityConstraint;

	SetMask(mask);
	local_linear_constraint = linear ? *linear : nVector(0, 0, 0);
	local_angular_constraint = angular ? *angular : nVector(0, 0, 0);

	SetAnchor(item);
	SetAnchorHook(NULL);
	SetTarget(NULL);
	SetTargetHook(NULL);
}
