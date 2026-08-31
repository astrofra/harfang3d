/*

GCollide	[Generic collision]
			Id: Library item.

			Emmanuel Julien 2004~2005.
			http://www.nengine.fr
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"


//----------------------------------------------------------------------------------------------
void				GColItem::DBG_Shape(nRenderer &render, uint lit_count, GColShape **lit_list)
//----------------------------------------------------------------------------------------------
{
	nColor			lit_color(1.0f, 0.f, 0.75f),
					normal_color(0.5f, 0.f, 0.f),
					*color;

	nLinkedListPool	pool;
	for	(GColShape *s; s = (GColShape *)shape_list.Pool(pool);)
	{
		// Collision does not support scaling.
		nMatrix4		sn_matrix = s->matrix.AsOrthonormalBase();

		// Get shape color.
		uint			n;
		for (n = 0; n < lit_count; ++n)
			if	(lit_list[n] == s)
				break;
		color = n < lit_count ? &lit_color : &normal_color;

		// Display.
		switch	(s->type)
		{
			case	GColSphere:
				render.DrawBall(sn_matrix, s->scale.x, color);
				break;

			case	GColCuboid:
			{
				nOBB		obb(&sn_matrix.GetRow(3), &(s->scale), &nMatrix3::FromMatrix4(sn_matrix));
				render.DrawOBB(obb, color);
				break;
			}

			case	GColMesh:
			{
				nGeometry	*geo = s->mesh_tree ? s->mesh_tree->GetGeometry() : NULL;
				if	(!geo)
					break;

				for	(uint n = 0; n < geo->pol_count; ++n)
				{
					nVector	v[2];
					v[1] = geo->vtx[geo->pol[n].bind[0]] * sn_matrix;
					for	(uint m = 1; m < (uint)(geo->pol[n].vtx_count - 1); ++m)
					{
						v[0] = v[1];
						v[1] = geo->vtx[geo->pol[n].bind[m]] * sn_matrix;
						render.Line3D(v[0], v[1], color);
					}
					v[0] = v[1];
					v[1] = geo->vtx[geo->pol[n].bind[0]] * sn_matrix;
					render.Line3D(v[0], v[1], color);
				}
			}
			break;

			default:
				break;
		}
/*
		if	(shape_list.GetCount() > 1)
			render.DrawAABB(s->minmax, &aabb_color);
*/
	}
}
