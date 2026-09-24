// HARFANG(R) Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/timer.h"

int main() {
	// Explicit shutdown and restart must remain supported.
	hg::start_timer();
	hg::stop_timer();
	hg::stop_timer();
	hg::start_timer();
	hg::start_timer();
	hg::run_periodic([] {}, hg::time_from_ms(1));
	// Deliberately omit stop_timer: normal process exit must join the worker,
	// not abort in the destructor of a joinable std::thread.
	return 0;
}
