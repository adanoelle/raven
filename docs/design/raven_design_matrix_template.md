# Raven Game Design Matrix Template

## How to Use This Template
1. Create a new Google Sheet
2. Create 5 tabs: "Entity Database", "Counter Matrix", "Item Synergy", "Playtesting Log", "Tag Reference"
3. Copy the data from each section below into the corresponding tab

---

## TAB 1: Entity Database

| Name | Type | Category | Tags | HP | Damage | Speed | Range | Special Mechanic | Notes |
|------|------|----------|------|-----|--------|-------|-------|------------------|-------|
| **PLAYER CLASSES** |
| Brawler | Class | Tank | melee, sustained, tank, mobile, aoe, health_scaling | 150 | 10 | 100 | Melee | Damage reduction increases with missing HP | Close-range powerhouse |
| Sharpshooter | Class | Glass Cannon | long, burst, fragile, static, single, precision | 60 | 25 | 80 | Long | Critical hits deal 3x damage | High risk, high reward |
| Tactician | Class | Control | medium, sustained, dodge, mobile, aoe, cc_focused | 100 | 8 | 110 | Medium | Slows and stuns enemies in area | Zone control specialist |
| Alchemist | Class | DoT | medium, ramp, fragile, mobile, aoe, dot_focused | 80 | 5 | 95 | Medium | Poison stacks deal damage over time | Multi-target pressure |
| **ENEMY TYPES** |
| Kiter | Enemy | Ranged | long, burst, dodge, mobile, single | 30 | 15 | 120 | Long | Flees when player approaches | Counters: Brawler |
| Swarm | Enemy | Melee Rush | melee, burst, fragile, mobile, single | 15 | 8 | 140 | Melee | Spawns in groups of 5-8 | Counters: Sharpshooter |
| Juggernaut | Enemy | Heavy | melee, sustained, tank, static, aoe, cc_immune | 200 | 20 | 60 | Melee | Immune to CC effects | Counters: Tactician |
| Viper | Enemy | Fast | short, burst, dodge, mobile, single, cleanse | 25 | 12 | 160 | Short | Cleanses debuffs from nearby allies | Counters: Alchemist |
| Bomber | Enemy | Explosive | medium, burst, fragile, static, aoe | 40 | 30 | 70 | Medium | Explodes on death | Punishes grouped players |
| Sniper | Enemy | Precision | long, burst, fragile, static, single, armor_pierce | 35 | 40 | 50 | Long | Ignores 50% armor, long windup | Punishes static positioning |
| **ITEMS - WEAPONS** |
| Heavy Rounds | Item | Weapon Mod | damage_up, firerate_down | - | +50% | - | - | Fire rate -30% | Strong for burst, bad for sustained |
| Rapid Fire | Item | Weapon Mod | firerate_up, damage_down | - | -25% | - | - | Fire rate +60% | Enables spray builds |
| Piercing Shot | Item | Weapon Mod | pierce, damage_down | - | -20% | - | - | Shots pierce 2 enemies | Multi-target focus |
| Explosive Ammo | Item | Weapon Mod | aoe, self_damage | - | +30% | - | - | 15% splash damage to self | Risk/reward |
| **ITEMS - DEFENSE** |
| Glass Armor | Item | Defense | armor_up, hp_down | -99% | - | - | - | Sets max HP to 1, gain 5 armor | High-stakes build enabler |
| Adrenaline Shot | Item | Mobility | speed_up, hp_down | -20% | - | +40% | - | Movement speed boost | Anti-tank |
| Shield Generator | Item | Defense | shield, speed_down | - | - | -20% | - | Gain 50 shield, regens out of combat | Sustain option |
| **ITEMS - UTILITY** |
| Greed Ring | Item | Economy | gold_up, enemy_hp_up | - | - | - | - | +100% coins, enemies +30% HP | Early game gamble |
| Homing Chip | Item | Utility | homing, projectile_speed_down | - | - | - | - | Shots track, -60% projectile speed | Auto-aim |
| Berserker Mask | Item | Power | scaling_power, no_healing | - | Special | - | - | Dmg scales with missing HP, disables healing | Commit to glass cannon |

---

## TAB 2: Counter Matrix

**Player vs Enemy Effectiveness**

