/*

GCollide	Collision library.
			Written by Emmanuel Julien.
			All rights reserved 2000~2005.

			Emmanuel Julien
			http://www.nengine.fr
			mailto:ejulien@nengine.fr

*/


		#include	"config.h"
		#include	"physic.h"


		/// Collision callback function.
void	(*GCollide::CB_Collision)(uint count, GColNode *node, GColContact *ctc) = NULL;


//----------------------------------------------------------------------------------------
void				GColPair::StoreContact(const nVector &_p, const nVector &_n, float _d)
//----------------------------------------------------------------------------------------
{
	if	(ctc_count == __GCOL_MONITOR_MAX_CONTACT_PER_PAIR__)
		return;

	// Check all contacts for a redundant contact.
	for (uint c = 0; c < ctc_count; ++c)
		if	((nVector::Dist2(p[c], _p) < Mm(5)) && (n[c].Dot(_n) > 0.996))		// 5mm and ~5° tolerance.
			return;		// Redundant.

	// Add contact.
	p[ctc_count] = _p;
	n[ctc_count] = _n;
	d[ctc_count] = _d;
	ctc_count++;
}

//-------------------------------------------------------------------------------------------
bool				GCollide::TestItemCollision(GColItem *sc, GColItem *dc, bool ignore_mask)
//-------------------------------------------------------------------------------------------
{
	if	(sc->self_mask & dc->col_mask)
	{
		nLinkedListPool	pool_a, pool_b;
		for	(GColShape *sa; sa = (GColShape *)sc->shape_list.Pool(pool_a);)
		{
			pool_b.Reset();
			for	(GColShape *sb; sb = (GColShape *)dc->shape_list.Pool(pool_b);)
				if	(PopulateShapeCollision(sa, sb, NULL, NULL, NULL, NULL))
					return true;
		}
	}
	return false;
}

//-----------------------------------------------------------
bool				GCollide::PopulateItemCollision
										(
											GColItem *sc,
											GColItem *dc,
											GColNode *node,
											uint *node_count,
											GColContact *ctc,
											uint *ctc_count
										)
//-----------------------------------------------------------
{
	bool		ret = false;
	if	(sc->self_mask & dc->col_mask)
	{
		nLinkedListPool	pool_a, pool_b;
		for	(GColShape *sa = NULL; sa = (GColShape *)sc->shape_list.Pool(pool_a);)
		{
			pool_b.Reset();
			for	(GColShape *sb = NULL; sb = (GColShape *)dc->shape_list.Pool(pool_b);)
				ret |= PopulateShapeCollision(sa, sb, node, node_count, ctc, ctc_count);
		}
	}
	return ret;
}

//------------------------------------------------------------
bool				GCollide::PopulateConvexCollision
										(
											GColShape *sc,
											GColShape *dc,
											GColNode *node,
											uint *node_count,
											GColContact *ctc,
											uint *ctc_count
										)
