#ifndef MAZE_GAME_UI_H
#define MAZE_GAME_UI_H

#include <ncurses.h>

extern WINDOW *win;

extern int view_x0;
extern int view_y0;
extern int view_start_x;
extern int view_start_y;
extern int view_w;
extern int view_h;
extern int view_hud;

void setup_ui();
void title_screen();
void print_maze();
void game_end_screen(int win);

#endif //MAZE_GAME_UI_H
