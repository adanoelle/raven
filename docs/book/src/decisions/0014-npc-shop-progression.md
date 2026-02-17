# 14. NPC Shop Rooms for In-Run Progression

Date: 2026-02-13 Status: Proposed

## Context

Raven's current run loop is: enter room, clear waves, exit to next room,
repeat through 3 stages. The only mid-run power acquisition is weapon pickups
(temporary, 10s decay) and stabilizers (make weapon permanent). There is no
leveling, no persistent in-run upgrades, and no way for the player to build
toward a synergistic loadout across rooms. The design document calls for
"meaningful item choices under pressure" and defines a full item synergy
matrix, but there is no delivery mechanism for those items in the game yet.

Games in the genre solve this differently:

- **Nuclear Throne:** On XP level-up, choose 1 mutation from 4 random options.
  No shops — upgrades are tied to combat performance. Simple, fast, but
  limited player agency over build direction.

- **Hades:** Boons from NPC gods between rooms, plus a shop (Charon) with
  purchasable upgrades. Players can influence their build by choosing which
  NPC door to enter. Strong narrative integration — each NPC has personality
  and dialogue.

- **Blazing Beaks:** Items collected during rooms have hidden effects (some
  beneficial, some harmful). An NPC shop between levels lets players trade
  collected items for weapons and health. Risk/reward: holding more items
  gives better shop options but the negative effects accumulate.

Raven's design pillars emphasize class identity, item anti-synergies as
features, and dual viability (solo/co-op). An NPC shop system fits these
pillars: NPCs can offer class-contextual upgrades, the anti-synergy warning
system can surface at point of purchase, and co-op teams can coordinate who
buys what.

## Decision

**Add NPC shop rooms as optional encounters between combat rooms.** After
clearing a combat room, one of the exits may lead to a shop room instead of
the next combat room. Shop rooms contain a friendly NPC who offers upgrades
for sale using a run currency (coins earned from combat).

### Room flow

```
[Combat Room] → Exit A → [Combat Room]       (always available)
              → Exit B → [NPC Shop Room]      (random, not every room)
                          └→ [Combat Room]    (single exit out)
```

Shop rooms are safe — no enemies spawn. The player browses, buys (or doesn't),
and exits through a single door back into the combat progression. The shop
room does not replace a combat room; it is an optional side path that adds one
room to the run.

### NPC design

Each NPC is a distinct character with personality and a specialty:

- NPCs are tied to **item categories** (weapon mods, defense, utility) rather
  than offering random items from the full pool. This lets the player choose
  which category of upgrade to pursue by choosing which door to enter (when
  multiple shop exits are available).
- NPCs have short contextual dialogue lines that hint at what they sell and
  react to the player's current state (low HP, specific class, items already
  held).
- NPC sprites use the medium tier (32x32 frame, 24x24 body) and follow the
  same chibi proportion guides as player characters.

### Upgrade model

Upgrades are the items defined in the design document's item system (Heavy
Rounds, Glass Armor, Shield Generator, etc.). They are **permanent for the
run** — no decay timer. The anti-synergy warning system (already specified in
the design doc) activates at point of purchase, surfacing DANGER/WARNING
indicators before the player commits.

**Currency:** Coins drop from enemies on death, scaled by enemy tier and a
Greed Ring multiplier if held. The `ScoreValue` component already exists on
all enemies; a parallel `CoinValue` component would drive drops. Coins
persist across rooms but reset on death (roguelike convention).

**Pricing:** Items have a base cost modified by rarity. The shop offers 3
items per visit. The player can buy any combination they can afford. Unpurchased
items are lost when the player leaves the shop.

### Progression feel

The target experience is: the player clears a hard combat room, sees a shop
door, and feels relief and anticipation. The shop is a breather room — a
moment to assess the build, read item descriptions, weigh anti-synergies, and
make a strategic choice. The NPC's personality adds flavor. Then back into
combat with a stronger (or riskier) loadout.

