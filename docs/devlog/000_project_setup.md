# Setting Up a C++ Game from Scratch

Date: 2026-02-08 Tags: tooling, nix, cmake, architecture

## What I Built

The complete project foundation: a Nix flake for the dev environment, CMake
build system with CPM dependency management, and the core engine — fixed
timestep game loop, virtual resolution renderer, ECS with seven systems, and a
stack-based scene manager.

## The Interesting Problem

Reproducible C++ environments are notoriously painful. Different distros ship
different SDL2 versions, clang-format disagrees between versions, and "just
install these packages" doesn't scale. The solution was a Nix flake that pins
every tool and library version. Anyone with Nix can `nix develop` and get an
identical environment — same GCC 14, same SDL2, same clang-tools.

The split between Nix-managed system dependencies (SDL2, audio backends, X11
libs) and CPM-managed C++ libraries (EnTT, nlohmann/json, spdlog, Catch2) was
deliberate. System libraries need pkg-config and linker flags that Nix handles
well. C++ header-only or source-built libraries are better pulled by CMake so
they integrate with the build graph.

## How It Works

The `flake.nix` defines a dev shell with everything needed to build, debug,
profile, and create assets. The `justfile` wraps common CMake invocations into
short commands (`just build`, `just test`, `just run`).

The engine is structured around an EnTT registry owned by the `Game` class.
Systems are free functions in `raven::systems` that take a registry reference
and operate on component views. The fixed timestep clock (`src/core/clock.hpp`)
accumulates frame deltas and returns a step count, with a cap of 4 steps per
frame to prevent the spiral of death.

Rendering uses SDL2's render target system: the game draws to a 480x270 texture,
which is then scaled to the window with nearest-neighbour filtering. This gives
pixel-perfect visuals at any window size.

## What I'd Do Differently

I'd add the Doxyfile and mdBook setup from day one. Documentation infrastructure
is easier to maintain when it grows alongside the code rather than being bolted
on after the fact.

## Next

Implement room generation, enemy spawning, and the first player weapon. Start
the roguelike gameplay loop.
