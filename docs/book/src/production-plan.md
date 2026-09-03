# Production Plan

What to work on, in what order, from where the game stands today to a
demo-quality vertical slice and then a release. Written for a solo
developer who is also the artist, so art and code steps are interleaved
rather than listed as separate tracks. Companion to the
[Switch Port Plan](guide/switch-port-plan.md), which runs alongside
Phase 4 and does not gate anything here.

The ordering rule throughout: **do the thing a reviewer notices first.**
Feel and identity are graded; breadth and progression are appreciated.
See the [Creative Vision](creative-vision.md) for the pillars every
step serves.

---

## Where Things Stand (2026-09-02)

**Solid:** the combat loop (shoot, melee disarm, dash, weapon
steal/decay/stabilize), three mechanical classes, four enemy
archetypes, data-driven stages and patterns, LDtk tilemaps with
collision, HUD, pause/options/victory/game-over flow, persisted
settings and best score, CI on Linux and Windows, ~120 tests, and a
console-ready core after the readiness pass.

**Placeholder:** every sprite except the knight's blocked-in sheet, the
tileset (four 16px tiles), the font atlas, and all seven sound effects
(synthesised by `tools/gen_sfx.py`).

**Missing outright:**

- Stages 2 and 3 reference LDtk levels `Room_02` and `Room_03` that do
  not exist; they load no tilemap and play in an open field.
