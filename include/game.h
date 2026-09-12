#ifndef NCURSES_MAZE_GAME_GAME_H
#define NCURSES_MAZE_GAME_GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAZE_ROWS 30
#define MAZE_COLS 40
#define MAX_SPIKES 50

typedef enum {
    GAME_TITLE,
    GAME_PLAYING,
    GAME_LOST,
    GAME_WON
} GameStatus;

typedef enum {
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_UP,
    DIRECTION_DOWN
} Direction;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position position;
    bool active;
    uint32_t active_ms;
    uint32_t inactive_ms;
    uint64_t next_toggle_ms;
} Spike;

typedef struct {
    char terrain[MAZE_ROWS][MAZE_COLS];
    char items[MAZE_ROWS][MAZE_COLS];
    Position player;
    Position exit;
    Spike spikes[MAX_SPIKES];
    size_t spike_count;
    unsigned int move_count;
    unsigned int powerups;
    uint32_t random_state;
    GameStatus status;
    bool limited_sight;
    bool has_key;
} Game;

void game_init(Game *game, uint32_t seed);
void game_start(Game *game, uint64_t now_ms);
void game_return_to_title(Game *game);
void game_toggle_limited_sight(Game *game);

bool game_move_player(Game *game, Direction direction);
bool game_break_wall(Game *game, int maze_x, int maze_y);
void game_update_spikes(Game *game, uint64_t now_ms);

char game_cell_at(const Game *game, int x, int y);
bool game_position_is_reachable(const Game *game, Position destination);
size_t game_count_cells(const Game *game, char cell);

#endif
