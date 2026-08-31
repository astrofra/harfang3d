/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"
		#include	"../manager/manager.h"


		/*!
			@short	Max allowable solving time (display frequency * value).
			@note	This value is an upper bound only. If the solver cannot
					keep up	with the target frame rate the physic system will
					appear to slow down its motion and will not eat all of the
					application CPU time.
		*/
#define	TAU_DEFAULT_FPS_CLAMP_VALUE		4


//---------------------------------------------------------
TauConstraint		*Tau::AddConstraint(TauConstraint *cst)
//---------------------------------------------------------
{	__SUBSYSTEM(System::SystemPhysic);
	return (TauConstraint *)constraint_list.Add(cst);	}

//-----------------------------------------------------------
bool				Tau::DeleteConstraint(TauConstraint *cst)
//-----------------------------------------------------------
{	return constraint_list.Delete(cst);	}

//----------------------------------------------------------
bool				Tau::FreezeItem(TauItem &t, bool freeze)
//----------------------------------------------------------
{
	__SUBSYSTEM(System::SystemPhysic);

	if	(freeze)
	{
		bool	valid;
		if	(!item_list.Extract(&t))	// Should be the shortest list of the two.
				valid = sleep_list.Extract(&t);
		else	valid = true;

		if	(valid)
			freeze_list.Add(&t);
//		else	__LOG_E__ << "trying to freeze an item not present in any active lists.\n";
	}
	else
	{
		if	(freeze_list.Extract(&t))
			item_list.Add(&t);
//		else	__LOG_E__ << "trying to unfreeze an item not in the frozen list.\n";
		t.WakeUp();
	}
	return true;
}

//---------------------------------------
float				Tau::Update(float dt)
//---------------------------------------	
{
	__SUBSYSTEM(System::SystemPhysic);

	const float		k_clock = 1.f / (float)System::GetClockFrequency();
	statistics.system.Start();

	float			dt_t = (float)System::GetClock();
	timer -= dt;

	update_dt = dt;
	update_timer_origin = timer;

	merge_statistics = false;

	// Execute n fractional simulation time step.
	k_tstep = 1.f / fq;

	while	(timer <= 0.f)
	{
		// Integrate time step.
		nLinkedListForeach(TauItem, ci, item_list)
		{
			nItem	*item = ci->GetMItem()->GetBaseItem();

			item->SetPosition(ci->position);
			item->SetRotation(ci->rotation);

			if	(ci->flag.Test(TauItem::FlagMobile))
				ci->rotation_matrix = item->GetOrientationMatrix();

			if	(ci->inv_mass)
			{
				ci->prv_state = *ci;		// Backup physic state.
				if	(!ci->flag.Test(TauItem::FlagNoGravity))
					ci->ApplyLinearForce(gravity * (ci->gravity_scale / ci->inv_mass));
			}
		}

		SolveSystem();	// A physic step is taken from this function.

		// All following solver calls should merge stats.
		merge_statistics = true;

		// Physic step.
		timer += k_tstep;

		// Abort solver if it is taking too long.
		float	dt_limit = ((float)System::GetClock() - dt_t) * k_clock;
		if	(dt_limit > (1.f / 60.f))
		{
			__LOG_W__ << "Physic skipping " << int(-timer / k_tstep) << " update(s)...\n";
			while (timer <= 0.f)
			{
				/*
					While the physic system aborts we still update scripts as
					they could be hard-coded onto the physic system frequency.
				*/
				if	(event)
					event->PhysicStep(event_user_data, false);
				timer += k_tstep;
			}
			break;
		}
	}
	statistics.system.Stop();

	// System statistics.
	statistics.resting_contact_count = 0;
	statistics.sleep_count = sleep_list.GetCount();

	statistics.node_count = colnode_count;
	statistics.contact_count = 0;
	for	(uint nit = 0; nit < colnode_count; nit++)
		statistics.contact_count += colnode[nit].ctc_count;

	// Iteration step.
	return (timer * fq);
}

