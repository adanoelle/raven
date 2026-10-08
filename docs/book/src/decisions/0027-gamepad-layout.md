# 27. Gamepad Layout

Date: 2026-10-08 Status: Accepted

## Context

Real playtests will be played on a controller, and the gamepad path had
never been tried. The layout came from the game's bullet-hell beginnings:
shoot on A, ability on B, melee on X, dash on the left bumper, and Focus
(slow movement) on the right bumper. In a twin-stick shooter the right thumb
stays on the aim stick, so with fire on A the player could not aim and
shoot at the same time. The Sharpshooter's hold-to-charge made it worse.
Focus was left over from that earlier design, and nothing in the game read
it.

The [pre-content review](../devlog/2026-10-08-pre-content-review.md) also
found input problems that a controller session would hit first:

- Mouse use was detected by comparing positions, so opening or resizing the
  window counted as moving the mouse. From then on, a centred aim stick
  aimed at a hidden cursor (G9).
- Buttons were read once per frame, so a tap shorter than a frame was lost.
  That matters at the 30-40 fps a Steam Deck is often capped to (G8).
- The game-over and victory screens said PRESS START, but Start did nothing
  there, and the fire button also confirmed, so holding fire through a death
  skipped the score (G10).
- The stick deadzone was applied to each axis separately, which zeroed the
  small component of a shallow diagonal and pulled movement onto the axes.

## Decision

**Combat actions go on the shoulders and triggers, so the right thumb never
leaves the aim stick; the face buttons repeat them.**

| Control      | Action                                             |
| ------------ | -------------------------------------------------- |
| Left stick   | Move (D-pad too)                                   |
| Right stick  | Aim                                                |
| RT           | Fire (held; the Sharpshooter holds to charge)      |
| RB           | Melee                                              |
| LB           | Dash                                               |
| LT           | Class ability                                      |
| A / X / B    | Dash / melee / ability, repeating the shoulders    |
| A / B        | Confirm / cancel in menus                          |
| Start        | Pause                                              |
| Back/Select  | Free, for the playtest marker ([ADR-0026](0026-record-and-replay-playtests.md)) |

- Melee gets the fastest digital button on the aiming hand, because
  disarming is the core loop. Dash sits on the left hand, which already
  steers it with the stick.
- Focus is removed.
- Bindings are tables, `GamepadLayout` and `KeyboardLayout` in
  `core/input.hpp`, with up to two controls per action. Each action is
  listed once, and polling, event latching and edge detection all loop over
  the same table. Presets, and later full remapping, are different tables.
  They aren't persisted yet: that comes with a presets or remapping screen,
  after `settings.json` loading is made tolerant of bad fields (review G12).
- Triggers are read as buttons with hysteresis: a pull past 0.5 presses, and
  coming back below 0.3 releases. A trigger resting near one threshold would
  otherwise flicker, firing a stray Sharpshooter shot on each release.
- The left stick uses one round deadzone (0.2) and rescales the travel beyond
  it, so diagonals keep both components and movement starts from zero. The
  right stick uses the same round deadzone, but passes its raw direction
  through, since only the direction matters for aim.
- Key, button and trigger presses are also latched from SDL events, so a tap
  that goes down and up between two polls still counts. A fire tap fires
  once even when the button is already up by the next tick.
- Only mouse motion or a click makes the mouse the aiming device.
- The end screens accept Start as well as confirm, and ignore input for
  0.75 s after appearing.

Steam Input on PC and Steam Deck, and the Switch's system settings, already
let players remap buttons. So in-game remapping can wait: presets first,
full remapping only if players ask for it.

## Consequences

**Positive:**

- Aiming, firing, melee, dash and ability can all happen at once, as in the
  rest of the genre.
- The layout is data. Presets such as southpaw or face-button fire, and a
  remapping screen later, change tables, not input code.
- The tests drive a virtual SDL gamepad. They check the layout, the trigger
  hysteresis, the round deadzone and tap latching without real hardware.

**Negative:**

- Players who prefer firing on a face button have to wait for presets, or
  remap with Steam Input.
- The A button is both dash (in play) and confirm (in menus), and B is both
  ability and cancel. That's fine while play and menus never read input at
  the same time; the end-screen delay covers the hand-off between them.
- Button prompts still name keys and buttons in text. Controller-specific
  glyphs (Xbox, PlayStation, Nintendo) are still to do.
