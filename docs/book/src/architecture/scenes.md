# Scene Management

Raven uses a stack-based scene manager to control game state transitions. The
`SceneManager` holds a stack of `Scene` objects; the top scene receives `update`
and `render` calls each frame. Push adds overlays (like a pause menu), swap
replaces the top scene for state transitions, and pop removes the top scene to
return to whatever is beneath it. Each scene receives `on_enter` and `on_exit`
lifecycle callbacks when it becomes active or is removed.

## Scene flow

```
TitleScene  ──(confirm)──>  CharacterSelectScene  ──(confirm)──>  GameScene
                                                                      │
                                                                 (game over)
                                                                      │
                                                                      v
TitleScene  <──(confirm)──  GameOverScene
```

On victory (final stage cleared + exit overlap), `GameScene` swaps directly back
to `TitleScene`. On death, it swaps to `GameOverScene`, which then swaps to
`TitleScene` on confirm.

## TitleScene

Entry point for the game. Renders a dark purple background with a placeholder
title rectangle and a blinking white "press start" prompt. On confirm input,
swaps to `CharacterSelectScene`.

## CharacterSelectScene

Lets the player choose between Brawler and Sharpshooter. Renders two coloured
boxes (red for Brawler, blue for Sharpshooter) centred on the 480x270 virtual
resolution. Left/right input toggles selection; the active box is bright, the
inactive one is dimmed. A blinking underline and "press confirm" prompt guide
the player. On confirm, passes the selected `ClassId::Id` to `GameScene`'s
constructor and swaps.

## GameScene

Main gameplay scene. Owns the tilemap, pattern library, stage loader, and system
execution order. On enter, spawns the player entity with universal components,
applies the chosen class recipe, loads the stage manifest, and enters the first
room. Each tick runs the full ECS system pipeline. Handles room transitions (via
exit overlap), wave progression, and game-over detection. On exit, clears the
registry.

## GameOverScene

Displays the final score and a blinking restart prompt on a dark red background.
Captures the score from `GameState` in `on_enter`. On confirm, swaps to
`TitleScene`. On exit, clears the registry and erases `GameState` from context
for a fresh start.

## Key files

| File                                    | Role                                    |
| --------------------------------------- | --------------------------------------- |
| `src/scenes/scene.hpp`                  | `Scene` interface, `SceneManager` stack |
| `src/scenes/title_scene.hpp/.cpp`       | Title screen                            |
| `src/scenes/char_select_scene.hpp/.cpp` | Character selection                     |
| `src/scenes/game_scene.hpp/.cpp`        | Main gameplay, system wiring            |
| `src/scenes/game_over_scene.hpp/.cpp`   | Game over screen                        |
