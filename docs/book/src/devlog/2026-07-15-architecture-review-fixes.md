# Architecture Review and Follow-up Fixes

Date: 2026-07-15 Tags: review, performance, audio, rendering, settings

## The Review

With Steam and (eventually) Nintendo as targets, the whole stack got a
design review: EnTT, SDL3, the no-engine decision, the fixed 120 Hz
timestep, and the dependency set.

**The verdict on the big choices: keep all of them.**

- **EnTT** is used idiomatically (free-function systems over views, POD
  components, registry-context singletons) and is console-proven. Nothing
  at this entity count challenges it.
- **SDL3** with `SDL_Renderer`, logical presentation, and
  `SDL_SCALEMODE_PIXELART` is the right abstraction height for 480x270
  pixel art — no direct graphics API use is what makes a console port
  tractable. NDA'd console ports of SDL exist for registered developers,
  so the Switch story is real. Watch-item: the SDL 3.4 floor is
  bleeding-edge; console SDK ports lag, so a `SDL_SCALEMODE_NEAREST`
  fallback is cheap insurance. SDL3_image is the one system (pkg-config)
  dependency — vendoring or replacing it would simplify cross-builds.
- **Native SDL3 audio** (ADR-0019) turned out better than the reviewer's
  own initial suggestion of SDL3_mixer: every dependency we don't have is
  one less port to line up.
- **The `AudioQueue` seam** (systems push enum events; the scene drains
  to the engine) keeps gameplay systems pure and testable and is the
  pattern to repeat for future engine-facing effects.

## What the Review Found

Ordered by impact; all fixed in this change unless noted:

1. **Same-tick SFX stacking.** N copies of one effect in a single tick
   played sample-aligned and summed to N-times amplitude — a distorted pop
   whenever a multi-pellet burst connected. The drain in `GameScene` now
   dedupes effects per tick with a bitmask.
2. **Unbounded audio voices.** Each `play()` binds a new stream with no
   cap. `AudioEngine` now enforces `MAX_VOICES` (32): it reaps finished
   streams first and drops the play if still saturated.
3. **String round-trips in hot paths.** The render loop resolved each
   sprite's interned `StringId` back to a `std::string` and did a
   string-keyed hash lookup per sprite per frame (same pattern per emitter
   per tick in the emitter/melee systems). `SpriteSheetManager` and
   `PatternLibrary` are now keyed by `StringId` directly; the string
   overloads remain as cold-path/test conveniences.
4. **Unstable sprite sort.** `std::sort` by layer only let equal-layer
   sprites swap draw order between frames (z-flicker) because view
   iteration order changes as entities churn. Now `std::stable_sort` by
   (layer, y) — which also gives top-down depth within a layer.
5. **Settings written on every slider step.** Dragging a volume slider
   wrote `settings.json` ten times. `apply_settings()` no longer persists;
   `OptionsScene::on_exit` saves once. Batching writes matters more on
   console save-data backends than on PC.
6. **Linear volume slider.** Perceived loudness is roughly logarithmic;
   volume now maps through a quadratic curve so the slider feels even.
7. **Interner overflow.** `StringInterner::intern` silently wrapped its
   `uint16_t` index past 65,535 strings, aliasing two strings to one ID.
   Now asserts in debug and returns the invalid sentinel in release.

## Still Open (tracked, not fixed here)

- **Bullet owner tags** (`PlayerBullet`/`EnemyBullet` instead of an enum
  filter) so collision views only touch their own population. No spatial
  broadphase until profiling asks for it.
- **Room transitions re-parse the LDtk file from disk**; cache the parsed
  project when maps grow.
- **SFX for GroundSlam and ConcussionShot** — the two active abilities
  are silent while dash/melee are not.
- **RNG distribution portability** — `std::uniform_real_distribution` is
  implementation-defined; hand-roll it if seeded runs/replays land.
- **Menu navigation is triplicated** across title/pause/options scenes;
  extract a cursor helper next time a menu is touched.
- Vendor or replace SDL3_image; `SDL_SCALEMODE_NEAREST` fallback.
