# Scene Management

Raven uses a stack-based scene manager to control game state transitions. The
`SceneManager` holds a stack of `Scene` objects; the top scene receives `update`
calls each frame, while **all** scenes render bottom-up (so overlays like the
pause menu can draw over a frozen gameplay frame). Push adds overlays, swap
replaces the top scene for state transitions, and pop removes the top scene to
return to whatever is beneath it. Each scene receives `on_enter` and `on_exit`
lifecycle callbacks when it becomes active or is removed.

## Deferred transitions

Transitions requested from inside a scene's own `update()` are **queued and
applied after the update returns** (see
[ADR-0015](../decisions/0015-deferred-scene-transitions.md)). Popping a scene
destroys it; applying a `swap()` immediately would free the scene whose
`update()` is still on the call stack — a use-after-free on every menu
transition. `SceneManager` tracks an `updating_` flag: while it is set,
`push`/`pop`/`swap` append to a pending-operation queue that `update()` drains
in order afterwards. Outside of `update()` (e.g. the initial push in
`Game::init`, or `clear()` during shutdown), operations apply immediately.

Multiple queued operations compose: the pause menu's "quit to title" requests
`pop()` (remove the overlay) followed by `swap()` (replace the `GameScene`
beneath), and both apply in order.

`SceneManager::clear(game)` pops every scene with `on_exit` callbacks. It runs
in `Game::shutdown()` **before** the renderer is destroyed, because scene
destructors free SDL textures (tilemaps) that must not outlive the renderer.

## Scene flow

```
TitleScene  ──(confirm)──>  CharacterSelectScene  ──(confirm)──>  GameScene
                                                                   │      ^
                                                              (pause)  (resume)
                                                                   │      │
                                                                   v      │
                                                                  PauseScene
                                                                      │
                                                              (quit to title)
                                                                      │
TitleScene  <──(confirm)──  GameOverScene  <──(game over)──  GameScene
```

On victory (final stage cleared + exit overlap), `GameScene` swaps directly back
to `TitleScene`. On death, it swaps to `GameOverScene`, which then swaps to
`TitleScene` on confirm. Pause pushes `PauseScene` as an overlay.

## TitleScene

Entry point for the game. Renders the RAVEN title and a blinking "PRESS START"
prompt with the bitmap font (see
[Rendering Pipeline](rendering.md#bitmap-font-text)) on a dark purple
background. On confirm input, swaps to `CharacterSelectScene`.

## CharacterSelectScene

Lets the player choose between Brawler and Sharpshooter. Renders two coloured
boxes (red for Brawler, blue for Sharpshooter) with class name labels, centred
on the 480x270 virtual resolution. Left/right input toggles selection; the
active box and label are bright, the inactive ones are dimmed. A blinking
underline and "PRESS CONFIRM" prompt guide the player. On confirm, passes the
selected `ClassId::Id` to `GameScene`'s constructor and swaps.

## GameScene

Main gameplay scene. Owns the tilemap, pattern library, stage loader, and system
execution order. On enter, spawns the player entity with universal components,
applies the chosen class recipe, loads the stage manifest, and enters the first
room. Each tick runs the full ECS system pipeline. Handles room transitions (via
exit overlap), wave progression, and game-over detection. On exit, clears the
registry.

## PauseScene

Overlay pushed on top of `GameScene` when pause is pressed (Esc / Start). While
it is on top, `GameScene` stops updating — only the top scene receives
`update()` — but keeps rendering beneath, because the scene stack renders
bottom-up. `PauseScene::render` deliberately does **not** clear; it draws a
translucent black layer over the frozen frame, then the menu.

Two subtleties:

- **Interpolation freeze** — `GameScene::render` checks
  `game.scenes().is_top(this)` and snaps interpolation alpha to 1.0 when
  covered by an overlay. Positions stop changing while paused, but the clock's
  accumulator keeps cycling; a varying alpha would make sprites shimmer between
  their previous and current tick positions.
- **Menu navigation** — movement axes are held state (not edge-detected), so
  the scene tracks the previous `move_y` value itself and moves the selection
  on threshold crossings.

Resume pops the overlay; quit-to-title pops and then swaps the `GameScene`
beneath to `TitleScene` (two queued operations, applied in order).

## GameOverScene

Displays "GAME OVER", the final score, and a blinking restart prompt on a dark
red background. Captures the score from `GameState` in `on_enter`. On confirm,
swaps to `TitleScene`. On exit, clears the registry and erases `GameState` from
context. (`GameScene::on_enter` also erases any stale `GameState`, because the
victory path bypasses this scene entirely.)

## Key files

| File                                    | Role                                    |
| --------------------------------------- | --------------------------------------- |
| `src/scenes/scene.hpp`                  | `Scene` interface, `SceneManager` stack |
| `src/scenes/scene_manager.cpp`          | Deferred transition queue               |
| `src/scenes/title_scene.hpp/.cpp`       | Title screen                            |
| `src/scenes/char_select_scene.hpp/.cpp` | Character selection                     |
| `src/scenes/game_scene.hpp/.cpp`        | Main gameplay, system wiring            |
| `src/scenes/pause_scene.hpp/.cpp`       | Pause overlay                           |
| `src/scenes/game_over_scene.hpp/.cpp`   | Game over screen                        |
