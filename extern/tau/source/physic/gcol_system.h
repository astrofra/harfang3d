/*

GCollide	[Generic collision]
			Collision system header.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__GCOLLIDE_SYSTEM__
#define	__GCOLLIDE_SYSTEM__


		#include	"framework/data_structure/pair.h"


struct	nPolygon;
class	nMaterial;
class	nGeometryBIH;
struct	GColTraceResult;
class	GColShape;
class	GColItem;
typedef	GColItem *	pGColItem;


		/*!
			Define to enable the sweep and prune collision broad phase.
			(Between 10 to 100 times faster than the brute force approach.)
		*/
#define	__GCOL_ENABLE_SAP__

		/*!
			Maximum default number of colliding items in the system when using
			the Sweep and Prune detection phase.
			(Default: 4096, uses 16KB of memory.)

			@see	SAP_Allocate().
		*/
#define	__GCOL_SAP_DEFAULT_MAX__				4096

		/*!
			Maximum number of contacts reported per pair in the collision
			callback system.

			@Note: The collision system has no contact limit per pair
			This is only a limit on the report to external systems through the
			collision monitor.
		*/
#define	__GCOL_MONITOR_MAX_CONTACT_PER_PAIR__	4


/*!
	@short	GCollide contact.
*/
struct	GColContact
{
		nVector		p;					///< Contact world position.
		float		d;					///< Contact depth.
};

/*!
	@short	GCollide collision structure.
	@author	Emmanuel Julien (ejulien@nworks.fr)
*/
struct	GColNode
{
		GColShape	*a, *b;				///< Colliding shapes.

		nVector		n;					///< Collision normal.
		float		d;					///< Overlap depth.
		ushort		ctc_start;			///< Start of contact list in system array.
		ushort		ctc_count;			///< Number of contacts.
};

/*!
	@short	GCollide fast collision query.

	The structure used by the standard game collision code path.
	This code path usually does not use advanced collision informations
	such as exact contact points.

	@author Emmanuel Julien (ejulien@nworks.fr)
*/
struct		GColFastCollisionQuery
{
		uint			ip;				///< Polygon indices.
		uint			cnt;			///< Collision routine iteration count.

		nGeometryBIH	*tree;

		nVector			itr;			///< Intersection point.
		nVector			nrm;			///< Intersection point normal.
		nMaterial		*mat;			///< Pointer to material.

		bool			ovel_set;
		nVector		ovel;
};

/*!
	@short	GCollide system statistics.

	@note	Benchmark values are normalized to the second
			(ie. divided by the framework clock frequency).
	@see	bfmk::GetClockFrequency().

	@author Emmanuel Julien (ejulien@nworks.fr)
*/
struct		GColStatistics
{
		uint			spheresphere_count;
		uint			sphereobb_count;
		uint			obbobb_count;
		uint			polyobb_count;
		uint			polysphere_count;
		uint			polynode_merge;
};

/*!
	@short	GCollide generic contact report collision structure.
*/
struct		GColCtcInfo
{
		nVector			*n;				///< Collision normal output.
		float			*d;				///< Collision depth output.

		uint			cmax;			///< Maximum contact to report.
		nVector			*p;				///< Contact output in world space.
		float			*t;				///< Per-contact depth output.

		GColCtcInfo()
		{
			n = p = NULL;
			d = t = NULL;
			cmax = 0;
		};
};

/*
	@short	GCollide contact pair.
*/
struct		GColPair : public ntPairItem <GColItem>
{
		uint			ctc_count;
		nVector			p[__GCOL_MONITOR_MAX_CONTACT_PER_PAIR__],
						n[__GCOL_MONITOR_MAX_CONTACT_PER_PAIR__];
		float			d[__GCOL_MONITOR_MAX_CONTACT_PER_PAIR__];

		/// Store pair contact.
		void			StoreContact(const nVector &p, const nVector &n, float d);

		GColPair(GColItem *a, GColItem *b) : ntPairItem <GColItem> (a, b)
		{	ctc_count = 0;	}
};

/*!
	@short	GCollide interference/collision detection package.

	@author Emmanuel Julien (ejulien@nworks.fr)
*/
class		GCollide
{
protected:

/*!
	@name	Internals.
	@{
*/
		nLinkedList		item_list;		///< Collision managed item list.
		nLinkedList		freeze_list;	///< Freeze item list.
		ntPairList <GColPair, GColItem>
						collision_pair;	///< Collision monitor.

		GColStatistics	statistics;		///< System statistics.

		/// Collision callback function.
static	void (*CB_Collision)(uint count, GColNode *node, GColContact *contact);