| Enemy Type | Brawler | Sharpshooter | Tactician | Alchemist | Reasoning |
|------------|---------|--------------|-----------|-----------|-----------|
| Kiter | ⚠️ WEAK | ✓ STRONG | ~ NEUTRAL | ✗ WEAK | Brawler can't catch, Sharpshooter outranges, Tactician can zone, Alchemist DoT wasted on kiting |
| Swarm | ✓ STRONG | ⚠️ WEAK | ✓ STRONG | ✓ STRONG | Brawler cleaves, Sharpshooter overwhelmed, Tactician AoE CC, Alchemist spreads poison |
| Juggernaut | ~ NEUTRAL | ✓ STRONG | ✗ WEAK | ~ NEUTRAL | Brawler facetanks fine, Sharpshooter bursts, Tactician CC useless, Alchemist grinds down |
| Viper | ⚠️ WEAK | ✗ WEAK | ✓ STRONG | ✗ WEAK | Brawler can't catch, Sharpshooter struggles with speed, Tactician zones, Alchemist countered hard |
| Bomber | ✓ STRONG | ~ NEUTRAL | ✓ STRONG | ⚠️ WEAK | Brawler tanks explosion, Sharpshooter picks off, Tactician controls, Alchemist wastes DoT |
| Sniper | ~ NEUTRAL | ⚠️ WEAK | ~ NEUTRAL | ~ NEUTRAL | Armor pierce hurts everyone, but telegraph allows dodging |

**Legend:**
- ✓ STRONG = Class excels against this enemy
- ~ NEUTRAL = Fair fight
- ⚠️ WEAK = Class struggles
- ✗ WEAK = Hard counter, needs team help

---

## TAB 3: Item Synergy Matrix

**Item Effectiveness by Class** (++ Strong Synergy, + Good, ~ Neutral, - Bad, -- Anti-Synergy)

| Item | Brawler | Sharpshooter | Tactician | Alchemist | Why It Works / Doesn't Work |
|------|---------|--------------|-----------|-----------|------------------------------|
| Heavy Rounds | + | ++ | - | -- | Burst enhances Sharpshooter crits, destroys Alchemist tick rate |
| Rapid Fire | ~ | - | + | ++ | Enables Alchemist poison spread, wastes Sharpshooter's precision |
| Piercing Shot | + | ~ | ++ | ++ | Synergizes with AoE classes, neutral for single-target |
| Explosive Ammo | -- | ~ | - | ~ | Self-damage catastrophic for melee Brawler |
| Glass Armor | -- | ++ | ~ | + | Turns Brawler into one-shot, enables hyper-glass Sharpshooter |
| Adrenaline Shot | -- | + | ++ | + | Removes Brawler's HP advantage, enables kiting classes |
| Shield Generator | ++ | ~ | + | ~ | Strong sustain for Brawler, okay for others |
| Greed Ring | + | - | ~ | ~ | Brawler can tank longer fights, Sharpshooter can't handle +30% HP enemies solo |
| Homing Chip | - | -- | + | ++ | Wastes precision for Sharpshooter, great for Alchemist spray |
| Berserker Mask | ++ | ++ | ~ | + | Incredible for low-HP builds, less useful for control |

**Anti-Synergy Warnings to Display:**
- Heavy Rounds + (DoT builds) = "Reduces fire rate - your DoT application will suffer"
- Glass Armor + (Tank class) = "DANGER: Your max HP becomes 1"
- Explosive Ammo + (Melee builds) = "WARNING: You will take splash damage"
- Berserker Mask + (Healing items) = "CONFLICT: Disables all healing"

---

## TAB 4: Playtesting Log

| Date | Tester | Class | Items Taken | Enemy Wave | Issue / Feedback | Severity | Action Taken |
|------|--------|-------|-------------|------------|------------------|----------|--------------|
| 2/11/26 | Ada | Brawler | Glass Armor, Shield Gen | Swarm | One-shot invincibility loop is broken OP | Critical | Cap shield regen rate |
| 2/11/26 | Artist | Alchemist | Heavy Rounds, Rapid Fire | Mixed | Conflicting fire rate mods feel confusing | Medium | Add tooltip warnings |
| | | | | | | | |
| | | | | | | | |

**Severity Levels:**
- Critical = Breaks game, must fix before next playtest
- High = Feels unfair or unfun, address soon
- Medium = Minor imbalance or confusion
- Low = Polish / nice-to-have

---

## TAB 5: Tag Reference

**Quick reference for what each tag means**

| Tag | Category | Meaning | Used By |
|-----|----------|---------|---------|
| melee | Range | Close-range combat | Brawler, Swarm, Juggernaut |
| short | Range | Short-range (slightly beyond melee) | Viper |
| medium | Range | Mid-range combat | Tactician, Alchemist, Bomber |
| long | Range | Long-range combat | Sharpshooter, Kiter, Sniper |
| burst | Pace | High damage in short window | Sharpshooter, Kiter, Swarm, Viper |
| sustained | Pace | Consistent damage over time | Brawler, Tactician, Juggernaut |
| ramp | Pace | Damage increases over time | Alchemist |
| tank | Defense | High HP, damage reduction | Brawler, Juggernaut |
| dodge | Defense | Avoidance-based defense | Tactician, Kiter, Viper |
| fragile | Defense | Low HP, relies on positioning | Sharpshooter, Alchemist, most enemies |
| static | Mobility | Doesn't move much | Sharpshooter, Bomber, Sniper |
| mobile | Mobility | High movement speed | Most classes and enemies |
| dash | Mobility | Has dash ability | (Reserved for future) |
| single | Target | Single-target damage | Sharpshooter, Kiter, Viper |
| aoe | Target | Area-of-effect damage | Brawler, Tactician, Alchemist, Bomber |
| pierce | Target | Shots go through enemies | Piercing Shot item |
| health_scaling | Resource | Power scales with HP | Brawler, Berserker Mask |
| kill_scaling | Resource | Power scales with kills | (Reserved for future) |
| time_scaling | Resource | Power scales with time | (Reserved for future) |
| cc_focused | Mechanic | Crowd control specialist | Tactician |
| cc_immune | Mechanic | Immune to crowd control | Juggernaut |
| dot_focused | Mechanic | Damage over time specialist | Alchemist |
| precision | Mechanic | High accuracy, critical hits | Sharpshooter |
| cleanse | Mechanic | Removes debuffs | Viper |
| armor_pierce | Mechanic | Ignores armor | Sniper |

