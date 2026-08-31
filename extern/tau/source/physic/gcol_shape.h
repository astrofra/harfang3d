/*

GCollide	[Generic collision]
			Collision item library.

			Emmanuel Julien 2004.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__GCOLLIDE_SHAPE__
#define	__GCOLLIDE_SHAPE__


		#include	"framework/geometry/boundingbox.h"


class	GColItem;
class	GColShape;


enum	GColTraceMask
{
		GColTraceMesh		=	(1 << 0),
		GColTraceSphere		=	(1 << 1),
		GColTraceCuboid		=	(1 << 2)
};

/*!
	@short	GCollide raytrace result structure.
*/
struct		GColTraceResult
{
		nVector			n;				///< Trace hit point normal.
		nVector			p;				///< Trace hit point world location.
		GColShape		*shape;			///< Trace collision shape.
		nMaterial		*m;
		float			d;				///< Distance from ray origin to hit point.

		GColTraceResult()
		{
			shape = 0;
			m = 0;
			d = -1;
		}
};

/*!
	@short	GCollide collision primitive types.
	@warning	Any changes in the ordering of this enumeration	elements will
				require modifications to the collision dispatcher.
*/
enum	GColShapeType
{
		GColNone			=	0,
		GColMesh,						///< Mesh.
		GColSphere,						///< Sphere.
		GColCuboid						///< Cuboid.
};

/*!
	@short	GCollide shape.

	Component of a collision item.

	@see	GColItem.
	@author Emmanuel Julien
*/
class	GColShape : public nLinkedListEntry
{
		friend	class	GCollide;
		friend	class	GColItem;
		friend	class	Tau;
		friend	class	TauItem;

protected:

		GColItem		*item;

public:

		/// Return the collision item this shape belongs to.
		GColItem		*GetItem() const
		{	return item;	}

/*!
	@name	Shape properties.
	@{
*/
		GColShapeType	type;				///< Shape type.
		bool			active;				///< Active/unactive.

		/// Shape position in item's space.
		nVector			position;
		/// Shape orientation in item's space.
		nMatrix3		orientation_matrix;
		/// Shape scale.
		nVector			scale;

		nMinMax			minmax;				///< Shape world minmax.
		nMatrix4		matrix,				///< Shape world matrix.
						imatrix,			///< Shape inverse world matrix.
						prv_matrix,
						prv_imatrix;

		float			mass,				///< Shape mass.
						static_friction,	///< Static friction.
						dynamic_friction,	///< Dynamic friction.
						restitution;		///< Restitution.
/*!
	@}
	@name	Polymesh shape.
	@{
*/
		nGeometryBIH	*mesh_tree;
/// @}

public:

		/*!
			@short	Set shape friction (longitudinal with anisotropic friction enabled).

			@note	The static friction is a force opposing external forces
					acting on a resting body. The dynamic friction is a force
					opposing the motion of a moving body.
			@see	SetAnisotropicFriction().
		*/
		void			SetFriction(float dync = 0.5f, float sttc = 0.75f)
		{	dynamic_friction = dync; static_friction = sttc;	}

		/*!
			@short	Activate or deactivate a collision shape.

			@note	Unactive collision shape are still taken into account
					when calculating physic items properties.
					Only the collision part is deactivated.
		*/
		void			Active(bool a = true)
		{	active = a;	}

		// Switch shape to a sphere collider.
		bool			AsSphere(float radius = Mtr(1), nVector *position = NULL, nVector *euler = NULL);
		// Switch shape to a cuboid collider.
		bool			AsCuboid(nVector *dimensions = NULL, nVector *position = NULL, nVector *euler = NULL);
		// Switch shape to a mesh collider.
		bool			AsMesh(nGeometry *geometry, nVector *position = NULL, nVector *euler = NULL);

		/// Raytrace shape (see GColTraceMask enum).
		bool			Raytrace(nVector &from, nVector &d, GColTraceResult &result, uint shape_mask = ~0);

/*!
	@name	System Serialization.
	@{
*/
		/// @see bfmk_MLEntity
		bool			FromMetaTag(nMetaFile &file, nMetaTag &tag);
		/// @see bfmk_MLEntity
		nMetaTag		*AsMetaTag(nMetaFile &file) const;
/// @}

		GColShape();
};


#endif	// __GCOLLIDE_SHAPE__
