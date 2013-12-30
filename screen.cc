#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "threewm.h"
#include "events.h"

static void draw_outline(Client *c)
{
  char buf[50];
  int width_inc = 1, height_inc = 1;

  XDrawRectangle(dpy, root, invert_gc,
		 c->x - c->border, c->y - c->border,
		 c->width + c->border, c->height + c->border);

  if (c->size->flags & PResizeInc) {
    width_inc = c->size->width_inc;
    height_inc = c->size->height_inc;
  }

  snprintf(buf, sizeof(buf), "%s: %dx%d+%d+%d",
	   c->name, c->width/width_inc, c->height/height_inc, c->x, c->y);
  XDrawString(dpy, root, invert_gc,
	      c->x + c->width - XTextWidth(font, buf, strlen(buf)) - SPACE,
	      c->y + c->height - SPACE,
	      buf, strlen(buf));
}

void get_mouse_position(int *x, int *y)
{
  Window dw1, dw2;
  int t1, t2;
  unsigned int t3;

  XQueryPointer(dpy, root, &dw1, &dw2, x, y, &t1, &t2, &t3);
}

static void recalculate_sweep(Client *c, int x1, int y1, int x2, int y2)
{
  int basex, basey;

  c->width = abs(x1 - x2);
  c->height = abs(y1 - y2);

  if (c->size->flags & PResizeInc) {
    basex = (c->size->flags & PBaseSize) ? c->size->base_width :
      (c->size->flags & PMinSize) ? c->size->min_width : 0;
    basey = (c->size->flags & PBaseSize) ? c->size->base_height :
      (c->size->flags & PMinSize) ? c->size->min_height : 0;
    c->width -= (c->width - basex) % c->size->width_inc;
    c->height -= (c->height - basey) % c->size->height_inc;
  }

  if (c->size->flags & PMinSize) {
    if (c->width < c->size->min_width)
      c->width = c->size->min_width;
    if (c->height < c->size->min_height)
      c->height = c->size->min_height;
  }

  if (c->size->flags & PMaxSize) {
    if (c->width > c->size->max_width)
      c->width = c->size->max_width;
    if (c->height > c->size->max_height)
      c->height = c->size->max_height;
  }

  c->x = (x1 <= x2) ? x1 : x1 - c->width;
  c->y = (y1 <= y2) ? y1 : y1 - c->height;
}

#define GRAB(w, mask, curs)						\
  (XGrabPointer(dpy, w, False, mask, GrabModeAsync, GrabModeAsync,	\
		None, curs, CurrentTime) == GrabSuccess)

static void sweep(Client *c)
{
  XEvent ev;
  int old_cx = c->x;
  int old_cy = c->y;

  if (!GRAB(root, MouseMask, resize_curs))
    return;

  XGrabServer(dpy);
  draw_outline(c);
  warp_pointer(c->window, c->width, c->height);

  for ( ; ; ) {
    XMaskEvent(dpy, MouseMask, &ev);
    switch (ev.type) {
    case MotionNotify:
      draw_outline(c); /* clear */
      recalculate_sweep(c, old_cx, old_cy, ev.xmotion.x, ev.xmotion.y);
      draw_outline(c);
      break;
    case ButtonRelease:
      draw_outline(c); /* clear */
      XUngrabServer(dpy);
      XUngrabPointer(dpy, CurrentTime);
      return;
    }
  }
}

void show_info(Client *c)
{
  XEvent ev;

  XGrabServer(dpy);
  draw_outline(c);
  XMaskEvent(dpy, KeyReleaseMask, &ev);
  draw_outline(c);
  XUngrabServer(dpy);
}

