#ifndef MAZE_GAME_GLOBALS_H
#define MAZE_GAME_GLOBALS_H

typedef enum {
    TITLE_SCREEN = 0,
    GAME_START = 1,
    GAME_OVER = 2,
    GAME_WIN = 3
} Status;

extern Status status;
extern int limit_sight;
extern int player_has_key;
extern int powerups;

#endif //MAZE_GAME_GLOBALS_H