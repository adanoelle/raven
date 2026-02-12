# Learning Resources

Curated resources for getting started with pixel art and Aseprite, with emphasis
on styles relevant to Raven.

## Style References

Games whose art style directly inspires Raven:

- **Blazing Beaks** — Vibrant saturated palette, strong character-background
  contrast, charming minimalist bird characters at 16×16, expressive enemy
  designs. Made in GameMaker by Applava (Vilnius, Lithuania). Primary
  inspiration cited by the devs: Nuclear Throne. Reviewers also compare to Enter
  the Gungeon and The Binding of Isaac.
- **Celeste** — 320×180 base resolution, 8×8 tile grid, Pedro Medeiros' "every
  pixel must communicate" philosophy. Notable techniques: sparse outlines at low
  res, sub-pixel animation, consistent anchor points, particle effects using
  single pixels. Blog post on the art process:
  <https://exok.com/posts/2019-12-10-celeste-pixel-art/>
- **Nuclear Throne** — Top-down roguelike with gritty, high-contrast pixel art
  and expressive animation at small sprite sizes.
- **Enter the Gungeon** — Clean, readable sprites with strong silhouettes, good
  example of projectile readability in dense bullet patterns.

## Getting Started with Aseprite

- [Aseprite official docs](https://www.aseprite.org/docs/) — covers the full
  tool
- [Aseprite tutorial page](https://www.aseprite.org/docs/tutorial/) — animation
  tags, onion skinning, exporting
- [Sprite sheet export](https://www.aseprite.org/docs/sprite-sheet/) — exporting
  sheets in the format Raven expects
- [Animation docs](https://www.aseprite.org/docs/animation/) — tagging frame
  ranges, loop modes

## Pixel Art Fundamentals

- [Derek Yu — Pixel Art Tutorial: Basics](https://www.derekyu.com/makegames/pixelart.html)
  — Step-by-step sprite creation from the creator of Spelunky. The definitive
  beginner tutorial.
- [Derek Yu — Pixel Art: Common Mistakes](https://derekyu.com/makegames/pixelart2.html)
  — Avoid gradients, pillow shading, too many colors.
- [Pedro Medeiros (Saint11) — Pixel Art Tutorials](https://saint11.art/blog/pixel-art-tutorials/)
  — 70+ free mini-tutorials covering animation, sub-pixel movement, effects,
  consistency. Pedro is the artist behind Celeste and TowerFall.
- [SLYNYRD Pixelblog](https://www.slynyrd.com/pixelblog-catalogue) — Deep dives
  on character sprites, tiles, shmup design (uses 480×270 — same as Raven),
  color theory.
- [Lospec Tutorials](https://lospec.com/pixel-art-tutorials) — Large curated
  index of pixel art tutorials by topic.

## YouTube Channels

Each entry includes the channel focus and a recommended "start here" video.

- **Brandon James Greer** — Beginner-friendly breakdowns of sprite sizing, color
  theory, and design process. Start with: _"What Size is Pixel Art? (Intro to
  Sprite and Canvas Size)"_ — directly relevant to choosing sprite dimensions
  for Raven.
- **AdamCYounis** — Prolific channel covering Aseprite workflows, pixel art
  theory, character design, animation, and game dev. Start with his Aseprite
  workflow videos.
- **MortMort** — Entertaining, accessible tutorials with strong Aseprite
  coverage. Start with: _"Aseprite Guide for Beginners (Pixelart Tutorial)"_ —
  one of the most-watched Aseprite tutorials on YouTube.
- **Saultoons** — Art fundamentals applied to pixel art, Aseprite tips, and
  workflow optimization.
- **Pixel Pete (Peter Milko)** — Pixel art integrated with game development
  context, good for seeing art in a game-dev workflow.

## Tools and Palettes

- [Lospec Palette List](https://lospec.com/palette-list) — 2,500+ downloadable
  palettes in formats importable by Aseprite. Good for picking a starting
  palette (the art spec calls for 16–32 colors).
- [Lospec Pixel Editor](https://lospec.com/) — Browser-based pixel editor for
  quick sketching.

## Tips for Raven's Style

Practical pointers tying the resources back to the project's
[art spec](art-spec.md):

- Start with a 16–32 color palette from Lospec, matching the art spec's palette
  guidance.
- Work at 1:1 on a 480×270 canvas to see sprites in context.
- Study Blazing Beaks for character contrast and Celeste for animation economy.
- Use Aseprite animation tags to match Raven's sheet layout (one row per state).
- Export sheets with no padding, uniform grid, transparent background — per the
  art spec.
