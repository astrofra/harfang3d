/*

GCollide	[Generic collision]
			Collision sweep and prune.

			Emmanuel Julien 2004.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


#ifdef	__GCOL_ENABLE_SAP__


		/*!
			@short	Select SAP axis test order.

			0 = X axis,<br>
			1 = Y axis,<br>
			2 = Z axis.<br>

			The {1,2,0} combination gives better stability to Tau since gravity
			is usually along the Y axis. Sadely enough this is about the worst
			performing combination in most scenarios (except for vertical
			scenes).

			The {0, 2, 1} combination gives the best performances overall.
		*/
		#define		SAP_AXIS0		0
		#define		SAP_AXIS1		2
		#define		SAP_AXIS2		1


//--------------------------------------
void				GCollide::SAP_Free()
//--------------------------------------
{
	SafeDeleteArray(SAP_array);
	SAP_arraysize = 0;
	SAP_totalsize = 0;
}

//--------------------------------------------------
void				GCollide::SAP_Allocate(uint max)
//--------------------------------------------------
{
	pGColItem	*narray = new pGColItem[max];
	if	(!narray)
		__ERRRAW__(__LOG_E__ << "Failed to allocate SAP structures (n = " << max << ").\n")

	uint	n;
	for	(n = 0; n < SAP_arraysize; n++)
		narray[n] = SAP_array[n];

	SAP_Free();
	SAP_array = narray;
	SAP_arraysize = n;
	SAP_totalsize = max;
}

//--------------------------------------
void				GCollide::SAP_Sort()
//--------------------------------------
{
	int			n, lo = 0, hi = SAP_arraysize;
	GColItem	*swap;

	for	(;;)
	{
		swap = NULL;
		for	(n = lo; n < (hi - 1); n++)
			if	(SAP_array[n]->world_minmax.GetMin(SAP_AXIS0) > SAP_array[n + 1]->world_minmax.GetMin(SAP_AXIS0))
			{
				swap = SAP_array[n];
				SAP_array[n] = SAP_array[n + 1];
				SAP_array[n + 1] = swap;
			}
		if	(!swap)
			return;
		hi--;

		swap = NULL;
		for	(n = (hi - 1); n > lo; n--)
			if	(SAP_array[n]->world_minmax.GetMin(SAP_AXIS0) < SAP_array[n - 1]->world_minmax.GetMin(SAP_AXIS0))
			{
				swap = SAP_array[n];
				SAP_array[n] = SAP_array[n - 1];
				SAP_array[n - 1] = swap;
			}
		if	(!swap)
			return;
		lo++;
	}
}

//------------------------------------------------------
void				GCollide::DBG_SAP(nRenderer &render)
//------------------------------------------------------
{
	if	(!use_sap)
		return;

	nColor	active_color(0.25f, 0, 1.f);
	nColor	deactive_color(0.75f, 0, 0.2f);

	for	(uint n = 0; n < SAP_arraysize; n++)
	{
		GColItem	*ci = SAP_array[n];
		if	(ci->active)
				render.DrawAABB(ci->world_minmax, &active_color);
		else	render.DrawAABB(ci->world_minmax, &deactive_color);
	}
}
#endif	// __GCOL_ENABLE_SAP__

//----------------------------------------------------------------
uint				GCollide::PopulateSystemCollision
										(
											/// @todo Move collision nodes inside GCollide.
											GColNode *node,
											uint node_max,
											GColContact *contact,
											uint contact_max,
											bool prohibit_callback
										)
//----------------------------------------------------------------
{
	GColItem			*sc, *dc;
	uint				node_count = 0, contact_count = 0;
	bool				abort = true;

	nLinkedListForeach(GColItem, __sc, item_list)
		if	(__sc->active)
		{
			__sc->ComputeWorldMinMax(__sc->world_minmax);
			__sc->collided = false;
			abort = false;
		}

	// Reset statistics.
	statistics.spheresphere_count = 0;
	statistics.sphereobb_count = 0;
	statistics.obbobb_count = 0;
	statistics.polyobb_count = 0;
	statistics.polynode_merge = 0;

	// Early exit.
	if	(abort)
		return 0;

#ifdef	__GCOL_ENABLE_SAP__
	if	(use_sap && SAP_arraysize)
	{
		// Insert-sort SAP array.
		SAP_Sort();

		// Sweep through the list and collect overlapping boxes.
		for	(uint n = 0; n < (SAP_arraysize - 1); ++n)
		{
			sc = SAP_array[n];
			const float		cmax = sc->world_minmax.GetMax(SAP_AXIS0);

			for	(uint m = n + 1; m < SAP_arraysize; ++m)
			{
				dc = SAP_array[m];
				if	(dc->world_minmax.GetMin(SAP_AXIS0) > cmax)
					break;

				if	(sc->active || dc->active)
				{
					uint		node_start = node_count;

					// Populate item pair contacts.
					if	(
							sc->world_minmax.TestAxisOverlap(dc->world_minmax, SAP_AXIS1) &&
							sc->world_minmax.TestAxisOverlap(dc->world_minmax, SAP_AXIS2) &&
							PopulateItemCollision(sc, dc, node, &node_count, contact, &contact_count)
						)
						{
							// Locate pair.
							GColPair	*pair = collision_pair.Find(sc, dc);

							// Allocate if none found.
							if	(!pair)
								pair = collision_pair.Add(sc, dc);

							// Store pair contacts for feedback.
							float		scale = pair->a == sc ? 1.f : -1.f;
							for (uint n = node_start; n < node_count; ++n)
								for (uint c = 0; c < node[n].ctc_count; ++c)
									pair->StoreContact(contact[node[n].ctc_start + c].p, node[n].n * scale, node[n].d);

							// Log collision.
							sc->collided = dc->collided = true;
						}
				}
			}
		}
	}
	else
#endif	// __GCOL_ENABLE_SAP__
	{
		nLinkedListPool		pool_s, pool_d;
		while	(sc = (GColItem *)item_list.Pool(pool_s))
		{
			sc->lock = true;
			pool_d.Reset();
			while	(dc = (GColItem *)item_list.Pool(pool_d))
				if	(!dc->lock)
					if	(sc->active || dc->active)
						if	(sc->world_minmax.TestOverlap(dc->world_minmax))
							PopulateItemCollision(sc, dc, node, &node_count, contact, &contact_count);
		}
		pool_s.Reset();
		while	(sc = (GColItem *)item_list.Pool(pool_s))
			sc->lock = false;
	}

	// Collision callback.
	if	(!prohibit_callback && CB_Collision)
		CB_Collision(node_count, node, contact);

	// Sort collision node contact list per depth.
/*
*/
	return node_count;
}
