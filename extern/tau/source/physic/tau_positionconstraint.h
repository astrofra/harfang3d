/*

Physic		[Generic physic & collision engine]
			Library headers.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__NPOSITIONCONSTRAINT__
#define	__NPOSITIONCONSTRAINT__


class	nScene;


/*!
	@short	Position constraint.
	@author	Emmanuel Julien (ejulien@nworks.fr)
*/
class	TauPositionConstraint : public TauConstraint
{
protected:

		uint			dof;				///< Constraint Degrees of freedom.
		void			Setup();

public:

		/// Set the constraint dof.
		void			SetDof(uint d);
		/// Return the joint dof.
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

		TauPositionConstraint
									(
										Tau &t,
										TauItem *a = NULL,
										TauItem *b = NULL,
										nVector *hook_a = NULL,
										nVector *hook_b = NULL,
										uint dof = DofNone		// Default to ball joint.
									);
};


#endif	// __NPOSITIONCONSTRAINT__
