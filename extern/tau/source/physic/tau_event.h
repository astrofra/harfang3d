/*

nEngine		[3D Engine]
			Tau event base class module.

			Emmanuel Julien 2004~2005.
			http://xbarr.ninomojo.com
			mailto:ejulien@nengine.fr
----------------------------------------

	Revision History:

		Created	16/8/2004	(Emmanuel Julien)

*/


#ifndef	__TAU_EVENT__
#define	__TAU_EVENT__


/*!
	@short	The Tau event base class.

	Derive this class and register a derived instance in a bfmk_Tau
	object to get time independent function call.
	The Step() method is guaranteed to be called at an exact frequency.

	@note	Although between two display refresh the call frequency is fixed,
			the calls to Step() are not guaranteed to be equally spaced in
			time!

	@author Emmanuel Julien (ejulien@nworks.fr)
*/
class		TauEvent
{
public:
			/*!
				Pure virtual method to be implemented in a derived class.
				This function is called by the scene the class was registered
				into at a constant frequency.
				All time independent behaviors should be done from this function.

				@note	The physic step taken boolean indicate whether the physic
						engine actually updated physic items or not.
						This is useful if you are doing physic manipulations in this
						callback (eg. in a script OnPhysicStep() method).
						This can happen when the CPU load is too high and the physic
						engine has to skip evaluation.
						The PhysicStep() method is still called for each skipped step
						to ensure synchronization between engine components.

				@see	nTau::SetFrequency().
			*/
virtual		void	PhysicStep(void *user_data, bool physic_step_taken) = 0;
};


#endif	// __TAU_EVENT__
