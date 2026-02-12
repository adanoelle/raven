# Raven Game Design Matrix - Import Instructions

This package contains the complete game design documentation for the Raven roguelike project.

## Files Included

### Context Document
- **raven_game_design_context.md** - Comprehensive design document for Claude Code agent
  - Game philosophy and design pillars
  - Complete class, enemy, and item specifications
  - Death message system
  - Solo vs co-op balancing
  - Implementation considerations
  - ECS architecture examples

### CSV Files (for Google Sheets)

1. **entity_database.csv** - All game entities (classes, enemies, items)
2. **counter_matrix.csv** - Class vs enemy effectiveness ratings
3. **item_synergy_matrix.csv** - Item synergies by class
4. **death_messages.csv** - Contextual death message templates
5. **playtesting_log.csv** - Template for tracking playtesting feedback
6. **tag_reference.csv** - Complete tag system documentation
7. **anti_synergy_warnings.csv** - Item warning system rules

## How to Import into Google Sheets

### Method 1: Create Individual Sheets (Recommended)

1. Go to Google Sheets (sheets.google.com)
2. Create a new spreadsheet called "Raven Game Design Matrix"
3. For each CSV file:
   - Click File → Import
   - Click "Upload" tab
   - Drag the CSV file or click "Browse"
   - Select "Insert new sheet(s)" as import location
   - Click "Import data"
   - Rename the sheet to match the CSV filename

You should end up with 7 tabs:
- Entity Database
- Counter Matrix
- Item Synergy Matrix
- Death Messages
- Playtesting Log
- Tag Reference
- Anti-Synergy Warnings

### Method 2: All in One Sheet

1. Create a new Google Sheet
2. Create 7 separate tabs manually
3. For each tab:
   - Go to File → Import
   - Upload the corresponding CSV
   - Select "Replace current sheet" as import location

## Using the Design Matrix

### Entity Database
Track all game entities with standardized tags. Use this as your source of truth for:
- Class stats and mechanics
- Enemy behaviors and counters
- Item effects and synergies

**Pro tip:** Use Google Sheets filters to:
- Show only items with specific tags (e.g., filter Tag column for "solo_bonus")
- Show only enemies that counter a specific class
- Show only weapon mods vs defense items

### Counter Matrix
Quick reference for which classes struggle against which enemies.

**Legend:**
- ✓ STRONG = Class excels against this enemy
- ~ NEUTRAL = Fair fight
- ⚠️ WEAK = Class struggles
- ✗ WEAK = Hard counter, needs team help

**Usage:** When playtesting, check if deaths align with expected counters

### Item Synergy Matrix
Shows how each item affects each class.

**Rating Scale:**
- ++ = Strong synergy
- + = Good
- ~ = Neutral
- - = Bad
- -- = Anti-synergy (trap)

**Usage:** During item design, ensure:
- Every class has ++ items (power fantasy)
- Every class has -- items (strategic risk)
- No item is ++ for all classes (homogenization)

### Death Messages
Context-aware messages that teach mechanics.

**Columns:**
- Class + Enemy Type = specific teaching moment
- Solo Solution vs Co-op Solution = different advice based on context
- Death Count = escalating messages (1st, 2nd, 3rd+ death)

**Usage:** Implement decision tree in code to select appropriate message

### Playtesting Log
Track issues discovered during playtesting.

**Severity Levels:**
- Critical = Breaks game, must fix before next playtest
- High = Feels unfair or unfun, address soon
- Medium = Minor imbalance or confusion
- Low = Polish / nice-to-have

**Usage:** After each playtest session:
1. Log all "feels broken" moments
2. Log all "feels bad" moments
3. Review weekly to identify patterns
4. Prioritize Critical/High issues

### Tag Reference
Complete documentation of all tags used in the system.

**Usage:**
- Reference when adding new entities
- Ensure consistent tag usage across all entities
- Build tag-based logic in code (EnTT ECS component queries)

### Anti-Synergy Warnings
Rules for when to display warnings during item pickup.

**Severity Levels:**
- DANGER = Major build conflict (e.g., Glass Armor on Tank)
- WARNING = Significant anti-synergy (e.g., Heavy Rounds on DoT)
- INFO = Suboptimal but not terrible

**Usage:** Implement warning system that checks:
1. Player's current class
2. Player's collected items/tags
3. New item's anti-synergy tags
4. Display appropriate warning before pickup confirmation

## Design Validation Checklist

Use this periodically to audit your design:

### Coverage Checks
- [ ] Every player class has at least 2 enemy types that counter it
- [ ] Every player class has at least 2 enemy types it's strong against
- [ ] Every enemy type counters at least 1 player class
- [ ] No single class is strong against >60% of enemies

### Item Balance Checks
- [ ] Every class has at least 3 strong-synergy items
- [ ] Every class has at least 2 trap/anti-synergy items
- [ ] No item has ++ synergy with all classes
- [ ] Anti-synergy items have clear warning tooltips planned

### Co-op Checks (4-player)
- [ ] Every enemy composition has enemies that threaten different classes
- [ ] At least 3 enemy types force spatial positioning decisions
- [ ] No class can completely negate another class's weakness through items alone
- [ ] Shared-screen specific enemies exist

### Solo Checks
- [ ] Every class has at least 3 solo-viable item adaptation paths
- [ ] Solo wave compositions are balanced (60% neutral, 30% challenge, 10% elite)
- [ ] Solo-specific power items exist (Lone Wolf Emblem, etc.)
- [ ] Death messages don't blame players for playing solo

## Tips for Effective Use

### During Design Phase
1. Fill out Entity Database first - this is your foundation
2. Build Counter Matrix to ensure class diversity
3. Design items with Synergy Matrix in mind
4. Write Death Messages that teach, not punish

### During Implementation
1. Use Tag Reference to build ECS component system
2. Implement Anti-Synergy Warnings before item drops
3. Set up Playtesting Log workflow early
4. Reference Context Document for design philosophy questions

### During Playtesting
1. Log every "feels broken" moment immediately
2. Track which class-enemy pairings feel unfair
3. Note when death messages don't match actual death cause
4. Observe when players pick anti-synergy items unknowingly

### During Balancing
1. Review Playtesting Log for patterns
2. Cross-reference with Counter Matrix
3. Adjust item drop rates based on synergy feedback
4. Tune enemy HP/damage based on class win rates

## Advanced Features

### Conditional Formatting (Google Sheets)
Set up color coding:
- **Counter Matrix:** Green for ✓, Yellow for ~, Red for ✗/⚠️
- **Item Synergy:** Green for ++, Red for --
- **Playtesting Log:** Red for Critical, Yellow for High/Medium

### Data Validation
Add dropdowns for:
- Playtesting Log → Severity (Critical, High, Medium, Low)
- Entity Database → Type (Class, Enemy, Item)
- Death Messages → Class (Brawler, Sharpshooter, Tactician, Alchemist)

### Pivot Tables
Create pivot tables to analyze:
- Item pickup frequency by class
- Death rate by enemy type
- Most common anti-synergy pickups

## Exporting to Code

### JSON Export
You can export these CSVs to JSON for game engine loading:

```python
import csv
import json

def csv_to_json(csv_file, json_file):
    data = []
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            data.append(row)
    
    with open(json_file, 'w') as f:
        json.dump(data, f, indent=2)

# Example usage
csv_to_json('entity_database.csv', 'entities.json')
csv_to_json('death_messages.csv', 'death_messages.json')
```

### C++ Data Loading
Use libraries like RapidJSON or nlohmann/json to parse:

```cpp
#include <nlohmann/json.hpp>
#include <fstream>

void LoadEntities(const std::string& filepath) {
    std::ifstream file(filepath);
    nlohmann::json j = nlohmann::json::parse(file);
    
    for (auto& entity : j) {
        std::string name = entity["Name"];
        std::string type = entity["Type"];
        // Parse tags, stats, etc.
    }
}
```

## Questions?

If you need help with:
- **Design decisions** → Reference raven_game_design_context.md
- **Implementation** → Use context document with Claude Code
- **Balancing** → Review Playtesting Log patterns
- **New features** → Check Design Validation Checklist first

## Quick Start Workflow

**Week 1-2:** Set up core entities
1. Import all CSVs to Google Sheets
2. Review Entity Database and Counter Matrix
3. Implement 2 classes + 2 enemies in code
4. Set up basic item system

**Week 3-4:** First playtesting
1. Add 3-4 items from Item Synergy Matrix
2. Implement basic death messages
3. Run first playtest and log issues
4. Review Playtesting Log for patterns

**Week 5-6:** Iteration
1. Balance based on playtest feedback
2. Add remaining classes and enemies
3. Expand item pool to 10-15 items
4. Implement anti-synergy warnings

**Week 7+:** Polish
1. Tune wave compositions
2. Refine death messages
3. Add solo-specific items
4. Implement co-op spatial puzzles

Good luck with Raven! 🎮
