/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//----------------------------------------------------------------------
bool				TauItem::FromMetaTag(nMetaFile &file, nMetaTag &tag)
//----------------------------------------------------------------------
{
	if	(tag.id != "TauItem")
		__ERR__(__LOG_E__ << "Could not parse Physic item. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	mode = 0;
	flag.Reset();
	mass = Kg(1);
	gravity_scale = 1.f;

	// Parse root tags.
	nMetaTag		*pt;
	nMetaPool		pool;

	while	(pt = tag.Pool(pool, &file))
	{
		if	(pt->id == "Mode")
		{
			nMetaTag		*mt;
			nMetaPool		pool;

			while	(mt = pt->Pool(pool, &file))
			{
						if	(mt->id == "Rigid")
						mode |= ModeRigidBody;
				else	if	(mt->id == "Soft")
						mode |= ModeSoftBody;
			}
		}
		else	if	(pt->id == "Flag")
		{
			nMetaTag		*st;
			nMetaPool		pool;

			while	(st = pt->Pool(pool, &file))
			{
						if	(st->id == "NoGravity")
						flag.Set(FlagNoGravity);
				else	if	(st->id == "NoForceField")
						flag.Set(FlagNoForceField);
				else	if	(st->id == "Mobile")
						flag.Set(FlagMobile);
				else	if	(st->id == "Ghost")
						flag.Set(FlagGhost);
			}
		}

		else	if	(pt->id == "Mass")
				mass = pt->GetReal();
		else	if	(pt->id == "ITensor")
				inv_tensor.FromMetaTag(file, *pt);
		else	if	(pt->id == "Pivot")
				pivot.FromMetaTag(*pt);

		else	if	(pt->id == "LinearThreshold")
				linear_threshold = pt->GetReal();
		else	if	(pt->id == "AngularThreshold")
				k_angular_threshold = pt->GetReal();

		else	if	(pt->id == "LinearDamping")
				linear_damping = pt->GetReal();
		else	if	(pt->id == "AngularDamping")
				angular_damping = pt->GetReal();

		else	if	(pt->id == "GravityScale")
				gravity_scale = pt->GetReal();

		else	__LOG_W__ << "Unknown tag '" << pt->id.c_str() << "' in <TauItem>.\n";
	}
	return true;
}

//------------------------------------------------------------
nMetaTag			*TauItem::AsMetaTag(nMetaFile &file) const
//------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("TauItem");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create physic item root tag to serialize.\n", NULL);

	if	(mode)
	{
		nMetaTag	*mtag = new nMetaTag("Mode");

		root->AddChild(mtag);
		if	(mode & ModeRigidBody)
			mtag->AddChild(new nMetaTag("Rigid"));
		if	(mode & ModeSoftBody)
			mtag->AddChild(new nMetaTag("Soft"));
	}
	if	(flag.Get())
	{
		nMetaTag	*ftag = new nMetaTag("Flag");

		root->AddChild(ftag);
		if	(flag.Test(FlagNoGravity))
			ftag->AddChild(new nMetaTag("NoGravity"));
		if	(flag.Test(FlagNoForceField))
			ftag->AddChild(new nMetaTag("NoForceField"));
		if	(flag.Test(FlagMobile))
			ftag->AddChild(new nMetaTag("Mobile"));
		if	(flag.Test(FlagGhost))
			ftag->AddChild(new nMetaTag("Ghost"));
	}

	if	(mass != Kg(1))
		root->AddChild(new nMetaTag("Mass", mass));

	root->AddChild(inv_tensor.AsMetaTag(file, "ITensor"));
	root->AddChild(pivot.AsMetaTag(file, "Pivot"));

	root->AddChild(new nMetaTag("LinearThreshold", linear_threshold));
	root->AddChild(new nMetaTag("AngularThreshold", k_angular_threshold));

	root->AddChild(new nMetaTag("LinearDamping", linear_damping));
	root->AddChild(new nMetaTag("AngularDamping", angular_damping));

	root->AddChild(new nMetaTag("GravityScale", gravity_scale));

	return root;
}
