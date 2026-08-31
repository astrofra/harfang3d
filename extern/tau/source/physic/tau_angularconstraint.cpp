/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//-------------------------------------------------------------------------------------
float		TauAngularConstraint::TransformConstraintAxis(TauItem *item[2], nVector *r)
//-------------------------------------------------------------------------------------
{
	return 0;
}

//--------------------------------------------------------
void		TauAngularConstraint::Transform(bool do_world)
//--------------------------------------------------------
{
}

//----------------------------------------------------------------------------------------------
void		TauAngularConstraint::ApplyConstraintImpulse(TauItem *item[2], nVector *r, float iK)
//----------------------------------------------------------------------------------------------
{
}

//-----------------------------------------
void		TauAngularConstraint::Process()
//-----------------------------------------
{
}

//-------------------------------------------------------
TauAngularConstraint::TauAngularConstraint
									(
										Tau &t,
										TauItem *a,
										TauItem *b,
										uint d
									) : TauConstraint(t)
//-------------------------------------------------------
{
	type = Type_AngularConstraint;
}
