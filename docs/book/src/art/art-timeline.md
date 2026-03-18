# Art Asset Timeline

Solo developer art plan. Assumes familiarity with Aseprite/LibreSprite, Pigment
for palette work, and ~2-3 hours/day of art time alongside code work.

---

## Guiding Principles

1. **Playable before pretty.** Get one full loop looking decent before
   polishing anything.
2. **Palette first.** Lock the palette with Pigment before drawing a single
   sprite. Recoloring later is painful.
3. **Silhouette before detail.** Block out shapes at 1x zoom. If it reads,
   add detail. If it doesn't, reshape.
4. **Ship idle+walk+death first.** These three states cover 90% of screen
   time. Attack and hurt can be 1-frame placeholders initially.
5. **Batch by size.** Do all 16x16 enemies together, all 8x8 projectiles
   together. Context-switching between scales is slow.

---

## Asset Inventory

### Player Characters (32x32 canvas, 24x24 body)

| Character    | States | Frames | Status  | Priority |
| ------------ | ------ | ------ | ------- | -------- |
| Knight       | 7      | ~30    | Planned | Phase 1  |
| Brawler      | 7      | ~30    | Planned | Phase 2  |
| Sharpshooter | 7      | ~30    | Planned | Phase 2  |
| Tactician    | 7      | ~30    | Planned | Phase 4  |
| Alchemist    | 7      | ~30    | Planned | Phase 4  |

States per character: idle (4), walk (6), attack (4), dash (3), hurt (2),
death (5), ability (4-6).

### Enemies (16x16)

| Enemy      | States | Frames | Priority |
| ---------- | ------ | ------ | -------- |
| Kiter      | 5      | ~16    | Phase 1  |
| Swarm      | 5      | ~16    | Phase 1  |
| Juggernaut | 5      | ~16    | Phase 2  |
| Viper      | 5      | ~16    | Phase 2  |
| Bomber     | 5      | ~16    | Phase 3  |
| Sniper     | 5      | ~16    | Phase 3  |

States per enemy: idle (2-4), walk (4), attack (3-4), hurt (2), death (4).

### Projectiles & VFX (8x8 and 16x16)

| Asset              | Size  | Frames | Priority |
| ------------------ | ----- | ------ | -------- |
| Player bullet      | 8x8   | 2-3    | Phase 1  |
| Enemy bullet small | 8x8   | 2      | Phase 1  |
| Enemy bullet large | 16x16 | 2-3    | Phase 2  |
| Hit spark          | 16x16 | 3-4    | Phase 1  |
| Explosion          | 16x16 | 5-6    | Phase 2  |
| Muzzle flash       | 8x8   | 2      | Phase 2  |
| Pickup shimmer     | 8x8   | 4      | Phase 3  |
| Melee slash arc    | 16x16 | 3      | Phase 1  |
| Dash trail         | 8x8   | 2-3    | Phase 2  |
| Ground slam VFX    | 24x24 | 4      | Phase 2  |
| Concussion shot    | 16x16 | 4      | Phase 2  |
| Charged shot glow  | 8x8   | 3      | Phase 2  |

### Items & Pickups (8x8)

| Asset        | Frames | Priority |
| ------------ | ------ | -------- |
| Weapon drop  | 1      | Phase 3  |
| Stabilizer   | 1      | Phase 3  |
| Health pickup| 1-2    | Phase 3  |
| Heavy Rounds | 1      | Phase 3  |
| Rapid Fire   | 1      | Phase 3  |
| Piercing Shot| 1      | Phase 3  |
| Explosive Ammo| 1     | Phase 3  |
| Glass Armor  | 1      | Phase 3  |
| Adrenaline   | 1      | Phase 3  |
| Shield Gen   | 1      | Phase 3  |
| Greed Ring   | 1      | Phase 3  |
| Homing Chip  | 1      | Phase 3  |
| Berserker Mask| 1     | Phase 3  |
| Lone Wolf    | 1      | Phase 3  |

### Tiles (16x16)

| Tileset        | Tiles | Priority |
| -------------- | ----- | -------- |
| Stage 1 floor  | 4-6   | Phase 1  |
| Stage 1 walls  | 8-12  | Phase 1  |
| Stage 2 floor  | 4-6   | Phase 3  |
| Stage 2 walls  | 8-12  | Phase 3  |
| Stage 3 floor  | 4-6   | Phase 4  |
| Stage 3 walls  | 8-12  | Phase 4  |
| Door / exit    | 2-3   | Phase 1  |
| Spawn point    | 1-2   | Phase 2  |

### UI Elements (8x8)

| Asset           | Count | Priority |
| --------------- | ----- | -------- |
| Heart full      | 1     | Phase 1  |
| Heart empty     | 1     | Phase 1  |
| Life pip        | 1     | Phase 1  |
| Score digits 0-9| 10    | Phase 2  |
| Ability icon    | 1/class | Phase 3  |
| Weapon icon     | 1     | Phase 3  |
| Wave dot        | 2     | Phase 2  |

### Screens & Menus

| Asset               | Size    | Priority |
| -------------------- | ------- | -------- |
| Title screen logo    | 128x64  | Phase 4  |
| Character select BG  | 480x270 | Phase 4  |
| Game over text       | 64x16   | Phase 4  |
| Menu cursor          | 8x8     | Phase 4  |

---

## Phased Timeline

### Phase 1 — Playable Prototype (Weeks 1-2)

**Goal:** Replace all placeholder art for a single complete gameplay loop with
one character, two enemies, and one tileset. This is the "screenshot-worthy"
milestone.

