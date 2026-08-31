/*

GCollide	[Generic collision]
			Collision item library.

			Emmanuel Julien 2004.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__GCOLLIDE_ITEM__
#define	__GCOLLIDE_ITEM__


		#include	"framework/math/matrix4.h"
		#include	"framework/geometry/boundingbox.h"


class	nItem;
class	nGeometry;
class	nRenderer;
class	nMItem;
class	TauItem;
class	GColShape;
struct	GColTraceResult;


/*!
	@short	GCollide item.

	@author Emmanuel Julien
*/
class	GColItem : public nLinkedListEntry
{
		friend	class	nMItem;
		friend	class	GCollide;

protected:

		nVector			old_gposition;	///< Previous world position.
		nVector			gposition;		///< World position.
		nVector			scale;			///< World scale.

		/// Previous orientation matrix.
		nMatrix3		old_orientation_matrix;
		/// orientation matrix.
		nMatrix3		orientation_matrix;

#ifndef	__GCOL_ENABLE_SAP__
		bool			lock;			///< Collision lock (internal use only).
#endif
		bool			active;			///< Item is active.

		nMItem			*mitem;			///< Managed item.
		TauItem			*tau_item;		///< Physic item.

/*!
	@name	Collision shapes.

	A collision item is composed of one or more collision shapes.

	@see	GColShape, GColShapeType.
	@{
*/
		nOBB			item_minmax;	///< Item space minmax.

		bool			collided;		///< Has collided.
		bool			reset;

public:

		nLinkedList		shape_list;		///< The item's shape list.

		uint			self_mask,		///< Self mask.
						col_mask;		///< Collision mask.

		/*!
			@short	Was this item involved in a collision.

			Return whether a collision involving this item was registered
			during last update.

			@warning	This flag is <b>only</b> valid after a call
						to PopulateSystemCollision().
		*/
		bool			Collided() const		{	return collided;	}
		/*
			@short	Return whether the item is currently active.
			@note	An inactive item will still register collisions
					it is simply considered immobile and several
					optimizations do apply in this case.
		*/
		bool			IsActive() const		{	return active;	}

		/// Synchronize item's shapes.
		void			SynchronizeShapes();

		/// Return shape list total mass.
		float			GetTotalMass() const;

		/// Return a pointer to the managed item associated with this collision item.
		nMItem			*GetMItem() const		{	return mitem;	}
		/// Return a pointer to an associated physic item.
		TauItem			*GetTauItem() const		{	return tau_item;	}

		/// Free all shape buffers.
		void			ClearShapeList();

		/// Add shape to item.
		GColShape		*AddShape();
		/// Remove shape from item.
		bool			RemoveShape(GColShape *shp);
		/// Add a sphere shape to the item.
		GColShape		*AddShapeSphere(float radius = Mtr(1), nVector *position = 0, nVector *euler = 0);
		/// Add a cuboid shape to the item.
		GColShape		*AddShapeCuboid(nVector *dimensions = 0, nVector *position = 0, nVector *euler = 0);
		/// Add a polygon mesh shape to the item.
		GColShape		*AddShapePolymesh(nGeometry *geometry, nVector *position = 0, nVector *euler = 0);

		/// Display all collision shapes in the item.
		void			DBG_Shape(nRenderer &render, uint lit_count = 0, GColShape **lit_list = 0);
/*!
	@}
	@name	Sweep and prune.
	@{
*/
protected:

		nMinMax		world_minmax;	///< World min-max.

		/// Compute item's min~max after transformation to world space.
		void			ComputeWorldMinMax(nMinMax &minmax);
/*!
	@}
	@name	Setup.
	@{
*/
public:

		/// Setup internals.
		void			Setup();
		/// Reset internals.
		void			Reset();
/*!
	@}
	@name	Runtime.
	@{
*/
		/// Return the item's world minmax structure.
		nMinMax			&GetWorldMinMax()		{	return world_minmax;	}
		/// Return item matrix.
		nMatrix4		GetMatrix() const;
		/// Return item inverse matrix.
		nMatrix4		GetInverseMatrix() const;

		/// Raytrace item.
		bool			Raytrace(nVector &from, nVector &d, GColTraceResult &result, uint shape_mask = ~0);

		/// Synchronize database entry state.
		void			SynchronizeState(const nVector &position, const nMatrix3 *rotation = 0, const nVector *scale = 0, bool force = false);
		/// Synchronize previous state to current state.
		void			RecordPreviousState();
		/// Force item activation.
		void			Activate()			{	active = true;	}
		/// Deactivate item until a next call to synchronize reactivate it.
		void			Deactivate()		{	active = false;	}
/// @}

/*!
	@name	System Serialization.
	@{
*/
		/// @see bfmk_MLEntity
		bool			FromMetaTag(nMetaFile &file, nMetaTag &tag);
		/// @see bfmk_MLEntity
		nMetaTag		*AsMetaTag(nMetaFile &file) const;
/// @}

		GColItem(uint group_id = 1);
};


/// Pointer to item.
typedef		GColItem *		pGColItem;


#endif	// __GCOLLIDE_ITEM__
