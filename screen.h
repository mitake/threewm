#ifndef SCREEN_H_
#define SCREEN_H_

#include "client.h"

void get_mouse_position(int *x, int *y);
void move(Client *c, int set);
void resize(Client *c, int set);
void maximise_vert(Client *c);
void maximise_horiz(Client *c);
void show_info(Client *c);
void unhide(Client *c, int raise);
void next(Client *c);
void arrow_next(Client* c, int x, int y);
void hide(Client *c);
void switch_vdesk(int v);
void iconize(Client* c);
void change_vdesk(Client *c, int vd);

void focus(Client* c);
void set_focus(Client* c);
void force_set_focus(void);

#endif /* SCREEN_H_ */
