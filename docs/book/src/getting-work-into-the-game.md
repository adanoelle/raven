# Getting Work Into the Game

How a sprite sheet or an audio file becomes part of Raven. The short
version: name the file what the spec names it, and it drops into place.

---

## No Tooling Required

Nothing on the art or music side depends on the development environment,
the command line, or git. This book is published as a website precisely so
all of it is readable anywhere. If you *like* working in the repo,
everything works that way too — `just run` builds and launches the game —
but drawing and composing never require it.

## Reading Paths

Depending on the work at hand, the short path through the book:

**Art**

1. [Creative Vision](creative-vision.md) — what the game is and why it looks
   this way
2. [Art Specification](art/art-spec.md) — the reference for every size,
   format, and animation timing
3. [Aseprite Setup Guide](art/art-aseprite-guide.md) — the recommended
   workflow, template setup, and export settings
4. [Color Theory](art/color-theory.md) and
   [Tileset Palette Design](art/tileset-palette.md) — the palette system
   the work lives inside
5. [Game Feel](game-feel.md) and [Enemy Archetypes](enemy-archetypes.md) —
   what the mechanics ask of the art: which effects and animations exist to
   serve a gameplay moment, and the personality each enemy's movement
   demands

**Music and sound**

1. [Creative Vision](creative-vision.md) — especially the Sound section
2. [Music Track List](music-track-list.md) — the per-track brief: moods,
   tempo and key targets, loop rules
3. [Audio Specification](audio-spec.md) — sound-effect catalog, formats,
   loudness targets, naming
4. [Game Feel](game-feel.md) — when each sound triggers in play and what it
   must communicate

## File Names Do the Wiring

The single most important convention: **files carry the exact names the
specs define** (`mus_area_01.ogg`, `shoot.wav`, `player.png`, …). The game
finds assets by name, so a correctly named file drops into place with zero
code changes — and a renamed one doesn't.

Everything currently in the game (synthesized sound effects, placeholder
sprites) is designed to be *replaced* this way: new work supersedes a
placeholder just by having the right name. Brand-new sounds and sprites
that don't replace an existing placeholder also need a one-line
registration in the manifest — agree on the name first, and whoever is at
the keyboard adds the line.

## When a Piece Is Ready

Wherever a piece comes from — a shared drive, a branch, a folder on the
studio machine — three things travel with it:

1. **The file**, named per the spec.
2. **The spec's pre-flight list, run through once.** Every spec ends with
   one ([art](art/art-spec.md), [music](music-track-list.md#4-file-format),
   [SFX](audio-spec.md)). It catches the round-trip mistakes — wrong export
   settings, off-grid frames, clipped loop tails — before anyone hits them
   in-engine.
3. **A note on anything intentional but non-obvious** — e.g. "loop point is
   at sample 441000, tagged in the file" or "the cape intentionally breaks
   the body zone during the dash".

## Looking at It in Context

New work gets judged in the game, not just in the editor: sprites on the
darkest and brightest rooms, music looping for ten minutes, effects
layered over full combat. Notes tie back to the spec ("gold washes out on
the daylight palette — see the readability tests in the knight page"), not
vibes, and a piece iterates as many rounds as it needs before it's
committed and appears in the next build.

Playable builds are always available — ask for the current one to check
work in motion, and the [Playtest Playbook](playtest.md) is a structured
way to poke at it.

## When the Spec Is Wrong

Specs are references, not scripture — if a rule fights the work (a
silhouette that needs one more pixel of overflow, a track that wants a
slower tempo than the target), say so. The specs get amended when the work
teaches us something; several sections of this book exist because an
earlier draft was wrong.
