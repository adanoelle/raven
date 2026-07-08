# Introduction

**Raven** is a pixel art roguelike inspired by
[Blazing Beaks](https://blazingbeaks.com/). You explore procedurally generated
rooms, fight enemies with twin-stick controls, and collect items that change
your build run-to-run.

This book documents how the game is built from scratch in C++20. It covers the
development environment, engine architecture, and the decisions made along the
way. Each chapter is written as the feature it describes is implemented, so the
book grows with the project.

## Technology Stack

| Layer             | Choice          | Why                                                      |
| ----------------- | --------------- | -------------------------------------------------------- |
| Language          | C++20           | Modern features, high performance for 120 Hz tick rate   |
| ECS               | EnTT            | Header-only, cache-friendly, widely used in indie games  |
| Windowing / Audio | SDL3 (>= 3.4)   | Cross-platform; native stream mixing; `SCALEMODE_PIXELART` |
| Rendering         | SDL_Renderer    | Hardware-accelerated 2D — ideal for pixel art            |
| Build             | CMake + Ninja   | Industry standard; CPM for dependency management         |
| Debug UI          | Dear ImGui      | Real-time tuning of hitboxes, spawn rates, etc.          |
| Data              | nlohmann/json   | Level data, config, bullet pattern definitions           |
| Logging           | spdlog          | Fast, fmt-based structured logging                       |
| Testing           | Catch2 v3       | BDD-style assertions, good CMake integration             |

The game builds and runs on Linux and Windows (MSVC), with CI covering both.
Steamworks integration is available as an opt-in build flag (see
[ADR-0021](decisions/0021-optional-steamworks.md)).

## What You'll Learn

- Setting up a reproducible C++ dev environment with Nix flakes
- Structuring a game around an Entity Component System (EnTT)
- Implementing a fixed-timestep game loop at 120 Hz
- Pixel-perfect rendering with virtual resolution scaling
- Bitmap-font text rendering and menu/overlay scenes
- Sound effects on SDL3's native audio, driven by ECS events
- Data-driven bullet pattern definitions loaded from JSON
- Stack-based scene management for game states
- Cross-platform builds (Linux + Windows) and CI for a C++ game

## Source Code

The full source is available on GitHub. Each chapter references specific files
so you can read along with the code.
