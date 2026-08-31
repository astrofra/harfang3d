/*

Physic		[Generic physic & collision engine]
			Library headers.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__NHINGECONSTRAINT__
#define	__NHINGECONSTRAINT__


class	nScene;


/*!
	@short	Hinge constraint.

	@note	The hinge constraint restricts two dof only.
			This constraint does not restrict position.
	@author	Emmanuel Julien (ejulien@nworks.fr)
*/
class	TauHingeConstraint : public TauConstraint
{
protected:

		void			Setup();

		TauDistanceConstraint
						hinge0, hinge1;

public:

		nVector			axis;

virtual	void			SetAnchor(TauItem *i)
		{
			hinge0.SetAnchor(i);
			hinge1.SetAnchor(i);
			TauConstraint::SetAnchor(i);
		}
virtual	void			SetTarget(TauItem *i)
		{
			hinge0.SetTarget(i);
			hinge1.SetTarget(i);
			TauConstraint::SetTarget(i);
		}

		/// Set the hinge axis.
		void			SetAxis(nVector a)
		{	axis = a;	}
		/// Return the hinge axis.
		nVector			GetAxis() const
		{	return axis;	}

		/// Transform constraint.
virtual	void			Transform(bool do_world = true);
		/// Process constraint.
virtual	void			Process();

virtual	void			SetStrength(float s = 1)
		{
			hinge0.SetStrength(s);
			hinge1.SetStrength(s);
			TauConstraint::SetStrength(s);
		}

		/// @see bfmk_MLEntity
		bool				FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid = NULL);
		/// @see bfmk_MLEntity
		nMetaTag			*AsMetaTag(nMetaFile &file);

		TauHingeConstraint
									(
										Tau &t,
										TauItem *a,
										TauItem *b,
										nVector *hook_a,
										nVector *hook_b,
										nVector *axis
									);
};


#endif	// __NPOSITIONCONSTRAINT__