static void drag(Client *c)
{
  XEvent ev;
  int x1, y1;
  int old_cx = c->x;
  int old_cy = c->y;

  if (!GRAB(root, MouseMask, move_curs))
    return;

  get_mouse_position(&x1, &y1);

  for (;;) {
    XMaskEvent(dpy, MouseMask, &ev);
    switch (ev.type) {
    case MotionNotify:
      c->x = old_cx + (ev.xmotion.x - x1);
      c->y = old_cy + (ev.xmotion.y - y1);
      XMoveWindow(dpy, c->parent, c->x - c->border,
		  c->y - c->border);
      send_config(c);
      break;
    case ButtonRelease:
      XUngrabPointer(dpy, CurrentTime);
      return;
    default:
      break;
    }
  }

  XUngrabServer(dpy);
}

void move(Client *c, int set)
{
  if (!c)
    return;

  if (!set)
    drag(c);

  XMoveWindow(dpy, c->parent, c->x - c->border, c->y - c->border);
  send_config(c);
  XRaiseWindow(dpy, c->parent);
}

void resize(Client *c, int set)
{
  if (!c)
    return;

  if (!set)
    sweep(c);

  XMoveResizeWindow(dpy, c->parent, c->x - c->border,
		    c->y - c->border, c->width + (c->border*2),
		    c->height + (c->border*2));
  XMoveResizeWindow(dpy, c->window, c->border, c->border,
		    c->width, c->height);
  send_config(c);
  XRaiseWindow(dpy, c->parent);
  driveEnterNotify = 0;
}

void maximise_horiz(Client *c)
{
  if (c->oldw) {
    c->x = c->oldx;
    c->width = c->oldw;
    c->oldw = 0;
  } else {
    c->oldx = c->x;
    c->oldw = c->width;
    recalculate_sweep(c, 0, c->y, DisplayWidth(dpy, screen),
		      c->y + c->height);
  }
}

void maximise_vert(Client *c)
{
  if (c->oldh) {
    c->y = c->oldy;
    c->height = c->oldh;
    c->oldh = 0;
  } else {
    c->oldy = c->y;
    c->oldh = c->height;
    recalculate_sweep(c, c->x, 0, c->x + c->width,
		      DisplayHeight(dpy, screen));
  }
}

void hide(Client *c)
{
  if (!c)
    return;

  XUnmapWindow(dpy, c->parent);
  XUnmapWindow(dpy, c->window);
  set_wm_state(c, IconicState);
}

void unhide(Client *c, int raise)
{
  XMapWindow(dpy, c->window);
  raise ? XMapRaised(dpy, c->parent) : XMapWindow(dpy, c->parent);
  set_wm_state(c, NormalState);
}

static void focus(Client* c)
{
  if (c && (c->vdesk == vdesk || c->vdesk == -1)) {
    unhide(c, RAISE);
    /* adhoc, enforcing focus out event */
    warp_pointer(c->window, c->width + c->border - 1,
		 c->height + c->border - 1);
    XSetInputFocus(dpy, c->window, RevertToParent, CurrentTime);
  }
}

static int calc_clients_diff(int srcX, int srcY, Client* dst, int x, int y)
{
  int dstX, dstY;
  dstX = dst->x + dst->width / 2;
  dstY = dst->y + dst->height / 2;

  if (x == 0) {
    int p = (dstY - srcY) * y;
    if (p > 0)
      return 3000 - p - abs(dstX - srcX) * 3;
    else
      return p - abs(dstX - srcX) * 3;
  } else if (y == 0) {
    int p = (dstX - srcX) * x;
    if (p > 0)
      return 3000 - p - abs(dstY - srcY) * 3;
    else
      return p * x - abs(dstY - srcY) * 3;
  } else {
    int px = (dstX - srcX) * x;
    int py = (dstY - srcY) * y;
    int ret = 0;

    if (px > 0)
      ret += 3000 - px;
    else
      ret += px;

    if (py > 0)
      ret += 3000 - py;
    else
      ret += py;

    ret -= abs(px - py) * 3;
    return ret;
  }
}

