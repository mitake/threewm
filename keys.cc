#include "keys.h"
#include "events.h"
#include "threewm.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <vector>
using namespace std;

#define SHIFT (1 << 24)
#define CTRL (1 << 25)
#define ALT (1 << 26)

class key_entry {
public:
  int code;

  key_entry(int key_code, void (*key_ev)(const string&), const string key_arg):
    code {key_code}, event_fn {key_ev}, arg {key_arg} {};

  void exe();

private:
  void (*event_fn)(const string&);
  const string arg;
};

#define KEY(c, e, a) key_entry(c, e, a),
static vector<class key_entry> keys = {
#include "config_key.def"
};
#undef KEY

void key_entry::exe()
{
  event_fn(arg);
}

static int keys_run_impl(int code)
{
  for (auto &k: keys) {
    if (k.code == code) {
      k.exe();
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
