/*

Physic		[Generic physic & collision engine]
			Library headers.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__NVELOCITYCONSTRAINT__
#define	__NVELOCITYCONSTRAINT__


class	nScene;


/*!
	@short	Velocity constraint.
	@author	Emmanuel Julien (ejulien@nworks.fr)
*/
class	TauVelocityConstraint : public TauConstraint
{
public:

enum
{
	Velocity_LinearX	= (1 << 0),
	Velocity_LinearY	= (1 << 1),
	Velocity_LinearZ	= (1 << 2),
	Velocity_AngularX	= (1 << 3),
	Velocity_AngularY	= (1 << 4),
	Velocity_AngularZ	= (1 << 5),

	Velocity_Linear		= Velocity_LinearX | Velocity_LinearY | Velocity_LinearZ,
	Velocity_Angular	= Velocity_AngularX | Velocity_AngularY | Velocity_AngularZ
};

protected:

		uint			mask;			///< Constraint mask.
		void			Setup();

public:

		nVector			local_linear_constraint;
		nVector			local_angular_constraint;

		/// Set the constraint mask.
		void			SetMask(uint m)
		{	mask = m;	}
		/// Return the joint dof.
		uint			GetMask() const
		{	return mask;	}

		/// Transform constraint.
virtual	void			Transform(bool do_world = true)
		{}
		/// Process constraint.
virtual	void			Process();

		/// @see bfmk_MLEntity
		bool				FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid = NULL);
		/// @see bfmk_MLEntity
		nMetaTag			*AsMetaTag(nMetaFile &file);

		TauVelocityConstraint
									(
										Tau &t,
										TauItem *a = NULL,
										nVector *local_linear_constraint = NULL,
										nVector *local_angular_constraint = NULL,
										uint mask = Velocity_Angular	// Default to angular velocity constraint.
									);
};


#endif	// __NVELOCITYCONSTRAINT__
