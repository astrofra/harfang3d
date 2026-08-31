/*

Tau			[Physic engine]

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


		#include	"physic.h"
//		#include	"../manager/manager.h"


//----------------------------------------------------
void				Tau::PredictInterference(float dt)
//----------------------------------------------------
{
	nLinkedListForeach(TauItem, ci, item_list)
	{
		/*
			We do not care about keeping the physic and collision
			states synchronized for this simple interference test.
		*/
		nVector		bck_pos = ci->position;
		ci->position += ci->vel_linear * dt * 1.1f;	// Expand the delta frame so as to minimize collision miss.
		ci->SynchronizeCollision();
		ci->position = bck_pos;

		if	(!ci->next)
			break;
	}

	statistics.collision.Start();
	colnode_count = gcol->PopulateSystemCollision(colnode, colnode_max, contact, contact_max, true);
	statistics.collision.Stop();
}