//----------------------------------------------------------------
void				Tau::InterpolateStaticColliderTransformation()
//----------------------------------------------------------------
{
	return;

#if	(!__ENABLE_BULLET__)
	float		k0 = (timer - update_timer_origin) / update_dt,
				k1 = (timer + k_tstep - update_timer_origin) / update_dt; 

	nVector		p, s;
	nMatrix3	r;

	for	(int nit = 0; nit < (int)colnode_count; ++nit)
	{
		TauItem		*a = colnode[nit].a->GetItem()->GetTauItem(),
					*b = colnode[nit].b->GetItem()->GetTauItem();

		if	(!a->inv_mass)
		{
			GColItem	&gcol_item = a->GetMItem()->GetCollisionInterface();

			nMatrix4::LerpAsOrthonormalBase(a->GetMItem()->GetBaseItem()->GetPreviousMatrix(), a->GetMItem()->GetBaseItem()->GetMatrix(), k0).Decompose(&p, &s, &r);
			gcol_item.SynchronizeState(p, &r, &s);
			nMatrix4::LerpAsOrthonormalBase(a->GetMItem()->GetBaseItem()->GetPreviousMatrix(), a->GetMItem()->GetBaseItem()->GetMatrix(), k1).Decompose(&p, &s, &r);
			gcol_item.SynchronizeState(p, &r, &s);
		}
		if	(!b->inv_mass)
		{
			GColItem	&gcol_item = b->GetMItem()->GetCollisionInterface();

			nMatrix4::LerpAsOrthonormalBase(b->GetMItem()->GetBaseItem()->GetPreviousMatrix(), b->GetMItem()->GetBaseItem()->GetMatrix(), k0).Decompose(&p, &s, &r);
			gcol_item.SynchronizeState(p, &r, &s);
			nMatrix4::LerpAsOrthonormalBase(b->GetMItem()->GetBaseItem()->GetPreviousMatrix(), b->GetMItem()->GetBaseItem()->GetMatrix(), k1).Decompose(&p, &s, &r);
			gcol_item.SynchronizeState(p, &r, &s);
		}
	}
#endif
}

