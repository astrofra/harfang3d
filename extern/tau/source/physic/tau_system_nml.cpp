/*

Tau			[Physic engine]
			Written by Emmanuel Julien.
			All rights reserved 2000~2005.

			Emmanuel Julien
			http://www.nengine.fr
			mailto:ejulien@nengine.fr

*/


		#include	"config.h"
		#include	"physic.h"
		#include	"../manager/manager.h"


//-----------------------------------------------------------------------------------------------------------------------------------
bool				Tau::FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid, nGroup **group, bool load_settings)
//-----------------------------------------------------------------------------------------------------------------------------------
{
	if	(tag.id != "Tau")
	{
		__LOG_E__ << "could not parse physic system. Root tag id is incorrect (" << tag.id.c_str() << ").\n";
		return false;
	}

	load_settings = true;
	static	nString
				_ctci("ContactIteration"), _csti("ConstraintIteration"),
				_iter("IterationCount"), _sleep("SleepDelay"), _dist("DistanceConstraint"), _posc("PositionConstraint"),
				_hngc("HingeConstraint"), _velc("VelocityConstraint"),
				_frequency("Frequency");

	nMetaPool	pool;
	nMetaTag	*t;
	while	(t = tag.Pool(pool, &file))
	{
		TauConstraint	*cst = NULL;

				if	((t->id == _iter) && load_settings)		// Backward compatibility.
		{
			constraint_iteration = (uint)t->GetInteger();
			contact_iteration = (uint)t->GetInteger();
		}
		else	if	((t->id == _ctci) && load_settings)
				contact_iteration = (uint)t->GetInteger();
		else	if	((t->id == _csti) && load_settings)
				constraint_iteration = (uint)t->GetInteger();

		else	if	((t->id == _sleep) && load_settings)
				delay = (uint)t->GetInteger();

		else	if	((t->id == _frequency) && load_settings)
				fq = t->GetReal();

		// Constraints.
		else	if	(t->id == _dist)
		{
#if	__ENABLE_BULLET__
// TODO BULLET
#else
			TauDistanceConstraint	*c = new TauDistanceConstraint(*this, NULL, NULL, NULL, NULL);
			if	(c)
			{
				c->FromMetaTag(scene, file, *t, mapuid);
				scene.GetPhysicInterface().AddConstraint(c);
				cst = c;
			}
			else	__LOG_E__ << "failed to allocate a new distance constraint.\n";
#endif
		}
		else	if	(t->id == _posc)
		{
#if	__ENABLE_BULLET__
// TODO BULLET
#else
			TauPositionConstraint	*c = new TauPositionConstraint(*this, NULL, NULL, NULL, NULL);
			if	(c)
			{
				c->FromMetaTag(scene, file, *t, mapuid);
				scene.GetPhysicInterface().AddConstraint(c);
				cst = c;
			}
			else	__LOG_E__ << "failed to allocate a new position constraint.\n";
#endif
		}
		else	if	(t->id == _hngc)
		{
#if	__ENABLE_BULLET__
// TODO BULLET
#else
			TauHingeConstraint		*c = new TauHingeConstraint(*this, NULL, NULL, NULL, NULL, NULL);
			if	(c)
			{
				c->FromMetaTag(scene, file, *t, mapuid);
				scene.GetPhysicInterface().AddConstraint(c);
				cst = c;
			}
			else	__LOG_E__ << "failed to allocate a new hinge constraint.\n";
#endif
		}
		else	if	(t->id == _velc)
		{
#if	__ENABLE_BULLET__
// TODO BULLET
#else
			TauVelocityConstraint	*c = new TauVelocityConstraint(*this, NULL, NULL, NULL);
			if	(c)
			{
				c->FromMetaTag(scene, file, *t, mapuid);
				scene.GetPhysicInterface().AddConstraint(c);
				cst = c;
			}
			else	__LOG_E__ << "failed to allocate a new velocity constraint.\n";
#endif
		}

		// Add constraint to group.
//		if	(cst && group && group[0])
//			group[0]->Add(cst);
	}
	return true;
}


//--------------------------------------------------------
nMetaTag			*Tau::AsMetaTag(nMetaFile &file) const
//--------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("Tau");
	if	(!root)
	{
		__LOG_E__ << "could not create physic system root tag to serialize.\n";
		return NULL;
	}
	root->AddChild(new nMetaTag("ContactIteration", (int)contact_iteration));
	root->AddChild(new nMetaTag("ConstraintIteration", (int)constraint_iteration));
	root->AddChild(new nMetaTag("SleepDelay", (int)delay));
	root->AddChild(new nMetaTag("Frequency", fq));

	// Save constraints.
	nLinkedListPool	pool;
	for	(nLinkedListEntry *e; e = GetConstraintList().Pool(pool); )
		root->AddChild(((TauConstraint *)e)->AsMetaTag(file));
	return root;
}
