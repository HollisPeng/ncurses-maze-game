# ncurses 迷宫游戏

> 本项目最初是我大一 ESTR1002（Problem Solving by Programming，應用程式設計）课程的期末项目。衷心感谢教授给予我 A。🙏
>
> 课程结束后，我继续完善了本项目，并在此过程中使用了 GPT-5.6 Sol 辅助。

[![CI](https://github.com/HollisPeng/ncurses-maze-game/actions/workflows/ci.yml/badge.svg)](https://github.com/HollisPeng/ncurses-maze-game/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

[English](README.md) · 简体中文

这是一个使用 C11 和宽字符 ncurses 库开发的终端迷宫游戏。玩家需要穿过迷宫、取得钥匙、避开定时尖刺，并利用传送点和 Powerup 追上不断远离的出口。

## 功能

### 基础玩法

- 全屏 ncurses 标题菜单
- 使用 WASD 移动，并具有墙体碰撞
- 游戏中按 ESC 返回标题页
- 每次开始新游戏时完整重置
- 胜利和失败界面
- 退出时安全恢复终端状态

### 已实现的可选功能

| 功能 | 行为 |
| --- | --- |
| 滚动视口 | 终端无法显示完整迷宫时跟随玩家 |
| 全角字符 | 在 UTF-8 终端中显示接近正方形的格子 |
| 限定视野 | 可切换以玩家为中心的 11×11 视野 |
| 传送点 | 将玩家传送至另一个随机传送点 |
| 定时尖刺 | 在安全和致命状态之间周期切换 |
| 钥匙 | 玩家必须先取得钥匙才能进入出口 |
| Powerup | 收集后可通过鼠标左键破坏内部墙体 |
| 移动出口 | 玩家每成功移动两次后远离玩家一次 |

迷宫布局本身保持固定。道具、危险区域和出口通过每局独立的伪随机状态放置，并保证位于玩家可到达的区域。

## 环境要求

- Linux
- 支持 C11 的编译器（GCC 或 Clang）
- CMake 3.16 或更高版本
- 宽字符 ncurses 开发包
- UTF-8 locale 和终端
- 如需使用鼠标破墙，终端需要支持鼠标事件报告

Debian 或 Ubuntu：

```bash
sudo apt update
sudo apt install build-essential cmake libncurses-dev
```

建议终端尺寸至少为 80×32。较小的终端会自动使用滚动视口。

## 构建与运行

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/ncurses-maze-game
```

## 操作

### 标题页

| 按键 | 操作 |
| --- | --- |
| W / S 或 ↑ / ↓ | 切换菜单项 |
| Enter | 确认 |
| ESC | 退出 |

### 游戏中

| 输入 | 操作 |
| --- | --- |
| W / A / S / D | 移动 |
| ESC | 返回标题页 |
| 鼠标左键 | 消耗一个 Powerup，破坏可见的内部墙体 |

## 符号

| 符号 | 含义 |
| --- | --- |
| ＃ | 墙 |
| ｏ | 玩家 |
| ｘ | 出口 |
| ＊ | 传送点 |
| ｋ | 钥匙 |
| Ｐ | Powerup |
| ｗ | 激活的尖刺 |

未激活的尖刺显示为普通通道。

## 测试

构建并运行测试：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

启用 AddressSanitizer 和 UndefinedBehaviorSanitizer：

```bash
cmake -S . -B build-sanitized \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMAZE_ENABLE_SANITIZERS=ON
cmake --build build-sanitized --parallel
ctest --test-dir build-sanitized --output-on-failure
```

GitHub Actions 会在 Ubuntu 上执行 GCC、Clang 和 Sanitizer 构建。

## 项目结构

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

游戏规则与 ncurses 界面相互独立，因此状态切换、移动、道具生成、危险区域和可达性都可以在非交互环境中测试。

## TODO / 未来计划

以下原 Project 要求中的可选功能尚未实现，未来可能继续完成：

- [ ] 随机生成迷宫，并保证出口可达
- [ ] 无限平铺迷宫与循环滚动
- [ ] 玩家能量及能量耗尽后的失败机制

## 原始课程提交

仓库最初的未修改版本保存在 [`archive/course-submission`](https://github.com/HollisPeng/ncurses-maze-game/tree/archive/course-submission) 分支。当前版本是在课程结束后继续完善的版本。

## 许可证

本项目采用 [MIT License](LICENSE)。
