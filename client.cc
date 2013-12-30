#include "threewm.h"
#include "client.h"
#include "screen.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int send_xmessage(Window w, Atom a, long x) {
  XEvent ev;

  ev.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = a;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = x;
  ev.xclient.data.l[1] = CurrentTime;

  return XSendEvent(dpy, w, False, NoEventMask, &ev);
}

/*
 * used all over the place.  return the client that has specified window as
 * either window or parent
 */
Client *find_client(Window w)
{
  Client *c;

  for (c = head_client; c; c = c->next)
    if (w == c->parent || w == c->window) return c;

  return NULL;
}

void set_wm_state(Client *c, int state)
{
  long data[2];

  data[0] = (long) state;
  data[1] = None; /* icon window */

  XChangeProperty(dpy, c->window, xa_wm_state, xa_wm_state,
		  32, PropModeReplace, (unsigned char *)data, 2);
}

void send_config(Client *c)
{
  XConfigureEvent ce;

  ce.type = ConfigureNotify;
  ce.event = c->window;
  ce.window = c->window;
  ce.x = c->x;
  ce.y = c->y;
  ce.width = c->width;
  ce.height = c->height;
  ce.border_width = 0;
  ce.above = None;
  ce.override_redirect = 0;

  XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent *)&ce);
}

static char null_str[] = "";

void remove_client(Client *c, int from_cleanup)
{
  Client *p = NULL;

  XGrabServer(dpy);
  XSetErrorHandler(ignore_xerror);

  if (c->name && c->name != null_str)
    XFree(c->name);

  if (from_cleanup)
    unhide(c, NO_RAISE);
  else if (!from_cleanup)
    set_wm_state(c, WithdrawnState);

  ungravitate(c);
  XReparentWindow(dpy, c->window, root, c->x, c->y);
  XRemoveFromSaveSet(dpy, c->window);
  XSetWindowBorderWidth(dpy, c->window, 1);

  XDestroyWindow(dpy, c->parent);

  if (head_client == c)
    head_client = c->next;
  else {
    for (p = head_client; p && p->next; p = p->next)
      if (p->next == c) p->next = c->next;
  }

  if (c->size)
    XFree(c->size);

  /* an enter event should set this up again */
  if (c == current) {
    current = NULL;
    if (prev_focused[c->vdesk] == c)
      prev_focused[c->vdesk] = NULL;
  }

  free(c);

  XSync(dpy, False);
  XSetErrorHandler(handle_xerror);
  XUngrabServer(dpy);
}

void change_gravity(Client *c, int multiplier)
{
  int dx = 0, dy = 0;
  int gravity = (c->size->flags & PWinGravity) ?
    c->size->win_gravity : NorthWestGravity;

  switch (gravity) {
  case NorthWestGravity:
  case SouthWestGravity:
  case NorthEastGravity:
    dx = c->border;
  case NorthGravity:
    dy = c->border;
    break;
  }

  c->x += multiplier * dx;
  c->y += multiplier * dy;
}

void send_wm_delete(Client *c)
{
  int n, found = 0;
  Atom *protocols;

  if (c) {
    if (XGetWMProtocols(dpy, c->window, &protocols, &n)) {
      for (int i = 0; i < n; i++)
	if (protocols[i] == xa_wm_delete) found++;

      XFree(protocols);
    }

    if (found)
      send_xmessage(c->window, xa_wm_protos, xa_wm_delete);
    else
      XKillClient(dpy, c->window);
  }

  next(NULL);
}

static void init_position(Client *c)
{
  int x, y;
  int xmax = DisplayWidth(dpy, screen);
  int ymax = DisplayHeight(dpy, screen);

  if (c->size->flags & USSize) {
    c->width = c->size->width;
    c->height = c->size->height;
  }

  if (c->width < MINSIZE)
    c->width = MINSIZE;
  if (c->height < MINSIZE)
    c->height = MINSIZE;
  if (c->width > xmax)
    c->width = xmax;
  if (c->height > ymax)
    c->height = ymax;

  if (c->size->flags & USPosition) {
    c->x = c->size->x;
    c->y = c->size->y;

    if (c->x < 0 || c->y < 0 || c->x > xmax || c->y > ymax)
      c->x = c->y = c->border;
  } else {
    get_mouse_position(&x, &y);
    c->x = (int) ((x / (float)xmax) * (xmax - c->border - c->width));
    c->y = (int) ((y / (float)ymax) * (ymax - c->border - c->height));
  }
}

static void reparent(Client *c)
{
  XSetWindowAttributes p_attr;

  XSelectInput(dpy, c->window, EnterWindowMask | PropertyChangeMask);

  p_attr.override_redirect = True;
  p_attr.background_pixel = bg.pixel;
  p_attr.event_mask = ChildMask | ButtonPressMask | ExposureMask | EnterWindowMask;
  c->parent = XCreateWindow(dpy, root, c->x-c->border, c->y-c->border,
			    c->width+(c->border*2), c->height + (c->border*2), 0,
			    DefaultDepth(dpy, screen), CopyFromParent,
			    DefaultVisual(dpy, screen),
			    CWOverrideRedirect | CWBackPixel | CWEventMask, &p_attr);

  XAddToSaveSet(dpy, c->window);
  XSetWindowBorderWidth(dpy, c->window, 0);
  XReparentWindow(dpy, c->window, c->parent, c->border, c->border);

  send_config(c);
}

void make_new_client(Window w)
{
  Client *c, *p;
  XWindowAttributes attr;
  long dummy;
  XWMHints *hints;
  XSizeHints* shints;

  XGrabServer(dpy);
  XSetErrorHandler(ignore_xerror);
  hints = XGetWMHints(dpy, w);

  shints = XAllocSizeHints();
  XGetWMNormalHints(dpy, w, shints, &dummy);

  c = (Client *)calloc(1, sizeof(Client));
  c->next = head_client;
  head_client = c;

  c->window = w;
  c->name = null_str;

  /* try these here instead */
  c->size = shints;

  XClassHint* hint = XAllocClassHint();
  XGetClassHint(dpy, w, hint);
  if (hint->res_class) {
    c->name = hint->res_class;
    if (hint->res_name)
      XFree(hint->res_name);
  }
  XFree(hint);

  XGetWindowAttributes(dpy, c->window, &attr);

  c->x = attr.x;
  c->y = attr.y;
  c->width = attr.width;
  c->height = attr.height;
  c->border = opt_bw;
  c->oldw = c->oldh = 0;
  c->vdesk = vdesk;
  c->index = 0;

  if (!(attr.map_state == IsViewable)) {
    init_position(c);
    if (hints && hints->flags & StateHint)
      set_wm_state(c, hints->initial_state);
  }

  if (hints)
    XFree(hints);

  /* client is initialised */

  gravitate(c);
  reparent(c);

  XGrabButton(dpy, AnyButton, Mod1Mask, c->parent, False,
	      ButtonMask, GrabModeSync, GrabModeAsync, None, None);
  XGrabButton(dpy, AnyButton, 0, c->parent, False,
	      ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  XMapWindow(dpy, c->window);
  XMapRaised(dpy, c->parent);
  set_wm_state(c, NormalState);

  XSync(dpy, False);
  XSetErrorHandler(handle_xerror);
  XUngrabServer(dpy);

  throwUnmaps = 0;

 again:
  for (p = c->next; p; p = p->next) {
    if (c->index == p->index && !strcmp(c->name, p->name)) {
      c->index++;
      goto again;
    }
  }
}
