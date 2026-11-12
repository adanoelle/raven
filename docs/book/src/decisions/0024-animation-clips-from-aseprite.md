# 24. Animation Clips from Aseprite Tag Data

Date: 2026-10-08 Status: Accepted

## Context

Animation timing lived in two places. Artists set frame durations and frame
tags in Aseprite, and the export command threw that data away: only the PNG
was written. The game then re-typed the same information as a switch in
`GameScene::update` (row, first and last frame, seconds per frame, loop
flag) for the player only. Enemies never animated, adding a state meant
editing C++, and a new character's timing could only be changed by a
programmer. The [pre-content review](../devlog/2026-10-08-pre-content-review.md)
named this the biggest thing slowing art work.

Two sources were possible: Aseprite's own JSON export, or clip definitions
typed into `config.json`. Typed definitions need no export step, but keep
the timing in two places that drift apart.

## Decision

**Aseprite's JSON export is the source of animation data.** One frame tag
becomes one clip.

- `just export-art` (`tools/export_art.sh`) exports every
  `art/**/*.aseprite`, except templates and sketches, to
  `assets/sprites/<name>.png` and `<name>.json`. It uses one row per tag
  (`--sheet-type rows --split-tags`), the json-array data format, and
  `--list-tags`, and never exports a layer named `guides`.
- A sheet's `config.json` entry names its data with an optional
  `"animations"` path. `AnimationLibrary`, kept in the registry context,
  loads each export. Every frame must be a cell of the sheet's grid; frame
  durations come from Aseprite; the tag's direction (forward, reverse,
  pingpong, pingpong_reverse) is applied at load; and the tag's repeat count
  makes a one-shot (repeat 1 plays once and holds the last frame).
- `Animation` names the clip it plays. `update_animation` advances it and
  writes the frame's column and row into the `Sprite`. A one-shot clip
  reports `finished` only after its last frame has shown for its full
  duration.
- Tag names are the contract between art and code: `idle`, `walk`,
  `attack` (melee and ground slam) and `dash`. `update_player_animation`
  picks the player's clip from what they are doing. When a sheet lacks a
  tag, it falls back to walk, then idle, and logs the missing tag once.
  Enemies play `idle` when their sheet has it.
- Placeholder sheets made by `tools/gen_placeholder_sprites.py` get JSON in
  the same format, so the engine has one path for both.
- `tests/test_content.cpp` checks each export against its PNG: same size,
  every frame inside the image, and `idle` and `walk` on every player
  character sheet.

## Consequences

**Positive:**

- Timing set in Aseprite is the timing in the game, including per-frame
  holds. Changing it is an export, not a code change.
- New states and new animated characters need only tags and an export.
- Enemies animate as soon as their sheet has an `idle` tag.
- The one-shot hold now lasts the final frame's full duration, fixing
  follow-through frames that showed for a single tick.

**Negative:**

- Aseprite is needed to change real art's timing. Placeholders don't need
  it.
- The export JSON records the Aseprite version, so re-exporting with a newer
  Aseprite changes a line in every data file.
- Until the knight has `attack` and `dash` tags, its actions play the walk
  clip, not the old fast walk-row reuse. The log names the missing tags.