//------------------------------------------------
void				Tau::Solver(bool contact_pass)
//------------------------------------------------
{
	//----------------------------------------------
	// vel_linear += J * inv_mass;
	// vel_angular += r.Cross(J) * inv_world_tensor;
	#define	FastImpulse(_I_, _J_, _R_)\
	{\
		nVector	Jj;\
		Vec3MulConst(Jj, _J_, _I_->inv_mass);\
		Vec3Inc(_I_->vel_linear, Jj);\
		Vec3Cross(Jj, _R_, _J_);\
		Jj *= _I_->inv_world_tensor;\
		Vec3Inc(_I_->vel_angular, Jj);\
	}
	//----------------------------------------------

	for	(uint k = 0; k < contact_iteration; ++k)
	{
//		#pragma omp parallel for
		for	(uint _nit = 0; _nit < colnode_count; ++_nit)
		{
			int			nit = (k & 1) ? _nit : colnode_count - _nit - 1;

			TauItem		*a = colnode[nit].a->GetItem()->GetTauItem(),
						*b = colnode[nit].b->GetItem()->GetTauItem();
			if	(!a->inv_mass && !b->inv_mass)
				continue;

			nVector		&n = colnode[nit].n;
			GColShape	*sa = colnode[nit].a, *sb = colnode[nit].b;

			const float	e = contact_pass ? 0.f : nMath::Clamp(sa->restitution + sb->restitution, 0.f, 1.f);

			nVector		Pn = !contact_pass ? n * colnode[nit].d * fq / 8.f : nVector(0, 0, 0);

			/*
				Sequential contact processing.
			*/
			uint			idx = colnode[nit].ctc_start;
			for	(uint _it = 0; _it < colnode[nit].ctc_count; ++_it)
			{
				int		it = (k & 1) ? _it : colnode[nit].ctc_count - _it - 1;

				nVector		ra, rb;
				float		ka = 0, kb = 0;

				if	(a->inv_mass)
				{
					Vec3Sub(ra, contact[idx + it].p, a->world_pivot);
					ka = a->inv_mass + n.Dot((ra.Cross(n) * a->inv_world_tensor).Cross(ra));
				}
				else	ra = contact[idx + it].p * a->GetMItem()->GetBaseItem()->GetInverseMatrix();
				
				if	(b->inv_mass)
				{
					Vec3Sub(rb, contact[idx + it].p, b->world_pivot);
					kb = b->inv_mass + n.Dot((rb.Cross(n) * b->inv_world_tensor).Cross(rb));
				}
				else	rb = contact[idx + it].p * b->GetMItem()->GetBaseItem()->GetInverseMatrix();

				const float	K = ka + kb;
				nVector		U, va, vb;
				a->PointVelocity(ra, va);
				b->PointVelocity(rb, vb);
				Vec3Sub(U, va, vb);
				float		Udn = Vec3Dot(U, n);

				if	(Udn >= 0.f)
				{
					float		iK = K ? 1.f / K : 0.f;

					// Compute velocity components: Un = n * Udn; Ut = U - Un;
					nVector		Un, Ut;
					Vec3MulConst(Un, n, Udn);
					Vec3Sub(Ut, U, Un);

					// Limit drift by canceling out the normal part of the impulse.
					#define		ANTIDRIFT_N_THRESHOLD	0.1f
					if	((U.Len2() < (ANTIDRIFT_N_THRESHOLD * ANTIDRIFT_N_THRESHOLD)) && (colnode[nit].d < 0.01f))
						Un *= 0.25f;

					// Compute impulse: J = (Un * -(1 + e) - Ut * f * 0.5f + Pn) * iK;
					float		_e = -(1 + e);
					Vec3ScaleConst(Un, _e);

					Ut = Ut * sa->dynamic_friction * sb->dynamic_friction;

					nVector		J = ((Un - Ut * 0.45f + Pn) * iK),
								iJ = J.Reverse();

					if	(a->inv_mass && !b->flag.Test(TauItem::FlagGhost))
						FastImpulse(a, J, ra)
					if	(b->inv_mass && !a->flag.Test(TauItem::FlagGhost))
						FastImpulse(b, iJ, rb)
				}
			}
		}	// #
	}

	if	(contact)
	{
		// Pre-transform constraints.
		TransformConstraints();

		// Process constraints.
		nLinkedListPool		pool;
		nLinkedListEntry	*e;
		for	(uint k = 0; k < constraint_iteration; ++k)
		{
			pool.Reset();
			while	(e = constraint_list.Pool(pool))
				((TauConstraint *)e)->Process();
		}
	}
}

//---------------------------------------
void				Tau::Wake(TauItem *t)
//---------------------------------------
{
	__SUBSYSTEM(System::SystemPhysic);

	if	(t->GetMode() != TauItem::ModeNone)		// Do not wake static items.
		if	(sleep_list.Extract(t))
		{
			t->WakeUp();

			t->gcol_item->Activate();
			t->SynchronizeCollision();

			item_list.Add(t);
		}
}

//----------------------------------------
void				Tau::Sleep(TauItem *t)
//----------------------------------------
{
	__SUBSYSTEM(System::SystemPhysic);

	if	(item_list.Extract(t))
	{
		t->active = false;
//		t->prv_state.vel_linear.Set();
//		t->prv_state.vel_angular.Set();

		t->gcol_item->Deactivate();

		sleep_list.Add(t);
	}
}

//------------------------------------------
void				Tau::PopulateCollision()
//------------------------------------------
{
	colnode_count = gcol->PopulateSystemCollision(colnode, colnode_max, contact, contact_max, true);
}