Over the course of a full run, a player who visits 2–3 shops should feel
meaningfully different from one who skipped shops. The item synergy matrix
ensures that two players of the same class with different shop choices play
differently.

## ECS integration

Following the component recipe pattern (ADR-0011):

**New components:**

- `CoinValue { int amount; }` — on enemies, parallel to `ScoreValue`
- `CoinDrop { int amount; }` — spawned entity that the player walks over
- `PlayerWallet` — in registry context or on player entity, tracks coins
- `Inventory` — list of held item IDs, drives synergy checks and stat mods
- `ShopKeeper { std::string npc_id; }` — marks an NPC entity in a shop room
- `ShopItem { ItemDef def; int cost; }` — marks a purchasable item entity
- `ItemEffect` — component(s) emplaced on the player when an item is bought,
  modifying stats via systems (same pattern as class recipes)

**New data files:**

- `assets/data/items/item_manifest.json` — item definitions with stats, tags,
  synergy data, costs, rarity
- `assets/data/npcs/npc_manifest.json` — NPC definitions with dialogue lines,
  item category specialties, sprite references
- Stage JSON extended with a `shop` flag or `room_type` field on room
  definitions

**System additions:**

- `update_coin_drops` — spawns `CoinDrop` entities on enemy death, handles
  player pickup collision
- `update_shop_interaction` — detects player proximity to `ShopItem` entities,
  shows item info overlay, processes purchase on confirm input
- `apply_item_effect` — emplaces stat-modifying components when item is bought
- `check_anti_synergy` — evaluates held items against prospective purchase,
  returns warning level

Shop rooms reuse the existing room transition system. A shop room is an LDtk
level with `ShopKeeper` and `ShopItem` spawn points instead of `EnemySpawn`
points. The `Exit` component works identically. The wave system simply has
zero waves, so `room_cleared` is immediately true and the exit opens on entry.

## Consequences

**Positive:**

- Delivers the item system already designed in the design document
- Creates the build-crafting depth that makes runs feel unique
- NPC characters add personality and world-building to the run
- Anti-synergy warnings at point of purchase make informed risk-taking possible
- Shop rooms as breather pacing balances combat intensity
- Co-op teams can coordinate purchases ("you take Glass Armor, I'll tank")
- Data-driven items and NPCs (JSON) allow rapid design iteration without
  recompilation
- Reuses existing room transition and exit systems

**Negative:**

- Significant implementation scope: currency system, item effect system,
  inventory tracking, shop UI, NPC sprites, dialogue rendering
- Item balance is complex — the synergy matrix has 40+ interactions to tune
- Shop UI needs to communicate item stats, cost, and anti-synergy warnings
  clearly at 480x270 resolution, which is tight
- Art requirements increase: NPC sprites (at least 3), item icons, shop UI
  elements, potentially NPC portraits for dialogue
- Risk of power creep if shop items are too strong or too available — the
  number of shop rooms per run and item pricing need careful tuning
- The "optional side path" model means speed-runners may skip shops entirely,
  which is acceptable but means the progression system won't be visible in
  all playstyles

## Open questions

1. **How many shop rooms per run?** Too few and the system feels vestigial;
   too many and combat pacing suffers. Starting point: 1 shop opportunity per
   stage (3 per full run), with the door appearing after the 2nd or 3rd
   combat room in each stage.

2. **Should items be lost on death?** Roguelike convention says yes (full
   reset). But Hades has persistent meta-progression (the mirror). A
   meta-progression layer is out of scope for this ADR but could be a future
   addition.

3. **Item stacking:** Can the player buy the same item twice? If so, do
   effects stack linearly or with diminishing returns? The design doc does
   not specify.

4. **Co-op shop sharing:** In co-op, does each player get their own shop
   instance, or do all players share one pool of 3 items? Shared pool creates
   interesting negotiation but may frustrate in public games.
