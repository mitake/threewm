#ifndef EVENTS_H_
#define EVENTS_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

void handle_key_event(XKeyEvent *e);
void handle_button_event(XEvent *e);
void handle_client_message(XClientMessageEvent *e);
void handle_configure_request(XConfigureRequestEvent *e);
void handle_enter_event(XCrossingEvent *e);
void handle_focus_change_event(XFocusChangeEvent *e);
void handle_map_request(XMapRequestEvent *e);
void handle_property_change(XPropertyEvent *e);
void handle_unmap_event(XUnmapEvent *e);
void handle_destroy_event(XDestroyWindowEvent *e);

typedef char* EvArgs;

void ev_move_window_x(EvArgs args);
void ev_move_window_y(EvArgs args);
void ev_move_window_upleft(EvArgs args);
void ev_move_window_upright(EvArgs args);
void ev_move_window_downleft(EvArgs args);
void ev_move_window_downright(EvArgs args);
void ev_delete_window(EvArgs args);
void ev_info_window(EvArgs args);
void ev_maximise_window(EvArgs args);
void ev_maximise_window_vert(EvArgs args);
void ev_fix_window(EvArgs args);
void ev_exec_command(EvArgs args);
void ev_next_focus(EvArgs args);
void ev_switch_vdesk(EvArgs args);
void ev_next_vdesk(EvArgs args);
void ev_prev_vdesk(EvArgs args);
void ev_fix_next_vdesk(EvArgs args);
void ev_fix_prev_vdesk(EvArgs args);
void ev_up_focus(EvArgs args);
void ev_down_focus(EvArgs args);
void ev_right_focus(EvArgs args);
void ev_left_focus(EvArgs args);
void ev_upleft_focus(EvArgs args);
void ev_upright_focus(EvArgs args);
void ev_downleft_focus(EvArgs args);
void ev_downright_focus(EvArgs args);
void ev_wm_quit(EvArgs args);
void ev_mark_client(EvArgs args);
void ev_goto_mark(EvArgs args);
void ev_warp_pointer_x(EvArgs args);
void ev_warp_pointer_y(EvArgs args);
void ev_set_mode(EvArgs args);
void ev_raise_window(EvArgs args);
void ev_lower_window(EvArgs args);
void ev_mouse_move(EvArgs args);
void ev_mouse_resize(EvArgs args);
void ev_resize_window_x(EvArgs args);
void ev_resize_window_y(EvArgs args);
void ev_do_nothing(EvArgs args);

void warp_pointer(Window w, int x, int y);

#include "client.h"
void set_focus(Client* c);

extern int driveEnterNotify;

#endif // EVENTS_H_
