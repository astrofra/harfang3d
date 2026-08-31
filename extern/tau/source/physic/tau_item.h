/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__TAU_ITEM__
#define	__TAU_ITEM__


		#include	"framework/math/unit.h"
		#include	"physic/tau_physicstate.h"


class	Tau;
class	nMItem;
class	GColItem;


/*!
	@short	Physic engine item.

	@note	Item's restitution and friction coefficient are set through the
			collision item's shape structure.

	@author Emmanuel Julien (ejulien@nworks.fr)
*/
class	TauItem : public TauPhysicState, public nLinkedListEntry
{
		friend	class	Tau;
		friend	class	TauConstraint;
		friend	class	nMItem;

public:

enum	Mode
{
		ModeNone			= 0,
		ModeRigidBody		= (1 << 0),
		ModeSoftBody		= (1 << 1)
};
enum	Flag
{
		FlagNone			= 0,
		FlagNoGravity		= (1 << 0),			///< No gravity.
		FlagNoForceField	= (1 << 1),			///< No force field.
		FlagMobile			= (1 << 2),			///< No rotational effect.
		FlagGhost			= (1 << 3)			///< Item does not affect other physic items.
};

protected:

		nMItem			*mitem;			///< Managed item.
		GColItem		*gcol_item;		///< GCollide item.

static	bool			default_state;	///< Default item state.

		Tau				&tau;			///< Item's physic engine.

		bool			cflag;			///< Collision flag.

		float			inv_mass;		///< Item's inverse mass.
		float			mass;			///< Item's mass.

		nMatrix3		inv_tensor;		///< Inverse inertia tensor in body space.

		/// Linear velocity threshold.
		float			linear_threshold;
		/// Angular velocity threshold.
		float			k_angular_threshold;
		/// Angular velocity threshold vector.
		nVector			angular_threshold;

		int				sleep;			///< Sleep timeout.
		bool			active;			///< Item is sleeping.

		/// Integrate forces/acceleration, update velocity.
		void			UpdateVelocity();
		/// Integrate velocity, update position.
		void			UpdatePosition(float step = 1.f);

		/// Wake up item.
		void			WakeUp();

		uint			mode;			///< Physic mode.

public:

/*!
	@name	Internals.
	@{
*/
		float			linear_damping,
						angular_damping;
		float			gravity_scale;	///< Gravity scale coefficient.

		nBitFlag		flag;			///< Item flag.

		nVector			pivot;			///< Center of mass.

		TauPhysicState	prv_state;		///< Previous physical step simulation state.

		/// Get Tau.
		Tau				&GetTau() const
		{	return tau;	}
		/// Return managed item.
		nMItem			*GetMItem() const
		{	return mitem;	}

		/// Get Item pivot.
		nVector			&GetPivot()
		{	return pivot;	}

		/// Return item inverse mass.
		float			GetInverseMass() const
		{	return inv_mass;	}
		/// Active/sleep state.
		bool			IsActive();
		/// Return whether item's velocity are under their threshold.
		bool			TestVelocityThreshold(float k = 1.f);
/// @}

/*!
	@name	Damping, activation and physic parameters.
	@{
*/
		/*!
			@short	Set sleep linear velocity threshold.
			@param	l	Linear velocity threshold.
			@note		Default: 0.00075
		*/
		void			SetSleepLinearVelocityThreshold(float l);
		/*!
			@short	Set sleep angular velocity threshold.
			@param	a	Angular velocity threshold.
			@note		Default: 0.003
		*/
		void			SetSleepAngularVelocityThreshold(float a);

		/*!
			@short	Set linear damping.
			@note	Default: 0.9999f
		*/
		void			SetLinearDamping(float d)
		{	linear_damping = d;	}
		/*!
			@short	Set angular damping.
			@note	Default: 0.9995f
		*/
		void			SetAngularDamping(float d)
		{	angular_damping = d;	}

