#include <stdlib.h>
#include <string.h>

#include "maze.h"
#include "globals.h"

int x_move[4] = {-1, 1, 0, 0};
int y_move[4] = {0, 0, -1, 1};

Position player;
Position goal;
int player_move_count = 0;

#define MAX_SPIKES 50
Spike spikes[MAX_SPIKES];
int spike_count = 0;

static int spike_map[ROW][COL];

char maze[ROW][COL] = {
    "########################################",
    "#   #   #       # #   #   #           ##",
    "### # # ##### # # # # # # # ######### ##",
    "#   # #       # #   #   # # #   #     ##",
    "# ### ######### ####### # # # # # ### ##",
    "#     #       # #     # #   # #   #   ##",
    "####### ####### # ### ####### ##### ####",
    "#       #     # # # #           #   #  #",
    "# ### ### ### # # # ########### # ###  #",
    "# # # #     # #   #         #   #   # ##",
    "# # # # ##### ##### ######### # # # # ##",
    "# # #    *#         #         #   #   ##",
    "# # ####### ##### ### ########### ###  #",
    "#   #       #     #   #       # #     ##",
    "### # ### ####### # ### # ### # ########",
    "#   #   # #     # # #   #   #   #     ##",
    "# ##### ### ### ### # ##### ##### ### ##",
    "# #   #     # #     # #   #   #   #   ##",
    "# ### ####### ####### ### ### # ### ####",
    "#             #     #       #     #   ##",
    "############# # ### ##### ########### ##",
    "#         #   # #*#   # # #       #   ##",
    "######### # ### # ### # # # # ### # #  #",
    "#         # #   #   # #   # # #   # # ##",
    "# ####### # ### ### # # ### # # ### ####",
    "# #*      #   #   # # #  *# # #   #   ##",
    "# ########### ### # # ##### # ### ### ##",
    "#                 #         #         ##",
    "####################   ########### #####",
    "########################################",
};

char maze_cpy[ROW][COL];

static int cell_passable(char c) {
    return c != '#' && c != 'w';
}

static int cell_degree(int x, int y) {
    int d = 0;
    for (int k = 0; k < 4; k++) {
        int nx = x + x_move[k];
        int ny = y + y_move[k];
        if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) continue;
        if (cell_passable(maze[ny][nx])) d++;
    }
    return d;
}

static void compute_dist(int dist[ROW][COL]) {
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COL; j++)
            dist[i][j] = -1;
    int qx[ROW * COL];
    int qy[ROW * COL];
    int head = 0;
    int tail = 0;
    dist[player.y][player.x] = 0;
    qx[tail] = player.x;
    qy[tail] = player.y;
    tail++;
    while (head < tail) {
        int x = qx[head];
        int y = qy[head];
        head++;
        for (int k = 0; k < 4; k++) {
            int nx = x + x_move[k];
            int ny = y + y_move[k];
            if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) continue;
            if (dist[ny][nx] != -1) continue;
            if (!cell_passable(maze[ny][nx])) continue;
            dist[ny][nx] = dist[y][x] + 1;
            qx[tail] = nx;
            qy[tail] = ny;
            tail++;
        }
    }
}

static int exit_can_increase(int x, int y, int dist[ROW][COL]) {
    int cur = dist[y][x];
    if (cur < 0) return 0;
    for (int k = 0; k < 4; k++) {
        int nx = x + x_move[k];
        int ny = y + y_move[k];
        if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) continue;
        if (maze[ny][nx] != ' ') continue;
        if (spike_map[ny][nx]) continue;
        if (dist[ny][nx] > cur) return 1;
    }
    return 0;
}

static void move_exit() {
    int dist[ROW][COL];
    compute_dist(dist);
    int best_x = goal.x;
    int best_y = goal.y;
    int best_d = dist[goal.y][goal.x];
    for (int i = 0; i < 4; i++) {
        int nx = goal.x + x_move[i];
        int ny = goal.y + y_move[i];
        if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) continue;
        if (maze[ny][nx] != ' ') continue;
        if (spike_map[ny][nx]) continue;
        if (dist[ny][nx] > best_d) {
            best_d = dist[ny][nx];
            best_x = nx;
            best_y = ny;
        }
    }
    if (best_x != goal.x || best_y != goal.y) {
        maze[goal.y][goal.x] = ' ';
        goal.x = best_x;
        goal.y = best_y;
        maze[goal.y][goal.x] = 'x';
    }
}

static int pick_cell(Position *out, int dist[ROW][COL], int min_d) {
    Position candidates[ROW * COL];
    int c = 0;
    for (int i = 1; i < ROW - 1; i++) {
        for (int j = 1; j < COL - 1; j++) {
            if (maze[i][j] != ' ') continue;
            if (spike_map[i][j]) continue;
            if (i == player.y && j == player.x) continue;
            if (dist[i][j] < min_d) continue;
            candidates[c].x = j;
            candidates[c].y = i;
            c++;
        }
    }
    if (c <= 0) return 0;
    int r = rand() % c;
    *out = candidates[r];
    return 1;
}

