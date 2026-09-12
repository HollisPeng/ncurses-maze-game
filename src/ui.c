#include "ui.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <curses.h>

enum {
    CELL_SCREEN_WIDTH = 2,
    GAME_INPUT_TIMEOUT_MS = 50,
    MIN_VIEW_SIZE = 5
};

static bool ui_active = false;

static int centered_column(int terminal_columns, size_t text_length) {
    const int width = (int)text_length;
    return terminal_columns > width ? (terminal_columns - width) / 2 : 0;
}

static void draw_centered(int row, const char *text) {
    int terminal_rows;
    int terminal_columns;
    getmaxyx(stdscr, terminal_rows, terminal_columns);
    (void)terminal_rows;
    mvaddnstr(row, centered_column(terminal_columns, strlen(text)), text,
              terminal_columns);
}

static wchar_t cell_symbol(char cell) {
    switch (cell) {
        case '#': return L'＃';
        case 'o': return L'ｏ';
        case 'x': return L'ｘ';
        case '*': return L'＊';
        case 'k': return L'ｋ';
        case 'P': return L'Ｐ';
        case 'w': return L'ｗ';
        default: return L'　';
    }
}

bool ui_init(void) {
    if (setlocale(LC_ALL, "") == NULL) {
        fprintf(stderr, "warning: could not activate the user's locale\n");
    }
    if (initscr() == NULL) {
        fprintf(stderr, "ncurses initialization failed\n");
        return false;
    }

    ui_active = true;
    if (has_colors()) {
        start_color();
    }
    cbreak();
    noecho();
    intrflush(stdscr, false);
    keypad(stdscr, true);
    (void)curs_set(0);
    mousemask(BUTTON1_CLICKED | BUTTON1_PRESSED, NULL);
    mouseinterval(0);
    set_escdelay(25);
    refresh();
    return true;
}

void ui_shutdown(void) {
    if (ui_active) {
        timeout(-1);
        (void)curs_set(1);
        endwin();
        ui_active = false;
    }
}

void ui_use_blocking_input(void) {
    timeout(-1);
}

void ui_use_game_input(void) {
    timeout(GAME_INPUT_TIMEOUT_MS);
}

void ui_draw_title(const Game *game, int menu_selection) {
    static const char *const title = "NCURSES MAZE GAME";
    static const char *const help =
        "W/S or arrows: select | Enter: confirm | ESC: exit";
    static const char *const start = "Start Game";
    static const char *const exit_game = "Exit";
    char sight[32];
    int rows;
    int columns;
    getmaxyx(stdscr, rows, columns);
    erase();

    if (rows < 11 || columns < 44) {
        draw_centered(rows / 2, "Terminal too small (minimum 44x11).");
        refresh();
        return;
    }

    (void)snprintf(sight, sizeof(sight), "Limited Sight: %s",
                   game->limited_sight ? "ON" : "OFF");
    draw_centered(rows / 2 - 4, title);

    const char *const options[] = {start, sight, exit_game};
    for (int index = 0; index < 3; ++index) {
        if (index == menu_selection) {
            attron(A_REVERSE);
        }
        draw_centered(rows / 2 - 1 + index, options[index]);
        if (index == menu_selection) {
            attroff(A_REVERSE);
        }
    }
    draw_centered(rows / 2 + 4, help);
    refresh();
}

static void show_small_terminal(int rows) {
    draw_centered(rows / 2, "Terminal too small to display the maze.");
}

void ui_draw_game(const Game *game, Viewport *viewport) {
    int rows;
    int columns;
    getmaxyx(stdscr, rows, columns);
    erase();
    *viewport = (Viewport){0};
    viewport->hud_height = 2;

    const int maximum_width = columns / CELL_SCREEN_WIDTH;
    const int maximum_height = rows - viewport->hud_height;
    const int target_width = game->limited_sight ? 11 : MAZE_COLS;
    const int target_height = game->limited_sight ? 11 : MAZE_ROWS;
    const int visible_width =
        maximum_width < target_width ? maximum_width : target_width;
    const int visible_height =
        maximum_height < target_height ? maximum_height : target_height;

    if (visible_width < MIN_VIEW_SIZE || visible_height < MIN_VIEW_SIZE) {
        show_small_terminal(rows);
        refresh();
        return;
    }

    viewport->width = visible_width;
    viewport->height = visible_height;
    viewport->screen_x =
        (columns - visible_width * CELL_SCREEN_WIDTH) / 2;
    viewport->screen_y =
        (rows - visible_height - viewport->hud_height) / 2;
    viewport->maze_x = game->player.x - visible_width / 2;
    viewport->maze_y = game->player.y - visible_height / 2;

    if (viewport->maze_x < 0) viewport->maze_x = 0;
    if (viewport->maze_y < 0) viewport->maze_y = 0;
    if (viewport->maze_x > MAZE_COLS - visible_width) {
        viewport->maze_x = MAZE_COLS - visible_width;
    }
    if (viewport->maze_y > MAZE_ROWS - visible_height) {
        viewport->maze_y = MAZE_ROWS - visible_height;
    }

    char status[80];
    (void)snprintf(status, sizeof(status), "Key: %s | Powerups: %u",
                   game->has_key ? "YES" : "NO", game->powerups);
    mvaddnstr(viewport->screen_y, viewport->screen_x, status,
              visible_width * CELL_SCREEN_WIDTH);
    mvaddnstr(viewport->screen_y + 1, viewport->screen_x,
              "WASD: move | ESC: title | Left click: break wall",
              visible_width * CELL_SCREEN_WIDTH);

    for (int view_y = 0; view_y < visible_height; ++view_y) {
        for (int view_x = 0; view_x < visible_width; ++view_x) {
            const int maze_x = viewport->maze_x + view_x;
            const int maze_y = viewport->maze_y + view_y;
            const wchar_t symbol[] = {
                cell_symbol(game_cell_at(game, maze_x, maze_y)), L'\0'};
            mvaddwstr(viewport->screen_y + viewport->hud_height + view_y,
                      viewport->screen_x + view_x * CELL_SCREEN_WIDTH,
                      symbol);
        }
    }
    refresh();
}

void ui_draw_end_screen(bool won) {
    int rows;
    int columns;
    getmaxyx(stdscr, rows, columns);
    erase();
    if (rows < 7 || columns < 44) {
        show_small_terminal(rows);
    } else {
        draw_centered(rows / 2 - 1,
                      won ? "Congratulations! You win!" : "Game over.");
        draw_centered(rows / 2 + 1,
                      "Press ESC to return to the title screen.");
    }
    refresh();
}

bool ui_mouse_to_maze(const Viewport *viewport, int screen_x, int screen_y,
                      int *maze_x, int *maze_y) {
    if (viewport->width <= 0 || viewport->height <= 0) {
        return false;
    }
    const int relative_x = screen_x - viewport->screen_x;
    const int relative_y =
        screen_y - viewport->screen_y - viewport->hud_height;
    if (relative_x < 0 ||
        relative_x >= viewport->width * CELL_SCREEN_WIDTH ||
        relative_y < 0 || relative_y >= viewport->height) {
        return false;
    }
    *maze_x = viewport->maze_x + relative_x / CELL_SCREEN_WIDTH;
    *maze_y = viewport->maze_y + relative_y;
    return true;
}
