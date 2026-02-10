# Scene Management

<!-- TODO: expand with scene lifecycle diagrams and transition patterns -->

Raven uses a stack-based scene manager. The top scene receives updates and
renders. Push adds overlays (like pause menus), swap transitions between states.

Key files: `src/scenes/scene.hpp`, `src/scenes/title_scene.hpp`,
`src/scenes/game_scene.hpp`.
