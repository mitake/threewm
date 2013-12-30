#include "threewm.h"
#include "events.h"
#include "client.h"
#include "screen.h"
#include "keys.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <ios>
using namespace std;

static Client* cl;

int driveEnterNotify = 1;

void handle_key_event(XKeyEvent *e)
{
  cl = current;

  if (keys_run(e)) {
    /* Resend events which we didn't handle. */
    if (cl) {
      e->time = CurrentTime;
      e->window = cl->window;
      XSendEvent(dpy, cl->window, True, NoEventMask, (XEvent *)e);
      XSync(dpy, False);
    }
  }
}

void handle_button_event(XEvent *ev)
{
  XButtonEvent* e = &ev->xbutton;
  /* ad-hoc addition for click and focus model */
  Client *target = find_client(e->window);

  if (target)
    set_focus(target);

  cl = current;
  keys_run_button(e);

  XSync(dpy, False);
  XAllowEvents(dpy, ReplayPointer, CurrentTime);
  XSync(dpy, False);
}

void handle_configure_request(XConfigureRequestEvent *e)
{
  Client *c = find_client(e->window);
  XWindowChanges wc;

  wc.sibling = e->above;
  wc.stack_mode = e->detail;
  if (c) {
    ungravitate(c);
    if (e->value_mask & CWX) c->x = e->x;
    if (e->value_mask & CWY) c->y = e->y;
    if (e->value_mask & CWWidth) c->width = e->width;
    if (e->value_mask & CWHeight) c->height = e->height;
    gravitate(c);

    wc.x = c->x - c->border;
    wc.y = c->y - c->border;
    wc.width = c->width + (c->border*2);
    wc.height = c->height + (c->border*2);
    wc.border_width = 0;
    XConfigureWindow(dpy, c->parent, e->value_mask, &wc);
    send_config(c);
  }

  wc.x = c ? c->border : e->x;
  wc.y = c ? c->border : e->y;
  wc.width = e->width;
  wc.height = e->height;
  XConfigureWindow(dpy, e->window, e->value_mask, &wc);
}

void handle_map_request(XMapRequestEvent *e)
{
  Client *c = find_client(e->window);

  driveEnterNotify = 1;

  if (c) {
    if (c->vdesk == vdesk) unhide(c, RAISE);
  } else
    make_new_client(e->window);
}

void handle_unmap_event(XUnmapEvent* e)
{
  Client* c = find_client(e->window);

  driveEnterNotify = 1;

  if (c) hide(c);
}

void handle_destroy_event(XDestroyWindowEvent *e)
{
  Client *c = find_client(e->window);

  if (c)
    remove_client(c, NOT_QUITTING);
}

void handle_client_message(XClientMessageEvent *e)
{
  Client *c = find_client(e->window);

  if (c && e->message_type == xa_wm_change_state &&
      e->format == 32 && e->data.l[0] == IconicState)
    hide(c);
}

void handle_property_change(XPropertyEvent *e)
{
  Client *c = find_client(e->window);
  long dummy;

  if (!c)
    return;

  if (e->atom == XA_WM_NORMAL_HINTS)
    XGetWMNormalHints(dpy, c->window, c->size, &dummy);
}

void set_focus(Client* c)
{
  if (c->vdesk != vdesk && c->vdesk != -1)
    return;

  XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);

  if (current && c != current) {
    XSetWindowBackground(dpy, current->parent, bg.pixel);
    XClearWindow(dpy, current->parent);
  }

  XSetWindowBackground(dpy, c->parent, (c->vdesk==-1)?fc.pixel:fg.pixel);
  XClearWindow(dpy, c->parent);
  current = c;

  if (0 < c->vdesk)
    prev_focused[c->vdesk] = c;
}

void handle_enter_event(XCrossingEvent *e)
{
  Client *c = find_client(e->window);

  if (c && current != c) {
    if (e->type == EnterNotify && driveEnterNotify) {
      set_focus(c);
      driveEnterNotify = 0;
    }
  }
}

void handle_focus_change_event(XFocusChangeEvent *e)
{
  Client *c = find_client(e->window);

  if (c) set_focus(c);
}

static void do_move(void)
{
  move(cl, 1);
  warp_pointer(cl->window, cl->width + cl->border - 1,
	       cl->height + cl->border - 1);
  driveEnterNotify = 0;
}

void ev_move_window_x(const string& args)
{
  if (cl != 0) {
    cl->x += atoi(args.c_str());
    do_move();
  }
}

void ev_move_window_y(const string& args)
{
  if (cl != 0) {
    cl->y += atoi(args.c_str());
    do_move();
  }
}

static void do_resize(void)
{
  resize(cl, 1);
  warp_pointer(cl->window, cl->width + cl->border - 1,
	       cl->height + cl->border - 1);
  driveEnterNotify = 0;
}

void ev_resize_window_x(const string& args)
{
  if (cl != 0) {
    if((cl->width + atoi(args.c_str())) < 0)
      return;

    cl->width += atoi(args.c_str());
    do_resize();
  }
}

void ev_resize_window_y(const string& args)
{
  if (cl != 0) {
    if((cl->height + atoi(args.c_str())) < 0)
      return;

    cl->height += atoi(args.c_str());
    do_resize();
  }
}

