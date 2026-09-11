#include <stdlib.h>
#include <time.h>

#include "globals.h"
#include "maze.h"
#include "ui.h"

extern int menu_selection;

int main(void) {
    setup_ui();
    srand((unsigned)time(NULL));
    init_maze();

    while (1) {
        if (status == TITLE_SCREEN) {
            title_screen();
            int input_ch = getch();
            if (input_ch == 'w' || input_ch == 'W') {
                menu_selection--;
                if (menu_selection < 0) menu_selection = 2;
            } else if (input_ch == 's' || input_ch == 'S') {
                menu_selection++;
                if (menu_selection > 2) menu_selection = 0;
            } else if (input_ch == '\n' || input_ch == KEY_ENTER) {
                if (menu_selection == 0) {
                    status = GAME_START;
                    init_maze();
                    clear();
                } else if (menu_selection == 1) {
                    limit_sight = !limit_sight;
                } else if (menu_selection == 2) {
                    break;
                }
            } else if (input_ch == 27) {
                break;
            }
        }

        if (status == GAME_START) {
            print_maze();
            refresh();
            halfdelay(1);
            int input_ch = getch();
            cbreak();
            if (input_ch == 27) {
                status = TITLE_SCREEN;
            } else if (input_ch == 'w' || input_ch == 'W')
                move_player(UP);
            else if (input_ch == 's' || input_ch == 'S')
                move_player(DOWN);
            else if (input_ch == 'a' || input_ch == 'A')
                move_player(LEFT);
            else if (input_ch == 'd' || input_ch == 'D')
                move_player(RIGHT);
            else if (input_ch == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    if (event.bstate & BUTTON1_CLICKED) {
                        if (view_w > 0 && view_h > 0) {
                            int x1 = view_x0 + view_w * 2;
                            int y0 = view_y0 + view_hud;
                            int y1 = y0 + view_h;
                            if (event.x >= view_x0 && event.x < x1 && event.y >= y0 && event.y < y1) {
                                handle_mouse_click(event.x - view_x0, event.y - y0, view_start_y, view_start_x);
                            }
                        }
                    }
                }
            }
            if (status == GAME_START) update_spikes();
        }

        if (status == GAME_OVER || status == GAME_WIN) {
            game_end_screen(status == GAME_WIN);
            int input_ch = getch();
            if (input_ch == 27) {
                status = TITLE_SCREEN;
            }
        }
    }

    endwin();
    return 0;
}
