/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__NTAU_SYSTEM__
#define	__NTAU_SYSTEM__


		#include	"framework/tools/benchmark.h"
		#include	"framework/data_structure/linkedlist.h"
		#include	"physic/tau_item.h"


class	GCollide;
struct	GColNode;
struct	GColContact;
class	TauEvent;
class	TauConstraint;
class	nScene;


		/// Default maximum collision pair in system.
#define	TAU_DEFAULT_MAX_COLNODE			2048
		/// Default maximum contact in system.
#define	TAU_DEFAULT_MAX_CONTACT			4096

		/// Deactivation timer scale value.
#define	K_DEAK							0.75f

/*!
	@short	Physic engine statistics structure.

	@note	Benchmark values are normalized to the second
			(ie. divided by the framework clock frequency).
	@see	bfmk::GetClockFrequency().

	@author Emmanuel Julien (ejulien@nworks.fr)
*/
struct	TauStatistics
{
		nBenchmark		collision;
		nBenchmark		contact_cache;
		nBenchmark		solver;
		nBenchmark		system;

		uint			node_count;
		uint			contact_count;
		uint			resting_contact_count;
		uint			resting_pair_count;

		uint			sleep_count;		/// Number of sleeping physic items.

		//---------------------
		void			Reset()
		//---------------------
		{
			collision.Reset();
			contact_cache.Reset();
			solver.Reset();
			system.Reset();

			node_count = 0;
			contact_count = 0;
			resting_contact_count = 0;
			resting_pair_count = 0;

			sleep_count = 0;
		}
};

class	nGroup;

/*
	@short	Physic resting pair.
*/
struct		TauRestingPair
{
	TauItem		*a, *b;
	float		d;
};

/*!
	@short	Physic engine.
	@author Emmanuel Julien (ejulien@nworks.fr)
*/
class	Tau
{
		friend	class	TauItem;

protected:

		GCollide		*gcol;				///< Collision engine.

		bool			merge_statistics;
		float			fps_clamp;

		float			Jpm;				///< Penetration impulse magnitude.

		float			k_tstep,			///< System time step.
						timer,				///< Physic engine internal timer.
						update_dt,			///< Current update total dt.
						update_timer_origin;
		uint			delay;				///< Sleep delay.

		uint			colnode_max;
		GColNode		*colnode_a;			///< Collision node buffer 0.
		GColNode		*colnode_b;			///< Collision node buffer 1.

		uint			colnode_count;		///< Current collision node count.
		GColNode		*colnode;			///< Current collision node buffer.
		uint			prv_colnode_count;	///< Previous collision node count.
		GColNode		*prv_colnode;		///< Previous collision node buffer.

		uint			contact_max;
		GColContact		*contact;

		nLinkedList		item_list;			///< Active physic item list.
		nLinkedList		sleep_list;			///< Inactive/sleeping physic item list.
		nLinkedList		freeze_list;		///< Freeze item list.

		nLinkedListId	constraint_list;	///< Physic constraint list.

		void			*event_user_data;	///< Event user data.
		TauEvent		*event;				///< Event class.
		TauStatistics	statistics;			///< System statistics.

		nVector			gravity;			///< System's gravity vector.
		bool			leftright;			///< Collisions processing order flag.

		/// Update all constraints to world space.
		void			TransformConstraints();
		/// Physic solver step.
		void			Solver(bool e = false);
		/// Solve system.
		float			SolveSystem();
		// Interpolate all static items involved in a collision with a physical item.
		void			InterpolateStaticColliderTransformation();

		/// Synchronize sleep group.
		void			ResetSleepGroup(TauItem *item);

public:

		/// Wake an item.
		void			Wake(TauItem *t);
		/// Put an item to sleep.
		void			Sleep(TauItem *t);
		/// Freeze/unfreeze an item.
		bool			FreezeItem(TauItem &t, bool freeze);

/*!
	@name	Runtime.
	@{
*/
		bool			enable_deactivation;		///< Allow deactivation.

		/// Return a reference to the active item list.
const	nLinkedList		&GetList() const
		{	return item_list;	}
		/// Return a reference to the sleeping item list.
const	nLinkedList		&GetSleepList() const
		{	return sleep_list;	}
		/// Register a new item in the physic system.
		void			AddItem(TauItem &item);
		/// Remove an item.
		void			RemoveItem(TauItem &item, bool deletion = false);

		/// Set the default gravity vector.
		void			SetGravity(nVector &g)
		{	gravity = g;	}
		/// Get the default gravity vector.
		nVector			&GetGravity()
		{	return gravity;	}

