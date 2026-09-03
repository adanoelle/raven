# Art Files and Organization

Where art lives in the repository, how files are named, and how the
record of decisions grows as work accumulates. The goal is that anyone
can find the source for any sprite in the game, and the *why* behind it,
without asking.

---

## Two Trees: Sources and Shipped Assets

The repository separates **working files** from **what the game ships**:

```
art/         Source files — .aseprite, sketches, palettes, templates.
             Tracked in git, never bundled into a release.

assets/      What the engine loads — exported PNGs, audio, data.
             This whole tree ships with the game.
```

The rule is one-directional: everything in `assets/sprites/` is an
*export* of something in `art/`. Nothing is ever drawn directly into
`assets/` — if a PNG needs a fix, the fix happens in the `.aseprite`
source and gets re-exported. That keeps the source of truth in one
place and makes every shipped pixel reproducible.

## The `art/` Tree

```
art/
├── templates/                      Blank tier templates + mannequins
│   ├── small_24x24.aseprite        (see the Aseprite Setup Guide)
│   ├── medium_32x32.aseprite
│   ├── large_48x48.aseprite
│   ├── chibi_mannequin_24x24.aseprite
│   ├── chibi_mannequin_32x32.aseprite
│   └── boss_mannequin_48x48.aseprite
├── palettes/
│   └── raven.gpl                   The shared game palette (16–32 colors)
├── characters/
│   └── knight/
│       ├── knight.aseprite         The working file — tags, layers, guides
│       └── sketches/               Thumbnails, palette tests, explorations
│           └── 2026-08-28-silhouettes.aseprite
├── enemies/
│   └── <enemy_name>/               Same shape as characters/
├── bosses/
│   └── <boss_name>/
├── vfx/                            Slash arcs, shockwaves, explosions
├── ui/                             Icons, hearts, portraits, font work
└── tiles/                          Tileset sources per environment
```

Conventions:

- **One folder per character/enemy/boss**, holding one canonical
  working file plus its sketches. The working file carries the frame
  tags, layers, and guides; it is the file that gets re-opened for
  every revision.
- **Sketches are kept, not cleaned up.** Silhouette sheets, rejected
  palettes, and proportion experiments are the design history — they
  answer "did we already try this?" months later. Date-prefix them
  (`YYYY-MM-DD-topic`, same convention as the devlog) so they sort
  chronologically.
- **Templates and mannequins are shared**, not per-character. A new
  character starts by duplicating a template *into* its own folder —
  the template files themselves stay clean.
- **The palette file is the source of truth for hex values.** New
  colors enter `raven.gpl` deliberately (a commit, ideally with a note)
  rather than appearing ad hoc inside one character's frames.

## Naming Conventions

| File                  | Convention                          | Example                              |
| --------------------- | ----------------------------------- | ------------------------------------ |
| Working file          | `<name>.aseprite` in its own folder | `art/characters/knight/knight.aseprite` |
| Sketch / exploration  | `YYYY-MM-DD-<topic>.aseprite`       | `2026-08-28-silhouettes.aseprite`    |
| Exported sheet        | **Exactly** the name registered in `assets/data/config.json` — lowercase, underscores | `assets/sprites/knight.png`          |
| Palette               | `<scope>.gpl` (or `.pal`)           | `art/palettes/raven.gpl`             |
| Template              | `<tier>_<size>.aseprite`            | `medium_32x32.aseprite`              |

The export name deserves the emphasis: file names do the wiring
([Getting Work Into the Game](../getting-work-into-the-game.md#file-names-do-the-wiring)).
Each playable class has its own sheet: the knight's working file
exports to `knight.png`, registered in `config.json` as sheet id
`knight`, and the Knight class recipe points the player entity at it
([Player Classes](../architecture/player-classes.md)). The placeholder
bird, `player.png` (sheet id `player`), stays in the tree as the sheet
for classes that don't have final art yet. Each character's book page
records which export its source feeds, so the mapping is never tribal
knowledge.

## Exporting

Export settings are in the
[Aseprite Setup Guide](art-aseprite-guide.md#73-exporting-the-sprite-sheet):
By Rows, split tags, no border padding, into `assets/sprites/`. The
[pre-flight checklist](art-spec.md#12-pre-flight-checklist) is the
self-check before the export lands.

Both the changed source file and the re-exported PNG are committed
together — a commit that changes one without the other means the two
trees have drifted.

## Documenting as Art Accumulates

Three kinds of record, each with an existing home in this book:

**1. A page per character** — `art/characters/<name>.md`, with the
[Knight page](characters/knight.md) as the model. It holds the design
intent, references, silhouette rationale, palette (with final hex values
once settled), readability test results, animation notes, and an **Open
Questions** section at the bottom. The page is living: when an open
question gets answered in the work, the answer moves up into the body
and the question comes off the list. When a sheet ships, the page
records which export it feeds and any intentional-but-non-obvious
choices (the same notes that
[travel with a finished piece](../getting-work-into-the-game.md#when-a-piece-is-ready)).
Enemies and bosses get the same treatment as they're designed — a page
under `art/` in this book per design, however brief.

**2. Devlog entries for milestones** — when something significant lands
(the first final character sheet, a tileset for a new environment, a
palette revision), a short dated entry in the
[Development Log](../devlog/2026-02-08-project-setup.md) tells the story:
what changed, what was tried and rejected, what it unlocked. The devlog
is chronological narrative; the character pages are current-state
reference. Both exist so neither has to be the other.

**3. Git history for the files themselves.** Because sources live in the
repo, every revision of every `.aseprite` is recoverable and every
commit message is a chance to say *why* in one line ("widen pauldrons
1px — silhouette merged with brawler at 1x"). No separate changelog
file is needed; the history *is* the changelog, and the book pages hold
the conclusions.

The division of labor, in one line: **git remembers everything, the
character page states the current truth, and the devlog tells the
story.**
