/*

Tau			[Physic engine]
			Written by Emmanuel Julien.
			All rights reserved 2000~2005.

			Emmanuel Julien
			http://www.nengine.fr
			mailto:ejulien@nengine.fr

*/


		#include	"physic.h"
		#include	"../manager/manager.h"


//--------------------------------------------------------------------------------------------------------------
bool				TauHingeConstraint::FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid)
//--------------------------------------------------------------------------------------------------------------
{
	if	(tag.id != "HingeConstraint")
		__ERR__(__LOG_E__ << "Could not parse hinge constraint. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	nMetaPool		pool;
	nMetaTag		*t;

	while	(t = tag.Pool(pool, &file))
	{
				if	(t->id == "Axis")		axis.FromMetaTag(*t);

		else	if	(t->id == "Constraint")
				TauConstraint::FromMetaTag(scene, file, *t, mapuid);

		else	__LOG_W__ << "Unexpected <" << t->id.c_str() << "> sub-tag in <HingeConstraint>.\n";
	}
	return true;
}

//-----------------------------------------------------------------
nMetaTag			*TauHingeConstraint::AsMetaTag(nMetaFile &file)
//-----------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("HingeConstraint");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create hinge constraint root tag to serialize.\n", NULL)

	root->AddChild(axis.AsMetaTag(file, "Axis"));
	root->AddChild(TauConstraint::AsMetaTag(file));
	return root;
}

//-----------------------------------------------------------------------------------------------------------------
bool				TauPositionConstraint::FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid)
//-----------------------------------------------------------------------------------------------------------------
{
	if	(tag.id != "PositionConstraint")
		__ERR__(__LOG_E__ << "Could not parse position constraint. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	nMetaPool		pool;
	nMetaTag		*t;

	while	(t = tag.Pool(pool, &file))
	{
				if	(t->id == "Dof")		dof = (uint)t->GetInteger();

		else	if	(t->id == "Constraint")
				TauConstraint::FromMetaTag(scene, file, *t, mapuid);

		else	__LOG_W__ << "Unexpected <" << t->id.c_str() << "> sub-tag in <PositionConstraint>.\n";
	}
	return true;
}

//--------------------------------------------------------------------
nMetaTag			*TauPositionConstraint::AsMetaTag(nMetaFile &file)
//--------------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("PositionConstraint");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create position constraint root tag to serialize.\n", NULL)

	root->AddChild(new nMetaTag("Dof", (int)dof));
	root->AddChild(TauConstraint::AsMetaTag(file));
	return root;
}

//-----------------------------------------------------------------------------------------------------------------
bool				TauVelocityConstraint::FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid)
//-----------------------------------------------------------------------------------------------------------------
{
	if	(tag.id != "VelocityConstraint")
		__ERR__(__LOG_E__ << "Could not parse velocity constraint. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	nMetaPool		pool;
	nMetaTag		*t;

	while	(t = tag.Pool(pool, &file))
	{
				if	(t->id == "Mask")		mask = (uint)t->GetInteger();

		else	if	(t->id == "Constraint")
				TauConstraint::FromMetaTag(scene, file, *t, mapuid);

		else	__LOG_W__ << "Unexpected <" << t->id.c_str() << "> sub-tag in <VelocityConstraint>.\n";
	}
	return true;
}

//--------------------------------------------------------------------
nMetaTag			*TauVelocityConstraint::AsMetaTag(nMetaFile &file)
//--------------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("VelocityConstraint");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create velocity constraint root tag to serialize.\n", NULL)

	root->AddChild(new nMetaTag("Mask", (int)mask));
	root->AddChild(TauConstraint::AsMetaTag(file));
	return root;
}

//----------------------------------------------------------------------------------------------------------------
bool				TauAngularConstraint::FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid)
//----------------------------------------------------------------------------------------------------------------
{
	if	(tag.id != "AngularConstraint")
		__ERR__(__LOG_E__ << "Could not parse angular constraint. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	nMetaPool		pool;
	nMetaTag		*t;

	while	(t = tag.Pool(pool, &file))
	{
				if	(t->id == "Dof")		dof = (uint)t->GetInteger();

		else	if	(t->id == "Constraint")
				TauConstraint::FromMetaTag(scene, file, *t, mapuid);

		else	__LOG_W__ << "Unexpected <" << t->id.c_str() << "> sub-tag in <AngularConstraint>.\n";
	}
	return true;
}

//-------------------------------------------------------------------
nMetaTag			*TauAngularConstraint::AsMetaTag(nMetaFile &file)
//-------------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("AngularConstraint");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create angular constraint root tag to serialize.\n", NULL)

	root->AddChild(new nMetaTag("Dof", (int)dof));
	root->AddChild(TauConstraint::AsMetaTag(file));
	return root;
}