void arrow_next(Client* c, int x, int y)
{
  Client *newc = c;
  Client* max = 0;
  int maxDiff = 0;
  int srcX, srcY;

  if (!newc) {
    c = head_client;
    while (c && c->vdesk != vdesk && c->vdesk == -1) {
      c = c->next;
    }
    if (c == 0)
      return;

    newc = c = head_client;
    get_mouse_position(&srcX, &srcY);

    if ((newc->vdesk == vdesk || newc->vdesk == -1)) {
      int diff = calc_clients_diff(srcX, srcY, newc, x, y);
      if (max == 0 || maxDiff < diff) {
	maxDiff = diff;
	max = newc;
      }
    }
  } else {
    srcX = c->x + c->width / 2;
    srcY = c->y + c->height / 2;
  }

  do {
    if (newc->next) {
      newc = newc->next;
    } else {
      newc = head_client;
    }

    if ((newc->vdesk == vdesk || newc->vdesk == -1))
      {
	int diff = calc_clients_diff(srcX, srcY, newc, x, y);
	if (max == 0 || maxDiff < diff) {
	  maxDiff = diff;
	  max = newc;
	}
      }
  } while (newc != c);

  focus(max);
}

void next(Client *c)
{
  Client *newc = c;

  if (!newc) {
    Client* c = head_client;
    while (c && c->vdesk != vdesk && c->vdesk == -1) {
      c = c->next;
    }
    newc = c;
    if (newc == 0)
      return;
  } else {
    do {
      if (newc->next) {
	newc = newc->next;
      } else {
	newc = head_client;
      }
    } while ((newc->vdesk != vdesk && newc->vdesk != -1) && newc != c);
  }

  focus(newc);
}

static int inside_level(Client *c, int x, int y)
{
  int mx = c->x + c->width + c->border;
  int my = c->y + c->height + c->border;

  if (c->x <= x && mx >= x && c->y <= y && my >= y)
    return (x-mx) * (x-mx) + (y-my) * (y-my);
  else
    return INT_MAX;
}

void switch_vdesk(int v)
{
  Client *c;
  int x, y;
  int max_inside_level = -1;
  Client *max_inside = NULL;

  if (v == vdesk)
    return;

  if (current != NULL) {
    XSetWindowBackground(dpy, current->parent, bg.pixel);
    XClearWindow(dpy, current->parent);
  }

  get_mouse_position(&x, &y);
  current = NULL;

  for (c = head_client; c; c = c->next) {
    if (c->vdesk == vdesk)
      hide(c);
    else if (c->vdesk == v)
      unhide(c, NO_RAISE);

    if (c->vdesk == v || c->vdesk == -1) {
      int level = inside_level(c, x, y);
      if (max_inside == NULL || max_inside_level > level) {
	max_inside = c;
	max_inside_level = level;
      }
    }
  }

  vdesk = v;

  if (prev_focused[vdesk])
    c = prev_focused[vdesk];
  else if (max_inside)
    c = max_inside;

  if (!c)
    return;

  XRaiseWindow(dpy, c->parent);
  set_focus(c);
  warp_pointer(c->window, c->width + c->border - 1,
	       c->height + c->border - 1);
  XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);

  driveEnterNotify = 0;
  throwUnmaps = 1;
}

void change_vdesk(Client *c, int vd)
{
  if (vd == -1) {
    if (c->vdesk != -1 && c->vdesk != vdesk) {
      unhide(c, NO_RAISE);
    }
    XSetWindowBackground(dpy, c->parent, fg.pixel);
    XClearWindow(dpy, c->parent);
    c->vdesk = -1;
  } else {
    c->vdesk = vd;
    if (vdesk == c->vdesk)
      unhide(c, NO_RAISE);
    else
      hide(c);
  }
}

void force_set_focus(void)
{
  Client *c;

  if (current)
    return;

  for (c = head_client; c; c = c->next) {
    if (c->vdesk == vdesk) {
      set_focus(c);
      return;
    }
  }
}