		bool			IsRigid() const
		{	return mode & ModeRigidBody ? true : false;		}
		bool			IsSoft() const
		{	return mode & ModeSoftBody ? true : false;		}
		/// Return item physic mode.
		uint			GetMode() const
		{	return mode;	}
		/// Set item physic mode.
		void			SetMode(uint m = 0)
		{	mode = m;	}

		/*!
			@short	Set item's mass.

			Current inertia tensor is updated to reflect mass change.

			@note	When the new mass is 0 the inertia tensor is lost.
					You will have to explicitly restore it if you wish to
					enable item mobility back.
					When the previously set mass was 0 the inertia tensor is
					left unmodified and the mass change is performed.
		*/
		void			SetMass(float mass = Kg(1.f));

		/*!
			@short	Set inertia tensor and center of mass.

			Set item's inertia tensor. Please note that you can automatically
			derive the item's inertia tensor and center of mass from an
			associated GColItem description.
			@see	ComputeInertiaTensorAndCoM();
		*/
		void			SetInertiaTensorAndCoM(const nMatrix3 &tensor, const nVector &com);

		/// Wrapper for SetMode() and SetMass().
		void			SetModeMass(uint mode = 0, float mass = Kg(1.f))
		{
			SetMode(mode);
			SetMass(mass);
		}
/// @}

/*!
	@name	Physic interface.
	@{
*/
		/// Return the velocity of a body particle given in world space.
		void			WorldPointVelocity(const nVector &wp, nVector &v, const TauPhysicState *state = 0) const;
		/// Compute velocity of a body's particle (in relative body space).
		void			PointVelocity(const nVector &p, nVector &v, const TauPhysicState *state = 0) const;
		/// Compute inverse world inertia tensor.
		void			ComputeAuxilliaryDatas();

		/// Apply an impulse to a world space location on the body.
		void			ApplyImpulse(const nVector &p, const nVector &J);
		/// Apply an impulse to the center of mass of the body.
		void			ApplyLinearImpulse(const nVector &J);

		/// Apply a force to a given point on the rigid body.
		void			ApplyForce(const nVector &p, const nVector &f);
		/// Apply a force to the center of mass of the rigid body.
		void			ApplyLinearForce(const nVector &f);

		/// Apply torque to the item.
		void			ApplyTorque(const nVector &T);

		/// Set collision item.
		void			SetCollisionInterface(GColItem *ci = 0)
		{	gcol_item = ci;		}
		/// Synchronize collision item.
		void			SynchronizeCollision();

		/// Reset all forces acting on the item.
		void			ResetForces();
		/// Reset item.
		void			Reset(const nVector *pos = 0, const nVector *rot = 0);
/// @}

/*!
	@name	Item setup.
	@{
*/
		/*!
			@short	Set default activation state for new items.
			@note	Even with an active state defaulting to false bodies still
					have to go through minimal configuration checks to ensure
					that they can be put to sleep safely. If one of these tests
					was to fail the body will not go to sleep.
		*/
static	void			SetDefaultState(bool active = true)
		{	default_state = active;	}

		/*!
			@short	Compute item's inertia tensor and center of mass.

			This function automatically compute the item's inertia tensor and
			center of mass based on the associated GColItem description.

			@param	item	Optional GColItem from which the physic properties
							are to be derived. If no item is provided the
							collision item associated with this physic item is
							used.

			@note	If no GColItem is associated with the item no calculations
					will take place. In such a case you have to	manually
					provide both tensor and center of mass position.
			@see	SetInertiaTensorAndCoM().

			@note	This function also automatically set the item's mass to the
					sum of the shape masses in the collision item.
			@see	SetMass().

		*/
		void			ComputeInertiaTensorAndCoM(const GColItem *item = 0);

		/// Setup internals.
		void			Setup();
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

		TauItem(Tau &t);
};


#endif	// __TAU_ITEM__
