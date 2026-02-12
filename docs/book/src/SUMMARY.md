# Summary

[Introduction](introduction.md)

---

# Getting Started

- [Development Environment](guide/environment.md)
- [Building and Running](guide/building.md)
- [Project Tour](guide/project-tour.md)

---

# Architecture

- [Overview](architecture/overview.md)
- [Game Loop and Fixed Timestep](architecture/game-loop.md)
- [ECS with EnTT](architecture/ecs.md)
- [Rendering Pipeline](architecture/rendering.md)
- [Input Handling](architecture/input.md)
- [Aiming and Shooting](architecture/shooting.md)
- [Weapons and Pickups](architecture/weapons.md)
- [Melee Attack and Dash](architecture/melee-and-dash.md)
- [Room Progression and Waves](architecture/room-progression.md)
- [Smooth Character Movement](architecture/smooth-movement.md)
- [Sprite Animation](architecture/sprite-animation.md)
- [Tilemaps and LDtk Integration](architecture/tilemaps.md)
- [Scene Management](architecture/scenes.md)
- [Player Classes](architecture/player-classes.md)

---

# Playtesting

- [Playtest Playbook](playtest.md)

---

# Art & Animation

- [Art Specification](art-spec.md)
- [Art Integration Guide](art-integration.md)
- [Learning Resources](art-resources.md)

---

# Audio & Sound

- [Audio Specification](audio-spec.md)
- [Audio Integration Guide](audio-integration.md)
- [Learning Resources](audio-resources.md)

---

# Development Log

- [Project Setup](devlog/2026-02-08-project-setup.md)
- [From Danmaku to Roguelike](devlog/2026-02-08-danmaku-to-roguelike.md)
- [Melee and Dash System](devlog/2026-02-10-melee-dash-system.md)
- [Room Progression and HUD](devlog/2026-02-11-room-progression.md)
- [Player Class System](devlog/2026-02-12-player-class-system.md)
- [SDL3 Migration](devlog/2026-02-12-sdl3-migration.md)

---

# Decisions

- [ADR-0001: C++20 with EnTT and SDL2](decisions/0001-cpp20-entt-sdl2.md)
- [ADR-0002: Fixed Timestep at 120Hz](decisions/0002-fixed-timestep-120hz.md)
- [ADR-0003: Virtual Resolution 480x270](decisions/0003-virtual-resolution-480x270.md)
- [ADR-0004: Data-Driven Patterns via JSON](decisions/0004-data-driven-patterns.md)
- [ADR-0005: No std::filesystem](decisions/0005-no-std-filesystem.md)
- [ADR-0006: No Third-Party Engine](decisions/0006-no-third-party-engine.md)
- [ADR-0007: Deferred Entity Destruction](decisions/0007-deferred-entity-destruction.md)
- [ADR-0008: Melee Disarm Over Death Drops](decisions/0008-melee-disarm-over-death-drops.md)
- [ADR-0009: Data-Driven Wave Definitions](decisions/0009-data-driven-wave-definitions.md)
- [ADR-0010: SDL_mixer for Audio](decisions/0010-sdl2-mixer-audio.md)
- [ADR-0011: Player Classes as Component Recipes](decisions/0011-class-as-component-recipe.md)
- [ADR-0012: Concussion Shot VFX](decisions/0012-concussion-shot-vfx.md)
- [ADR-0013: SDL3 Migration](decisions/0013-sdl3-migration.md)
