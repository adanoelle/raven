# Raven Roguelike - Project Primer

## What is this?

This is the complete game design documentation for **Raven**, a cooperative
roguelike twin-stick shooter inspired by Blazing Beaks. This package contains
all the design philosophy, mechanics, and implementation guidance needed to
build the game.

## Why does this exist?

During a design conversation on February 11, 2026, we explored fundamental
questions about roguelike game design:

1. **How do you create distinct character classes that feel different
   mechanically AND thematically?**

   - Answer: Character personalities mirror mechanical strengths/weaknesses
   - Example: The Brawler is stubborn (personality) and can't adapt to ranged
     enemies (mechanics)

2. **How do you balance solo play vs co-op without making one feel like "the
   wrong way to play"?**

   - Answer: Treat them as strategically different, not difficulty tiers
   - Solo = mastery challenge (build adaptation, tight execution)
   - Co-op = coordination challenge (specialization, spatial puzzles)

3. **How do you create meaningful item choices under pressure?**

   - Answer: Items can both empower AND accidentally nerf your build
   - Players face high-stakes decisions with incomplete information (mid-combat
     pickups)

4. **How do you teach players without feeling preachy?**

   - Answer: Death messages that reinforce character personality while providing
     actionable lessons
   - Messages adapt based on context (solo vs co-op, first death vs repeated
     failures)

5. **How do you organize complex design matrices so they scale from prototype to
   production?**
   - Answer: Tag-based systems + spreadsheet matrices + validation checklists
   - This package provides those tools

## What problem is Raven solving?

**Design Problem:** Most co-op roguelikes either:

- Make solo play feel like "hard mode" (unfair, punishing)
- Make co-op feel like "easy mode" (boring, trivial)
- Don't create meaningful class synergies (all classes feel samey)
- Have items that always buff (no strategic tension)

**Raven's Approach:**

- Solo and co-op are _different strategic challenges_ with unique rewards
- Class identity is reinforced through personality, mechanics, and teaching
  moments
- Items create drama through anti-synergies (trap builds are possible and
  interesting)
- Enemy design forces adaptation (every class has counters)

## Game Overview

**Core Pitch:** Blazing Beaks-style roguelike where character personalities have
mechanical consequences, item choices matter under pressure, and solo/co-op are
equally compelling.

**Key Features:**

- 4 distinct classes with personality-driven mechanics
- Items that can power you up OR accidentally nerf you
- Enemy AI designed to counter specific classes
- Contextual death messages that teach mechanics
- Shared-screen co-op with spatial puzzles
- Solo-specific power items and balanced wave composition

**Target Platforms:**

- PC (Steam) - initial release
- Nintendo Switch - post-launch

**Development Status:**

- Production phase
- Working mechanics, 4 AI types implemented
- Active playtesting
- Team: Developer (Ada) + Artist/Musician

## What's in this package?

### **Context Document** (for Claude Code / Development)

**raven_game_design_context.md** - Complete design bible including:

- Design philosophy and pillars
- All classes, enemies, items with full specs
- Death message system
- Solo vs co-op balancing approach
- ECS implementation examples (C++20, EnTT)
- Playtesting framework

### **CSV Design Matrices** (for Google Sheets)

1. **entity_database.csv** - All game entities with tags and stats
2. **counter_matrix.csv** - Class vs enemy effectiveness
3. **item_synergy_matrix.csv** - Item-class compatibility ratings
4. **death_messages.csv** - Context-aware death message templates
5. **playtesting_log.csv** - Issue tracking template
6. **tag_reference.csv** - Tag system documentation
7. **anti_synergy_warnings.csv** - Item warning rules

### **Instructions**

**README_IMPORT_INSTRUCTIONS.md** - How to use these files, import to Google
Sheets, and follow the 8-week prototyping roadmap

## Quick Start

**If you're implementing the game:**

1. Read `raven_game_design_context.md` first
2. Import CSVs to Google Sheets for design reference
3. Start with 2 classes + 2 enemies (recommended: Brawler + Sharpshooter vs
   Kiter + Swarm)
4. Follow the 8-week prototyping roadmap in the README

**If you're a designer reviewing this:**

1. Read this primer
2. Open the Google Sheets matrices to see the design system
3. Read the Context Document for philosophy
4. Review death messages to see the teaching approach

**If you're using Claude Code:**

1. Share `raven_game_design_context.md` with your Claude Code agent
2. It contains everything needed to understand design intent
3. Includes ECS component examples and implementation guidance

## Design Philosophy Summary

**1. "Every weakness has multiple solutions"** No hard gates. Solo players can
adapt through items. Co-op players coordinate.