//-----------------------------------------------------------------------------------------------------------------
bool				TauDistanceConstraint::FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid)
//-----------------------------------------------------------------------------------------------------------------
{
	if	(tag.id != "DistanceConstraint")
		__ERR__(__LOG_E__ << "could not parse distance constraint. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	nMetaPool		pool;
	nMetaTag		*t;

	while	(t = tag.Pool(pool, &file))
	{
				if	(t->id == "Min")		min_dist = t->GetReal();
		else	if	(t->id == "Max")		max_dist = t->GetReal();

		else	if	(t->id == "Constraint")
				TauConstraint::FromMetaTag(scene, file, *t, mapuid);

		else	__LOG_W__ << "Unexpected <" << t->id.c_str() << "> sub-tag in <DistanceConstraint>.\n";
	}
	return true;
}

//--------------------------------------------------------------------
nMetaTag			*TauDistanceConstraint::AsMetaTag(nMetaFile &file)
//--------------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("DistanceConstraint");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create distance constraint root tag to serialize.\n", NULL)

	root->AddChild(new nMetaTag("Min", min_dist));
	root->AddChild(new nMetaTag("Max", max_dist));
	root->AddChild(TauConstraint::AsMetaTag(file));
	return root;
}

//---------------------------------------------------------------------------------------------------------
bool				TauConstraint::FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid)
//---------------------------------------------------------------------------------------------------------
{
#if	(!__ENABLE_BULLET__)
// TODO BULLET
	if	(tag.id != "Constraint")
		__ERR__(__LOG_E__ << "Could not parse constraint. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	nMetaPool		pool;
	nMetaTag		*t;
	enable = false;

	while	(t = tag.Pool(pool, &file))
	{
				if	(t->id == "Id")			id = t->GetString();

		else	if	(t->id == "Enable")
				enable = true;
		else	if	(t->id == "Strength")
				SetStrength(t->GetReal());
		else	if	(t->id == "MaxErrorCorrection")
				maximum_erc = t->GetReal();

		else	if	(t->id == "Anchor")
		{
			nMItem	*mitem = scene.FindFromUid(mapuid ? mapuid[t->GetInteger()] : t->GetInteger());
			SetAnchor(mitem ? &mitem->GetPhysicInterface() : NULL);
		}
		else	if	(t->id == "Target")
		{
			nMItem	*mitem = scene.FindFromUid(mapuid ? mapuid[t->GetInteger()] : t->GetInteger());
			SetTarget(mitem ? &mitem->GetPhysicInterface() : NULL);
		}

		else	if	(t->id == "AnchorHook")
				hook[0].FromMetaTag(*t);
		else	if	(t->id == "TargetHook")
				hook[1].FromMetaTag(*t);

		else	__LOG_W__ << "Unexpected <" << t->id.c_str() << "> sub-tag in <Constraint>.\n";
	}
#endif
	return true;
}

//------------------------------------------------------------
nMetaTag			*TauConstraint::AsMetaTag(nMetaFile &file)
//------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("Constraint");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create constraint root tag to serialize.\n", NULL)

	root->AddChild(new nMetaTag("Id", id.c_str()));

	if	(enable)
		root->AddChild(new nMetaTag("Enable"));
	root->AddChild(new nMetaTag("Strength", strength));
	root->AddChild(new nMetaTag("MaxErrorCorrection", maximum_erc));

	if	(item[0])
		root->AddChild(new nMetaTag("Anchor", item[0]->GetMItem()->GetUid()));
	root->AddChild(hook[0].AsMetaTag(file, "AnchorHook"));

	if	(item[1])
		root->AddChild(new nMetaTag("Target", item[1]->GetMItem()->GetUid()));
	root->AddChild(hook[1].AsMetaTag(file, "TargetHook"));
	return root;
}
