/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__TAU_DISTANCECONSTRAINT__
#define	__TAU_DISTANCECONSTRAINT__


class	nScene;


/*!
	@short	Distance constraint.
	@author Emmanuel Julien (ejulien@nworks.fr)
*/
class	TauDistanceConstraint : public TauConstraint
{
public:

		float			max_dist;				///< Constraint maximum distance.
		float			min_dist;				///< Constraint minimum distance.

		/// Transform constraint.
virtual	void			Transform(bool do_world = true);
		/// Process constraint.
virtual	void			Process();

		/// @see bfmk_MLEntity
		bool				FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid = NULL);
		/// @see bfmk_MLEntity
		nMetaTag			*AsMetaTag(nMetaFile &file);

		TauDistanceConstraint
									(
										Tau &t,
										TauItem *a = NULL,
										TauItem *b = NULL,
										nVector *hook_a = NULL,
										nVector *hook_b = NULL,
										float min_dist = 0,
										float max_dist = 0
									);
};


#endif	// __TAU_DISTANCECONSTRAINT__
