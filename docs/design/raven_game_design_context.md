# Raven - Roguelike Game Design Context Document

## Project Overview

**Game Name:** Raven  
**Genre:** Roguelike twin-stick shooter (cooperative, up to 4 players)  
**Inspiration:** Blazing Beaks  
**Platform Target:** Steam release, then Nintendo Switch  
**Development Stack:** C++20, EnTT ECS, SDL2  
**Team:** 2-person (developer + artist/musician)  
**Current Status:** Production phase with working mechanics, four AI types, active playtesting

---

## Core Design Philosophy

### Design Pillars

1. **Class Identity Through Personality and Mechanics**
   - Each character class has a distinct personality that mirrors their mechanical strengths/weaknesses
   - Character flaws are mechanical weaknesses (e.g., Tank's stubbornness = can't adapt to ranged enemies)
   - Death messages reinforce both personality and mechanical lessons

2. **Meaningful Item Choices Under Pressure**
   - Items can both power up AND accidentally nerf a build
   - Anti-synergies create strategic tension
   - Items drop mid-combat (Blazing Beaks style) forcing quick decision-making
   - Players should feel "oh no, that was a mistake" OR "perfect, this completes my build"

3. **Dual Viability: Solo and Co-op as Equally Compelling Paths**
   - Solo = Mastery challenge (build adaptation, tight execution, consistency)
   - Co-op = Coordination challenge (specialization, spatial puzzles, team synergy)
   - Neither mode is "easy mode" or "hard mode"
   - Both have unique rewards and strategic depth

4. **Counter-Based Combat Design**
   - Every player class has natural counters in enemy roster
   - Every enemy type threatens specific classes while being vulnerable to others
   - Forces build adaptation (solo) or team coordination (co-op)
   - No single class dominates all encounters

5. **Shared Screen Spatial Dynamics**
   - All co-op players share one screen (no split-screen)
   - Enemy AI creates positional puzzles (pincer spawns, zone control, tethers)
   - Players must balance staying together vs spreading out
   - Friendly fire and AoE attacks require spatial awareness

---

## Player Character Classes

### Tag System

All entities (classes, enemies, items) use standardized tags for interaction logic:

**Range:** melee, short, medium, long  
**Pace:** burst, sustained, ramp  
**Defense:** tank, dodge, fragile  
**Mobility:** static, mobile, dash  
**Target:** single, aoe, pierce  
**Resource:** health_scaling, kill_scaling, time_scaling  
**Mechanic:** cc_focused, cc_immune, dot_focused, precision, cleanse, armor_pierce

### Class 1: Brawler (Tank)

**Mechanical Identity:**
- Tags: melee, sustained, tank, mobile, aoe, health_scaling
- Base Stats: HP 150, Damage 10, Speed 100, Range Melee
- Special: Damage reduction increases with missing HP
- Strengths: Survives extended combat, cleaves groups, protects teammates
- Weaknesses: Can't reach ranged enemies, struggles with kiting enemies

**Personality:**
- Archetype: "The Immovable Wall"
- Traits: Stubborn, protective, refuses to retreat
- Fatal Flaw: Pride in endurance blinds them to threats they can't reach
- Voice: Stoic, protective, values honor in direct combat

**Natural Counters:**
- Kiter enemies (flee when approached)
- Sniper enemies (outrange, armor-piercing)
- Fast enemies that maintain distance

**Solo Adaptation Paths:**
- Find ranged weapon mods (Heavy Rounds)
- Find dash/mobility items
- Find slow/root effects
- Stack movement speed

**Item Synergies:**
- STRONG: Shield Generator (sustain), Greed Ring (can tank longer fights)
- GOOD: Piercing Shot (multi-hit in melee), any AoE
- BAD: Adrenaline Shot (removes HP advantage)
- TERRIBLE: Glass Armor (sets HP to 1, removes tank identity), Explosive Ammo (self-damage in melee range)

### Class 2: Sharpshooter (Glass Cannon)

**Mechanical Identity:**
- Tags: long, burst, fragile, static, single, precision
- Base Stats: HP 60, Damage 25, Speed 80, Range Long
- Special: Critical hits deal 3x damage
- Strengths: Massive single-target burst, long-range safety
- Weaknesses: Low HP, overwhelmed by swarms, poor mobility