//------------------------------------
float				Tau::SolveSystem()
//------------------------------------
{
	TauItem		*ci;

//-----------------------------------------------------------------------------
	// Wake up sleeping items on external force.
	if	(sleep_list.GetCount())
		for	(ci = (TauItem *)sleep_list.GetRoot(); ;)
		{
			nLinkedListEntry	*n = ci->next;
			if	(ci->total_force.Len2() > 0.0001f)
					Wake(ci);
			else	ci->ResetForces();
			if	(!n)
				break;
			ci = (TauItem *)n;
		}
//-----------------------------------------------------------------------------

	if	(!gcol || !item_list.GetCount())
	{
		colnode_count = 0;
		return float(1);
	}

	//
	TauItem		**items = item_list.Consolidate <TauItem *> ();
	int			item_count = item_list.GetCount();

	// Swap collision node buffers.
	prv_colnode = colnode;
	prv_colnode_count = colnode_count;
	colnode = (colnode == colnode_a) ? colnode_b : colnode_a;

//-----------------------------------------------------------------------------

//	#pragma omp parallel for
	for (int n = 0; n < item_count; ++n)
	{
		items[n]->UpdateVelocity();

		items[n]->UpdatePosition();
		items[n]->ComputeAuxilliaryDatas();
		items[n]->SynchronizeCollision();

		items[n]->vel_linear = items[n]->prv_state.vel_linear;
		items[n]->vel_angular = items[n]->prv_state.vel_angular;
	}

	statistics.collision.Start(merge_statistics);
	colnode_count = gcol->PopulateSystemCollision(colnode, colnode_max, contact, contact_max, true);
	statistics.collision.Stop();
	
	// Wake-up bodies on impact.
	for	(uint nit = 0; nit < colnode_count; ++nit)
	{
		TauItem		*a = colnode[nit].a->GetItem()->GetTauItem(),
					*b = colnode[nit].b->GetItem()->GetTauItem();

				if	(!a->IsActive() && !b->flag.Test(TauItem::FlagGhost) && !b->TestVelocityThreshold(2100.f))
				Wake(a);
		else	if	(!b->IsActive() && !a->flag.Test(TauItem::FlagGhost) && !a->TestVelocityThreshold(2100.f))
				Wake(b);
	}

//	InterpolateStaticColliderTransformation();
	statistics.solver.Start(merge_statistics);
	Solver(false);	// Collision pass.
	statistics.solver.Stop();

//-----------------------------------------------------------------------------

	// Advance velocities.
//	#pragma omp parallel for
	for (int n = 0; n < item_count; ++n)
	{
		items[n]->position = items[n]->prv_state.position;
		items[n]->rotation = items[n]->prv_state.rotation;

		items[n]->rotation_matrix = items[n]->prv_state.rotation_matrix;
		items[n]->inv_world_tensor = items[n]->prv_state.inv_world_tensor;
		items[n]->world_pivot = items[n]->prv_state.world_pivot;

		items[n]->UpdateVelocity();
		items[n]->ResetForces();

		items[n]->UpdatePosition();
		items[n]->ComputeAuxilliaryDatas();
		items[n]->SynchronizeCollision();
		items[n]->gcol_item->Activate();
	}

	statistics.collision.Start(true);
	colnode_count = gcol->PopulateSystemCollision(colnode, colnode_max, contact, contact_max, true);
	statistics.collision.Stop();

	InterpolateStaticColliderTransformation();

	statistics.solver.Start(true);
	Solver(true);	// Contact pass.
	statistics.solver.Stop();

	// Advance positions.
	const int	delay = int(fq * K_DEAK);

//	#pragma omp parallel for
	for	(int n = 0; n < item_count; ++n)
	{
		items[n]->position = items[n]->prv_state.position;
		items[n]->rotation = items[n]->prv_state.rotation;

		items[n]->rotation_matrix = items[n]->prv_state.rotation_matrix;
		items[n]->inv_world_tensor = items[n]->prv_state.inv_world_tensor;
		items[n]->world_pivot = items[n]->prv_state.world_pivot;

		items[n]->UpdatePosition();
	}

	if	(event)
		event->PhysicStep(event_user_data, true);

	for	(int n = 0; n < item_count; ++n)
	{
		items[n]->ComputeAuxilliaryDatas();
		items[n]->SynchronizeCollision();

		// Deactivation test.
		if	(items[n]->TestVelocityThreshold(2000.f) && enable_deactivation)
		{
			items[n]->sleep++;
			if	(items[n]->sleep >= delay)
			{
				nVector	 bl = items[n]->vel_linear;
				nVector	 ba = items[n]->vel_angular;

				items[n]->vel_linear.Set();
				items[n]->vel_angular.Set();

//				#pragma omp critical
					Sleep(items[n]);
			}
		}
		else	items[n]->sleep = 0;

		items[n]->vel_linear *= items[n]->linear_damping;
		items[n]->vel_angular *= items[n]->angular_damping;
	}

	SafeDeleteArray(items);
	return float(1);
}