		/// Merge polygon node.
		void			AppendMergeNode
									(
										GColShape *sc,
										GColShape *dc,
										uint nctc,
										GColCtcInfo &cinfo,
										GColNode *node,
										uint *node_count,
										GColContact *ctc,
										uint *ctc_count
									);
		/// Populate collision node list for a mesh shape.
		bool			PopulatePolymeshCollision
									(
										GColShape *sc,
										GColShape *dc,
										GColNode *node,
										uint *node_count,
										GColContact *contact,
										uint *contact_count
									);
		/// Populate collision node list for a convex shape.
		bool			PopulateConvexCollision
									(
										GColShape *sc,
										GColShape *dc,
										GColNode *node,
										uint *node_count,
										GColContact *contact,
										uint *contact_count
									);
		/// Populate collision node list for a given shape pair.
		bool			PopulateShapeCollision
									(
										GColShape *sc,
										GColShape *dc,
										GColNode *node,
										uint *node_count,
										GColContact *contact,
										uint *contact_count
									);
/// @}

/*!
	@name	Collision core.
	@{
*/
		/// Collide sphere with a polygon list.
		bool			CollideSpherePolygonList
									(
										GColFastCollisionQuery *query,
										GColItem *item,
										nVector *vel,
										float vel_len
									);
		/// Collide a swept sphere with a mesh.
		bool			CollideSweptSphereMesh
									(
										GColFastCollisionQuery *query,
										GColItem *item
									);

public:

		/*!
			@short	Test OBB/OBB overlap.

			@param	a			First OBB in pair to test.
			@param	b			First OBB in pair to test.
			@param	normal		The contact normal if any.
								Pass NULL to disable all contact determination
								code and subsequent tests, only the overlapping
								test will take place.
			@param	depth		Penetration depth output.
								Moving one of the OBB along depth * ctc_normal
								will resolve the overlap.
								Pass NULL to disable feature identification code
								and all subsequent tests.
			@param	contact		An array of contact location in world space.
								Pass NULL to disable contact determination.
			@param	ctc_depth	An array of depth for each contact.
								Pass NULL to disable contact depth reporting.
			@param	max_contact	The maximum number of contacts to report.
								Passing 0 gives the same result as passing NULL
								as the contact vector array.
			@return				0 if no overlap was detected, the number of
								contacts if they were to be determined, 1
								otherwise.

			@note	Uses the separating axis method as described in Eberly's
					'Dynamic Collision Detection' paper.
		*/
static	int				OBBOverlapOBB
									(
										const nOBB &a,
										const nOBB &b,
										GColCtcInfo *cinfo = NULL
									);
		/*!
			@short	Test OBB/Polymesh overlap.

			@param	a			OBB to test.
			@param	b			Polymesh transformation matrix.
			@param	p			Polygon to test.
			@param	geometry	The geometry the polygon belongs to.
			@param	normal		The contact normal if any.
								Pass NULL to disable all contact determination
								code and subsequent tests, only the overlapping
								test will take place.
			@param	depth		Penetration depth output.
								Moving one of the object along depth * ctc_normal
								will resolve the overlap.
								Pass NULL to disable feature identification code
								and all subsequent tests.
			@param	contact		An array of contact location in world space.
								Pass NULL to disable contact determination.
			@param	ctc_depth	An array of depth for each contact.
								Pass NULL to disable contact depth reporting.
			@param	max_contact	The maximum number of contacts to report.
								Passing 0 gives the same result as passing NULL
								as the contact vector array.
			@return				0 if no overlap was detected, the number of
								contacts if they were to be determined, 1
								otherwise.

			@note	Uses the separating axis method as described in Eberly's
					'Dynamic Collision Detection' paper.
		*/
static	int				OBBOverlapPolygon
									(
										nOBB &a,
										nMatrix4 &b,
										nPolygon &p,
										nGeometry &geometry,
										GColCtcInfo *cinfo = NULL
									);
static	int				SweptSpherePolygon
									(
										nVector &prv_pos,
										nVector &pos,
										float radius,
										uint i_poly,
										nGeometry &geometry,
										GColCtcInfo *cinfo = NULL
									);
static	int				OBBSweptSphere
									(
										nOBB &prv_obb,
										nOBB &obb,
										nVector &prv_pos,
										nVector &pos,
										float radius,
										GColCtcInfo *cinfo = NULL
									);
static	int				SphereSweptSphere
									(
										nVector &prv_pos_a,
										nVector &pos_a,
										float radius_a,
										nVector &prv_pos_b,
										nVector &pos_b,
										float radius_b,
										GColCtcInfo *cinfo = NULL
									);
/// @}

#ifdef	__GCOL_ENABLE_SAP__
/*!
	@name	Sweep and Prune.
	@{
*/
protected:

