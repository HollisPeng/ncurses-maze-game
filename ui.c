#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "maze.h"
#include "ui.h"
#include "globals.h"

WINDOW *win;

int menu_selection = 0;
int view_x0 = 0;
int view_y0 = 0;
int view_start_x = 0;
int view_start_y = 0;
int view_w = 0;
int view_h = 0;
int view_hud = 2;

void setup_ui() {
    setlocale(LC_ALL, "");
    if ((win = initscr()) == NULL) {
        printf("Initialization failed\n");
        exit(1);
    }
    start_color();
    cbreak();
    noecho();
    intrflush(win, FALSE);
    keypad(win, TRUE);
    curs_set(0);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    set_escdelay(25);
    refresh();
}

void title_screen() {
    clear();
    int row, col;
    getmaxyx(stdscr, row, col);
    const char *opt0 = "Start Game";
    char opt1[30];
    sprintf(opt1, "Limited Sight: %s", limit_sight ? "ON" : "OFF");
    const char *opt2 = "Exit";
    const char *msg = "WS to select, Enter to confirm";
    if (row < 10 || col < 40) {
        mvprintw(row / 2, (col - 19) / 2, "Terminal too small.");
    } else {
        if (menu_selection == 0) attron(A_REVERSE);
        mvprintw(row / 2 - 2, (col - (int)strlen(opt0)) / 2, "%s", opt0);
        attroff(A_REVERSE);
        if (menu_selection == 1) attron(A_REVERSE);
        mvprintw(row / 2 - 1, (col - (int)strlen(opt1)) / 2, "%s", opt1);
        attroff(A_REVERSE);
        if (menu_selection == 2) attron(A_REVERSE);
        mvprintw(row / 2, (col - (int)strlen(opt2)) / 2, "%s", opt2);
        attroff(A_REVERSE);
        mvprintw(row / 2 + 2, (col - (int)strlen(msg)) / 2, "%s", msg);
    }
    refresh();
}

static void show_small_terminal() {
    int row, col;
    getmaxyx(stdscr, row, col);
    const char *err = "Terminal too small.";
    mvprintw(row / 2, (col - (int)strlen(err)) / 2, "%s", err);
}

void print_maze() {
    clear();
    int row, col;
    getmaxyx(stdscr, row, col);
    view_w = 0;
    view_h = 0;
    int max_w = col / 2;
    int max_h = row - view_hud;
    if (max_w <= 0 || max_h <= 0) {
        show_small_terminal();
        return;
    }
    int target_w = limit_sight ? 11 : COL;
    int target_h = limit_sight ? 11 : ROW;
    int show_w = max_w < target_w ? max_w : target_w;
    int show_h = max_h < target_h ? max_h : target_h;
    if (show_h > ROW) show_h = ROW;
    if (show_w < 5 || show_h < 5) {
        show_small_terminal();
        return;
    }
    view_w = show_w;
    view_h = show_h;
    int block_w = show_w * 2;
    int block_h = show_h + view_hud;
    view_x0 = (col - block_w) / 2;
    view_y0 = (row - block_h) / 2;
    if (view_x0 < 0) view_x0 = 0;
    if (view_y0 < 0) view_y0 = 0;
    view_start_y = player.y - show_h / 2;
    view_start_x = player.x - show_w / 2;
    if (view_start_y < 0) view_start_y = 0;
    if (view_start_y > ROW - show_h) view_start_y = ROW - show_h;
    if (view_start_y < 0) view_start_y = 0;
    if (view_start_x < 0) view_start_x = 0;
    if (view_start_x > COL - show_w) view_start_x = COL - show_w;
    if (view_start_x < 0) view_start_x = 0;
    char hud0[80];
    snprintf(hud0, sizeof(hud0), "Key: %s | Powerups: %d", player_has_key ? "YES" : "NO", powerups);
    const char *hud1 = "WSAD move | ESC title | Mouse: break";
    int hud0_x = view_x0;
    if ((int)strlen(hud0) < block_w) hud0_x = view_x0 + (block_w - (int)strlen(hud0)) / 2;
    int hud1_x = view_x0;
    if ((int)strlen(hud1) < block_w) hud1_x = view_x0 + (block_w - (int)strlen(hud1)) / 2;
    mvprintw(view_y0, hud0_x, "%s", hud0);
    mvprintw(view_y0 + 1, hud1_x, "%s", hud1);
    for (int i = 0; i < show_h; i++) {
        for (int j = 0; j < show_w; j++) {
            int my = view_start_y + i;
            int mx = view_start_x + j;
            int sy = view_y0 + view_hud + i;
            int sx = view_x0 + j * 2;
            char c = maze[my][mx];
            const char *display = "　";
            if (c == '#') display = "＃";
            else if (c == 'o') display = "ｏ";
            else if (c == 'x') display = "ｘ";
            else if (c == '*') display = "＊";
            else if (c == 'k') display = "ｋ";
            else if (c == 'P') display = "Ｐ";
            else if (c == 'w') display = "ｗ";
            mvprintw(sy, sx, "%s", display);
        }
    }
}

void game_end_screen(int won) {
    clear();
    int row, col;
    getmaxyx(stdscr, row, col);
    const char *msg1 = won ? "Congratulations! You win!" : "You Died! Game Over.";
    const char *msg2 = "Press ESC to go back to the title screen.";
    if (row < 10 || col < (int)strlen(msg2) + 2) {
        mvprintw(row / 2, (col - 18) / 2, "Terminal too small.");
    } else {
        mvprintw(row / 2, (col - (int)strlen(msg1)) / 2, "%s", msg1);
        mvprintw(row / 2 + 1, (col - (int)strlen(msg2)) / 2, "%s", msg2);
    }
    refresh();
}