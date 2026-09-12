#include "game.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum {
    KEY_MIN_DISTANCE = 20,
    POWERUP_COUNT = 4,
    SPIKE_TARGET_COUNT = 22
};

static const char *const MAZE_TEMPLATE[MAZE_ROWS] = {
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

static const int X_MOVE[] = {-1, 1, 0, 0};
static const int Y_MOVE[] = {0, 0, -1, 1};

static bool positions_equal(Position first, Position second) {
    return first.x == second.x && first.y == second.y;
}

static bool position_in_bounds(Position position) {
    return position.x >= 0 && position.x < MAZE_COLS &&
           position.y >= 0 && position.y < MAZE_ROWS;
}

static uint32_t random_u32(Game *game) {
    uint32_t value = game->random_state;
    if (value == 0U) {
        value = UINT32_C(0x6d2b79f5);
    }
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    game->random_state = value;
    return value;
}

static unsigned int random_below(Game *game, unsigned int upper_bound) {
    if (upper_bound == 0U) {
        return 0U;
    }
    return random_u32(game) % upper_bound;
}

static int spike_index_at(const Game *game, Position position) {
    for (size_t index = 0U; index < game->spike_count; ++index) {
        if (positions_equal(game->spikes[index].position, position)) {
            return (int)index;
        }
    }
    return -1;
}

static bool is_open_terrain(const Game *game, Position position) {
    return position_in_bounds(position) &&
           game->terrain[position.y][position.x] != '#';
}

static bool is_free_corridor(const Game *game, Position position) {
    return position.x > 0 && position.x < MAZE_COLS - 1 &&
           position.y > 0 && position.y < MAZE_ROWS - 1 &&
           game->terrain[position.y][position.x] == ' ' &&
           game->items[position.y][position.x] == '\0' &&
           !positions_equal(position, game->player) &&
           !positions_equal(position, game->exit) &&
           spike_index_at(game, position) < 0;
}

static void compute_distances(const Game *game, Position start,
                              int distances[MAZE_ROWS][MAZE_COLS]) {
    Position queue[MAZE_ROWS * MAZE_COLS];
    size_t head = 0U;
    size_t tail = 0U;

    for (int y = 0; y < MAZE_ROWS; ++y) {
        for (int x = 0; x < MAZE_COLS; ++x) {
            distances[y][x] = -1;
        }
    }

    if (!is_open_terrain(game, start)) {
        return;
    }

    distances[start.y][start.x] = 0;
    queue[tail++] = start;

    while (head < tail) {
        const Position current = queue[head++];
        for (size_t direction = 0U; direction < 4U; ++direction) {
            const Position next = {
                current.x + X_MOVE[direction],
                current.y + Y_MOVE[direction],
            };
            if (!position_in_bounds(next) ||
                distances[next.y][next.x] >= 0 ||
                !is_open_terrain(game, next)) {
                continue;
            }
            distances[next.y][next.x] =
                distances[current.y][current.x] + 1;
            queue[tail++] = next;
        }
    }
}

static bool pick_reachable_corridor(
    Game *game, int distances[MAZE_ROWS][MAZE_COLS],
    int minimum_distance, Position *result) {
    Position candidates[MAZE_ROWS * MAZE_COLS];
    size_t count = 0U;

    for (int y = 1; y < MAZE_ROWS - 1; ++y) {
        for (int x = 1; x < MAZE_COLS - 1; ++x) {
            const Position candidate = {x, y};
            if (distances[y][x] >= minimum_distance &&
                is_free_corridor(game, candidate)) {
                candidates[count++] = candidate;
            }
        }
    }

    if (count == 0U) {
        return false;
    }
    *result = candidates[random_below(game, (unsigned int)count)];
    return true;
}

static bool place_item(Game *game, char item,
                       int distances[MAZE_ROWS][MAZE_COLS],
                       int preferred_distance) {
    Position position;
    for (int distance = preferred_distance; distance >= 0; distance -= 5) {
        if (pick_reachable_corridor(game, distances, distance, &position)) {
            game->items[position.y][position.x] = item;
            return true;
        }
    }
    return false;
}

static int corridor_degree(const Game *game, Position position) {
    int degree = 0;
    for (size_t direction = 0U; direction < 4U; ++direction) {
        const Position next = {
            position.x + X_MOVE[direction],
            position.y + Y_MOVE[direction],
        };
        if (is_open_terrain(game, next)) {
            ++degree;
        }
    }
    return degree;
}

static bool can_move_farther(
    const Game *game, Position position,
    int distances[MAZE_ROWS][MAZE_COLS]) {
    const int current_distance = distances[position.y][position.x];
    for (size_t direction = 0U; direction < 4U; ++direction) {
        const Position next = {
            position.x + X_MOVE[direction],
            position.y + Y_MOVE[direction],
        };
        if (position_in_bounds(next) && is_free_corridor(game, next) &&
            distances[next.y][next.x] > current_distance) {
            return true;
        }
    }
    return false;
}

static bool choose_exit(Game *game,
                        int distances[MAZE_ROWS][MAZE_COLS],
                        Position *result) {
    int maximum_distance = -1;
    for (int y = 1; y < MAZE_ROWS - 1; ++y) {
        for (int x = 1; x < MAZE_COLS - 1; ++x) {
            const Position position = {x, y};
            if (is_free_corridor(game, position) &&
                distances[y][x] > maximum_distance) {
                maximum_distance = distances[y][x];
            }
        }
    }

    for (int margin = 8; margin <= 32; margin += 8) {
        Position candidates[MAZE_ROWS * MAZE_COLS];
        size_t count = 0U;
        const int minimum_distance = maximum_distance - margin;
        for (int y = 2; y < MAZE_ROWS - 2; ++y) {
            for (int x = 2; x < MAZE_COLS - 2; ++x) {
                const Position position = {x, y};
                if (distances[y][x] >= minimum_distance &&
                    is_free_corridor(game, position) &&
                    corridor_degree(game, position) >= 2 &&
                    can_move_farther(game, position, distances)) {
                    candidates[count++] = position;
                }
            }
        }
        if (count > 0U) {
            *result =
                candidates[random_below(game, (unsigned int)count)];
            return true;
        }
    }

    for (int distance = maximum_distance; distance >= 0; --distance) {
        for (int y = 1; y < MAZE_ROWS - 1; ++y) {
            for (int x = 1; x < MAZE_COLS - 1; ++x) {
                const Position position = {x, y};
                if (distances[y][x] == distance &&
                    is_free_corridor(game, position)) {
                    *result = position;
                    return true;
                }
            }
        }
    }
    return false;
}

static void place_spikes(
    Game *game, uint64_t now_ms,
    int distances[MAZE_ROWS][MAZE_COLS]) {
    unsigned int attempts = 0U;
    while (game->spike_count < (size_t)SPIKE_TARGET_COUNT &&
           game->spike_count < MAX_SPIKES && attempts < 2000U) {
        ++attempts;
        Position position;
        if (!pick_reachable_corridor(game, distances, 6, &position)) {
            break;
        }
        const int player_distance =
            abs(position.x - game->player.x) +
            abs(position.y - game->player.y);
        if (player_distance < 6 ||
            (corridor_degree(game, position) > 2 &&
             random_below(game, 4U) != 0U)) {
            continue;
        }

        Spike *spike = &game->spikes[game->spike_count++];
        spike->position = position;
        spike->active = false;
        spike->active_ms = 1000U + random_below(game, 1801U);
        spike->inactive_ms = 600U + random_below(game, 1201U);
        spike->next_toggle_ms = now_ms + spike->inactive_ms;
    }
}

static void move_exit_away(Game *game) {
    int distances[MAZE_ROWS][MAZE_COLS];
    compute_distances(game, game->player, distances);
    Position best = game->exit;
    int best_distance = distances[best.y][best.x];

    for (size_t direction = 0U; direction < 4U; ++direction) {
        const Position candidate = {
            game->exit.x + X_MOVE[direction],
            game->exit.y + Y_MOVE[direction],
        };
        if (!position_in_bounds(candidate) ||
            !is_free_corridor(game, candidate)) {
            continue;
        }
        const int candidate_distance =
            distances[candidate.y][candidate.x];
        if (candidate_distance > best_distance) {
            best = candidate;
            best_distance = candidate_distance;
        }
    }
    game->exit = best;
}

static void teleport_player(Game *game) {
    Position destinations[MAZE_ROWS * MAZE_COLS];
    size_t count = 0U;

    for (int y = 0; y < MAZE_ROWS; ++y) {
        for (int x = 0; x < MAZE_COLS; ++x) {
            const Position position = {x, y};
            if (game->terrain[y][x] == '*' &&
                !positions_equal(position, game->player)) {
                destinations[count++] = position;
            }
        }
    }

    if (count > 0U) {
        game->player =
            destinations[random_below(game, (unsigned int)count)];
    }
}

void game_init(Game *game, uint32_t seed) {
    memset(game, 0, sizeof(*game));
    game->random_state =
        seed == 0U ? UINT32_C(0x6d2b79f5) : seed;
    game->status = GAME_TITLE;
    game->exit = (Position){-1, -1};
}

void game_start(Game *game, uint64_t now_ms) {
    const bool limited_sight = game->limited_sight;
    const uint32_t random_state = game->random_state;
    memset(game, 0, sizeof(*game));
    game->limited_sight = limited_sight;
    game->random_state = random_state;
    game->status = GAME_PLAYING;
    game->player = (Position){1, 1};
    game->exit = (Position){-1, -1};

    for (int y = 0; y < MAZE_ROWS; ++y) {
        memcpy(game->terrain[y], MAZE_TEMPLATE[y], MAZE_COLS);
    }

    if (!is_open_terrain(game, game->player)) {
        for (int y = 1; y < MAZE_ROWS - 1; ++y) {
            for (int x = 1; x < MAZE_COLS - 1; ++x) {
                const Position candidate = {x, y};
                if (is_open_terrain(game, candidate)) {
                    game->player = candidate;
                    y = MAZE_ROWS;
                    break;
                }
            }
        }
    }

    int distances[MAZE_ROWS][MAZE_COLS];
    compute_distances(game, game->player, distances);
    const bool key_placed =
        place_item(game, 'k', distances, KEY_MIN_DISTANCE);
    bool all_powerups_placed = true;
    for (int count = 0; count < POWERUP_COUNT; ++count) {
        if (!place_item(game, 'P', distances, 6)) {
            all_powerups_placed = false;
            break;
        }
    }
    place_spikes(game, now_ms, distances);
    if (!key_placed || !all_powerups_placed ||
        !choose_exit(game, distances, &game->exit)) {
        game->status = GAME_LOST;
    }
}

void game_return_to_title(Game *game) {
    game->status = GAME_TITLE;
}

void game_toggle_limited_sight(Game *game) {
    if (game->status == GAME_TITLE) {
        game->limited_sight = !game->limited_sight;
    }
}

bool game_move_player(Game *game, Direction direction) {
    if (game->status != GAME_PLAYING ||
        direction < DIRECTION_LEFT || direction > DIRECTION_DOWN) {
        return false;
    }

    const Position next = {
        game->player.x + X_MOVE[direction],
        game->player.y + Y_MOVE[direction],
    };
    if (!position_in_bounds(next) ||
        game->terrain[next.y][next.x] == '#') {
        return false;
    }
    if (positions_equal(next, game->exit) && !game->has_key) {
        return false;
    }

    const int spike_index = spike_index_at(game, next);
    game->player = next;
    if (spike_index >= 0 && game->spikes[spike_index].active) {
        game->status = GAME_LOST;
        return true;
    }

    const char item = game->items[next.y][next.x];
    if (item == 'k') {
        game->has_key = true;
        game->items[next.y][next.x] = '\0';
    } else if (item == 'P') {
        ++game->powerups;
        game->items[next.y][next.x] = '\0';
    }

    if (game->terrain[next.y][next.x] == '*') {
        teleport_player(game);
    }

    if (positions_equal(game->player, game->exit)) {
        game->status = GAME_WON;
        return true;
    }

    ++game->move_count;
    if (game->move_count % 2U == 0U) {
        move_exit_away(game);
    }
    return true;
}

bool game_break_wall(Game *game, int maze_x, int maze_y) {
    if (game->status != GAME_PLAYING || game->powerups == 0U ||
        maze_x <= 0 || maze_x >= MAZE_COLS - 1 ||
        maze_y <= 0 || maze_y >= MAZE_ROWS - 1 ||
        game->terrain[maze_y][maze_x] != '#') {
        return false;
    }
    game->terrain[maze_y][maze_x] = ' ';
    --game->powerups;
    return true;
}

void game_update_spikes(Game *game, uint64_t now_ms) {
    if (game->status != GAME_PLAYING) {
        return;
    }

    for (size_t index = 0U; index < game->spike_count; ++index) {
        Spike *spike = &game->spikes[index];
        unsigned int transitions = 0U;
        while (now_ms >= spike->next_toggle_ms &&
               transitions++ < 16U) {
            spike->active = !spike->active;
            spike->next_toggle_ms +=
                spike->active ? spike->active_ms : spike->inactive_ms;
        }
        if (spike->active &&
            positions_equal(spike->position, game->player)) {
            game->status = GAME_LOST;
            return;
        }
    }
}

char game_cell_at(const Game *game, int x, int y) {
    const Position position = {x, y};
    if (!position_in_bounds(position)) {
        return '#';
    }
    if (positions_equal(position, game->player)) {
        return 'o';
    }
    if (positions_equal(position, game->exit)) {
        return 'x';
    }
    if (game->items[y][x] != '\0') {
        return game->items[y][x];
    }
    const int spike_index = spike_index_at(game, position);
    if (spike_index >= 0 && game->spikes[spike_index].active) {
        return 'w';
    }
    return game->terrain[y][x];
}

bool game_position_is_reachable(const Game *game, Position destination) {
    if (!position_in_bounds(destination)) {
        return false;
    }
    int distances[MAZE_ROWS][MAZE_COLS];
    compute_distances(game, game->player, distances);
    return distances[destination.y][destination.x] >= 0;
}

size_t game_count_cells(const Game *game, char cell) {
    size_t count = 0U;
    for (int y = 0; y < MAZE_ROWS; ++y) {
        for (int x = 0; x < MAZE_COLS; ++x) {
            if (game_cell_at(game, x, y) == cell) {
                ++count;
            }
        }
    }
    return count;
}