//------------------------------------------------------------
{
	nVector		normal, contact[8];
	float		depth, depths[8];
	uint		nctc = 0;

	GColCtcInfo	cinfo;
	cinfo.n = &normal;
	cinfo.d = &depth;

	if	(ctc)
	{
		cinfo.cmax = 8;
		cinfo.p = contact;
		cinfo.t = depths;
	}

	switch	(sc->type)
	{
		case	GColSphere:
			switch	(dc->type)
			{
				case	GColSphere:
				{
					statistics.spheresphere_count++;
					nVector		sprvp(sc->prv_matrix.GetRow(3));
					nVector		sp(sc->matrix.GetRow(3));
					nVector		dprvp(dc->prv_matrix.GetRow(3));
					nVector		dp(dc->matrix.GetRow(3));

					nctc = SphereSweptSphere(sprvp, sp, sc->scale.x, dprvp, dp, dc->scale.x, &cinfo);
				}
				break;

				case	GColCuboid:
				{
					statistics.sphereobb_count++;
					nOBB		prv_obb	(
												&dc->prv_matrix.GetRow(3),
												&(dc->scale * dc->item->scale),
												&nMatrix3::FromMatrix4(dc->prv_matrix)
											);
					nOBB		obb		(
												&dc->matrix.GetRow(3),
												&(dc->scale * dc->item->scale),
												&nMatrix3::FromMatrix4(dc->matrix)
											);
					nVector		prv_pos(sc->prv_matrix.GetRow(3));
					nVector		pos(sc->matrix.GetRow(3));

					nctc = OBBSweptSphere(prv_obb, obb, prv_pos, pos, sc->scale.x, &cinfo);
				}
				break;

				default:
					break;
			}
			break;

		case	GColCuboid:
			switch	(dc->type)
			{
				case	GColCuboid:
				{
					statistics.obbobb_count++;
					nOBB		obb_a	(
												&sc->matrix.GetRow(3),
												&(sc->scale * sc->item->scale),
												&nMatrix3::FromMatrix4(sc->matrix)
											);
					nOBB		obb_b	(
												&dc->matrix.GetRow(3),
												&(dc->scale * dc->item->scale),
												&nMatrix3::FromMatrix4(dc->matrix)
											);

					nctc = OBBOverlapOBB(obb_a, obb_b, &cinfo);
				}
				break;

				default:
					break;
			}
			break;

		default:
			break;
	}
	if	(!nctc)
		return false;

	if	(node)
	{
		const uint	nc = *node_count;

		node[nc].a = sc;
		node[nc].b = dc;
		node[nc].n = normal;
		node[nc].d = depth;

		if	(ctc)
		{
			uint	cc = ctc_count[0];
			node[nc].ctc_start = cc;
			node[nc].ctc_count = nctc;
			for	(uint n = 0; n < nctc; ++n)
			{
				ctc[cc].p = contact[n];
				ctc[cc++].d = depths[n];
			}
			ctc_count[0] = cc;
		}
		else
			node[nc].ctc_count = 0;

		node_count[0]++;
	}
	return true;
}

#define	POLYMESH_DEPTH_DAMPEN_FACTOR	1.f

//----------------------------------------------------------
void				GCollide::AppendMergeNode
									(
										GColShape *sc,
										GColShape *dc,
										uint nctc,
										GColCtcInfo &cinfo,
										GColNode *node,
										uint *node_count,
										GColContact *ctc,
										uint *ctc_count
									)
//----------------------------------------------------------
{
	float		depth = cinfo.d[0] * POLYMESH_DEPTH_DAMPEN_FACTOR;
	uint		nc = node_count[0];
	uint		cc = ctc_count[0];

	// Check for merge against current node.
	bool		merge = (nc > 0) &&
						(node[nc - 1].n.Dot(cinfo.n[0]) > 0.99f) &&
						(node[nc - 1].a == sc) &&
						(node[nc - 1].b == dc);

	if	(merge)
	{	// Append to previous node.
		nc--;
		node[nc].n = (node[nc].n + cinfo.n[0]).Normalize();
		node[nc].d = (node[nc].d + depth) * 0.5f;
		statistics.polynode_merge++;
	}
	else
	{	// Open new node.
		node[nc].a = sc;
		node[nc].b = dc;
		node[nc].n = cinfo.n[0];
		node[nc].d = depth;

		node[nc].ctc_start = cc;
		node[nc].ctc_count = 0;
	}

	for	(uint n = 0, m; n < nctc; ++n)
	{
		// Merge redundant contacts.
		for	(m = 0; m < node[nc].ctc_count; ++m)
			if	(nVector::Dist2(ctc[node[nc].ctc_start + m].p, cinfo.p[n]) < 0.001f)
				break;

		if	(m == node[nc].ctc_count)
		{
			ctc[cc].p = cinfo.p[n];
			ctc[cc++].d = cinfo.t[n];
			node[nc].ctc_count++;
		}
	}

	ctc_count[0] = cc;
	node_count[0] = nc + 1;
}

//-----------------------------------------------------------
bool				GCollide::PopulatePolymeshCollision
										(
											GColShape *sc,
											GColShape *dc,
											GColNode *node,
											uint *node_count,
											GColContact *ctc,
											uint *ctc_count
										)
