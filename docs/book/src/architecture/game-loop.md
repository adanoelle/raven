# Game Loop and Fixed Timestep

<!-- TODO: expand with diagrams and code walkthroughs -->

Raven uses a fixed timestep of 1/120 seconds for all game logic. This chapter
will cover the accumulator pattern, spiral-of-death prevention, and render
interpolation.

Key files: `src/core/clock.hpp`, `src/core/game.cpp`.