		/// Update system.
		float			Update(float dt);
		/*
			Predicts interference that are likely to occur during the next simulation step.
			Results are stored in the gcol node array just as the standard collision process ones.
		*/
		void			PredictInterference(float dt);
		/// Populate system collision.
		void			PopulateCollision();

		/// Raytrace system.
//		bool			Raytrace(nVector &from, nVector &to, );
/// @}

/*!
	@name	Tools.
	@{
*/
		/*!
			@short	Set event callback functions.
			@see	TauEvent.
		*/
		void			SetEvent(TauEvent *e, void *udata = NULL)
		{
			event = e;
			event_user_data = udata;
		}

		/// Return last update execution statistics.
const	TauStatistics	&QueryStatistics() const
		{	return statistics;	}
		/// Reset statistic object.
		void			ResetStatistics()
		{	statistics.Reset();	}

		/*!
			@short	Display contact debug informations.

			Display all contact informations for the previous update.

			@note	The purple cross is a contact point, the light blue arrow
					is the contact normal and the dark purple one represents
					the penetration depth.
		*/
		void			DBG_Contact(nRenderer &render, float scale = 1);
		/*!
			@short	Display bodies informations.
			@note	Greenish bodies are physically active. Blue ones are not.
		*/
		void			DBG_Body(nRenderer &render, float scale = 1);
		/*!
			@short	Display constraints informations.
			@note	Constraints varies from blue to red depending on how much
					their are being violated.
		*/
		void			DBG_Constraint(nRenderer &render, float scale = 1);

		/// Compute the inertia tensor matrix of a cuboid.
static	void			ComputeCuboidInertiaTensor
									(
										const nVector &dimension,
										float mass,
										nMatrix3 &tensor
									);
		/// Compute the inertia tensor matrix of a sphere.
static	void			ComputeSphereInertiaTensor
									(
										float radius,
										float mass,
										nMatrix3 &tensor
									);
		/// Compute the inertia tensor matrix of a geometry.
static	void			ComputeGeometryInertiaTensor
									(
										nGeometry &geo,
										nMatrix3 &tensor
									);
/// @}

/*!
	@name	Constraint.
	@{
*/
		/// Register a new constraint in the solver.
		TauConstraint	*AddConstraint(TauConstraint *cst);
		/// Delete a constraint from the constraint list.
		bool			DeleteConstraint(TauConstraint *cst);

		/// Return the system constraint list.
const	nLinkedListId	&GetConstraintList() const
		{	return constraint_list;	}
/// @}

/*!
	@name	Setup.
	@{
*/
		/*!
			@short	Set the penetration correction impulse magnitude.
		*/
		void			SetJpm(float j = 0.1f)
		{	Jpm = j;	}

		/*!
			@short	Allocate the collision node array.
			@note	Nodes are not preserved on reallocation.
		*/
		void			AllocateCollisionNode
									(
										uint node = TAU_DEFAULT_MAX_COLNODE,
										uint contact = TAU_DEFAULT_MAX_CONTACT
									);
		/// Free the memory used by the collision nodes.
		void			FreeCollisionNode();

		/*!
			@short	Return the number of collision node count.

			Return the number of collision node collected by
			the system during its last update.
		*/
		uint			GetCollisionNodeCount() const
		{	return colnode_count;	}
		/*!
			@short	Return the collision node array.

			Return the collision node array collected by
			the system during its last update.
		*/
		GColNode		*GetCollisionNode() const
		{	return colnode;	}

		float			fq;						///< Physic frequency.
		uint			contact_iteration,		///< Number of contact solver iteration.
						constraint_iteration;	///< Number of constraint solver iteration.

		/// Set the collision engine used by the physic solver.
		void			SetCollisionEngine(GCollide *c = NULL)
		{	gcol = c;	}

		/// Free system, optionally free memory.
		void			Free(bool free_data = false);
		/// Reset physic engine.
		void			Reset();
/// @}

/*!
	@name	System Serialization.
	@{
*/
		/// @see bfmk_MLEntity
		bool			FromMetaTag(nScene &scene, nMetaFile &file, nMetaTag &tag, uint *mapuid = NULL, nGroup **group = NULL, bool load_settings = true);
		/// @see bfmk_MLEntity
		nMetaTag		*AsMetaTag(nMetaFile &file) const;
/// @}

		Tau();
		~Tau();
};


#endif	// __NTAU_SYSTEM__
