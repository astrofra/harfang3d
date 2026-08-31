/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"
		#include	"../manager/manager.h"


//---------------------------------------------------------------------------------------------
void				Tau::ComputeSphereInertiaTensor(float radius, float mass, nMatrix3 &tensor)
//---------------------------------------------------------------------------------------------
{
	const float	i = 2.f / 5.f * mass * radius * radius;
	tensor.Set(i, 0, 0, 0, i, 0, 0, 0, i);
}

//------------------------------------------------------------------------------------------------------
void				Tau::ComputeCuboidInertiaTensor(const nVector &extend, float mass, nMatrix3 &tensor)
//------------------------------------------------------------------------------------------------------
{
	const float	x2 = extend.x * extend.x,
				y2 = extend.y * extend.y,
				z2 = extend.z * extend.z,
				im12 = mass / 12.f;

	tensor.Set((y2 + z2) * im12, 0, 0, 0, (x2 + z2) * im12, 0, 0, 0, (x2 + y2) * im12);
}

//---------------------------------------------------------------------------
void				TauItem::ComputeInertiaTensorAndCoM(const GColItem *item)
//---------------------------------------------------------------------------
{
	if	(!item)
		__ERRRAW__(__LOG_E__ << "Cannot compute inertia tensor with no GColItem description.\n")

	// Compute center of mass position.
	float			total_mass = 0;
	pivot.Set(0, 0, 0);
	nLinkedListPool	pool;
	GColShape		*s;
	while	(s = (GColShape *)item->shape_list.Pool(pool))
	{
// Highly suspicious!
		pivot += (s->position * item->GetMItem()->GetBaseItem()->offset_matrix.GetInverse()) * s->mass;
		total_mass += s->mass;
	}
	mass = total_mass;
	if	(total_mass)
	{
		inv_mass = 1.f / total_mass;
		pivot *= inv_mass;
	}
	else	inv_mass = 0.f;

	// Compute combined inertia tensor.
	nMatrix3		tensor(0, 0, 0, 0, 0, 0, 0, 0, 0);

	pool.Reset();
	while	(s = (GColShape *)item->shape_list.Pool(pool))
	{
		nMatrix3	shape_tensor;

		switch	(s->type)
		{
			case GColSphere:
				Tau::ComputeSphereInertiaTensor(s->scale.x, s->mass, shape_tensor);
				break;

			case GColCuboid:
				Tau::ComputeCuboidInertiaTensor(s->scale, s->mass, shape_tensor);
				break;

			case GColMesh:
				__LOG_W__ << "Mesh inertia tensor UNIMPLEMENTED!\n";
				shape_tensor = nMatrix3::IdentityMatrix();
				break;

			default:
				break;
		}

		//-----------------------------
		#define		CleanTensor(T)\
		{\
			(T).m[1][0] = (T).m[0][1];\
			(T).m[2][0] = (T).m[0][2];\
			(T).m[2][1] = (T).m[1][2];\
		}
		//-----------------------------

		// Orient tensor.
		shape_tensor = s->orientation_matrix * shape_tensor * s->orientation_matrix.Transpose();
		CleanTensor(shape_tensor);

		// Translate tensor.
		// From ODE: I + mass*(crossmat(c)^2 - crossmat(c+a)^2)
// Highly suspicious!
		nMatrix3	cm2(nMatrix3::CrossProductMatrix((s->position * item->GetMItem()->GetBaseItem()->offset_matrix.GetInverse())));
		cm2 *= cm2;
		shape_tensor -= cm2 * s->mass;
		CleanTensor(shape_tensor);

		// Total tensor.
		tensor += shape_tensor;
	}

	// Compute combined inverse inertia tensor.
	tensor.Inverse(inv_tensor);
	CleanTensor(inv_tensor);
/*
	// Restore user mass if any was set.
	if	(org_inv_mass)
		SetMass(1.f / org_inv_mass);
*/
}