void ev_move_window_upleft(const string& args)
{
  if (cl != 0) {
    cl->x = cl->border;
    cl->y = cl->border;
    do_move();
  }
}

void ev_move_window_upright(const string& args)
{
  if (cl != 0) {
    cl->x = DisplayWidth(dpy, screen) - cl->width - cl->border;
    cl->y = cl->border;
    do_move();
  }
}

void ev_move_window_downleft(const string& args)
{
  if (cl != 0) {
    cl->x = cl->border;
    cl->y = DisplayHeight(dpy, screen)
      - cl->height - cl->border;
    do_move();
  }
}

void ev_move_window_downright(const string& args)
{
  if (cl != 0) {
    cl->x = DisplayWidth(dpy, screen) - cl->width - cl->border;
    cl->y = DisplayHeight(dpy, screen)
      - cl->height - cl->border;
    do_move();
  }
}

void ev_delete_window(const string& args)
{
  if (cl != 0)
    send_wm_delete(cl);
}

void ev_info_window(const string& args)
{
  if (cl != 0)
    show_info(cl);
}

void ev_maximise_window(const string& args)
{
  if (cl != 0) {
    maximise_horiz(cl);
    maximise_vert(cl);
    resize(cl, 1);
    warp_pointer(cl->window, cl->width + cl->border - 1,
		 cl->height + cl->border - 1);
    driveEnterNotify = 0;
  }
}

void ev_maximise_window_vert(const string& args)
{
  if (cl != 0) {
    maximise_vert(cl);
    resize(cl, 1);
    warp_pointer(cl->window, cl->width + cl->border - 1,
		 cl->height + cl->border - 1);
  }
}

void ev_fix_window(const string& args)
{
  if (cl != 0) {
    if (cl->vdesk == vdesk || cl->vdesk == -1) {
      XSetWindowBackground(dpy, cl->parent,
			   cl->vdesk == -1 ? fg.pixel : fc.pixel);
      XClearWindow(dpy, cl->parent);
      cl->vdesk = cl->vdesk == -1 ? vdesk : -1;
    }
  }
}

static void spawn(char* cmd[])
{
  pid_t pid;

  pid = fork();

  if (pid == 0) {
    execvp(cmd[0], (char *const *)&cmd[0]);
    fprintf(stderr, "%s: error exec\n", cmd[0]);
    exit(0);
  }
}

void ev_exec_command(const string& args)
{
  char *a[64];
  char *tmp = (char *)calloc(strlen(args.c_str()) + 1, sizeof(char));
  int i = 0;

  strcpy(tmp, args.c_str());
  a[i++] = strtok(tmp, " ");
  while ((a[i++] = strtok(NULL, " ")) != NULL) {}
  spawn(a);
  free(tmp);
}

void ev_next_focus(const string& args)
{
  next(cl);
}

void ev_switch_vdesk(const string& args)
{
  driveEnterNotify = 0;
  switch_vdesk(atoi(args.c_str()));
}

void ev_next_vdesk(const string& args)
{
  driveEnterNotify = 0;

  if (vdesk < opt_vd)
    switch_vdesk(vdesk + 1);
  else
    switch_vdesk(1);
}

void ev_prev_vdesk(const string& args)
{
  driveEnterNotify = 0;

  if (vdesk > 1)
    switch_vdesk(vdesk - 1);
  else
    switch_vdesk(opt_vd);
}

void ev_fix_next_vdesk(const string& args)
{
  ev_fix_window(args);
  ev_next_vdesk(args);
  ev_fix_window(args);
}

void ev_fix_prev_vdesk(const string& args)
{
  ev_fix_window(args);
  ev_prev_vdesk(args);
  ev_fix_window(args);
}

void ev_up_focus(const string& args)
{
  arrow_next(cl, 0, -1);
}

void ev_down_focus(const string& args)
{
  arrow_next(cl, 0, 1);
}

void ev_right_focus(const string& args)
{
  arrow_next(cl, 1, 0);
}

void ev_left_focus(const string& args)
{
  arrow_next(cl, -1, 0);
}

void ev_upleft_focus(const string& args)
{
  arrow_next(cl, -1, -1);
}

void ev_upright_focus(const string& args)
{
  arrow_next(cl, 1, -1);
}

void ev_downleft_focus(const string& args)
{
  arrow_next(cl, -1, 1);
}

void ev_downright_focus(const string& args)
{
  arrow_next(cl, 1, 1);
}

void ev_wm_quit(const string& args)
{
  quit_nicely();
}

void warp_pointer(Window w, int x, int y)
{
  driveEnterNotify = 1;
  XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y);
}

void ev_warp_pointer_x(const string& args)
{
  warp_pointer(None, atoi(args.c_str()), 0);
}

void ev_warp_pointer_y(const string& args)
{
  warp_pointer(None, 0, atoi(args.c_str()));
}

void ev_raise_window(const string& args)
{
  if (cl) XRaiseWindow(dpy, cl->parent);
}

void ev_lower_window(const string& args)
{
  if (cl) XLowerWindow(dpy, cl->parent);
}

void ev_mouse_move(const string& args)
{
  if (cl) move(cl, 0);
}

void ev_mouse_resize(const string& args)
{
  if (cl) resize(cl, 0);
}

void ev_do_nothing(const string& args)
{
}
