# Rendering Pipeline

<!-- TODO: expand with virtual resolution scaling details and sprite batching -->

Raven renders to a 480x270 virtual target and scales it to the window. Sprites are drawn from texture atlases, sorted by layer.

Key files: `src/rendering/renderer.hpp`, `src/rendering/sprite_sheet.hpp`, `src/ecs/systems/render_system.hpp`.
