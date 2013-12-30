// Local Variables:
// mode: c++
// End:

#ifndef KEYS_H
#define KEYS_H

#include <X11/Xlib.h>

#include "events.h"

int keys_run(XKeyEvent* e);
void keys_run_button(XButtonEvent* e);

#endif	// KEYS_H
