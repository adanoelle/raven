# 15. Deferred Scene Transitions

Date: 2026-07-03 Status: Accepted

## Context

`SceneManager` holds scenes in a stack of `std::unique_ptr<Scene>`. Every
transition in the game is requested from *inside* the current scene's own
`update()` — the title screen swaps to character select on confirm, the game
scene swaps to game over on death, and so on.

`swap()` was implemented as an immediate `pop()` + `push()`. Popping destroys
the top scene — the very object whose `update()` is still executing on the
call stack. The rest of that `update()` then ran on a freed `this`:
use-after-free on every menu transition. It only appeared to work because
every call site happened to `return` immediately after requesting the swap;
one added line after any `swap()` call, or an allocator that reuses freed
memory eagerly, would corrupt the heap. AddressSanitizer flags it instantly.

The same lifetime problem family also appeared at shutdown: `Game::shutdown()`
destroyed the SDL renderer while the scene stack was still alive, so scene
destructors later freed `SDL_Texture`s through a dead renderer (crash on
quit-mid-game).

## Decision

**Scene transitions requested during `update()` are queued and applied after
the update returns.**

`SceneManager` sets an `updating_` flag around the top scene's `update()`
call. While the flag is set, `push()`/`pop()`/`swap()` append a `PendingOp` to
a queue instead of mutating the stack; `update()` drains the queue in order
afterwards. Outside of update (initial push in `Game::init`, `clear()` during
shutdown) operations still apply immediately, so `on_enter`/`on_exit`
lifecycle timing is unchanged for those callers.

Queued operations compose in request order — e.g. the pause menu's
quit-to-title requests `pop()` (remove the overlay) then `swap()` (replace the
GameScene beneath it), and both apply sequentially.

Additionally, `SceneManager::clear()` pops all scenes with proper `on_exit`
callbacks, and `Game::shutdown()` calls it **before** destroying the renderer
so scene-owned SDL resources are released while the renderer still exists.

## Consequences

**Positive:**

- Scene code may request a transition at any point in `update()` without
  lifetime hazards; the "must return immediately after swap" landmine is gone
- Overlay flows (pause) can queue multi-step transitions safely
- Clean shutdown from any scene, including mid-run

**Negative:**

- A transition takes effect after the current tick rather than instantly; the
  requesting scene's `update()` runs to completion first (in practice all call
  sites return immediately anyway)
- `render()` runs once more on the old scene in the same frame the transition
  was requested — invisible in practice, but worth knowing when debugging
