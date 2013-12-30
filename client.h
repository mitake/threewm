// Local Variables:
// mode: c++
// End:

#ifndef CLIENT_H_
#define CLIENT_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* client structure */

typedef struct Client Client;

struct Client {
  Client  *next;
  Window  window;
  Window  parent;

  XSizeHints  *size;

  int     x, y, width, height;
  int     oldx, oldy, oldw, oldh;  /* used when maximising */
  int     border;
  int     vdesk;
  char*   name;
  int     index;
  char    mark;
};

Client* find_client(Window w);
void change_gravity(Client *c, int multiplier);
void remove_client(Client *c, int from_cleanup);
void send_config(Client *c);
void send_wm_delete(Client *c);
void set_wm_state(Client *c, int state);

void make_new_client(Window w);

#endif /* CLIENT_H_ */
