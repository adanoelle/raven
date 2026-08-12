# Handoff Guide for Artists and Musicians

How to contribute art or audio to Raven **without touching code, git, or
build tools**. If you can produce the files, we can take it from there.

---

## What You Don't Need

- You do **not** need to install the development environment, compile the
  game, or use the command line.
- You do **not** need a GitHub account or any git knowledge (if you have
  them, great — but they're optional).
- You do **not** need to place files into the project yourself. Integration
  is our job.

This entire book is published as a website precisely so you can read
everything without any tooling.

## Read This First

Depending on your discipline, this is the short reading path:

**Artists**

1. [Creative Vision](creative-vision.md) — what the game is and why it looks
   this way
2. [Art Specification](art/art-spec.md) — sizes, formats, animation timing,
   and the delivery checklist. **This is the contract.**
3. [Aseprite Setup Guide](art/art-aseprite-guide.md) — the recommended
   workflow, template setup, and export settings
4. [Color Theory](art/color-theory.md) and
   [Tileset Palette Design](art/tileset-palette.md) — the palette system
   your work needs to live inside

**Musicians / sound designers**

1. [Creative Vision](creative-vision.md) — especially the Sound section
2. [Music Track List](music-track-list.md) — the per-track brief: moods,
   tempo and key targets, loop rules. **This is the contract for music.**
3. [Audio Specification](audio-spec.md) — sound-effect catalog, formats,
   loudness targets, naming

## File Names Are the Contract

The single most important convention: **deliver files under the exact names
the specs define** (`mus_area_01.ogg`, `sfx_player_shoot.wav`,
`player.png`, …). The game finds assets by name, so a correctly named file
drops into place with zero code changes — and a renamed one doesn't.

Everything currently in the game (synthesized sound effects, placeholder
sprites) is designed to be *replaced* this way. Your work supersedes a
placeholder just by having the right name.

## Delivering Work

The delivery channel (shared drive, email, file transfer, or a git workflow
if you prefer one) is agreed individually with each contributor — ask and
we'll set one up. Whatever the channel, each delivery should include:

1. **The files**, named per the spec.
2. **The relevant checklist, checked.** Every spec ends with a delivery
   checklist ([art](art/art-spec.md),
   [music](music-track-list.md#4-delivery),
   [SFX](audio-spec.md)). Running through it before sending saves everyone
   a round trip.
3. **A note on anything intentional but non-obvious** — e.g. "loop point is
   at sample 441000, tagged in the file" or "the cape intentionally breaks
   the body zone during the dash".

## What Happens Next

1. **Review** — we check the delivery against the spec and, more
   importantly, in context: sprites on the darkest and brightest backgrounds,
   music looping for ten minutes, effects layered over combat.
2. **Feedback** — you get specific notes tied to the spec ("gold washes out
   on the daylight palette — see the readability tests in the knight page"),
   not vibes.
3. **Iterate** — as many rounds as the piece needs.
4. **Integration** — we commit the files into the project. Your work appears
   in the next build.

## Seeing Your Work in the Game

You never need to build the game yourself — we provide playable builds.
Ask for the current build when you want to check your work in motion, and
use the [Playtest Playbook](playtest.md) if you'd like a structured way to
poke at it.

## When Something Is Unclear

Specs are contracts, but they're not scripture — if a rule fights the work
(a silhouette that needs one more pixel of overflow, a track that wants a
slower tempo than the target), say so. The specs get amended when the work
teaches us something; several sections of this book exist because an earlier
draft was wrong.
