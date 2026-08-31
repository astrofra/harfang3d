/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__TAU_CONSTRAINT__
#define	__TAU_CONSTRAINT__


		#include	"framework/math/vector.h"
		#include	"framework/math/matrix3.h"


class	nScene;
class	Tau;
class	TauItem;


/*!
	@short	Physic constraint.
	@author Emmanuel Julien (ejulien@nworks.fr)
*/
class	TauConstraint : public nLinkedListEntryId
{
public:

enum	Type
{
	Type_Constraint = 0,
	Type_DistanceConstraint,
	Type_VelocityConstraint,
	Type_PositionConstraint,
	Type_HingeConstraint,
	Type_AngularConstraint
};

		/// Constraint type to string.
static	const char		*TypeToString(Type type)
		{
			switch	(type)
			{
				case	Type_Constraint:
					return "Constraint";
				case	Type_DistanceConstraint:
					return "Distance Constraint";
				case	Type_VelocityConstraint:
					return "Velocity Constraint";
				case	Type_PositionConstraint:
					return "Position Constraint";
				case	Type_HingeConstraint:
					return "Hinge Constraint";
				case	Type_AngularConstraint:
					return "Angular Constraint";

				default:
					return "Invalid constraint type";
			}
		}

enum
{
		DofNone			= 0,

		DofPositionX	= (1 << 0),
		DofPositionY	= (1 << 1),
		DofPositionZ	= (1 << 2),
		DofRotationX	= (1 << 3),
		DofRotationY	= (1 << 4),
		DofRotationZ	= (1 << 5),

		DofPosition		= DofPositionX | DofPositionY | DofPositionZ,
		DofRotation		= DofRotationX | DofRotationY | DofRotationZ,
		DofAll			= DofPosition | DofRotation
};

protected:

		Tau				&tau;
		Type			type;				///< Constraint type.

		float			iK;					///< Impulse denominator.

		TauItem			*item[2];			///< Constraint items.
		nMatrix3		anchor_matrix;		///< Anchor rotation matrix.

		bool			use_correction;
		nVector			correction_vector;	///< Correction vector.

public:

		bool			enable;				///< Constraint enable flag.

		/// Set the constraint correction vector.
		void			SetCorrectionVector(nVector &v)
		{	correction_vector = v.Normalize();	}
		/// Set correction vector limitation.
		void			EnableCorrectionVector(bool b)
		{	use_correction = b;	}

		/// Get the anchor item.
		nMatrix3		&GetAnchorMatrix()
		{	return anchor_matrix;	}

		float			strength;			///< Constraint damping.
		float			maximum_erc;		///< Maximum error correction.

virtual	void			SetAnchor(TauItem *i)
		{	item[0] = i;	}
virtual	void			SetTarget(TauItem *i)
		{	item[1] = i;	}
		TauItem			*GetAnchor() const
		{	return item[0];	}
		TauItem			*GetTarget() const
		{	return item[1];	}

		void			SetAnchorHook(nVector *h)
		{
			hook[0] = h ? *h : nVector(0, 0, 0);
			if	(!item[0])
				world_hook[0] = hook[0];
		}
		void			SetTargetHook(nVector *h)
		{
			hook[1] = h ? *h : nVector(0, 0, 0);
			if	(!item[1])
				world_hook[1] = hook[1];
		}

		nVector			GetAnchorHook() const
		{	return world_hook[0];	}
		nVector			GetTargetHook() const
		{	return world_hook[1];	}

		/// Return the constraint type.
		Type			GetType() const
		{	return type;	}

		nVector			hook[2];			///< Constraint original hook location (space is type specific).
		nVector			world_hook[2];		///< constraint hook in world space.

		/// Get the current distance between constraint hooks.
		float			GetCurrentDistance() const
		{	return nVector::Dist(world_hook[0], world_hook[1]);	}

		/// Transform constraint.
virtual	void			Transform(bool do_world = true);
		/// Process constraint.
virtual void			Process() = 0;

		/// Set the constraint strength.
virtual	void			SetStrength(float s = 1)
		{	strength = s;	}

		/// @see bfmk_MLEntity
virtual	bool				FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid = NULL);
		/// @see bfmk_MLEntity
virtual	nMetaTag			*AsMetaTag(nMetaFile &file);

		TauConstraint(Tau &t);
};


#endif	// __TAU_CONSTRAINT__
