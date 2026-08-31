/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//-------------------------------------
void		Tau::TransformConstraints()
//-------------------------------------
{
	nLinkedListPool	pool;
	for	(nLinkedListEntry *e; e = constraint_list.Pool(pool); )
		((TauConstraint *)e)->Transform();
}

//-------------------------------------------------
void		TauConstraint::Transform(bool do_world)
//-------------------------------------------------
{
	nVector	dt = world_hook[1] - world_hook[0];
	nVector	nl = dt.Normalize();

	float		k[2];
	for	(uint n = 0; n < 2; ++n)
		if	(item[n] && item[n]->inv_mass)
		{
			nVector	r = world_hook[n] - item[n]->world_pivot;
			k[n] = item[n]->inv_mass + nl.Dot((r.Cross(nl) * item[n]->inv_world_tensor).Cross(r));
		}
		else	k[n] = 0;

	const float	K = k[0] + k[1];
	iK = K ? 1.f / K : 0.f;
}

//-------------------------------------------
TauConstraint::TauConstraint(Tau &t) : tau(t)
//-------------------------------------------
{
	id = "Constraint";
	type = Type_Constraint;

	iK = 0.f;
	item[0] = NULL;
	item[1] = NULL;

	hook[0].Set(0, 0, 0);
	world_hook[0].Set(0, 0, 0);
	hook[1].Set(0, 0, 0);
	world_hook[1].Set(0, 0, 0);

	use_correction = false;
	maximum_erc = -1.f;
	SetStrength();

	enable = true;
}
