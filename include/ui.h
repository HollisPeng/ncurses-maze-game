#ifndef NCURSES_MAZE_GAME_UI_H
#define NCURSES_MAZE_GAME_UI_H

#include <stdbool.h>

#include "game.h"

typedef struct {
    int screen_x;
    int screen_y;
    int maze_x;
    int maze_y;
    int width;
    int height;
    int hud_height;
} Viewport;

bool ui_init(void);
void ui_shutdown(void);
void ui_use_blocking_input(void);
void ui_use_game_input(void);

void ui_draw_title(const Game *game, int menu_selection);
void ui_draw_game(const Game *game, Viewport *viewport);
void ui_draw_end_screen(bool won);

bool ui_mouse_to_maze(const Viewport *viewport, int screen_x, int screen_y,
                      int *maze_x, int *maze_y);

#endif