//-----------------------------------------------------------
{
/*
	switch	(sc->type)
	{
		case	GColSphere:
		case	GColCuboid:
			break;

		default:
			return false;
	}
*/
	float			depth = 1.f;

	nGeometry		*geo = dc->mesh_tree->GetGeometry();
	uint			polyarray[1024];		// 4k

	nOBB			grab_obb(nOBB::FromMinMax(sc->minmax));
	grab_obb.Transform(dc->imatrix);
	nMinMax			qminmax;
	grab_obb.ComputeMinMax(qminmax);
	uint			polycount = 0;

	polycount = dc->mesh_tree->Intersect(qminmax, polyarray, 1024);

	// Collision structures.
	nVector			normal;

	GColCtcInfo		cinfo;
	cinfo.n = &normal;
	cinfo.d = &depth;

	// Contact infos.
	nVector			contact[32];
	float			depths[32];

	if	(ctc)
	{
		cinfo.cmax = 32;
		cinfo.p = contact;
		cinfo.t = depths;
	}

	switch	(sc->type)
	{
		case	GColSphere:
		{
			nMatrix4		ipmtx = dc->imatrix * sc->prv_matrix;
			nMatrix4		imtx = dc->imatrix * sc->matrix;
			nVector			prv_pos = ipmtx.GetRow(3), pos = imtx.GetRow(3);

			for	(uint n = 0; n < polycount; ++n)
			{
				statistics.polysphere_count++;
				uint			nctc;
				if	(nctc = SweptSpherePolygon(prv_pos, pos, sc->scale.x, polyarray[n], *geo, &cinfo))
				{
					if	(!node)
						break;

					uint	nc = node_count[0];
					node[nc].a = sc;
					node[nc].b = dc;
					dc->matrix.ApplyRotation(&node[nc].n, &normal);
					node[nc].d = depth;

					if	(ctc)
					{
						ctc[ctc_count[0]].p = cinfo.p[0] * dc->matrix;
						ctc[ctc_count[0]].d = cinfo.t[0];

						node[nc].ctc_start = ctc_count[0];
						node[nc].ctc_count = 1;
						ctc_count[0]++;
					}
					node_count[0]++;
				}
			}
			if	(!node)
			{
				depth = 0;
				break;
			}
		}
		break;

		case	GColCuboid:
		{
			nOBB		obb;
			obb.bb_position = sc->matrix.GetRow(3);
			obb.bb_scale = sc->scale * sc->item->scale;
			obb.bb_rotation = nMatrix3::FromMatrix4(sc->matrix);

			for	(uint n = 0; n < polycount; ++n)
			{
				statistics.polyobb_count++;
				uint			nctc;
				if	(nctc = OBBOverlapPolygon(obb, dc->matrix, geo->pol[polyarray[n]], *geo, &cinfo))
				{
					if	(ctc)
						AppendMergeNode(sc, dc, nctc, cinfo, node, node_count, ctc, ctc_count);
					else
					{
						if	(!node)
							break;

						uint	nc = node_count[0];
						node[nc].a = sc;
						node[nc].b = dc;
						node[nc].n = normal;
						node[nc].d = depth * POLYMESH_DEPTH_DAMPEN_FACTOR;
						node_count[0]++;
					}
				}
			}
			if	(!node)
			{
				depth = 0;
				break;
			}
		}
		break;

		default:
			break;
	}
	return (depth < 1) ? true : false;
}

//-----------------------------------------------------------
bool				GCollide::PopulateShapeCollision
										(
											GColShape *sc,
											GColShape *dc,
											GColNode *node,
											uint *node_count,
											GColContact *ctc,
											uint *ctc_count
										)
