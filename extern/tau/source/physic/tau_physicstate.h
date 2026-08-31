/*

		nEngine
		Emmanuel Julien 2000-2009
		All Rights Reserved

		http://www.nengine.fr
		Please refer to the included license.txt for license informations.

*/



#ifndef	__TAU_PHYSICSTATE__
#define	__TAU_PHYSICSTATE__


		#include	"framework/math/vector.h"
		#include	"framework/math/matrix3.h"
		#include	"framework/math/quaternion.h"


/*!
	@short	Tau physic state.

	@author Emmanuel Julien (ejulien@nworks.fr)
*/
struct	TauPhysicState
{
		friend	class	Tau;
		friend	class	TauItem;
		friend	class	TauConstraint;

		bool			active;				///< Sleeping.

		nVector			total_force;		///< Total force.
		nVector			total_torque;		///< Total torque.

		nVector			vel_linear;			///< Linear velocity.
		nVector			vel_angular;		///< Angular velocity.
		nVector			world_pivot;		///< Pivot in world space.
		nMatrix3		inv_world_tensor;	///< Inverse inertia tensor in world space.

		nVector			position;			///< Position.
		/*!
			@short	State rotation quaternion.

			@note	The quaternion rotation of an item can change during the
					execution of a physic state. If you require a final fixed
					rotation please use the rotation matrix.
			@see	rotation_matrix.

			@note	Outside the execution of a physic update both the
					quaternion and the matrix represent the same rotation.
		*/
		nQuaternion		rotation;
		/*!
			@short	State rotation matrix.

			@note	Contrary to the rotation quaternion, the rotation matrix is
					updated only once at the end of a physic update.
			@see	rotation.
		*/
		nMatrix3		rotation_matrix;	///< Rotation matrix.

		/// Return the velocity of a body particle given in world space.
		void			WorldPointVelocity(nVector &wp, nVector &v) const;
		/// Compute velocity of a body's particle (in relative body space).
		void			PointVelocity(nVector &p, nVector &v) const;

		/// Reset physic state.
		void			ResetState(const nMatrix3 &inv_tensor, const nVector *pos = 0, const nVector *rot = 0);

		TauPhysicState() : total_force(0, 0, 0), total_torque(0, 0, 0),
							vel_linear(0, 0, 0), vel_angular(0, 0, 0),
							world_pivot(0, 0, 0), position(0, 0, 0), rotation(0, 0, 0)
		{
			active = true;
		}
};


#endif	// __TAU_PHYSICSTATE__