**2. "Solo and co-op are different games, not better/worse"** Each has unique
strategic depth and rewards.

**3. "Teach through failure, not punishment"** Death messages educate and
empower, never scold.

**4. "Items create drama"** Anti-synergies are features. High-stakes decisions
under pressure.

**5. "Personality reinforces mechanics"** Character flaws mirror mechanical
weaknesses.

**6. "Shared screen is a puzzle, not a penalty"** Spatial challenges are
content, not constraints.

**7. "Specialization is a choice, not a requirement"** Viable generalist builds
exist.

## The Core Question We're Answering

**"How do you make a roguelike where classes feel distinct, items create
meaningful tension, and solo/co-op are both compelling without feeling like
difficulty modes?"**

This documentation package is the answer.

## Project Timeline

**Current Phase:** Production (mechanics working, playtesting active)

**8-Week Prototyping Plan:**

- Weeks 1-2: Core framework (2 classes, 2 enemies, basic items)
- Weeks 3-4: Synergy discovery (add remaining classes/enemies/items)
- Weeks 5-6: Balance iteration (playtest log review, tuning)
- Weeks 7+: Polish & depth (advanced items, elite enemies, tooltips)

**Post-Prototype:**

- Content expansion (more items, enemies, bosses)
- Biome variety
- Meta-progression systems
- Steam release → Switch port

## Who is this for?

**Primary Audience:**

- Ada (Developer) - implementing the game
- Artist/Musician (Team) - understanding design vision
- Claude Code agents - AI pair programming assistance

**Secondary Audience:**

- Playtesters - understanding design intent
- Future team members - onboarding to design philosophy
- Portfolio reviewers - showcasing design methodology

## Key Innovations

1. **Personality-Driven Class Design**

   - Brawler is stubborn → can't adapt to ranged enemies
   - Sharpshooter is isolated → overwhelmed by crowds
   - Tactician is controlling → useless against unstoppable foes
   - Alchemist is patient → vulnerable when rushed

2. **Context-Aware Death Messages**

   - Same death, different lessons for solo vs co-op
   - Escalating messages for repeated failures
   - Item-specific warnings for anti-synergy pickups

3. **Tag-Based Counter System**

   - Standardized tags enable emergent interactions
   - Easy to validate coverage (every class has counters)
   - Scales cleanly into ECS architecture

4. **Solo/Co-op Dual Viability**
   - Different wave compositions
   - Different item drop weights
   - Mode-specific power items
   - Neither feels like "hard mode" or "easy mode"

## Technical Notes

**Stack:**

- C++20
- EnTT (Entity Component System)
- SDL2

**Architecture:**

- Tag-based entity system
- JSON data loading for design iteration
- Runtime anti-synergy checking
- Context-aware message selection

**Development Approach:**

- Design-first (spreadsheets before code)
- Playtest-driven (log everything, iterate)
- Modular (tags enable clean component queries)

## Success Metrics

**The game succeeds when:**

- Players have "aha!" moments discovering item synergies
- Death messages make players go "oh, I see what I did wrong"
- Solo players feel powerful for adapting smartly
- Co-op players feel powerful for coordinating well
- No one feels like they're playing the game "wrong"

**The design succeeds when:**

- Playtesting log shows balanced class win rates (solo & co-op)
- Players willingly take anti-synergy items for strategic reasons
- Death messages correlate with actual failure causes
- Every class has viable counter-strategies for their weaknesses

## Next Steps

1. **Review the Context Document** - Understand design philosophy
2. **Import CSVs to Google Sheets** - Set up design matrices
3. **Implement Core Loop** - 2 classes, 2 enemies, basic items
4. **Playtest Early and Often** - Use the playtesting log template
5. **Iterate Based on Data** - Review logs, tune balance, expand content

## Questions?

The complete design documentation in this package should answer most questions,
but here's where to look:

- **Design philosophy** → Context Document, Design Pillars section
- **Class mechanics** → Entity Database CSV + Context Document class specs
- **Item balance** → Item Synergy Matrix + Anti-Synergy Warnings
- **Enemy design** → Counter Matrix + Context Document enemy specs
- **Death messages** → Death Messages CSV + Context Document examples
- **Implementation** → Context Document, ECS Architecture section
- **Prototyping roadmap** → README Import Instructions

---

**Built:** February 11, 2026  
**Version:** 1.0  
**Project:** Raven (Blazing Beaks-inspired roguelike)  
**Developer:** Ada  
**Team:** 2-person (developer + artist/musician)  
**Target:** Steam → Nintendo Switch

Let's build something compelling. 🎮