//------------------------------
void				Tau::Reset()
//------------------------------
{
	timer = 0;
	colnode_count = 0;
	fps_clamp = 1;
	leftright = true;

	// Wake up all items (move all lists to the active one).
	if	(sleep_list.GetCount())
		for	(TauItem *ci = (TauItem *)sleep_list.GetRoot(); ;)
		{
			nLinkedListEntry	*n = ci->next;
			Wake(ci);
			if	(!n)
				break;
			ci = (TauItem *)n;
		}
}

//------------------------------------------
void				Tau::FreeCollisionNode()
//------------------------------------------
{
	colnode_max = 0;
	SafeDeleteArray(colnode_a);
	SafeDeleteArray(colnode_b);

	colnode = NULL;
	colnode_count = 0;
	prv_colnode = NULL;
	prv_colnode_count = 0;

	SafeDeleteArray(contact);
	contact_max = 0;
}

//-------------------------------------------
void				Tau::Free(bool free_data)
//-------------------------------------------
{
	item_list.DeleteAll(free_data);
	sleep_list.DeleteAll(free_data);
	freeze_list.DeleteAll(free_data);

	constraint_list.DeleteAll(true);
}

//---------------------------------------------
void				Tau::AddItem(TauItem &item)
//---------------------------------------------
{
	__SUBSYSTEM(System::SystemPhysic);

	// Check all lists for duplicate.
	if	(item_list.Belong(&item))
		return;
	if	(sleep_list.Belong(&item))
		return;
	if	(freeze_list.Belong(&item))
		return;

	// We know the item is not in list.
	item_list.Add(&item, false);
}

//---------------------------------------------------------------
void				Tau::RemoveItem(TauItem &item, bool deletion)
//---------------------------------------------------------------
{
	if	(deletion)
	{
		// Remove all constraint references.
		nLinkedListForeach(TauConstraint, c, constraint_list)
		{
			if	(c->GetAnchor() == &item)
				c->SetAnchor(NULL);
			if	(c->GetTarget() == &item)
				c->SetTarget(NULL);
		}

		for (uint n = 0; n < constraint_list.GetCount(); ++n)
		{
			TauConstraint	*c = (TauConstraint *)constraint_list.Get(n);
			if	(!c->GetAnchor() && !c->GetTarget())
				constraint_list.Delete(c);
		}
	}

	// Remove from internal lists.
	if	(item_list.Extract(&item))
		return;
	if	(sleep_list.Extract(&item))
		return;
	if	(freeze_list.Extract(&item))
		return;
}

//------------------------------------------------------------
void				Tau::AllocateCollisionNode(uint n, uint c)
//------------------------------------------------------------
{
	__SUBSYSTEM(System::SystemPhysic);

	FreeCollisionNode();

	colnode_a = new GColNode[n];
	colnode_b = new GColNode[n];
	contact = new GColContact[c];

	__LOG__ << "Tau alloc = " << sizeof(GColNode) * n * 2 + sizeof(GColContact) * c << " byte(s) (" << n << " node(s), " << c << " contact(s)).\n";
	if	(!colnode_a || !colnode_b || !contact)
	{
		FreeCollisionNode();
		__ERRRAW__(__LOG_E__ << "Could not allocate Tau collision nodes.\n")
	}
	colnode_max = n;

	colnode = colnode_a;
	colnode_count = 0;
	prv_colnode = NULL;
	prv_colnode_count = 0;

	contact_max = c;
}

//--------
Tau::Tau()
//--------
{
	gravity.Set(0, Mtr(-9.8f), 0);

	event = NULL;

	Reset();
	fq = 75.f;
	SetCollisionEngine();

	SetJpm(0.01f);
	contact_iteration = 16;
	constraint_iteration = 16;

	colnode_a = colnode_b = NULL;
	colnode = NULL;
	colnode_count = 0;
	colnode_max = 0;

	contact = NULL;
	contact_max = 0;

	AllocateCollisionNode();

	enable_deactivation = true;
}

//---------
Tau::~Tau()
//---------
{
	FreeCollisionNode();
}
