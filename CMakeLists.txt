cmake_minimum_required(VERSION 3.16)

project(ncurses_maze_game LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)

add_executable(ncurses_maze_game
    main.c
    maze.c
    ui.c
    globals.c
)

target_include_directories(ncurses_maze_game PRIVATE
    ${CURSES_INCLUDE_DIRS}
    ${CURSES_INCLUDE_PATH}/ncursesw
)
target_link_libraries(ncurses_maze_game PRIVATE ${CURSES_LIBRARIES})
