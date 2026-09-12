#include "game.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static const int X_MOVE[] = {-1, 1, 0, 0};
static const int Y_MOVE[] = {0, 0, -1, 1};
static const Direction DIRECTIONS[] = {
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_UP,
    DIRECTION_DOWN,
};
static const Direction OPPOSITE[] = {
    DIRECTION_RIGHT,
    DIRECTION_LEFT,
    DIRECTION_DOWN,
    DIRECTION_UP,
};

static bool positions_equal(Position first, Position second) {
    return first.x == second.x && first.y == second.y;
}

static Game started_game(uint32_t seed) {
    Game game;
    game_init(&game, seed);
    game_start(&game, UINT64_C(1000));
    return game;
}

static int find_safe_direction(const Game *game) {
    for (int direction = 0; direction < 4; ++direction) {
        const int x = game->player.x + X_MOVE[direction];
        const int y = game->player.y + Y_MOVE[direction];
        const char cell = game_cell_at(game, x, y);
        if (cell == ' ' &&
            !(x == game->exit.x && y == game->exit.y)) {
            return direction;
        }
    }
    return -1;
}

static void test_generation_invariants(void) {
    for (uint32_t seed = 1U; seed <= 100U; ++seed) {
        const Game game = started_game(seed);
        assert(game.status == GAME_PLAYING);
        assert(game_position_is_reachable(&game, game.exit));
        assert(game_count_cells(&game, 'o') == 1U);
        assert(game_count_cells(&game, 'x') == 1U);
        assert(game_count_cells(&game, 'k') == 1U);
        assert(game_count_cells(&game, 'P') == 4U);
        assert(game_count_cells(&game, '*') == 4U);
        assert(game.spike_count == 22U);

        for (int x = 0; x < MAZE_COLS; ++x) {
            assert(game.terrain[0][x] == '#');
            assert(game.terrain[MAZE_ROWS - 1][x] == '#');
        }
        for (int y = 0; y < MAZE_ROWS; ++y) {
            assert(game.terrain[y][0] == '#');
            assert(game.terrain[y][MAZE_COLS - 1] == '#');
        }
    }
}

static void test_title_options(void) {
    Game game;
    game_init(&game, 1U);
    assert(!game.limited_sight);
    game_toggle_limited_sight(&game);
    assert(game.limited_sight);
    game_start(&game, 0U);
    game_toggle_limited_sight(&game);
    assert(game.limited_sight);
    game_return_to_title(&game);
    assert(game.status == GAME_TITLE);
}

static void test_wall_collision_and_move_count(void) {
    Game game = started_game(2U);
    const Position original = game.player;
    assert(!game_move_player(&game, DIRECTION_LEFT));
    assert(positions_equal(game.player, original));
    assert(game.move_count == 0U);

    const int direction = find_safe_direction(&game);
    assert(direction >= 0);
    assert(game_move_player(&game, DIRECTIONS[direction]));
    assert(game.move_count == 1U);
    assert(game_move_player(&game, OPPOSITE[direction]));
    assert(game.move_count == 2U);
    assert(game_position_is_reachable(&game, game.exit));
}

static void test_exit_requires_key(void) {
    Game game = started_game(3U);
    const int direction = find_safe_direction(&game);
    assert(direction >= 0);
    const Position original = game.player;
    game.exit = (Position){
        original.x + X_MOVE[direction],
        original.y + Y_MOVE[direction],
    };

    assert(!game_move_player(&game, DIRECTIONS[direction]));
    assert(positions_equal(game.player, original));
    assert(game.status == GAME_PLAYING);

    game.has_key = true;
    assert(game_move_player(&game, DIRECTIONS[direction]));
    assert(game.status == GAME_WON);
}

static void test_powerup_breaks_only_interior_walls(void) {
    Game game = started_game(4U);
    game.powerups = 2U;
    assert(game_break_wall(&game, 4, 1));
    assert(game.terrain[1][4] == ' ');
    assert(game.powerups == 1U);
    assert(!game_break_wall(&game, 0, 1));
    assert(!game_break_wall(&game, 1, 1));
    assert(game.powerups == 1U);
}

static void test_active_spike_causes_loss(void) {
    Game game = started_game(5U);
    assert(game.spike_count > 0U);
    game.player = game.spikes[0].position;
    game.spikes[0].active = false;
    game.spikes[0].next_toggle_ms = 2000U;
    game_update_spikes(&game, 2000U);
    assert(game.spikes[0].active);
    assert(game.status == GAME_LOST);
}

static void test_teleporter_preserves_terrain(void) {
    Game game = started_game(6U);
    bool tested = false;

    for (int y = 1; y < MAZE_ROWS - 1 && !tested; ++y) {
        for (int x = 1; x < MAZE_COLS - 1 && !tested; ++x) {
            if (game.terrain[y][x] != '*') {
                continue;
            }
            const Position teleporter = {x, y};
            for (int direction = 0; direction < 4; ++direction) {
                const Position start = {
                    x - X_MOVE[direction],
                    y - Y_MOVE[direction],
                };
                if (game_cell_at(&game, start.x, start.y) != ' ') {
                    continue;
                }
                game.player = start;
                assert(game_move_player(&game, DIRECTIONS[direction]));
                assert(!positions_equal(game.player, teleporter));
                assert(game.terrain[game.player.y][game.player.x] == '*');
                assert(game.terrain[teleporter.y][teleporter.x] == '*');
                tested = true;
                break;
            }
        }
    }
    assert(tested);
}

static void test_exit_moves_after_two_moves(void) {
    Game game = started_game(7U);
    const Position original_exit = game.exit;
    const int direction = find_safe_direction(&game);
    assert(direction >= 0);
    assert(game_move_player(&game, DIRECTIONS[direction]));
    assert(game_move_player(&game, OPPOSITE[direction]));
    assert(!positions_equal(game.exit, original_exit));
    assert(game_position_is_reachable(&game, game.exit));
}

static void test_restart_resets_progress(void) {
    Game game = started_game(8U);
    game.has_key = true;
    game.powerups = 3U;
    game.move_count = 9U;
    game.status = GAME_LOST;
    game_return_to_title(&game);
    game_start(&game, 5000U);
    assert(game.status == GAME_PLAYING);
    assert(!game.has_key);
    assert(game.powerups == 0U);
    assert(game.move_count == 0U);
    assert(game_count_cells(&game, 'k') == 1U);
    assert(game_count_cells(&game, 'P') == 4U);
}

int main(void) {
    test_generation_invariants();
    test_title_options();
    test_wall_collision_and_move_count();
    test_exit_requires_key();
    test_powerup_breaks_only_interior_walls();
    test_active_spike_causes_loss();
    test_teleporter_preserves_terrain();
    test_exit_moves_after_two_moves();
    test_restart_resets_progress();
    puts("All maze game tests passed.");
    return 0;
}
