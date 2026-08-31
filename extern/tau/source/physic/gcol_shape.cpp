/*

GCollide	Collision library.
			Written by Emmanuel Julien.
			All rights reserved 2000~2005.

			Emmanuel Julien
			http://www.nengine.fr
			mailto:ejulien@nengine.fr

*/


		#include	"physic.h"


//-------------------------------------------------------------------------
bool				GColShape::AsMesh(nGeometry *g, nVector *p, nVector *e)
//-------------------------------------------------------------------------
{
	nVector	pos(0, 0, 0);
	nVector	eul(0, 0, 0);

	if	(!g)
	{
		type = GColNone;
		__ERR__(__LOG_E__ << "Cannot create mesh shape with no geometry.\n", false);
	}
	if	(!p)	p = &pos;
	if	(!e)	e = &eul;

	g->CreateTree();
	type = GColMesh;
	mesh_tree = g->GetTree();
	position = *p;
	orientation_matrix = nMatrix3::FromEuler(e->x, e->y, e->z);
	return true;
}

//-------------------------------------------------------------------------
bool				GColShape::AsCuboid(nVector *d, nVector *p, nVector *e)
//-------------------------------------------------------------------------
{
	nVector	dim(1, 1, 1);
	nVector	pos(0, 0, 0);
	nVector	eul(0, 0, 0);

	if	(!d)	d = &dim;
	if	(!p)	p = &pos;
	if	(!e)	e = &eul;

	type = GColCuboid;
	position = *p;
	orientation_matrix = nMatrix3::FromEuler(e->x, e->y, e->z);
	scale = *d;
	return true;
}

//----------------------------------------------------------------------
bool				GColShape::AsSphere(float r, nVector *p, nVector *e)
//----------------------------------------------------------------------
{
	nVector	pos(0, 0, 0);
	nVector	eul(0, 0, 0);

	if	(!p)	p = &pos;
	if	(!e)	e = &eul;

	type = GColSphere;
	position = *p;
	orientation_matrix = nMatrix3::FromEuler(e->x, e->y, e->z);
	scale.Set(r, r, r);
	return true;
}

//--------------------
GColShape::GColShape()
//--------------------
{
	matrix = nMatrix4::IdentityMatrix();
	imatrix = nMatrix4::IdentityMatrix();
	prv_matrix = nMatrix4::IdentityMatrix();
	prv_imatrix = nMatrix4::IdentityMatrix();

	active = true;
	position.Set(0, 0, 0);

	mesh_tree = NULL;
	scale.Set(1, 1, 1);

	mass = Kg(1);

	SetFriction();
/*
	SetLateralFriction();
	anisotropic_friction = false;
*/
	restitution = 0.1f;
}
