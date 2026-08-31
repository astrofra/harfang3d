/*

GCollide	[Generic collision]
			Collision item library.

			Emmanuel Julien 2004.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"
		#include	"../manager/manager.h"


//------------------------------------------------------------------------
bool				GColShape::FromMetaTag(nMetaFile &file, nMetaTag &tag)
//------------------------------------------------------------------------
{
	if	(tag.id != "GColShape")
		__ERR__(__LOG_E__ << "Could not parse collision shape. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	active = false;
	type = GColNone;

	// Parse root tags.
	nString			_mss("Mass"), _damp("Damping"), _sfr("StaticFriction"), _dfr("DynamicFriction"), _res("Restitution"),
					_plm("Polymesh"), _msh("Mesh"), _dms("Dimensions"), _rds("Radius"), _pos("Position"), _ori("Orientation"),
					_act("Active"), _type("Type"), _aso("AnisotropicFriction"), _vsf("VStaticFriction"), _vdf("VDynamicFriction"),
					_scl("Scale");

	nMetaTag		*pt;
	nMetaPool		pool;

	while	(pt = tag.Pool(pool, &file))
	{
				if	(pt->id == _act)	active = true;
		else	if	(pt->id == _type)
		{
			nString		_type(pt->GetString());

					if	((_type == "Polymesh") || (_type == "Mesh"))
					type = GColMesh;
			else	if	(_type == "Sphere")
					type = GColSphere;
			else	if	(_type == "Cuboid")
					type = GColCuboid;
			else	type = GColNone;
		}

		else	if	(pt->id == _pos)
				position.FromMetaTag(*pt);
		else	if	(pt->id == _ori)
				orientation_matrix.FromMetaTag(file, *pt);

		else	if	(pt->id == _scl)
				scale.FromMetaTag(*pt);

		else	if	(pt->id == _rds)	// LEGACY
				scale.Set(pt->GetReal(), pt->GetReal(), pt->GetReal());
		else	if	(pt->id == _dms)	// LEGACY
				scale.FromMetaTag(*pt);

		else	if	((pt->id == _plm) || (pt->id == _msh))
		{
			nGeometry	*geo = GetItem()->GetMItem()->GetBaseItem()->GetEngine().LoadGeometry(pt->GetString(), false, true, true);
			mesh_tree = geo ? geo->GetTree() : NULL;
			if	(!mesh_tree)
				type = GColNone;
		}

		else	if	(pt->id == _mss)
				mass = pt->GetReal();

		else	if	(pt->id == _sfr)
				static_friction = pt->GetReal();
		else	if	(pt->id == _dfr)
				dynamic_friction = pt->GetReal();

		else	if	(pt->id == _aso)
				;//anisotropic_friction = true;
		else	if	(pt->id == _vsf)
				;//v_static_friction = pt->GetReal();
		else	if	(pt->id == _vdf)
				;//v_dynamic_friction = pt->GetReal();

		else	if	(pt->id == _res)
				restitution = pt->GetReal();

		else	__LOG_W__ << "Unknown tag '" << pt->id.c_str() << "' in <GColShape>.\n";
	}
	return true;
}

//--------------------------------------------------------------
nMetaTag			*GColShape::AsMetaTag(nMetaFile &file) const
//--------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("GColShape");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create collision shape root tag to serialize.\n", NULL)

	if	(active)
		root->AddChild(new nMetaTag("Active"));

	/*
		Barr: 01/12/2008

		Serialize type first because during load:
		eg. In the case of a mesh the 'Type' tag might declare a shape as a
		mesh even if the 'Polymesh' tag failed to create the mesh shape
		because of eg. a missing geometry. Thus leading to an incorrect mesh
		shape with no mesh tree.

		05/20/2008: And let's drink some more next time...
	*/
	if	(type != GColNone)
	{
		nString	_type;
		switch	(type)
		{
			case	GColMesh:		_type = "Mesh";		break;
			case	GColSphere:		_type = "Sphere";	break;
			case	GColCuboid:		_type = "Cuboid";	break;

			default:
				break;
		}
		root->AddChild(new nMetaTag("Type", _type.c_str()));
	}

	root->AddChild(position.AsMetaTag(file, "Position"));
	root->AddChild(orientation_matrix.AsMetaTag(file, "Orientation"));

	switch	(type)
	{
		case	GColMesh:
			if	(mesh_tree && mesh_tree->GetGeometry())
				root->AddChild(new nMetaTag("Mesh", mesh_tree->GetGeometry()->id.c_str()));
			break;

		case	GColSphere:	root->AddChild(new nMetaTag("Radius", scale.x));	break;
		case	GColCuboid:	root->AddChild(scale.AsMetaTag(file, "Dimensions"));		break;

		default:
			break;
	}

	if	(mass != Kg(1))
		root->AddChild(new nMetaTag("Mass", mass));
	root->AddChild(new nMetaTag("StaticFriction", static_friction));
	root->AddChild(new nMetaTag("DynamicFriction", dynamic_friction));
/*
	if	(anisotropic_friction)
		root->AddChild(new nMetaTag("AnisotropicFriction"));
	root->AddChild(new nMetaTag("VStaticFriction", v_static_friction));
	root->AddChild(new nMetaTag("VDynamicFriction", v_dynamic_friction));
*/
	root->AddChild(new nMetaTag("Restitution", restitution));

	return root;
}

//-----------------------------------------------------------------------
bool				GColItem::FromMetaTag(nMetaFile &file, nMetaTag &tag)
//-----------------------------------------------------------------------
{
	if	(tag.id != "GColItem")
		__ERR__(__LOG_E__ << "Could not parse collision item. Root tag id is incorrect (" << tag.id.c_str() << ").\n", false)

	Reset();

	self_mask = 1;
	col_mask = 1;

	shape_list.DeleteAll();

	// Parse root tags.
	nMetaTag		*pt;
	nMetaPool		pool;

	while	(pt = tag.Pool(pool, &file))
	{
				if	(pt->id == "Active")	active = true;
		else	if	(pt->id == "SelfMask")	self_mask = (uint)pt->GetInteger();
		else	if	(pt->id == "Mask")		col_mask = (uint)pt->GetInteger();
		else	if	(pt->id == "Shapes")
		{
			nMetaTag		*st;
			nMetaPool		spool;

			while	(st = pt->Pool(spool, &file))
			{
				GColShape	*shp = AddShape();
				if	(!shp->FromMetaTag(file, *st))
					RemoveShape(shp);
			}
		}
		else	__LOG_W__ << "Unknown tag '" << pt->id.c_str() << "' in <GColItem>.\n";
	}
	return true;
}

//-------------------------------------------------------------
nMetaTag			*GColItem::AsMetaTag(nMetaFile &file) const
//-------------------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("GColItem");
	if	(!root)
		__ERR__(__LOG_E__ << "Could not create collision item root tag to serialize.\n", NULL)

	if	(active)
		root->AddChild(new nMetaTag("Active"));
	root->AddChild(new nMetaTag("SelfMask", (int)self_mask));
	root->AddChild(new nMetaTag("Mask", (int)col_mask));

	// Dump shapes.
	if	(shape_list.GetCount())
	{
		nMetaTag		*st = root->AddChild(new nMetaTag("Shapes"));
		nLinkedListPool	pool;
		for	(GColShape *shp; shp = (GColShape *)shape_list.Pool(pool); )
			st->AddChild(shp->AsMetaTag(file));
	}
	return root;
}
