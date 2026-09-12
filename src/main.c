#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curses.h>

#include "game.h"
#include "ui.h"

enum {
    MENU_START = 0,
    MENU_LIMITED_SIGHT = 1,
    MENU_EXIT = 2,
    MENU_ITEM_COUNT = 3
};

static volatile sig_atomic_t stop_requested = 0;

static void request_stop(int signal_number) {
    (void)signal_number;
    stop_requested = 1;
}

static bool install_signal_handlers(void) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = request_stop;
    if (sigemptyset(&action.sa_mask) != 0) {
        return false;
    }
    return sigaction(SIGINT, &action, NULL) == 0 &&
           sigaction(SIGTERM, &action, NULL) == 0;
}

static uint64_t monotonic_milliseconds(void) {
    struct timespec time_value;
    if (clock_gettime(CLOCK_MONOTONIC, &time_value) != 0) {
        return 0U;
    }
    return (uint64_t)time_value.tv_sec * UINT64_C(1000) +
           (uint64_t)time_value.tv_nsec / UINT64_C(1000000);
}

static uint32_t make_seed(void) {
    struct timespec time_value;
    if (clock_gettime(CLOCK_REALTIME, &time_value) != 0) {
        return UINT32_C(0x6d2b79f5);
    }
    return (uint32_t)time_value.tv_sec ^ (uint32_t)time_value.tv_nsec;
}

static bool is_enter(int input) {
    return input == '\n' || input == '\r' || input == KEY_ENTER;
}

static void handle_title_input(Game *game, int input, int *selection,
                               bool *running) {
    if (input == 'w' || input == 'W' || input == KEY_UP) {
        *selection = (*selection + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT;
    } else if (input == 's' || input == 'S' || input == KEY_DOWN) {
        *selection = (*selection + 1) % MENU_ITEM_COUNT;
    } else if (input == 27) {
        *running = false;
    } else if (is_enter(input)) {
        if (*selection == MENU_START) {
            game_start(game, monotonic_milliseconds());
        } else if (*selection == MENU_LIMITED_SIGHT) {
            game_toggle_limited_sight(game);
        } else if (*selection == MENU_EXIT) {
            *running = false;
        }
    }
}

static void handle_mouse(Game *game, const Viewport *viewport) {
    MEVENT event;
    if (getmouse(&event) != OK ||
        (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED)) == 0U) {
        return;
    }
    int maze_x;
    int maze_y;
    if (ui_mouse_to_maze(viewport, event.x, event.y, &maze_x, &maze_y)) {
        (void)game_break_wall(game, maze_x, maze_y);
    }
}

static void handle_game_input(Game *game, const Viewport *viewport, int input) {
    switch (input) {
        case 27: game_return_to_title(game); break;
        case 'w':
        case 'W': (void)game_move_player(game, DIRECTION_UP); break;
        case 's':
        case 'S': (void)game_move_player(game, DIRECTION_DOWN); break;
        case 'a':
        case 'A': (void)game_move_player(game, DIRECTION_LEFT); break;
        case 'd':
        case 'D': (void)game_move_player(game, DIRECTION_RIGHT); break;
        case KEY_MOUSE: handle_mouse(game, viewport); break;
        default: break;
    }
}

int main(void) {
    Game game;
    Viewport viewport = {0};
    int menu_selection = MENU_START;
    bool running = true;

    if (!install_signal_handlers()) {
        return EXIT_FAILURE;
    }
    game_init(&game, make_seed());
    if (!ui_init()) {
        return EXIT_FAILURE;
    }
    if (atexit(ui_shutdown) != 0) {
        ui_shutdown();
        return EXIT_FAILURE;
    }

    while (running && stop_requested == 0) {
        if (game.status == GAME_TITLE) {
            ui_use_blocking_input();
            ui_draw_title(&game, menu_selection);
            handle_title_input(&game, getch(), &menu_selection, &running);
        } else if (game.status == GAME_PLAYING) {
            ui_use_game_input();
            ui_draw_game(&game, &viewport);
            handle_game_input(&game, &viewport, getch());
            game_update_spikes(&game, monotonic_milliseconds());
        } else {
            ui_use_blocking_input();
            ui_draw_end_screen(game.status == GAME_WON);
            if (getch() == 27) {
                game_return_to_title(&game);
            }
        }
    }

    return EXIT_SUCCESS;
}
