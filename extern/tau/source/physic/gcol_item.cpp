/*

GCollide	[Generic collision]
			Id: Library item.

			Emmanuel Julien 2004~2005.
			http://www.nengine.fr
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"
		#include	"../manager/manager.h"


//---------------------------------------------
nMatrix4			GColItem::GetMatrix() const
//---------------------------------------------
{
	nMatrix4	m(nMatrix4::FromMatrix3(orientation_matrix));
	m.SetRow(3, gposition);		// Equivalent to vec_pos * mtx_rot.
	return m;
}

//----------------------------------------------------
nMatrix4			GColItem::GetInverseMatrix() const
//----------------------------------------------------
{	return nMatrix4::FromMatrix3(orientation_matrix.Transpose()) * nMatrix4::TranslationMatrix(gposition.Reverse());	}

//-------------------------------------------------
void				GColItem::RecordPreviousState()
//-------------------------------------------------
{
	nLinkedListForeach(GColShape, s, shape_list)
		if	(s->active)
		{
			s->prv_matrix = s->matrix;
			s->prv_imatrix = s->imatrix;
		}
}

//------------------------------------------------
float				GColItem::GetTotalMass() const
//------------------------------------------------
{
	float			total = 0;
	nLinkedListForeach(GColShape, s, shape_list)
		total += s->mass;
	return total;
}

//-----------------------------------------------------------------------------------
GColShape			*GColItem::AddShapePolymesh(nGeometry *g, nVector *p, nVector *e)
//-----------------------------------------------------------------------------------
{
	GColShape	*shape = AddShape();
	if	(shape)
		shape->AsMesh(g, p, e);
	return shape;
}

//-------------------------------------------------------------------------------
GColShape			*GColItem::AddShapeCuboid(nVector *d, nVector *p, nVector *e)
//-------------------------------------------------------------------------------
{
	GColShape	*shape = AddShape();
	if	(shape)
		shape->AsCuboid(d, p, e);
	return shape;
}

//----------------------------------------------------------------------------
GColShape			*GColItem::AddShapeSphere(float r, nVector *p, nVector *e)
//----------------------------------------------------------------------------
{
	GColShape	*shape = AddShape();
	if	(shape)
		shape->AsSphere(r, p, e);
	return shape;
}

//---------------------------------------
GColShape			*GColItem::AddShape()
//---------------------------------------
{
	__SUBSYSTEM(System::SystemCollision);

	GColShape	*shape = new GColShape;
	if	(!shape)
		return NULL;
	shape->type = GColNone;
	shape->item = this;
	shape_list.Add(shape);
	return shape;
}

//-------------------------------------------------------
bool				GColItem::RemoveShape(GColShape *shp)
//-------------------------------------------------------
{
	return shape_list.Delete(shp);
}

//---------------------------------------------------------------
void				GColItem::ComputeWorldMinMax(nMinMax &minmax)
//---------------------------------------------------------------
{
	nMinMax		prvmm;
	bool		set = false;

	nLinkedListForeach(GColShape, s, shape_list)
	{
		switch	(s->type)
		{
			case	GColSphere:
			{
				nVector		r(s->scale.x, s->scale.x, s->scale.x);

				prvmm.mn = s->prv_matrix.GetRow(3) - r;
				prvmm.mx = s->prv_matrix.GetRow(3) + r;
				s->minmax.mn = s->matrix.GetRow(3) - r;
				s->minmax.mx = s->matrix.GetRow(3) + r;
			}
			break;

			case	GColCuboid:
			{
				nOBB	pobb(&s->prv_matrix.GetRow(3), &(s->scale * scale), &nMatrix3::FromMatrix4(s->prv_matrix));
				pobb.ComputeMinMax(prvmm);
				nOBB	obb(&s->matrix.GetRow(3), &(s->scale * scale), &nMatrix3::FromMatrix4(s->matrix));
				obb.ComputeMinMax(s->minmax);
			}
			break;

			case	GColMesh:
				// @FIXME THIS IS SLOW!
				if	(s->mesh_tree)
				{
					s->mesh_tree->GetGeometry()->ComputeMinMax(prvmm, &s->prv_matrix);
					s->mesh_tree->GetGeometry()->ComputeMinMax(s->minmax, &s->matrix);
				}
				break;

			default:
				break;
		}
		s->minmax.Grow(prvmm);

		if	(set)
			minmax.Grow(s->minmax);
		else
		{
			minmax = s->minmax;
			set = true;
		}
	}
}

//-----------------------------------------------
void				GColItem::SynchronizeShapes()
//-----------------------------------------------
{
	RecordPreviousState();

	nLinkedListForeach(GColShape, s, shape_list)
		if	(s->active)
		{
			nVector	spos(((s->position * scale) * orientation_matrix) + gposition);
			s->matrix = nMatrix4::FromMatrix3(orientation_matrix * s->orientation_matrix);// * nMatrix4::TranslationMatrix(mitem->GetBaseItem()->offset_position.Reverse());
			s->matrix.SetRow(3, spos);
			s->imatrix = s->matrix.FastInverse();
		}
}

//-------------------------------------------------------------------------------------------------------------------------------
void				GColItem::SynchronizeState(const nVector &position, const nMatrix3 *rotation, const nVector *scl, bool force)
//-------------------------------------------------------------------------------------------------------------------------------
{
	// TODO scale.
	active = (gposition != position) || (rotation && (orientation_matrix != *rotation));
	if	(!active && !force)
		return;

	old_gposition = reset ? position : gposition;
	gposition = position;

	if	(rotation)
	{
		old_orientation_matrix = reset ? *rotation : orientation_matrix;
		orientation_matrix = *rotation;
	}
	if	(scl)
		scale = *scl;

	SynchronizeShapes();

	if	(reset)
	{
		RecordPreviousState();
		ComputeWorldMinMax(world_minmax);
		reset = false;
	}
}

//-----------------------------------
void				GColItem::Reset()
//-----------------------------------
{
	active = false;
	collided = false;
	reset = true;

	gposition.Set(0, 0, 0);
	orientation_matrix = nMatrix3::IdentityMatrix();
	scale.Set(1, 1, 1);

	SynchronizeShapes();
	RecordPreviousState();
	ComputeWorldMinMax(world_minmax);
}

//-----------------------------------
void				GColItem::Setup()
//-----------------------------------
{
	nLinkedListForeach(GColShape, s, shape_list)
		if	((s->type == GColMesh) && !s->mesh_tree)
		{
			s->type = GColNone;
			__LOG_E__ << "Collision shape declared as mesh but has no mesh tree registered.\n";
		}
}

//-------------------------------
GColItem::GColItem(uint group_id)
//-------------------------------
{
	active = true;

	mitem = NULL;
	tau_item = NULL;
#ifndef	__GCOL_ENABLE_SAP__
	lock = false;
#endif
	self_mask = 1;
	col_mask = 1;	// Default unique group.

	Reset();
}