**Personality:**
- Archetype: "The Lone Eagle"
- Traits: Isolated, perfectionist, calculated
- Fatal Flaw: Precision fails when chaos surrounds them
- Voice: Clinical, distant, values efficiency

**Natural Counters:**
- Swarm enemies (too many targets)
- Fast enemies that close distance
- Any enemy that gets into melee range

**Solo Adaptation Paths:**
- Find piercing shots (multi-target)
- Find explosive ammo (AoE)
- Find turrets/pets (area coverage)
- Find knockback/slow effects

**Item Synergies:**
- STRONG: Heavy Rounds (amplifies burst), Glass Armor (embrace fragility for power)
- GOOD: Homing Chip (easier precision)
- BAD: Rapid Fire (wastes precision), Greed Ring (can't handle +30% HP enemies solo)
- TERRIBLE: Explosive Ammo (self-damage with no HP buffer)

### Class 3: Tactician (Control)

**Mechanical Identity:**
- Tags: medium, sustained, dodge, mobile, aoe, cc_focused
- Base Stats: HP 100, Damage 8, Speed 110, Range Medium
- Special: Slows and stuns enemies in area
- Strengths: Zone control, area denial, kiting capability
- Weaknesses: Low damage, useless against CC-immune enemies

**Personality:**
- Archetype: "The Puppetmaster"
- Traits: Strategic, controlling, methodical
- Fatal Flaw: Plans crumble against unstoppable force
- Voice: Calculated, condescending when in control, frustrated by chaos

**Natural Counters:**
- Juggernaut enemies (CC-immune)
- Cleansing enemies (remove debuffs)
- Enemies that benefit from being grouped

**Solo Adaptation Paths:**
- Stack raw damage items
- Find armor penetration
- Build into DoT (bypasses CC immunity)
- Find burst damage backup

**Item Synergies:**
- STRONG: Piercing Shot (multi-target CC), Rapid Fire (more CC applications)
- GOOD: Homing Chip (easier CC landing), Shield Generator
- BAD: Heavy Rounds (reduces CC application rate)
- TERRIBLE: Any "CC immune" modifier on self

### Class 4: Alchemist (DoT)

**Mechanical Identity:**
- Tags: medium, ramp, fragile, mobile, aoe, dot_focused
- Base Stats: HP 80, Damage 5 (+ poison stacks), Speed 95, Range Medium
- Special: Poison stacks deal damage over time, spreads to nearby enemies
- Strengths: Multi-target pressure, doesn't need perfect aim, scales with time
- Weaknesses: Needs time for DoT to work, weak instant damage, countered by cleanse

**Personality:**
- Archetype: "The Patient Plague"
- Traits: Persistent, spreading influence, relies on time
- Fatal Flaw: Poison needs time to work - burst damage doesn't
- Voice: Patient, unsettling, talks about inevitability and spread

**Natural Counters:**
- Speed Demon enemies (too fast, die before DoT matters)
- Viper enemies (cleanse debuffs)
- Bomber enemies (die too quickly, waste DoT)

**Solo Adaptation Paths:**
- Find instant damage options
- Find slows (buy time for poison)
- Find turrets (zone fast enemies)
- Stack initial hit damage on poison application

**Item Synergies:**
- STRONG: Rapid Fire (more poison stacks), Homing Chip (spray and apply), Piercing Shot (spread poison)
- GOOD: Any slow effects
- BAD: Heavy Rounds (reduces application rate)
- TERRIBLE: Any "instant damage only" modifiers

---

## Enemy Design

### Enemy Archetypes

#### Kiter (Ranged Flee)
- Tags: long, burst, dodge, mobile, single
- Stats: HP 30, Damage 15, Speed 120
- Behavior: Maintains distance, flees when player approaches within medium range
- **Counters:** Brawler (can't catch), Alchemist (DoT wasted on kiting target)
- **Weak To:** Sharpshooter (outranges), Tactician (zones into corners)
- **Design Intent:** Punishes melee-only builds, rewards ranged precision

#### Swarm (Melee Rush)
- Tags: melee, burst, fragile, mobile, single
- Stats: HP 15, Damage 8, Speed 140
- Behavior: Spawns in groups of 5-8, rushes toward nearest player
- **Counters:** Sharpshooter (single-target can't clear fast enough), Alchemist (needs time to spread)
- **Weak To:** Brawler (AoE cleave), Tactician (AoE CC)
- **Design Intent:** Punishes single-target precision, rewards AoE and tankiness

#### Juggernaut (Heavy)
- Tags: melee, sustained, tank, static, aoe, cc_immune
- Stats: HP 200, Damage 20, Speed 60
- Behavior: Slow advance, immune to slows/stuns, devastating melee attacks
- **Counters:** Tactician (CC completely ineffective)
- **Weak To:** Sharpshooter (high burst DPS), Brawler (can facetank)
- **Design Intent:** Hard counter to control builds, forces raw damage checks

#### Viper (Speed + Cleanse)
- Tags: short, burst, dodge, mobile, single, cleanse
- Stats: HP 25, Damage 12, Speed 160
- Behavior: Extremely fast movement, cleanses debuffs from nearby allies
- **Counters:** Alchemist (cleanses poison), Brawler (too fast to catch)
- **Weak To:** Tactician (zones control), Sharpshooter (if can land shots)
- **Design Intent:** Counters DoT builds, supports other enemies

#### Bomber (Explosive)
- Tags: medium, burst, fragile, static, aoe
- Stats: HP 40, Damage 30 (explosion), Speed 70
- Behavior: Explodes on death, punishes grouped players
- **Counters:** Players grouped too closely in co-op
- **Weak To:** Anyone with good spacing and burst damage
- **Design Intent:** Shared-screen spatial puzzle, anti-grouping

#### Sniper (Precision)
- Tags: long, burst, fragile, static, single, armor_pierce
- Stats: HP 35, Damage 40, Speed 50
- Behavior: Long windup, ignores 50% armor, laser-sight telegraph
- **Counters:** Static positioning, Tank builds (armor-pierce)
- **Weak To:** Mobile classes, anyone who dodges the telegraph
- **Design Intent:** Punishes standing still, tests positioning

### Enemy Wave Composition System

**Solo Wave Generation:**
```
Party size: 1
Wave composition:
- 60% Neutral/advantageous enemies (player can handle)
- 30% Challenge enemies (counter, but manageable with skill)
- 10% Elite/rare (hard build checks)

Goal: Encourage adaptation through items, not overwhelming with counters
```

**Co-op Wave Generation (4-player):**
```
Party size: 4
Wave composition:
- 40% Mixed threats (everyone has someone to fight)
- 40% Hard counters (force players to cover each other)
- 20% Coordination checks (spatial puzzles, group mechanics)

Goal: Force teamwork and specialization
```

**Coverage Algorithm:**
```cpp
EnemyWave GenerateWave(int party_size, std::vector<CharacterClass> party_classes) {
    if (party_size == 1) {
        // Ensure solo player isn't overwhelmed by counters
        float counter_weight = 0.3f;
        float advantageous_weight = 0.6f;
        return CreateBalancedSoloWave(party_classes[0], counter_weight, advantageous_weight);
    } else {
        // Ensure every player has threats AND targets
        return CreateDiverseCoopWave(party_classes);
    }
}
```

---

## Item System

### Item Categories

**Weapon Mods:**
- Heavy Rounds: +50% damage, -30% fire rate
- Rapid Fire: +60% fire rate, -25% damage
- Piercing Shot: Shots pierce 2 enemies, -20% damage
- Explosive Ammo: +30% damage, 15% splash damage to self
- Homing Chip: Shots track enemies, -60% projectile speed

**Defense:**
- Glass Armor: Sets max HP to 1, gain 5 armor (high-stakes build)
- Adrenaline Shot: +40% movement speed, -20% max HP
- Shield Generator: Gain 50 shield, regenerates out of combat, -20% speed

**Utility:**
- Greed Ring: +100% coin drops, enemies have +30% HP
- Berserker Mask: Damage scales with missing HP, disables all healing

**Solo-Specific:**
- Lone Wolf Emblem: +30% damage, +20% movement speed (solo only, disabled in co-op)
- Self-Reliance: Heal 10 HP every 10 seconds (solo), divided among party (co-op)
- Focused Mind: +50% critical chance (solo), divided among party (co-op)

### Synergy Matrix

| Item | Brawler | Sharpshooter | Tactician | Alchemist | Reasoning |
|------|---------|--------------|-----------|-----------|-----------|
| Heavy Rounds | + | ++ | - | -- | Burst enhances Sharpshooter crits, destroys Alchemist tick rate |
| Rapid Fire | ~ | - | + | ++ | Enables Alchemist poison spread, wastes Sharpshooter precision |
| Piercing Shot | + | ~ | ++ | ++ | Synergizes with AoE classes |
| Explosive Ammo | -- | ~ | - | ~ | Self-damage catastrophic for melee Brawler |
| Glass Armor | -- | ++ | ~ | + | Turns Brawler into one-shot, enables hyper-glass Sharpshooter |
| Adrenaline Shot | -- | + | ++ | + | Removes Brawler HP advantage, enables kiting |
| Shield Generator | ++ | ~ | + | ~ | Strong sustain for Brawler |
| Greed Ring | + | - | ~ | ~ | Brawler can tank longer fights, Sharpshooter can't handle +30% HP |
| Homing Chip | - | -- | + | ++ | Wastes Sharpshooter precision, great for Alchemist spray |
| Berserker Mask | ++ | ++ | ~ | + | Incredible for low-HP builds |

### Anti-Synergy Warning System

Items should display contextual warnings based on player's current build:

```cpp
struct ItemWarning {
    std::string warning_text;
    WarningLevel severity;  // INFO, WARNING, DANGER
};

ItemWarning CheckAntiSynergy(Item item, PlayerBuild current_build) {
    // Check tag conflicts
    if (item.has_tag("firerate_down") && current_build.has_tag("dot_focused")) {
        return {"Reduces fire rate - your DoT application will suffer", WARNING};
    }
    
    if (item.id == "glass_armor" && current_build.character_class == Brawler) {
        return {"DANGER: Your max HP becomes 1", DANGER};
    }
    
    if (item.has_tag("self_damage") && current_build.has_tag("melee")) {
        return {"WARNING: You will take splash damage at close range", WARNING};
    }
    
    if (item.id == "berserker_mask" && current_build.has_any_healing()) {
        return {"CONFLICT: Disables all healing you've collected", DANGER};
    }
    
    return {"", NONE};
}
```

### Solo vs Co-op Item Drop Weighting

```cpp
float GetItemDropWeight(Item item, CharacterClass player_class, bool is_solo) {
    float base_weight = item.rarity_weight;
    
    if (is_solo) {
        // Boost items that cover class weaknesses
        if (item.covers_weakness(player_class)) {
            base_weight *= 1.5f;  // e.g., ranged mods for Brawler
        }
        
        // Boost solo-specific items
        if (item.has_tag("solo_bonus")) {
            base_weight *= 2.0f;
        }
        
        // Reduce team-synergy items
        if (item.requires_team_synergy) {
            base_weight *= 0.7f;
        }
    } else {
        // Co-op: encourage specialization
        if (item.amplifies_strength(player_class)) {
            base_weight *= 1.3f;  // Double-down on what you're good at
        }
    }
    
    return base_weight;
}
```

---

## Death Message System

### Philosophy

Death messages serve three purposes:
1. **Reinforce character personality** through poetic/thematic language
2. **Teach mechanical lessons** about what went wrong
3. **Provide actionable solutions** (items for solo, coordination for co-op)

**CRITICAL:** Messages should be **empowering, not guilt-tripping**. Solo play is not "playing wrong."

### Message Structure

```
[POETIC LINE - Character personality flavor]

[MECHANICAL DIAGNOSIS - What actually happened]

[CONTEXT-SPECIFIC SOLUTION - How to overcome this]
```

### Death Message Matrix

#### Brawler Death Messages

**Killed by Kiters (can't catch ranged enemies):**

*Solo Context:*
```
"Even the strongest armor becomes brittle when the enemy won't stand and fight."

What happened: Kiters flee from melee range, negating your strength.

Solo Strategy: Seek ranged weapons, dash abilities, or slow effects to adapt your build.
```

*Co-op Context:*
```
"Even the strongest armor becomes brittle when the enemy won't stand and fight."

What happened: Kiters flee from melee range, negating your strength.

Team Strategy: Draw aggro while ranged allies pin them down, or zone them into corners together.
```

**Killed by Snipers (armor-piercing):**
```
"A shield raised high leaves the heart exposed from afar."

What happened: Sniper shots ignore armor, your main defense.

Strategy: Watch for laser telegraphs and dodge. Your armor won't save you.
```

**Overwhelmed by Swarm:**
```
"One stone can hold back the tide, but even stones erode under pressure."

What happened: Too many enemies at once, couldn't kill them fast enough.

Strategy: Seek AoE damage or crowd control to handle groups.
```

**Generic Brawler Death:**
```
"Strength means nothing if you can't close the gap."

What happened: You died to enemies you couldn't reach.

Strategy: Adapt your build with ranged options or mobility items.
```

#### Sharpshooter Death Messages

**Killed by Swarm:**

*Solo:*
```
"The perfect shot means nothing when a hundred more follow."

What happened: Low fire rate + single-target focus couldn't handle the horde.

Solo Strategy: Find piercing shots, explosive ammo, or rapid-fire mods to gain AoE coverage.
```

*Co-op:*
```
"The perfect shot means nothing when a hundred more follow."

What happened: Low fire rate + single-target focus couldn't handle the horde.

Team Strategy: Let AoE classes handle swarms while you focus on high-value targets.
```

**Killed in Melee Range:**
```
"Distance is safety. You forgot to keep it."

What happened: Enemy closed the gap and you couldn't escape.

Strategy: Prioritize positioning and mobility. Find knockback or slow effects.
```

**Missed Critical Shots:**
```
"Precision demands patience you didn't have."

What happened: Rushed shots missed their mark when accuracy mattered most.

Strategy: Take your time. One perfect shot beats ten hasty ones.
```

**Generic Sharpshooter Death:**
```
"Even eagles fall when they hunt alone."

What happened: Low HP means positioning errors are fatal.

Strategy: Stay at maximum range and never let enemies approach.
```

#### Tactician Death Messages

**Killed by Juggernaut (CC-immune):**

*Solo:*
```
"Not everything can be slowed, frozen, or controlled."

What happened: Your crowd control had no effect on this enemy.

Solo Strategy: Stack raw damage items or build into DoT when CC fails.
```

*Co-op:*
```
"Not everything can be slowed, frozen, or controlled."

What happened: Your crowd control had no effect on this enemy.

Team Strategy: Zone other threats while high-damage allies burn down CC-immune targets.
```

**Killed by Chaos/Overwhelm:**
```
"The best laid plans shatter when the enemy refuses to follow them."

What happened: Enemy movement was too chaotic to control effectively.

Strategy: Adapt your zones to herd enemies, not freeze them in place.
```

**Killed by High Mobility:**
```
"You drew the lines. They simply didn't stay in them."

What happened: Fast enemies ignored your control zones.

Strategy: Predict movement patterns or stack slows to catch speed demons.
```

**Generic Tactician Death:**
```
"Control is an illusion when you stand alone."

What happened: Your control effects couldn't cover all threats.

Strategy: Prioritize high-value targets and let lesser enemies pass through zones.
```

#### Alchemist Death Messages

**Killed by Speed Demon/Viper (before DoT matters):**

*Solo:*
```
"Venom takes time. Time you weren't given."

What happened: Enemies died or cleansed before poison could work.

Solo Strategy: Find instant damage options or slows to buy time for poison.
```

*Co-op:*
```
"Venom takes time. Time you weren't given."

What happened: Enemies died or cleansed before poison could work.

Team Strategy: Let allies stall enemies while your poison spreads.
```

**Killed Too Quickly:**
```
"Seeds planted in haste never take root."

What happened: You died before your damage over time could accumulate.

Strategy: Prioritize survival items. Your strength is long fights.
```

**Killed by Cleanse Enemies:**
```
"Your toxins were purged before they could fester."

What happened: Cleanse enemies removed all your poison stacks.

Strategy: Focus cleansers first or find instant damage backup.
```

**Generic Alchemist Death:**
```
"Even the deadliest poison is useless if you don't survive to see it spread."

What happened: Low HP and weak instant damage left you vulnerable.

Strategy: Find sustain items or turrets to zone while poison works.
```

### Escalating Messages (Repeated Deaths)

Track deaths to same enemy type and escalate message tone:

**First Death to Kiters (Brawler):**
> "Even the strongest armor becomes brittle when the enemy won't stand and fight."

**Second Death to Kiters:**
> "Pride kept you charging forward. Distance kept them alive. You cannot win a fight you cannot reach."

**Third Death to Kiters:**
> "STUBBORN FOOL. Adapt or perish. Find ranged weapons or die chasing shadows."

### Item Anti-Synergy Death Messages

If player dies shortly after picking up a trap item:

**Brawler picks Glass Armor, dies:**
```
"You traded your greatest strength for fleeting power. One hit. That's all it took.

What happened: Glass Armor set your HP to 1, removing your tankiness.

Lesson: High-risk builds require defensive items or perfect play."
```

**Alchemist picks Heavy Rounds, dies to fast enemies:**
```
"You rushed your craft. Poison needs time, and you slowed your own application.

What happened: Heavy Rounds reduced your fire rate, crippling DoT buildup.

Lesson: Read item synergies carefully. Not every damage boost helps."
```

### Implementation Data Structure

```cpp
struct DeathMessage {
    std::string poetic_line;
    std::string diagnosis;
    std::string solo_solution;
    std::string coop_solution;
};

struct DeathContext {
    EnemyType killer_type;
    DamageSource damage_source;
    bool was_overwhelmed;
    bool took_anti_synergy_item_recently;
    int time_survived;
    int death_count_to_this_enemy;
};

class DeathMessageSystem {
    std::unordered_map<std::pair<CharacterClass, EnemyType>, DeathMessage> messages;
    std::map<EnemyType, int> death_counts;  // Track escalation
    
    std::string GetMessage(CharacterClass cls, DeathContext ctx, bool is_solo) {
        // Check for recent anti-synergy item pickup
        if (ctx.took_anti_synergy_item_recently) {
            return GetAntiSynergyDeathMessage(cls, ctx);
        }
        
        // Get escalation level
        int death_count = death_counts[ctx.killer_type];
        if (death_count >= 3) {
            return GetEscalatedMessage(cls, ctx.killer_type);
        }
        
        // Get specific message
        DeathMessage msg = messages[{cls, ctx.killer_type}];
        
        // Format with context
        return FormatMessage(msg, is_solo);
    }
};
```

---

## Balancing Solo vs Co-op

### Core Principle

**Solo and co-op are strategically different, not difficulty tiers.**

### Solo Gameplay Loop

**Strengths:**
- Focused execution (your skill directly determines success)
- Build mastery (item choices are critical)
- Predictable difficulty scaling
- Consistent practice for muscle memory

**Challenges:**
- Must cover own weaknesses through items
- No revives from teammates
- Limited ability to specialize (need balance)
- Full burden of spatial awareness

**Design Support:**
- Balanced enemy waves (60% neutral, 30% challenge, 10% elite)
- Higher drop rates for weakness-covering items
- Solo-specific power items (Lone Wolf Emblem, Self-Reliance, Focused Mind)
- Tighter skill expression (dodges matter more)

### Co-op Gameplay Loop

**Strengths:**
- Can hyper-specialize (teammates cover weaknesses)
- Revive mechanics and support
- Emergent synergies between players
- Social experience

**Challenges:**
- Shared screen spatial constraints
- Friendly fire and AoE coordination
- Communication requirements
- Item economy competition (who gets what?)

**Design Support:**
- Diverse enemy waves (force teamwork)
- Spatial puzzle enemies (shared screen challenges)
- Team synergy items (Shared Suffering, etc.)
- Role specialization rewards

### Difficulty Tuning

**Solo should feel like:**
- Chess match (planning, adaptation, execution)
- Skill ceiling is mechanical execution + build knowledge
- Victories feel earned through mastery

**Co-op should feel like:**
- Organized chaos (coordination amidst mayhem)
- Skill ceiling is communication + specialization + spatial coordination
- Victories feel earned through teamwork

**Neither should feel:**
- Like the "correct" way to play
- Easier or harder by default
- Missing content or features

---

## Shared Screen Co-op Specific Mechanics

### Spatial Puzzle Enemies

**Splitters:** Enemies that explode if players are too close together (force spreading out)

**Tethers:** Enemies that link to specific players, requiring coordination to break

**Zone Creators:** Bosses with safe spots that shrink, forcing 4 players into tight space

**Pincer Spawners:** Spawn on opposite sides of screen, forcing formation decisions

**Screen-Clear Telegraphs:** Big attacks with different safe zones per class archetype
- Melee safe close, ranged safe far
- Shared screen means compromise positioning

### Friendly Fire Considerations

Items like Explosive Ammo should have:
- Visual telegraph (show blast radius before shooting)
- Team color coding (friendly explosions = yellow, enemy = red)
- Optional "careful aim" mode (slows time slightly when aiming near allies)

### Co-op Achievements

- "Perfect Synergy" - Beat run where each player covered another's weakness
- "Friendly Fire is On" - Beat run without hitting teammates with explosive weapons
- "Balanced Team" - Everyone within 10% of each other in contribution
- "Carry" - One player gets 60%+ of kills (showcase specialization)

---

## Implementation Considerations

### ECS Architecture (EnTT)

**Component Examples:**

```cpp
// Tag components
struct MeleeRangeTag {};
struct LongRangeTag {};
struct TankTag {};
struct FragileTag {};
struct DoTFocusedTag {};
struct CCImmuneTag {};

// Data components
struct Health { float current, max; };
struct Damage { float base_damage; float crit_multiplier; };
struct MovementSpeed { float base, current; };
struct FireRate { float shots_per_second; };

struct ItemEffects {
    std::vector<StatModifier> modifiers;
    std::vector<std::string> tags_added;
    std::vector<std::string> tags_removed;
};

struct PlayerBuild {
    CharacterClass base_class;
    std::vector<Item> items_collected;
    std::set<std::string> active_tags;  // Computed from class + items
};

struct EnemyBehavior {
    BehaviorType type;  // FLEE, RUSH, STATIONARY, CLEANSE
    float aggro_range;
    float flee_threshold;  // For Kiters
};
```

**System Examples:**

```cpp
class ItemSynergySystem {
    void OnItemPickup(entt::entity player, Item item) {
        auto& build = registry.get<PlayerBuild>(player);
        
        // Check anti-synergies
        ItemWarning warning = CheckAntiSynergy(item, build);
        if (warning.severity != NONE) {
            DisplayWarning(warning);
        }
        
        // Apply item effects
        ApplyItemEffects(player, item);
        
        // Update active tags
        build.active_tags.insert(item.tags.begin(), item.tags.end());
        build.items_collected.push_back(item);
    }
};

class WaveGenerationSystem {
    EnemyWave GenerateWave(std::vector<entt::entity> players) {
        int party_size = players.size();
        std::vector<CharacterClass> classes = GetPlayerClasses(players);
        
        if (party_size == 1) {
            return GenerateSoloWave(classes[0]);
        } else {
            return GenerateCoopWave(classes);
        }
    }
    
    EnemyWave GenerateSoloWave(CharacterClass player_class) {
        EnemyWave wave;
        
        // 60% neutral/advantageous
        for (int i = 0; i < 6; i++) {
            wave.enemies.push_back(GetRandomNeutralEnemy(player_class));
        }
        
        // 30% challenge (counters)
        for (int i = 0; i < 3; i++) {
            wave.enemies.push_back(GetCounterEnemy(player_class));
        }
        
        // 10% elite
        wave.enemies.push_back(GetRandomElite());
        
        return wave;
    }
};

class DeathMessageSystem {
    void OnPlayerDeath(entt::entity player, DeathContext ctx) {
        auto& build = registry.get<PlayerBuild>(player);
        bool is_solo = GetPartySize() == 1;
        
        std::string message = GetDeathMessage(
            build.base_class,
            ctx,
            is_solo
        );
        
        DisplayDeathMessage(message);
        
        // Track for escalation
        death_tracker.RecordDeath(ctx.killer_type);
    }
};
```

### Data Files

Store design data in JSON for easy iteration:

```json
// classes.json
{
  "classes": [
    {
      "id": "brawler",
      "name": "Brawler",
      "personality": "The Immovable Wall",
      "tags": ["melee", "sustained", "tank", "mobile", "aoe", "health_scaling"],
      "base_stats": {
        "hp": 150,
        "damage": 10,
        "speed": 100,
        "range": "melee"
      },
      "countered_by": ["kiter", "sniper", "viper"],
      "strong_against": ["swarm", "bomber"]
    }
  ]
}

// items.json
{
  "items": [
    {
      "id": "heavy_rounds",
      "name": "Heavy Rounds",
      "rarity": "common",
      "effects": [
        {"stat": "damage", "modifier": 1.5},
        {"stat": "fire_rate", "modifier": 0.7}
      ],
      "tags": ["damage_up", "firerate_down"],
      "anti_synergy_tags": ["sustained", "dot_focused"],
      "warning_text": "Reduces fire rate significantly"
    }
  ]
}

// death_messages.json
{
  "death_messages": [
    {
      "class": "brawler",
      "enemy_type": "kiter",
      "poetic_line": "Even the strongest armor becomes brittle when the enemy won't stand and fight.",
      "diagnosis": "Kiters flee from melee range, negating your strength.",
      "solo_solution": "Seek ranged weapons, dash abilities, or slow effects to adapt your build.",
      "coop_solution": "Draw aggro while ranged allies pin them down, or zone them into corners together."
    }
  ]
}
```

---

## Playtesting Framework

### Metrics to Track

**Per-Run:**
- Class played
- Items picked up (in order)
- Death count by enemy type
- Time survived
- Solo vs co-op
- Final build tags

**Aggregate:**
- Class win rates (solo vs co-op)
- Most common death causes per class
- Most picked items per class
- Anti-synergy item pickup → death correlation
- Enemy type appearance vs threat level

### Playtesting Questions

**After each session, ask:**
1. Did you feel like your death was your fault or unfair?
2. Which items felt good/bad to pick up?
3. Were there moments where you wished for a specific item?
4. Did any enemy feel impossible to deal with?
5. (Co-op) Were there moments where teamwork felt required vs optional?

### Red Flags to Watch For

- **Class feels unviable solo:** Needs more weakness-covering items in drop pool
- **Item never gets picked:** Too niche or anti-synergies too punishing
- **Enemy type dominates:** Over-tuned or insufficient counter-play
- **Death messages feel blaming:** Rewrite to be more educational/empowering
- **Solo feels like hard mode:** Wave composition or item drops need tuning
- **Co-op feels like easy mode:** Need more coordination-specific challenges

---

## Future Expansion Ideas

### Additional Classes
- **Summoner:** Spawns turrets/pets, weak direct damage
- **Gambler:** Random crits and chaos effects, high variance
- **Berserker:** Damage scales with aggression, risky playstyle

### Boss Design
- Bosses that change tactics based on party composition
- Multi-phase fights with different class counters per phase
- Co-op specific mechanics (one player holds attention, others damage weak points)

### Meta-Progression
- Unlock alternate starting items per class
- Permanent build "presets" (start with 1-2 items)
- Character-specific challenges (Brawler: beat run without ranged items)

### Biome-Specific Enemies
- Ice biome: Slowing environmental hazards, benefit control classes
- Fire biome: DoT aura environments, punish Tank builds
- Shadow biome: Visibility reduction, rewards precise aim

---

## Key Design Mantras

1. **"Every weakness has multiple solutions"** - No hard gates, always adaptation paths
2. **"Solo and co-op are different games, not better/worse"** - Balance each separately
3. **"Teach through failure, not punishment"** - Death messages educate, don't scold
4. **"Items create drama"** - Anti-synergies are features, not bugs
5. **"Personality reinforces mechanics"** - Character flaws mirror mechanical weaknesses
6. **"Shared screen is a puzzle, not a penalty"** - Spatial challenges are content
7. **"Specialization is a choice, not a requirement"** - Viable generalist builds exist

---

## Development Priorities

### Phase 1: Core Loop (Current)
- 4 classes fully implemented
- 6 enemy types with counter behaviors
- 10-15 core items with synergies
- Basic wave generation
- Death message system

### Phase 2: Balance Iteration
- Playtesting with spreadsheet tracking
- Tune wave compositions (solo vs co-op)
- Adjust item drop rates
- Refine death messages based on feedback

### Phase 3: Content Expansion
- Additional items (20-30 total)
- More enemy variants
- Boss encounters
- Biome aesthetics

### Phase 4: Polish
- Visual telegraphs for all dangerous attacks
- Particle effects for item synergies
- Death message UI polish
- Co-op screen management improvements

---

## Questions for Claude Code Agent

When implementing features, consider:

1. **Tag System:** How to efficiently query entities by multiple tags in EnTT?
2. **Wave Composition:** Best data structure for enemy wave templates?
3. **Item Synergy Checking:** Runtime vs compile-time tag validation?
4. **Death Message Selection:** Decision tree vs lookup table for context-specific messages?
5. **Solo/Co-op Scaling:** Single codebase with parameters vs separate game modes?

---

## Closing Notes

This game is about **meaningful choices under pressure** in both solo mastery and co-op coordination contexts. Every system reinforces this: character personalities have mechanical flaws, items can help or hurt, enemies punish predictable play, and death teaches rather than punishes.

The design succeeds when:
- Players have "aha!" moments discovering item synergies
- Death messages make players go "oh, I see what I did wrong"
- Solo players feel powerful for adapting smartly
- Co-op players feel powerful for coordinating well
- No one feels like they're playing the game "wrong"

Good luck building Raven! 🎮
