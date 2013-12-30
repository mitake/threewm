#include "threewm.h"
#include "events.h"
#include "screen.h"
#include "keys.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

Display *dpy;
Client *current = NULL;
int screen;
Window root;
GC invert_gc;
XFontStruct *font;
Client *head_client;
Atom xa_wm_state;
Atom xa_wm_change_state;
Atom xa_wm_protos;
Atom xa_wm_delete;
Atom xa_wm_cmapwins;
Cursor move_curs;
Cursor resize_curs;
int opt_vd = DEF_VD;
int opt_bw = DEF_BW;
XColor fg, bg, fc;
int vdesk = 1;
char throwUnmaps;
Client **prev_focused;

static const char *opt_display = "";
static const char *opt_font = DEF_FONT;
static const char *opt_fg = DEF_FG;
static const char *opt_bg = DEF_BG;
static const char *opt_fc = DEF_FC;
static int wm_running;

static void cleanup(void)
{
  while(head_client)
    remove_client(head_client, QUITTING);

  XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
  XInstallColormap(dpy, DefaultColormap(dpy, screen));
  XCloseDisplay(dpy);
}

void quit_nicely(void)
{
  cleanup();
  exit(0);
}

static void handle_signal(int signo)
{
  if (signo == SIGCHLD) {
    wait(NULL);
    return;
  }

  quit_nicely();
}

static void throwAllUnmapEvent()
{
  XEvent ev;
  /* throw all event */
  while (XCheckTypedEvent(dpy, UnmapNotify, &ev)) {}
}

int handle_xerror(Display *d, XErrorEvent *e)
{
  Client *c = find_client(e->resourceid);

  if (e->error_code == BadAccess &&
      e->request_code == X_ChangeWindowAttributes) {
    exit(1);
  }

  fprintf(stderr, "XError %x %d ", e->error_code, e->request_code);

  if (c && e->error_code != BadWindow)
    remove_client(c, NOT_QUITTING);

  return 0;
}

int ignore_xerror(Display *d, XErrorEvent *e)
{
  return 0;
}

static void setup_display(void)
{
  XGCValues gv;
  XSetWindowAttributes attr;
  XColor dummy;

  dpy = XOpenDisplay(opt_display);
  if (!dpy) {
    fprintf(stderr, "XOpenDisplay() failed\n");
    exit(1);
  }

  XSetErrorHandler(handle_xerror);

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  xa_wm_state = XInternAtom(dpy, "WM_STATE", False);
  xa_wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  xa_wm_protos = XInternAtom(dpy, "WM_PROTOCOLS", False);
  xa_wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fg, &fg, &dummy);
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bg, &bg, &dummy);
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fc, &fc, &dummy);

  font = XLoadQueryFont(dpy, opt_font);
  if (!font)
    font = XLoadQueryFont(dpy, "*");

  move_curs = XCreateFontCursor(dpy, XC_fleur);
  resize_curs = XCreateFontCursor(dpy, XC_plus);

  gv.function = GXinvert;
  gv.subwindow_mode = IncludeInferiors;
  gv.line_width = 1;
  gv.font = font->fid;
  invert_gc = XCreateGC(dpy, root,
			GCFunction | GCSubwindowMode | GCLineWidth | GCFont, &gv);

  attr.event_mask = ChildMask | PropertyChangeMask | ButtonMask;

  XChangeWindowAttributes(dpy, root, CWEventMask, &attr);
  /* Unfortunately grabbing AnyKey under Solaris seems not to work */
  /* XGrabKey(dpy, AnyKey, ControlMask|Mod1Mask, root, True, GrabModeAsync, GrabModeAsync); */
  /* So now I grab each and every one. */
  XGrabKey(dpy, AnyKey, KEY_EVENT_MODIFIERS,
	   root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, AnyKey, KEY_EVENT_MODIFIERS | ShiftMask,
	   root, True, GrabModeAsync, GrabModeAsync);

  XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Tab), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
}

static void scan_windows(void)
{
  unsigned int nwins;
  Window dw1, dw2, *wins;
  XWindowAttributes attr;

  XQueryTree(dpy, root, &dw1, &dw2, &wins, &nwins);

  for (unsigned int i = 0; i < nwins; i++) {
    XGetWindowAttributes(dpy, wins[i], &attr);

    if (!attr.override_redirect && attr.map_state == IsViewable)
      make_new_client(wins[i]);
  }
  XFree(wins);
}

int main(int argc, char *argv[])
{
  struct sigaction act;
  XEvent ev;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-fn") && i+1<argc)
      opt_font = argv[++i];
    else if (!strcmp(argv[i], "-display") && i+1<argc)
      opt_display = argv[++i];
    else if (!strcmp(argv[i], "-fg") && i+1<argc)
      opt_fg = argv[++i];
    else if (!strcmp(argv[i], "-bg") && i+1<argc)
      opt_bg = argv[++i];
    else if (!strcmp(argv[i], "-fc") && i+1<argc)
      opt_fc = argv[++i];
    else if (!strcmp(argv[i], "-bw") && i+1<argc)
      opt_bw = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-vd") && i+1<argc) {
      opt_vd = atoi(argv[++i]);
    } else {
      printf("usage: threewm [-display display] [-vd num]\n");
      printf("\t[-fg colour] [-bg colour] [-bw borderwidth]\n");

      exit(2);
    }
  }

  prev_focused = (Client **)calloc(opt_vd, sizeof(Client *));
  if (!prev_focused) {
    fprintf(stderr, "error at allocating prev_focused\n");
    exit(1);
  }

  act.sa_handler = handle_signal;
  sigemptyset(&act.sa_mask);
#ifdef SA_NOCLDSTOP
  act.sa_flags = SA_NOCLDSTOP;  /* don't care about STOP, CONT */
#else
  act.sa_flags = 0;
#endif
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGCHLD, &act, NULL);
  signal(SIGPIPE, SIG_IGN);

  setup_display();
  scan_windows();
  keys_init();

  throwAllUnmapEvent();
  throwUnmaps = 0;

  wm_running = 1;
  /* main event loop here */
  while (wm_running) {
    XNextEvent(dpy, &ev);
    switch (ev.type) {
    case KeyPress:
      handle_key_event(&ev.xkey);
      break;
    case ButtonPress:
      handle_button_event(&ev);
      break;
    case ButtonRelease:
      XSync(dpy, False);
      XAllowEvents(dpy, ReplayPointer, CurrentTime);
      XSync(dpy, False);
      break;
    case ConfigureRequest:
      handle_configure_request(&ev.xconfigurerequest);
      break;
    case MapRequest:
      handle_map_request(&ev.xmaprequest);
      break;
    case ClientMessage:
      handle_client_message(&ev.xclient);
      break;
    case EnterNotify:
      handle_enter_event(&ev.xcrossing);
      break;
    case PropertyNotify:
      handle_property_change(&ev.xproperty);
      break;
    case UnmapNotify:
      if (!throwUnmaps)
	handle_unmap_event(&ev.xunmap);
      break;
    case DestroyNotify:
      handle_destroy_event(&ev.xdestroywindow);
      break;
    }

    force_set_focus();
  }

  printf("quitting threewm\n");
  return 1;
}

