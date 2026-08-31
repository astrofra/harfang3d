/*

Physic		[Generic physic & collision engine]
			Library headers.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
------------------------------------------

*/


#ifndef	__NPHYSIC__
#define	__NPHYSIC__


		/*!
			Define to enable the static contact cache system.
			This system has too many drawbacks (especially with strong joints)
			and has been deactivated.
		*/
#define	__ENABLE_STATICCONTACTCACHE__


		#include	"framework/framework.h"
		#include	"../core/core.h"

		#include	"gcol_shape.h"
		#include	"gcol_item.h"
		#include	"gcol_system.h"

		#include	"tau_event.h"
		#include	"tau_physicstate.h"
		#include	"tau_constraint.h"
		#include	"tau_distanceconstraint.h"
		#include	"tau_positionconstraint.h"
		#include	"tau_hingeconstraint.h"
		#include	"tau_angularconstraint.h"
		#include	"tau_velocityconstraint.h"
		#include	"tau_item.h"
		#include	"tau_system.h"


#endif	// __NPHYSIC__