**Week 1: Foundation**
- [ ] Lock global palette with Pigment (16-32 colors, export .pal)
- [ ] Knight idle (4 frames) — establish the character art style
- [ ] Knight walk (6 frames)
- [ ] Knight death (5 frames)
- [ ] Stage 1 floor tiles (4-6 variants)
- [ ] Stage 1 wall tiles (8-12, including corners and edges)

**Week 2: Combat Reads**
- [ ] Knight attack (4 frames)
- [ ] Knight dash (3 frames)
- [ ] Knight hurt (2 frames)
- [ ] Kiter enemy — all 5 states (~16 frames)
- [ ] Swarm enemy — all 5 states (~16 frames)
- [ ] Player bullet (2-3 frames, 8x8)
- [ ] Enemy bullet small (2 frames, 8x8)
- [ ] Hit spark (3-4 frames, 16x16)
- [ ] Melee slash arc (3 frames, 16x16)
- [ ] Heart full/empty, life pip (UI, 8x8)
- [ ] Door/exit tile (2-3 frames)

**Deliverable:** One character, two enemy types, one tileset, core VFX, basic
HUD icons. The game looks like a real game for the first time.

---

### Phase 2 — Class Variety (Weeks 3-4)

**Goal:** Add the remaining two implemented classes and fill out VFX/enemies
so the existing codebase is fully art-supported.

**Week 3: Characters + VFX**
- [ ] Brawler — all 7 states (~30 frames, 32x32 canvas)
- [ ] Ground slam VFX (4 frames, 24x24)
- [ ] Explosion VFX (5-6 frames, 16x16)
- [ ] Muzzle flash (2 frames, 8x8)
- [ ] Dash trail (2-3 frames, 8x8)
- [ ] Spawn point marker (1-2 frames)

**Week 4: More Characters + Enemies**
- [ ] Sharpshooter — all 7 states (~30 frames, 32x32 canvas)
- [ ] Charged shot glow (3 frames, 8x8)
- [ ] Concussion shot VFX (4 frames, 16x16)
- [ ] Juggernaut enemy — all 5 states (~16 frames)
- [ ] Viper enemy — all 5 states (~16 frames)
- [ ] Enemy bullet large (2-3 frames, 16x16)
- [ ] Score digits 0-9 (10 glyphs, 8x8)
- [ ] Wave progress dots (2 variants)

**Deliverable:** All three implemented classes and four of six enemies have
final art. All class-specific VFX complete.

---

### Phase 3 — Content Complete (Weeks 5-6)

**Goal:** Remaining enemies, all items, stage 2 tileset. Everything in the
current game design has art.

**Week 5: Enemies + Items**
- [ ] Bomber enemy — all 5 states (~16 frames)
- [ ] Sniper enemy — all 5 states (~16 frames)
- [ ] Pickup shimmer VFX (4 frames, 8x8)
- [ ] All 14 item icons (8x8, 1 frame each)
- [ ] Health pickup (1-2 frames)

**Week 6: Stage 2 + UI**
- [ ] Stage 2 floor tiles (4-6 variants)
- [ ] Stage 2 wall tiles (8-12)
- [ ] Weapon icon (8x8)
- [ ] Ability icons (one per class, 8x8)

**Deliverable:** All six enemies, all items, two complete tilesets, full HUD
iconography. The game is content-complete for current scope.

---

### Phase 4 — Polish & Expansion (Weeks 7-8+)

**Goal:** Final two classes, stage 3, menus, and visual polish. This is the
"trailer-ready" milestone.

- [ ] Tactician — all 7 states (~30 frames)
- [ ] Alchemist — all 7 states (~30 frames)
- [ ] Stage 3 floor tiles (4-6 variants)
- [ ] Stage 3 wall tiles (8-12)
- [ ] Title screen logo (128x64)
- [ ] Character select background (480x270)
- [ ] Game over text treatment (64x16)
- [ ] Menu cursor (8x8)
- [ ] Animation polish pass (add anticipation/follow-through frames)
- [ ] Palette refinement pass (test all sprites against all backgrounds)

**Deliverable:** Full art suite. Ready for trailer capture and Steam page
screenshots.

---

## Frame Count Summary

| Category        | Assets | Total Frames |
| --------------- | ------ | ------------ |
| Player chars    | 5      | ~150         |
| Enemies         | 6      | ~96          |
| Projectiles/VFX | 12     | ~40          |
| Items/pickups   | 15     | ~16          |
| Tiles           | 3 sets | ~60          |
| UI              | ~20    | ~25          |
| Screens         | 4      | ~4           |
| **Total**       |        | **~391**     |

At a sustainable pace of ~15-20 finished frames/day (including iteration),
the full asset set is roughly 20-26 working days of focused art time.

---

## Tips for Solo Dev Art

1. **Start ugly.** Your first pass will look bad. That's fine — refine after
   the silhouette and motion feel right.
2. **Use Pigment.** You built it for a reason. Lock your palette early and
   trust it.
3. **Reference liberally.** Blazing Beaks, Nuclear Throne, Enter the Gungeon,
   Hyper Light Drifter. Study their sprite sheets, not just their screenshots.
4. **Test in-game constantly.** A sprite that looks great in Aseprite might
   not read at 4x scale in motion. Export and test every session.
5. **Batch exports.** Set up Aseprite's CLI export to regenerate all sheets
   from .ase files. One command, all PNGs updated.
6. **Don't animate everything at once.** Idle + walk + death is the minimum
   viable character. Ship those three, then add attack and hurt later.
7. **Screenshot diary.** Take a screenshot each day. The progression is
   motivating and useful for marketing later.
