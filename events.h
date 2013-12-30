// Local Variables:
// mode: c++
// End:

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

#include <ios>

void ev_move_window_x(const std::string& args);
void ev_move_window_y(const std::string& args);
void ev_move_window_upleft(const std::string& args);
void ev_move_window_upright(const std::string& args);
void ev_move_window_downleft(const std::string& args);
void ev_move_window_downright(const std::string& args);
void ev_delete_window(const std::string& args);
void ev_info_window(const std::string& args);
void ev_maximise_window(const std::string& args);
void ev_maximise_window_vert(const std::string& args);
void ev_fix_window(const std::string& args);
void ev_exec_command(const std::string& args);
void ev_next_focus(const std::string& args);
void ev_switch_vdesk(const std::string& args);
void ev_next_vdesk(const std::string& args);
void ev_prev_vdesk(const std::string& args);
void ev_fix_next_vdesk(const std::string& args);
void ev_fix_prev_vdesk(const std::string& args);
void ev_up_focus(const std::string& args);
void ev_down_focus(const std::string& args);
void ev_right_focus(const std::string& args);
void ev_left_focus(const std::string& args);
void ev_upleft_focus(const std::string& args);
void ev_upright_focus(const std::string& args);
void ev_downleft_focus(const std::string& args);
void ev_downright_focus(const std::string& args);
void ev_wm_quit(const std::string& args);
void ev_mark_client(const std::string& args);
void ev_goto_mark(const std::string& args);
void ev_warp_pointer_x(const std::string& args);
void ev_warp_pointer_y(const std::string& args);
void ev_set_mode(const std::string& args);
void ev_raise_window(const std::string& args);
void ev_lower_window(const std::string& args);
void ev_mouse_move(const std::string& args);
void ev_mouse_resize(const std::string& args);
void ev_resize_window_x(const std::string& args);
void ev_resize_window_y(const std::string& args);
void ev_do_nothing(const std::string& args);

void warp_pointer(Window w, int x, int y);

#include "client.h"
void set_focus(Client* c);

extern int driveEnterNotify;

#endif // EVENTS_H_
