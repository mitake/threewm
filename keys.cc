#include "keys.h"
#include "events.h"
#include "threewm.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define SHIFT (1 << 24)
#define CTRL (1 << 25)
#define ALT (1 << 26)

#define KEY(K, E, A) K,
static int keys_key[] = {
#include "config_key.def"
};
#undef KEY

#define KEY(K, E, A) E,
static void (*keys_ev[]) (EvArgs) = {
#include "config_key.def"
};
#undef KEY

#define KEY(K, E, A) A,
static EvArgs keys_args[] = {
#include "config_key.def"
};
#undef KEY

static Key* keys;
static int key_len;

static int keys_run_impl(int k)
{
  for (int i = 0; i < key_len; i++) {
    if (keys[i].key == -1)
      continue;

    if (keys[i].key == k) {
      (*keys[i].ev)(keys[i].arg);
      return 0;
    }
  }

  return -1;
}

int keys_run(XKeyEvent* e)
{
  int k = XKeycodeToKeysym(dpy, e->keycode, 0);

  if (e->state & ShiftMask)
    k |= SHIFT;
  if (!(KEY_EVENT_MODIFIERS & ControlMask) && e->state & ControlMask)
    k |= CTRL;
  if (!(KEY_EVENT_MODIFIERS & Mod1Mask) && e->state & Mod1Mask)
    k |= ALT;

  return keys_run_impl(k);
}

void keys_run_button(XButtonEvent* be) {
  int k = 0;

  if (be->button == Button1)
    k = XK_Pointer_Button1;
  else if (be->button == Button2)
    k = XK_Pointer_Button2;
  else if (be->button == Button3)
    k = XK_Pointer_Button3;
  else if (be->button == Button4)
    k = XK_Pointer_Button4;
  else if (be->button == Button5)
    k = XK_Pointer_Button5;

  if (be->state & ShiftMask)
    k |= SHIFT;
  if (be->state & ControlMask)
    k |= CTRL;
  if (be->state & Mod1Mask)
    k |= ALT;

  keys_run_impl(k);
}

void keys_init(void)
{
  key_len = sizeof(keys_key) / sizeof(keys_key[0]);
  keys = (Key*)calloc(sizeof(Key), key_len);

  for (int i = 0; i < key_len; i++) {
    keys[i].key = keys_key[i];
    keys[i].ev = keys_ev[i];
    keys[i].arg =
      (char*)calloc(sizeof(char), strlen(keys_args[i]) + 1);
    strcpy(keys[i].arg, keys_args[i]);
  }
}
