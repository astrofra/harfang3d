/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"
		#include	"../manager/manager.h"


		bool		TauItem::default_state = true;


//----------------------------------------------------------------------------------------------------------------
void				TauPhysicState::ResetState(const nMatrix3 &inv_tensor, const nVector *pos, const nVector *rot)
//----------------------------------------------------------------------------------------------------------------
{
	total_force.Set();
	total_torque.Set();
	vel_linear.Set();
	vel_angular.Set();

	inv_world_tensor = inv_tensor;

	if	(pos)
			position = *pos;
	else	position.Set();
	if	(rot)
			rotation = nQuaternion::FromEuler(rot->x, rot->y, rot->z);
	else	rotation = nQuaternion::FromEuler(0, 0, 0);
}

//-------------------------------------------------------------------
void				TauItem::SetSleepLinearVelocityThreshold(float l)
//-------------------------------------------------------------------
{
	linear_threshold = l * l;	// Squared to save a sqrt.
}

//--------------------------------------------------------------------
void				TauItem::SetSleepAngularVelocityThreshold(float a)
//--------------------------------------------------------------------
{
	k_angular_threshold = a;
}

//---------------------------------------------------------
bool				TauItem::TestVelocityThreshold(float k)
//---------------------------------------------------------
{
	// Angular velocity.
	nVector		absv = (vel_angular / k).Abs();
	if	(
			(absv.x > angular_threshold.x) ||
			(absv.y > angular_threshold.y) ||
			(absv.z > angular_threshold.z)	)
		return false;

	if	(vel_linear.Len2() > (linear_threshold * k * 4.f))
		return false;

	return true;
}

//--------------------------------------------------------
void				TauItem::ApplyTorque(const nVector &T)
//--------------------------------------------------------
{
	if	(!IsActive())
		GetTau().Wake(this);
	total_torque += T;
}

//-------------------------------------------------------------------------
void				TauItem::ApplyForce(const nVector &p, const nVector &f)
//-------------------------------------------------------------------------
{
	if	(!IsActive())
		GetTau().Wake(this);
	total_force += f;
	total_torque += (p - position).Cross(f);
}

//---------------------------------------------------------------------------
void				TauItem::ApplyImpulse(const nVector &p, const nVector &J)
//---------------------------------------------------------------------------
{
	if	(J.Len2() < 0.0000001)
		return;
	if	(!IsActive())
		GetTau().Wake(this);

	nVector	r = p - world_pivot;
	nVector	n = J.Normalize();

	const float	K = inv_mass + n.Dot((r.Cross(n) * inv_world_tensor).Cross(r));
	if	(!K)
		return;
	nVector	j = J / K;
	vel_linear += j * inv_mass;
	vel_angular += r.Cross(j) * inv_world_tensor;
}

//---------------------------------------------------------------
void				TauItem::ApplyLinearImpulse(const nVector &J)
//---------------------------------------------------------------
{
	if	(!inv_mass)
		return;
	if	(!IsActive())
		GetTau().Wake(this);
	vel_linear += J;
}

//-------------------------------------------------------------
void				TauItem::ApplyLinearForce(const nVector &f)
//-------------------------------------------------------------
{
	if	(!IsActive())
		GetTau().Wake(this);
	total_force += f;
}

//-------------------------------------
bool 				TauItem::IsActive()
//-------------------------------------
{
	if	(!mode)
		return false;
	return active;
}

//-----------------------------------
void				TauItem::WakeUp()
//-----------------------------------
{
	sleep = 0;
	if	(!active)
	{
		vel_angular.Set();
		vel_linear.Set();
	}
	ResetForces();
	active = true;
}

//-------------------------------------------------
void				TauItem::SynchronizeCollision()
//-------------------------------------------------
{
	if	(!gcol_item)
		return;
// TODO Highly suspicious!
	nVector		dummy_scale(1, 1, 1),
				offset_crt = position * GetMItem()->GetBaseItem()->offset_matrix.GetInverse();// - GetMItem()->GetBaseItem()->offset_matrix.GetRow(3);
	gcol_item->SynchronizeState(offset_crt, &rotation_matrix, &dummy_scale);
}

//------------------------------------------------------------------------
void				TauItem::Reset(const nVector *pos, const nVector *rot)
//------------------------------------------------------------------------
{
	if	(GetMode() == ModeNone)
		return;

	// Wake up.
	GetTau().Wake(this);

	// Reset state and synchronize collision.
	ResetState(inv_tensor, pos, rot);

	// Compute current world pivot.
	rotation_matrix = rotation.AsMatrix3();
	world_pivot = pivot * rotation_matrix + position * GetMItem()->GetBaseItem()->offset_matrix.GetInverse();// - GetMItem()->GetBaseItem()->offset_matrix.GetRow(3);

	// Synchronize collision
	SynchronizeCollision();

	// Export to previous state.
	prv_state = *this;

	// Set base item transformation.
	nMatrix4	m = nMatrix4::FromMatrix3(rotation_matrix);
	m.SetRow(3, position);
	GetMItem()->GetBaseItem()->SnapshotTransformation(m);

	/*
		Build an angular deactivation threshold based on the item
		inertia tensor.
	*/
	angular_threshold.Set(inv_tensor.m[0][0], inv_tensor.m[1][1], inv_tensor.m[2][2]);
	float	a_t_l = angular_threshold.Len();
	angular_threshold *= a_t_l ? k_angular_threshold / a_t_l : 1.f;

	sleep = TauItem::default_state ? 0 : 500;
	if	(mode && inv_mass)
		active = true;
	cflag = false;
}