static int find_any_reachable(Position *out, int dist[ROW][COL]) {
    for (int i = 1; i < ROW - 1; i++) {
        for (int j = 1; j < COL - 1; j++) {
            if (maze[i][j] != ' ') continue;
            if (spike_map[i][j]) continue;
            if (i == player.y && j == player.x) continue;
            if (dist[i][j] < 0) continue;
            out->x = j;
            out->y = i;
            return 1;
        }
    }
    return 0;
}

static void clear_spike_map() {
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COL; j++)
            spike_map[i][j] = 0;
}

static int pick_goal(Position *out) {
    int dist[ROW][COL];
    compute_dist(dist);
    int maxd = -1;
    for (int i = 1; i < ROW - 1; i++)
        for (int j = 1; j < COL - 1; j++)
            if (maze[i][j] == ' ' && !spike_map[i][j] && dist[i][j] > maxd)
                maxd = dist[i][j];
    if (maxd < 0) return 0;
    int margins[] = {8, 12, 16, 20, 24, 28};
    for (int mi = 0; mi < 6; mi++) {
        int min_d = maxd - margins[mi];
        if (min_d < 10) min_d = 10;
        Position candidates[ROW * COL];
        int c = 0;
        for (int i = 2; i < ROW - 2; i++) {
            for (int j = 2; j < COL - 2; j++) {
                if (maze[i][j] != ' ') continue;
                if (spike_map[i][j]) continue;
                if (i == player.y && j == player.x) continue;
                if (dist[i][j] < min_d) continue;
                if (cell_degree(j, i) < 2) continue;
                if (!exit_can_increase(j, i, dist)) continue;
                candidates[c].x = j;
                candidates[c].y = i;
                c++;
            }
        }
        if (c > 0) {
            int r = rand() % c;
            *out = candidates[r];
            return 1;
        }
    }

    for (int i = 2; i < ROW - 2; i++) {
        for (int j = 2; j < COL - 2; j++) {
            if (maze[i][j] != ' ') continue;
            if (spike_map[i][j]) continue;
            if (i == player.y && j == player.x) continue;
            if (dist[i][j] < 0) continue;
            if (cell_degree(j, i) < 2) continue;
            if (!exit_can_increase(j, i, dist)) continue;
            out->x = j;
            out->y = i;
            return 1;
        }
    }
    return 0;
}

void init_maze() {
    static int first_run = 1;
    if (first_run) {
        memcpy(maze_cpy, maze, sizeof(maze));
        first_run = 0;
    } else {
        memcpy(maze, maze_cpy, sizeof(maze_cpy));
    }
    player.x = 1;
    player.y = 1;
    if (!cell_passable(maze[player.y][player.x]) || maze[player.y][player.x] == '#') {
        for (int i = 1; i < ROW - 1; i++) {
            for (int j = 1; j < COL - 1; j++) {
                if (maze[i][j] == ' ' || maze[i][j] == '*') {
                    player.x = j;
                    player.y = i;
                    i = ROW;
                    break;
                }
            }
        }
        if (maze[player.y][player.x] == '#') maze[player.y][player.x] = ' ';
    }
    player_has_key = 0;
    powerups = 0;
    player_move_count = 0;
    clear_spike_map();
    spike_count = 0;
    goal.x = -1;
    goal.y = -1;
    int dist[ROW][COL];
    compute_dist(dist);
    Position key_pos;
    int has_pos = pick_cell(&key_pos, dist, (ROW + COL) / 2);
    if (!has_pos) has_pos = pick_cell(&key_pos, dist, (ROW + COL) / 3);
    if (!has_pos) has_pos = pick_cell(&key_pos, dist, 10);
    if (!has_pos) has_pos = find_any_reachable(&key_pos, dist);
    if (has_pos) maze[key_pos.y][key_pos.x] = 'k';
    int powerup_total = 4;
    for (int i = 0; i < powerup_total; i++) {
        Position p;
        if (!pick_cell(&p, dist, 12))
            if (!pick_cell(&p, dist, 6))
                break;
        maze[p.y][p.x] = 'P';
    }
    int spike_total = 22;
    int placed = 0;
    int attempts = 0;
    while (placed < spike_total && spike_count < MAX_SPIKES && attempts < spike_total * 60) {
        attempts++;
        Position s;
        if (!pick_cell(&s, dist, 10))
            if (!pick_cell(&s, dist, 6))
                break;
        if (abs(s.x - player.x) + abs(s.y - player.y) < 6) continue;
        int deg = cell_degree(s.x, s.y);
        if (deg > 2 && (rand() % 4) != 0) continue;
        spikes[spike_count].x = s.x;
        spikes[spike_count].y = s.y;
        spikes[spike_count].active = 0;
        spikes[spike_count].timer = rand() % 12;
        spikes[spike_count].interval_on = (rand() % 18) + 10;
        spikes[spike_count].interval_off = (rand() % 12) + 6;
        spike_map[s.y][s.x] = 1;
        spike_count++;
        placed++;
    }
    if (!pick_goal(&goal)) {
        goal.x = COL - 2;
        goal.y = ROW - 2;
        if (maze[goal.y][goal.x] != ' ') {
            for (int i = ROW - 2; i >= 1; i--) {
                for (int j = COL - 2; j >= 1; j--) {
                    if (maze[i][j] == ' ' && !spike_map[i][j]) {
                        goal.x = j;
                        goal.y = i;
                        i = 0;
                        break;
                    }
                }
            }
        }
    }
    maze[player.y][player.x] = 'o';
    if (maze[goal.y][goal.x] == ' ') maze[goal.y][goal.x] = 'x';
}

