#ifndef THREEWM_H
#define THREEWM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "client.h"

/* default settings */
#define DEF_FONT	"lucidasans-10"
#define DEF_FG		"goldenrod"
#define DEF_BG		"grey50"
#define DEF_BW		1
#define DEF_FC		"blue"
#define SPACE		3
#define MINSIZE		15
#define DEF_VD		4

/* readability stuff */
#define NOT_QUITTING	0
#define QUITTING	1	/* for remove_client */
#define RAISE		1
#define NO_RAISE	0	/* for unhide() */

/* some coding shorthand */
#define ChildMask	(SubstructureRedirectMask | SubstructureNotifyMask)
#define ButtonMask	(ButtonPressMask | ButtonReleaseMask)
#define MouseMask	(ButtonMask | PointerMotionMask)

#define gravitate(c)				\
  change_gravity(c, 1)

#define ungravitate(c)				\
  change_gravity(c, -1)

extern Display *dpy;
extern Client *current;
extern int screen;
extern Window root;
extern GC invert_gc;
extern XFontStruct *font;
extern Client *head_client;
extern Atom xa_wm_state;
extern Atom xa_wm_change_state;
extern Atom xa_wm_protos;
extern Atom xa_wm_delete;
extern Cursor move_curs;
extern Cursor resize_curs;
extern int opt_vd;
extern int opt_bw;
extern XColor fg, bg, fc;

extern int vdesk;
extern char throwUnmaps;
extern Client **prev_focused;

int handle_xerror(Display *dpy, XErrorEvent *e);
int ignore_xerror(Display *dpy, XErrorEvent *e);

#define LOG(msg, ...) do {			\
    fprintf(stderr, msg,  ## __VA_ARGS__);	\
  } while (0)

void quit_nicely(void);

#define KEY_EVENT_MODIFIERS (ControlMask | Mod1Mask)

#endif // THREEWM_H
