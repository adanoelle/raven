# Creative Vision

What Raven is, why it looks and sounds the way it does, and the pillars every
piece of work — code, art, or music — should serve. This page states
direction; the specifics live in the linked specs.

---

## The Game in Three Sentences

Raven is a pixel art action roguelike in the lineage of
[Blazing Beaks](https://blazingbeaks.com/): twin-stick combat through
handcrafted rooms, where the weapons worth having are the ones you take from
your enemies — and everything you take is trying to leave. Runs are short,
losses are fair, and mastery is visible. It is built from scratch in C++ on a
small, deliberate engine so the game can go wherever SDL goes.

## The World

The game takes place in **the ruins of an ancient runic civilization**, long
overgrown, now repurposed as shelter by people hiding from a new alien
threat.

The emotional target is **"beautiful aftermath"** — not bleak
post-apocalypse. Think Studio Ghibli backgrounds and Zelda overworlds: there
is sadness in the ruins, but also beauty in how thoroughly nature has
embraced them. Runic stone still glows faintly with residual energy; vines
drape the architecture; people have strung warm lights through cold tunnels.
The world has moved on, and it is still worth fighting for.

Four environment moods carry a run (detailed in the
[Tileset Palette Design](art/tileset-palette.md)):

| Environment          | Mood                                                  |
| -------------------- | ----------------------------------------------------- |
| Dark Ruins           | Cool and compressed; runic glow is the light source   |
| Overgrown Ruins      | Warm, dappled, lush; nature winning                   |
| Open Daylight        | Bright, exposed, weathered; the widest value range    |
| Underground Shelter  | Ancient stone meets makeshift human warmth            |

## Design Pillars

Every feature, sprite, and track should be traceable to one of these.

### 1. Risk is the economy

The core loop is built on tempting, expiring power. Stolen weapons decay on a
timer; stabilizers make them permanent — unless the weapon is Legendary, in
which case the gamble never closes. Melee disarms an enemy instead of
killing it outright ([ADR-0008](decisions/0008-melee-disarm-over-death-drops.md)),
trading safety for reward. If a mechanic offers something for nothing, it
doesn't belong.

### 2. Readable chaos

The screen can be full of bullets and the player must never be confused. This
drives nearly every visual rule: the 480x270 canvas
([ADR-0003](decisions/0003-virtual-resolution-480x270.md)),
silhouette-first character design, black outlines on characters, the strict
color hierarchy (muted environments, saturated actors, glowing projectiles —
see [Color Theory](art/color-theory.md)), and the rule that runic scenery
glow must never share a hue with enemy fire. Audio follows the same law:
every gameplay event has a distinct, immediate sound.

### 3. Tight and fair

Deaths must always feel like the player's fault. Game logic runs at a fixed
120 Hz ([ADR-0002](decisions/0002-fixed-timestep-120hz.md)) so dense bullet
patterns behave deterministically; input edges are never dropped;
invulnerability windows are generous and consistent. "It felt unfair" is a
bug report, even when the code is correct.

### 4. Short runs, rising intensity

A full run is a single sitting. Stages escalate in enemy density, pattern
complexity, and — deliberately — music tempo (see the
[Music Track List](music-track-list.md)). A player should sense how deep
they are with their eyes closed.

## Characters

Playable characters are **mechanical recipes plus a visual identity**. The
engine currently ships two classes — the **Brawler** (melee-forward: Ground
Slam, heavy knockback) and the **Sharpshooter** (ranged precision: charged
piercing shots, Concussion Shot) — while the visual roster is being designed
art-first, beginning with the [Knight](art/characters/knight.md): fast and
armored, more Saber than slow medieval tank.

The mechanical classes and the visual roster will converge as designs are
finalized; the constant is the design rule from the
[color system](art/color-theory.md): a closed roster of about five
characters, each owning one signature color, all verified readable in every
environment.

## Sound

The sonic direction is **deliberately still open** — three candidate
identities (chiptune, modern electronic, hybrid) are laid out in the
[Audio Specification](audio-spec.md) so the decision can be made with a
composer, not before one. What is already fixed:

- The *feel* targets per context: brooding title, confidence-building first
  area, escalating tension, triumphant payoff — specified per track in the
  [Music Track List](music-track-list.md).
- SFX are short, punchy, and functional first: they confirm hits, warn of
  danger, and never smear into noise (the engine dedupes and caps voices to
  guarantee it).

## Touchstones

| Influence           | What we take from it                                    |
| ------------------- | ------------------------------------------------------- |
| Blazing Beaks       | The loop: twin-stick rooms, risk-reward item economy    |
| Nuclear Throne      | Intensity and escalation; chaos that stays readable     |
| Enter the Gungeon   | Weapon variety with strong individual identity          |
| Celeste             | Animation economy; emotional range on a small canvas    |
| Hyper Light Drifter | Atmosphere; a beautiful, wordless, ruined world         |
| Studio Ghibli       | Backgrounds: nature reclaiming, warmth inside ruin      |

## Where to Go Next

- **Artists** — the [Art Specification](art/art-spec.md) is the contract;
  the [Aseprite guide](art/art-aseprite-guide.md) is the workflow.
- **Musicians** — the [Music Track List](music-track-list.md) and
  [Audio Specification](audio-spec.md).
- **Developers** — the [Architecture Overview](architecture/overview.md).
- **The "why" behind any technical choice** — the
  [decision records](decisions/0001-cpp20-entt-sdl2.md), one page per
  decision.
