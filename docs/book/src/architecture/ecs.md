# ECS with EnTT

<!-- TODO: expand with component design rationale and system ordering -->

Raven uses EnTT as its Entity Component System. All game state is stored in a single `entt::registry`, and systems are free functions that operate on views.

Key files: `src/ecs/components.hpp`, `src/ecs/systems/`.