		uint			SAP_totalsize;	///< SAP array total size.
		uint			SAP_arraysize;	///< SAP array size.
		bool			use_sap;		///< SAP enable flag.

		/// SAP array for temporal sort.
		pGColItem		*SAP_array;
		/// Sweep and Prune temporal sort.
		void			SAP_Sort();
		/// Free SAP arrays.
		void			SAP_Free();

public:
		/// Enable/disable the sweep and prune broad phase collision detection.
		void			EnableSweepAndPrune(bool use = true)
		{	use_sap = use;	}

		/// Allocate SAP arrays.
		void			SAP_Allocate(uint size);
/// @}
#endif

public:

/*!
	@name	Setup.
	@{
*/
		/*!
			@short	Set a collision callback function.

			@note	The callback function is sent an array of collision nodes
					once all items in the system have been processed.
			@see	GColNode.
		*/
static	void			SetCollisionCallback
									(
										void (*func)(uint count, GColNode *node, GColContact *contact) = NULL
									)
		{
			CB_Collision = func;
		}

		/// Reset collision engine.
		void			Reset();

		/// Geometry list.
		nList <nGeometry *>
						geo_list;

		/// Free system, optionally free item array memory.
		void			Free(bool free_data = false);
/// @}

/*!
	@name	Runtime.
	@{
*/
		/// Return last update execution statistics.
const	GColStatistics	&QueryStatistics() const
		{	return statistics;	}
		/// Return collision monitor.
		ntPairList <GColPair, GColItem>
						&GetCollisionMonitor()
		{	return collision_pair;	}
#ifdef	__GCOL_ENABLE_SAP__
		/*!
			@short	Display SAP debug infos.

			Displays all system items AABB. When an item is active its AABB is
			displayed in white, when inactive in dark red.
		*/
		void			DBG_SAP(nRenderer &render);
#endif
		/*!
			@short	Display item collision structure.

			Display the current shape configuration of all items in the system.
		*/
		void			DBG_Item(nRenderer &render);

		/// Return the collision item list.
const	nLinkedList		&GetList() const
		{	return item_list;	}
		/// Register a new item in the collision system.
		void			AddItem(GColItem &item);
		/// Unregister an item.
		void			RemoveItem(GColItem &item);

		/// Deactivate an item from the collision detection.
		bool			FreezeItem(GColItem &item, bool freeze = true);

		/*!
			@short	Quick test collision between two items.

			Returns 'true' as soon as a shape/shape overlap is found.

			@note	This function cannot not return contacts.
			@see	PopulateItemCollision().
		*/
		bool			TestItemCollision(GColItem *sc, GColItem *dc, bool ignore_mask = false);
		/*!
			@short	Populate collision node list for a given item.

			This function performs a full shape/shape test optionally
			collecting contacts along the way.

			@note	If you only need to quickly test for item overlap then use
					the much faster TestItemCollision().
		*/
		bool			PopulateItemCollision
									(
										GColItem *sc,
										GColItem *dc,
										GColNode *node,
										uint *node_count,
										GColContact *contact,
										uint *contact_count
									);
		/*!
			@short	Populate collision node list for the entire system.

			@param	node				A node array to fill with overlap
										informations.
			@param	node_max			The node array size.
			@param	contact				A contact array to fill with contact
										informations. Pass NULL to disable
										contact determination.
			@param	contact_max			The contact array size.
			@param	prohibit_callback	Disable the contact callback.

			@see	GColNode.
		*/
		uint			PopulateSystemCollision
									(
										GColNode *node,
										uint node_max,
										GColContact *contact,
										uint contact_max,
										bool prohibit_callback = false
									);

		/*!
			@short	Raytrace the collision system.

			@todo	Implement some spatial subdivision structure.
			@see	GColTraceMask
		*/
		bool			Raytrace(nVector &from, nVector &d, GColTraceResult &result, uint mask = ~0, uint shape_mask = ~0);

		/*!
			@short	A proximity query.
			Returns entities around a given item.

			@note	The query supports group_id bitmask as a filter.
		*/
		uint			ItemProximityQuery
									(
										GColItem &i,
										GColItem **list,
										uint max,
										uint group_id = ~0
									);
/// @}

/*!
	@name	System Serialization.
	@{
*/
		/// @see bfmk_MLEntity
		bool			FromMetaTag(nMetaFile &file, nMetaTag &tag, bool load_settings = true);
		/// @see bfmk_MLEntity
		nMetaTag		*AsMetaTag(nMetaFile &file) const;
/// @}

		GCollide();
		~GCollide();
};


#endif