//-----------------------------------------------------------
{
	if	(!sc->active || !dc->active)
		return false;
	if	(!sc->minmax.TestOverlap(dc->minmax))
		return false;

	uint	node_start = node_count ? *node_count : 0;
	bool	swapped = false;

	if	(sc->type > dc->type)
	{
		GColShape	*tmp = sc;
		sc = dc;
		dc = tmp;
		swapped = true;
	}
	if	(!node_count)
		node = NULL;
	if	(!ctc_count)
		ctc = NULL;

	// NOTE Do not use UpdateTimer in this function!
	bool	rt = false;

	switch	(sc->type)
	{
		case	GColMesh:
			rt = PopulatePolymeshCollision(dc, sc, node, node_count, ctc, ctc_count);
			break;

		case	GColSphere:
		case	GColCuboid:
			rt = PopulateConvexCollision(sc, dc, node, node_count, ctc, ctc_count);
			break;

		default:
			break;
	}
	// NOTE Do not use UpdateTimer in this function!

	if	(rt)
		if	(node_count && swapped)
			for ( ; node_start < *node_count; ++node_start)
				node[node_start].n *= -1;
		
	return rt;
}

//-------------------------------------------------------
void				GCollide::DBG_Item(nRenderer &render)
//-------------------------------------------------------
{
	nLinkedListForeach(GColItem, sc, item_list)
		sc->DBG_Shape(render);
}

//-------------------------------------------------------------------
bool				GCollide::FreezeItem(GColItem &item, bool freeze)
//-------------------------------------------------------------------
{
	__SUBSYSTEM(System::SystemCollision);

	if	(freeze)
	{
		if	(!item_list.Belong(&item))
			return false;	// Do not deactivate an item that is not activated.
		RemoveItem(item);
		freeze_list.Add(&item);
	}
	else
	{
		if	(!freeze_list.Extract(&item))
			return false;	// Do not activate an item that is not deactivated.
		AddItem(item);
	}
	return true;
}

//---------------------------------------------------
void				GCollide::AddItem(GColItem &item)
//---------------------------------------------------
{
	__SUBSYSTEM(System::SystemCollision);

	if	(item_list.Belong(&item) || freeze_list.Belong(&item))
		return;

#ifdef	__GCOL_ENABLE_SAP__
	if	(SAP_arraysize == SAP_totalsize)
		__ERRRAW__(__LOG_E__ << "Cannot add item to the collision list, increase the Sweep and Prune limit or disable Sweep and Prune phase.\n")
#endif
	item_list.Add(&item, false);

#ifdef	__GCOL_ENABLE_SAP__
	SAP_array[SAP_arraysize] = &item;
	SAP_arraysize++;
#endif
}

//------------------------------------------------------
void				GCollide::RemoveItem(GColItem &item)
//------------------------------------------------------
{
	__SUBSYSTEM(System::SystemCollision);

	if	(!item_list.Extract(&item))
	{
		freeze_list.Extract(&item);
		/*
			The item could still have been in the freeze list
			in which case it won't be in the SAP array anyway.
		*/
		return;
	}

	// Remove from SAP.
#ifdef	__GCOL_ENABLE_SAP__
	uint	n = 0;
	for	(; n < SAP_arraysize; ++n)
		if	(SAP_array[n] == &item)
			break;

	if	(n < SAP_arraysize)
	{
		SAP_array[n] = SAP_array[SAP_arraysize - 1];		// We'll have some more sorting to do...
		SAP_arraysize--;
	}
#endif
}

//-----------------------------------
void				GCollide::Reset()
//-----------------------------------
{
	nLinkedListForeach(GColItem, sc, item_list)
		sc->Reset();
}

//------------------------------------------------
void				GCollide::Free(bool free_data)
//------------------------------------------------
{
	item_list.DeleteAll(free_data);
	GetCollisionMonitor().DeleteAll(free_data);

#ifdef	__GCOL_ENABLE_SAP__
	SAP_arraysize = 0;
#endif
}

//------------------
GCollide::GCollide()
//------------------
{
	__SUBSYSTEM(System::SystemCollision);

#ifdef	__GCOL_ENABLE_SAP__
	SAP_array = NULL;
	SAP_arraysize = 0;
	SAP_Allocate(__GCOL_SAP_DEFAULT_MAX__);
	EnableSweepAndPrune();
#endif
}

//-------------------
GCollide::~GCollide()
//-------------------
{
#ifdef	__GCOL_ENABLE_SAP__
	SAP_Free();
#endif
}
