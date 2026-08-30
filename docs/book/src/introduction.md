# Introduction

**Raven** is a pixel art roguelike inspired by
[Blazing Beaks](https://blazingbeaks.com/): twin-stick combat through the
overgrown ruins of an ancient civilization, where the best weapons are stolen
from your enemies and every one of them is on a timer. It is built from
scratch in C++20 on SDL3 — no engine, one small codebase.

Raven is in active development. The core game is playable end to end — two
character classes, three stages, persistent high scores, Windows and Linux
builds — and the current focus is content: rooms, enemies, art, and music.
The [Development Log](devlog/2026-02-08-project-setup.md) tracks progress as
it happens.

## Who This Book Is For

This book is the single reference for everyone working on Raven. Different
readers need different doors:

| You are…                        | Start here                                                                 |
| ------------------------------- | -------------------------------------------------------------------------- |
| An **artist** or **musician**   | [Creative Vision](creative-vision.md), then [Getting Work Into the Game](getting-work-into-the-game.md) — no coding or build tools required |
| A **developer**                 | [Development Environment](guide/environment.md), then the [Architecture Overview](architecture/overview.md) |
| A **playtester**                | The [Playtest Playbook](playtest.md)                                        |
| Curious how decisions were made | The [Development Log](devlog/2026-02-08-project-setup.md) and the [decision records](decisions/0001-cpp20-entt-sdl2.md) |

If you only read one page, read the [Creative Vision](creative-vision.md) —
it explains what the game is trying to be, which is context for everything
else here.

## How This Book Is Written

Each chapter is written as the feature it describes is implemented, so the
book grows with the project. Three kinds of pages have distinct jobs:

- **Specifications** (art, audio) are contracts: exact sizes, formats, and
  delivery checklists.
- **Architecture chapters** explain how a system works and why, with
  references to the real source files.
- **Decision records (ADRs)** are frozen history — each captures why a choice
  was made at the time, including choices later superseded.

## Source Code

The full source is available on
[GitHub](https://github.com/adanoelle/raven). Architecture chapters reference
specific files so you can read along with the code; the technology stack is
summarized in the [Architecture Overview](architecture/overview.md).
