/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__TAU_ANGULARCONSTRAINT__
#define	__TAU_ANGULARCONSTRAINT__


class	nScene;


/*!
	@short	Angular constraint.
	@author Emmanuel Julien (ejulien@nworks.fr)
*/
class	TauAngularConstraint : public TauConstraint
{
friend	class			TauJoint;

protected:

		float			iK_com;
		float			iK_X, iK_Y, iK_Z;

		uint			dof;

		float			TransformConstraintAxis
									(
										TauItem *item[2],
										nVector *r
									);
		void			ApplyConstraintImpulse
									(
										TauItem *item[2],
										nVector *r,
										float iK
									);

public:

		/// Set the constraint Degrees of freedom.
		void			SetDof(uint d)
		{	dof = d;	}	
		/// Return the constraint Degrees of freedom.
		uint			GetDof() const
		{	return dof;	}

		/// Transform constraint.
virtual	void			Transform(bool do_world = true);
		/// Process constraint.
virtual	void			Process();

		/// @see bfmk_MLEntity
		bool				FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid = NULL);
		/// @see bfmk_MLEntity
		nMetaTag			*AsMetaTag(nMetaFile &file);

		TauAngularConstraint
									(
										Tau &t,
										TauItem *a,
										TauItem *b,
										uint dof = DofNone		// Default to rigid joint.
									);
};


#endif	// __TAU_ANGULARCONSTRAINT__
