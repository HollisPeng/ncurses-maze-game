#ifndef MAZE_GAME_MAZE_H
#define MAZE_GAME_MAZE_H

#define ROW 30
#define COL 40

typedef enum {
    LEFT,
    RIGHT,
    UP,
    DOWN,
} Direction;

extern int x_move[4];
extern int y_move[4];

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    int x;
    int y;
    int active;
    int timer;
    int interval_on;
    int interval_off;
} Spike;

extern Position player;
extern Position goal;
extern char maze[ROW][COL];
extern char maze_cpy[ROW][COL];
extern int player_move_count;

void init_maze();
void move_player(Direction d);
void update_spikes();
void handle_mouse_click(int mx, int my, int offset_y, int offset_x);

#endif //MAZE_GAME_MAZE_H