**Tag Interaction Rules:**
- `firerate_down` + `sustained` or `dot_focused` = Anti-synergy warning
- `hp_down` + `tank` = Major anti-synergy warning
- `self_damage` + `melee` = Anti-synergy warning
- `no_healing` + (any healing items) = Conflict warning

---

## Design Validation Checklist

Use this to audit your design periodically:

### Coverage Checks
- [ ] Every player class has at least 2 enemy types that counter it
- [ ] Every player class has at least 2 enemy types it's strong against
- [ ] Every enemy type counters at least 1 player class
- [ ] No single class is strong against >60% of enemies

### Item Balance Checks
- [ ] Every class has at least 3 strong-synergy items
- [ ] Every class has at least 2 trap/anti-synergy items
- [ ] No item has ++ synergy with all classes (homogenization risk)
- [ ] Anti-synergy items have clear warning tooltips planned

### Co-op Checks (4-player)
- [ ] Every enemy composition has enemies that threaten different classes
- [ ] At least 3 enemy types force spatial positioning decisions
- [ ] No class can completely negate another class's weakness through items alone
- [ ] Shared-screen specific enemies exist (group punishers, tethers, etc.)

### Playtest Tracking
- [ ] Playtesting log is updated after every session
- [ ] Critical issues are addressed before next playtest
- [ ] "Feels bad" moments are tracked even if balanced
- [ ] Item pickup moments are specifically observed and logged

---

## Next Steps for Prototyping

**Phase 1: Core Framework (Week 1-2)**
1. Implement 2 player classes (suggest: Brawler + Sharpshooter for contrast)
2. Implement 2 enemy types (suggest: Kiter + Swarm for class counters)
3. Build basic item pickup system
4. Add 3-4 items with clear synergy differences
5. Playtest solo with each class

**Phase 2: Synergy Discovery (Week 3-4)**
1. Add remaining 2 classes
2. Add 4 more enemy types
3. Add 6 more items
4. First co-op playtest (2 players)
5. Log all "that combo feels broken" and "that felt bad" moments

**Phase 3: Balance Iteration (Week 5-6)**
1. Review playtesting log, identify patterns
2. Adjust anti-synergy warnings
3. Rebalance outlier items
4. Add enemy variety to waves
5. Full 4-player co-op playtest

**Phase 4: Polish & Depth (Week 7+)**
1. Add situational/advanced items
2. Add elite enemy variants
3. Implement warning tooltips
4. Second round of co-op testing
5. Begin content expansion based on solid foundation

---

## Tool Ideas for Automation

**CSV Export for Balance Testing:**
Export your Entity Database as CSV, then write Python scripts to:
- Calculate class coverage (does every class have counters?)
- Flag items with lopsided synergy scores
- Generate enemy wave compositions that ensure class diversity

**Sample Python Snippet:**
```python
import pandas as pd

# Load your entity database
df = pd.read_csv('entity_database.csv')

# Check class coverage
classes = df[df['Type'] == 'Class']['Name'].tolist()
enemies = df[df['Type'] == 'Enemy']['Name'].tolist()

for cls in classes:
    counters = count_counters(cls, enemies)  # Your logic
    if counters < 2:
        print(f"WARNING: {cls} lacks sufficient counters")
```

**JSON Export for Game Engine:**
Convert your spreadsheet to JSON that your C++ code can load:
```json
{
  "classes": [
    {
      "id": "brawler",
      "tags": ["melee", "sustained", "tank", "mobile", "aoe"],
      "stats": {"hp": 150, "damage": 10, "speed": 100}
    }
  ],
  "items": [
    {
      "id": "heavy_rounds",
      "anti_synergy_tags": ["sustained", "dot_focused"],
      "warning": "Reduces fire rate significantly"
    }
  ]
}
```

Good luck with raven! Let me know if you want me to expand any section or create additional sheets for boss encounters, biome-specific enemies, etc.
