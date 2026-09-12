# ncurses Maze Game

> This project was originally developed as my final project for ESTR1002 during my first year. My sincere thanks to the professor for awarding me an A. 🙏
>
> Development continued after the course ended, with assistance from GPT-5.6 Sol.

[![CI](https://github.com/HollisPeng/ncurses-maze-game/actions/workflows/ci.yml/badge.svg)](https://github.com/HollisPeng/ncurses-maze-game/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

English · [简体中文](README.zh-CN.md)

A terminal maze game written in C11 with the wide-character ncurses library. Navigate a fixed maze, collect the key, avoid timed spikes, use teleporters and powerups, and catch an exit that moves away from you.

## Features

### Core gameplay

- Full-screen ncurses title menu
- WASD movement with wall collision
- Clean return to the title screen with ESC
- Complete game reset when a new round starts
- Win and game-over screens
- Safe terminal restoration on exit

### Implemented optional features

| Feature | Behavior |
| --- | --- |
| Scrolling viewport | Follows the player when the full maze does not fit |
| Full-width characters | Renders square maze cells in UTF-8 terminals |
| Limited sight | Optional player-centered 11×11 view |
| Teleporters | Move the player to another random teleporter |
| Timed spikes | Alternate between safe and lethal states |
| Key | Must be collected before entering the exit |
| Powerups | Collected and spent by left-clicking an interior wall |
| Moving exit | Moves away after every two successful player moves |

The maze itself is intentionally fixed. Items, hazards, and the exit are placed from a deterministic per-game random state while remaining on reachable cells.

## Requirements

- Linux
- A C11 compiler (GCC or Clang)
- CMake 3.16 or newer
- The wide-character ncurses development package
- A UTF-8 locale and terminal
- Mouse reporting support for the wall-breaking feature

On Debian or Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake libncurses-dev
```

A terminal of at least 80×32 is recommended. Smaller terminals use the scrolling viewport automatically.

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/ncurses-maze-game
```

## Controls

### Title screen

| Key | Action |
| --- | --- |
| W / S or ↑ / ↓ | Change the selected menu item |
| Enter | Confirm |
| ESC | Exit |

### Gameplay

| Input | Action |
| --- | --- |
| W / A / S / D | Move |
| ESC | Return to the title screen |
| Left mouse button | Spend one powerup to break a visible interior wall |

## Symbols

| Symbol | Meaning |
| --- | --- |
| ＃ | Wall |
| ｏ | Player |
| ｘ | Exit |
| ＊ | Teleporter |
| ｋ | Key |
| Ｐ | Powerup |
| ｗ | Active spike |

An inactive spike is rendered as an ordinary corridor.

## Tests

Build and run the test suite:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Enable AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S . -B build-sanitized \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMAZE_ENABLE_SANITIZERS=ON
cmake --build build-sanitized --parallel
ctest --test-dir build-sanitized --output-on-failure
```

GitHub Actions performs GCC, Clang, and sanitizer builds on Ubuntu.

## Project structure

```text
.
├── .github/workflows/ci.yml
├── include/
│   ├── game.h
│   └── ui.h
├── src/
│   ├── game.c
│   ├── main.c
│   └── ui.c
├── tests/test_game.c
├── CMakeLists.txt
├── README.md
├── README.zh-CN.md
└── LICENSE
```

The game rules are kept independent from ncurses. This separation makes state transitions, movement, item placement, hazards, and reachability testable without an interactive terminal.

## TODO / Future Work

The following optional features from the original project specification have not yet been implemented and may be added in the future:

- [ ] Random maze generation with guaranteed exit reachability
- [ ] Infinite tiled maze with wrap-around scrolling
- [ ] Player energy and an energy-depletion losing condition

## Original course submission

The unmodified repository baseline is preserved on the [`archive/course-submission`](https://github.com/HollisPeng/ncurses-maze-game/tree/archive/course-submission) branch. The current version is a post-course refinement of that work.

## License

This project is available under the [MIT License](LICENSE).