//---------------------------------------------------------------------------------------------
void				TauItem::SetInertiaTensorAndCoM(const nMatrix3 &tensor, const nVector &com)
//---------------------------------------------------------------------------------------------
{
	pivot = com;
	tensor.Inverse(inv_tensor);
}

//----------------------------------
void				TauItem::Setup()
//----------------------------------
{
	active = true;
	inv_mass = mass ? 1.f / mass : 0.f;
//	pivot.Set();
//	inv_tensor = nMatrix3::ScaleMatrix(nVector(inv_mass, inv_mass, inv_mass));
//	ComputeInertiaTensorAndCoM(gcol_item);
}

//----------------------------------------
void				TauItem::ResetForces()
//----------------------------------------
{
	total_force.Set();
	total_torque.Set();
}

//-------------------------------------------
void				TauItem::UpdateVelocity()
//-------------------------------------------
{
	const float	ifq = 1.f / tau.fq;
	vel_linear += total_force * inv_mass * ifq;
	vel_angular += total_torque * inv_world_tensor * ifq;
}

//-----------------------------------------------------
void				TauItem::UpdatePosition(float step)
//-----------------------------------------------------
{
	const float	ifq = step / tau.fq;
	position += vel_linear * ifq;
	world_pivot += vel_linear * ifq;

	if	(!flag.Test(TauItem::FlagMobile))
	{
		rotation += nQuaternion(vel_angular.x, vel_angular.y, vel_angular.z, 0.f) * rotation * 0.5f * ifq;
		rotation = rotation.Normalize();
	}
}

//---------------------------------------------------------------------------------------------------------
void				TauItem::PointVelocity(const nVector &p, nVector &v, const TauPhysicState *state) const
//---------------------------------------------------------------------------------------------------------
{
	if	(!active || !inv_mass)
		v = (p * GetMItem()->GetBaseItem()->GetMatrix() - p * GetMItem()->GetBaseItem()->GetPreviousMatrix());// / nEngine::GetDeltaClockf();

	else
	{
		if	(!state)
			state = this;

		nVector		va;
		if	(flag.Test(TauItem::FlagMobile))
				va.Set(0, 0, 0);
		else	Vec3Cross(va, state->vel_angular, p);
		Vec3Add(v, state->vel_linear, va);
	}
}

//---------------------------------------------------------------------------------------------------------------
void				TauItem::WorldPointVelocity(const nVector &wp, nVector &v, const TauPhysicState *state) const
//---------------------------------------------------------------------------------------------------------------
{
	nVector		r;
	if	(!inv_mass)
			r = wp * GetMItem()->GetBaseItem()->GetInverseMatrix();
	else	r = wp - world_pivot;
	PointVelocity(r, v, state);
}

//----------------------------------------------
void				TauItem::SetMass(float _mss)
//----------------------------------------------
{
	if	(mass == _mss)
		return;

	// Update inertia tensor (if previously immobile do nothing).
	const float	k_tensor = inv_mass ? _mss * inv_mass : 1.f;
	inv_tensor *= 1.f / k_tensor;

	// Perform change.
	inv_mass = _mss ? 1.f / _mss : 0;
	mass = _mss;

	active = inv_mass != 0;
}

//---------------------------------------------------
void				TauItem::ComputeAuxilliaryDatas()
//---------------------------------------------------
{
	if	(!flag.Test(TauItem::FlagMobile))
	{
		rotation_matrix = rotation.AsMatrix3();
		inv_world_tensor = (rotation_matrix * inv_tensor) * rotation_matrix.Transpose();
		//-----------------------------
		#define		CleanTensor(T)\
		{\
			(T).m[1][0] = (T).m[0][1];\
			(T).m[2][0] = (T).m[0][2];\
			(T).m[2][1] = (T).m[1][2];\
		}
		//-----------------------------
		CleanTensor(inv_world_tensor);
	}

	/*
		Compute the new world space pivot position and correct the world
		position so that the rotation took place around the pivot.
	*/
	world_pivot = pivot * rotation_matrix;
	position += (prv_state.world_pivot - prv_state.position) - world_pivot;
	world_pivot += position;
}

//-----------------------------------------------
TauItem::TauItem(Tau &t) : tau(t), pivot(0, 0, 0)
//-----------------------------------------------
{
	SetLinearDamping(0.999f);
	SetAngularDamping(0.999f);
	SetSleepLinearVelocityThreshold(0.00075f);
	SetSleepAngularVelocityThreshold(0.00075f);

	angular_threshold.Set(0, 0, 0);

	gcol_item = NULL;
	mitem = NULL;

	SetMode();

	SetCollisionInterface();

	mass = 1.f;

	cflag = false;
	sleep = 0;
	active = true;

	inv_mass = 0.0f;
	gravity_scale = 1.f;
	inv_tensor.Set(0,0,0,0,0,0,0,0,0);
}