- No music, and no streaming path in `AudioEngine` to play any.
- The disarm — the signature reward — has no sound. Sixteen of the
  specced effects are unshipped ([audio spec §2](audio-spec.md#2-sound-effects-catalog)).
- The explosion, hit spark, slash arc, and concussion ring VFX have no
  art and, for the explosion, no renderer
  ([game feel §3](game-feel.md#3-event-catalog)).
- Room exits are invisible logic.
- The `pickups` sheet does not exist; weapon and stabilizer drops draw
  as grey rectangles.

---

## Phase 0 — Immediate Art (the next two to three weeks)

These are the pieces that turn the current build from "engine demo"
into something that looks like a game in a screenshot. Each lands in
the build the day it is finished; nothing waits for a batch.

### 0.1 Finish the knight's idle and walk

`art/characters/knight/knight.aseprite` is tagged and centred with a
blocking-pass bob. Draw the real poses per the
[blocking page](art/walk-cycle-blocking.md): leg split on contacts,
counter-swing, hair lag one frame late. Export with the guide's
command; nothing in code changes.

While in the file, close the knight's open questions on the
[character page](art/characters/knight.md): palette hex values,
secondary colour element, sword colour, and the stolen-weapon colour
shift. Those answers gate every later row.

**Done when:** the walk reads at 1x in a real room and the character
page has final hex values.

### 0.2 Knight action rows, in screen-time order

Attack (4f), dodge (3f), hurt (2f), death (5f), per the
[action animation guide](art/action-animation.md). Melee and dash
currently reuse the walk row at faster timings; when the attack and
dodge rows land, `game_scene.cpp` switches to them — a small, known
code change, flag it when the rows are in.

**Done when:** all six rows are in `knight.png` and the walk-row reuse
is gone from the code.

### 0.3 Pickups sheet

The cheapest visible win in the game: weapon drop, stabilizer, and the
explosion frames, on one `pickups.png` (16x16 frames) registered in
`config.json`. The stabilizer must read as *rarer* than a weapon.

**Done when:** no grey rectangles appear in a run.

### 0.4 The grunt enemy

The Chaser is the most common thing on screen after the player. One
24x24 sheet: idle (4f), walk (4–6f), death (3–4f), plus a wake-up
tell frame. The Drifter can share the base with a colour and accessory
change. Both replace `enemies.png`.

**Done when:** a stage-1 wave is entirely final art except the tiles.

### 0.5 Biome 1 tileset and the two missing rooms

A real 16px tileset for the first environment (the current `Dungeon`
set is four placeholder tiles), then author `Room_02` and `Room_03` in
LDtk so stages 2 and 3 actually have walls. This fixes a live bug and
produces the first screenshot-worthy frame at the same time.

**Done when:** all three stages load a tilemap and stage 1 is not
`Test_Room`.

### 0.6 Stalker mid and Coward boss

32x32 and 48x48 sheets. The boss needs an entrance pose and a
multi-frame death. These are last in Phase 0 because they are on screen
least; they can slip into Phase 2 without hurting the slice.

---

## Phase 1 — Feel (code and audio, interleaved with Phase 0)

The engine already emits every event; this phase gives them sound and
light. Work these on days when the art needs to rest.

1. **Decide the sonic identity.** [Audio spec §1](audio-spec.md#1-sonic-identity-undecided)
   is still marked undecided. Everything below depends on it; the
   decision is a paragraph, not a project.
2. **The disarm sound**, then `melee_connect`, `decay_warning`,
   `decay_explosion`, `stabilizer`, `player_death`, `wave_clear`,
   `enemy_shoot`. File names are the contract with `config.json`;
   each new `Sfx` enum value is a three-line code change.
3. **Explosion renderer + hit flash.** The decay explosion spawns an
   entity nothing draws. Wire `ExplosionVfx` to the pickups-sheet
   frames; add the brief white flash on enemy hit.
4. **Visible exits and a spawn-in effect.** Exits open when the room
   clears; enemies should not pop from nothing.
5. **Music streaming path** in `AudioEngine` (ADR-0019 left the door
   open; `music_volume` is already wired), then the first two tracks
   in the [track list's](music-track-list.md#2-priority-order) order:
   area 1, then title.
6. **Ability VFX**: slash arc, ground-slam shockwave, concussion ring
   ([ADR-0012](decisions/0012-concussion-shot-vfx.md)), dash trail.

**Done when:** a playtester can name what happened in a fight with
their eyes closed, and the disarm is the sound they remember.

---

## Phase 2 — Content to a Complete Biome

One environment, finished to the standard of the knight, is worth more
than three at placeholder quality. Target for the slice:

- **Rooms:** six to eight LDtk rooms for biome 1 with varied layouts
  (open, pillared, corridor, arena), each with `PlayerStart` and
  `Exit` spawns. Stage JSON per room; a run is a sequence of them.
- **Patterns:** from four to roughly twelve, covering each archetype
  with an easy and a hard variant. All JSON.
- **Enemy variants:** one variant per archetype (colour, speed, pattern)
  so the four sheets stretch to eight enemies.
- **Boss:** one multi-phase fight for the Coward, with an entrance
  and a phase-change tell.
- **Balance pass** using the [playtest playbook](playtest.md): wave
  pacing, HP and damage curves, tuned for a fresh save at 1.0x
  (the [ADR-0023](decisions/0023-salvage-materials-engineer-upgrades.md)
  baseline rule applies whether or not that ADR is ever adopted).

**Done when:** a stranger can play biome 1 start to finish, lose,
and immediately want to try again.

---

## Phase 3 — Validate the Slice

1. **Playtesting rounds** with people who have never seen the game.
   Watch, do not explain. Log in the devlog.
2. **Key rebinding** — the last open input item from the engine
   review — and, with it, Steam Input action sets.
3. **Demo build**: CPack ZIP on Windows, copied to a clean machine,
   run without a terminal (`WIN32_EXECUTABLE` for Release).
4. **Capture material**: with biome 1 final, take the screenshots and
   the 30-second clip the store page needs. This is the earliest point
   at which marketing assets are honest.

**Done when:** the demo has been played by ten people who are not you
and the top three complaints are fixed.

---

## Phase 4 — Breadth

Only after the slice validates:

- Biomes 2 and 3 (tileset + rooms + one new enemy each) and the
  matching area tracks.
- Second and third bosses.
- Converge the mechanical classes with the visual roster: the vision
  calls for about five characters, each owning one signature colour;
  the engine ships three mechanics today. Each new character is one
  sheet plus one recipe.
- Progression, if the slice shows players want it:
  [ADR-0014](decisions/0014-npc-shop-progression.md) shop rooms or
  [ADR-0023](decisions/0023-salvage-materials-engineer-upgrades.md)
  meta layer, in that order of cost.
- The [Switch Port Plan](guide/switch-port-plan.md) steps 3–6 run in
  parallel here, gated on SDK access.

---

## Phase 5 — Release

- Steam partner account and app ID (unblocks real Steamworks
  verification per [ADR-0021](decisions/0021-optional-steamworks.md)).
- Store page: capsule art, screenshots, trailer — all from Phase 3
  capture material plus one piece of key art.
- Achievements design; extend the `Steam` wrapper.
- macOS build check; packaging dry runs for every target.
- Steam Next Fest submission with the biome-1 demo.

---

## Decisions Needed Now

These block work in Phases 0–1 and cost nothing but a choice:

| Decision | Blocks | Where it lives |
| -------- | ------ | -------------- |
| Sonic identity (chiptune, hybrid, or acoustic-leaning) | every sound and track | audio spec §1 |
| Knight palette hex values and secondary colour | knight action rows, enemy palettes | knight page |
| Biome 1 theme (the placeholder set is "dungeon") | tileset, rooms, area-1 track mood | this page |
| Whether the Knight stays a third mechanical class or the roster converges to five with mechanics reassigned | class art after the knight | creative vision, Characters |

---

## Cadence

A pattern that fits a solo art-plus-code project: art in the fresh
hours, one code or audio item from Phase 1 when the eyes are tired,
and every finished piece into the build the same day. Update the
[tracking issue](https://github.com/adanoelle/raven/issues/14) as
phases complete; write a devlog entry when a phase closes, not per
task.
