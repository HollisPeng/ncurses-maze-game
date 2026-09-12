cmake_minimum_required(VERSION 3.16)

project(ncurses-maze-game VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MAZE_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(MAZE_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)

function(maze_set_compile_options target)
    if(MAZE_ENABLE_WARNINGS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        )
    endif()

    if(MAZE_ENABLE_SANITIZERS)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer
            )
        else()
            message(WARNING "Sanitizers are only configured for GCC and Clang")
        endif()
    endif()
endfunction()

add_library(maze_game_core src/game.c)
target_include_directories(maze_game_core PUBLIC include)
maze_set_compile_options(maze_game_core)

set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)

add_executable(ncurses-maze-game
    src/main.c
    src/ui.c
)
target_compile_definitions(ncurses-maze-game PRIVATE _XOPEN_SOURCE_EXTENDED=1)
target_include_directories(ncurses-maze-game PRIVATE
    include
    ${CURSES_INCLUDE_DIRS}
)
target_link_libraries(ncurses-maze-game PRIVATE
    maze_game_core
    ${CURSES_LIBRARIES}
)
maze_set_compile_options(ncurses-maze-game)

include(CTest)
if(BUILD_TESTING)
    add_executable(maze_game_tests tests/test_game.c)
    target_link_libraries(maze_game_tests PRIVATE maze_game_core)
    maze_set_compile_options(maze_game_tests)
    add_test(NAME maze_game_tests COMMAND maze_game_tests)
endif()