void update_spikes() {
    for (int i = 0; i < spike_count; i++) {
        spikes[i].timer++;
        if (spikes[i].active) {
            if (spikes[i].timer >= spikes[i].interval_on) {
                spikes[i].active = 0;
                spikes[i].timer = 0;
                if (maze[spikes[i].y][spikes[i].x] == 'w')
                    maze[spikes[i].y][spikes[i].x] = ' ';
            } else {
                if (player.x == spikes[i].x && player.y == spikes[i].y) {
                    status = GAME_OVER;
                    continue;
                }
                if (maze[spikes[i].y][spikes[i].x] == ' ')
                    maze[spikes[i].y][spikes[i].x] = 'w';
            }
        } else {
            if (spikes[i].timer >= spikes[i].interval_off) {
                spikes[i].active = 1;
                spikes[i].timer = 0;
                if (player.x == spikes[i].x && player.y == spikes[i].y) {
                    status = GAME_OVER;
                    continue;
                }
                if (maze[spikes[i].y][spikes[i].x] == ' ')
                    maze[spikes[i].y][spikes[i].x] = 'w';
            } else {
                if (maze[spikes[i].y][spikes[i].x] == 'w')
                    maze[spikes[i].y][spikes[i].x] = ' ';
            }
        }
    }
}

static void teleport_player() {
    int targets[50][2];
    int t_count = 0;
    for (int i = 0; i < ROW; i++) {
        for (int j = 0; j < COL; j++) {
            if (maze[i][j] == '*' && (i != player.y || j != player.x)) {
                targets[t_count][0] = j;
                targets[t_count][1] = i;
                t_count++;
                if (t_count >= 50) break;
            }
        }
        if (t_count >= 50) break;
    }
    if (t_count > 0) {
        int r = rand() % t_count;
        maze[player.y][player.x] = '*';
        player.x = targets[r][0];
        player.y = targets[r][1];
        maze[player.y][player.x] = 'o';
    }
}

void move_player(Direction d) {
    int next_x = player.x + x_move[d];
    int next_y = player.y + y_move[d];
    if (next_x < 0 || next_x >= COL || next_y < 0 || next_y >= ROW) return;
    char cell = maze[next_y][next_x];
    if (cell == '#') return;
    if (cell == 'w') {
        status = GAME_OVER;
        return;
    }
    if (cell == 'x' && !player_has_key) return;
    if (cell == 'k') player_has_key = 1;
    if (cell == 'P') powerups++;
    int old_x = player.x;
    int old_y = player.y;
    maze[old_y][old_x] = ' ';
    if (maze_cpy[old_y][old_x] == '*') maze[old_y][old_x] = '*';
    player.x = next_x;
    player.y = next_y;
    maze[player.y][player.x] = 'o';
    if (cell == '*') {
        teleport_player();
    }
    if (player.x == goal.x && player.y == goal.y) {
        status = GAME_WIN;
        return;
    }
    player_move_count++;
    if (player_move_count % 2 == 0) {
        move_exit();
    }
}

void handle_mouse_click(int mx, int my, int offset_y, int offset_x) {
    if (powerups <= 0) return;
    int maze_x = (mx / 2) + offset_x;
    int maze_y = my + offset_y;
    if (maze_x <= 0 || maze_x >= COL - 1 || maze_y <= 0 || maze_y >= ROW - 1) return;
    if (limit_sight) {
        if (abs(maze_x - player.x) > 5 || abs(maze_y - player.y) > 5) return;
    }
    if (maze[maze_y][maze_x] == '#') {
        maze[maze_y][maze_x] = ' ';
        powerups--;
    }
}
