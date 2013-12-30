#ifndef KEYS_H
#define KEYS_H

#include <X11/Xlib.h>

#include "events.h"

typedef struct {
  int key;
  void (*ev)(EvArgs);
  EvArgs arg;
} Key;

void keys_init(void);
int keys_run(XKeyEvent* e);
void keys_run_button(XButtonEvent* e);

#endif	// KEYS_H
