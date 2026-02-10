# ECS with EnTT

Raven uses [EnTT](https://github.com/skypjack/entt) as its Entity Component
System. All game state is stored in a single `entt::registry`, and systems are
free functions that operate on views. This chapter covers the conventions and
patterns that keep the ECS layer consistent as the codebase grows.

## Component design

Components live in a single header, `src/ecs/components.hpp`, inside the
`raven` namespace. Every component is a small POD struct with sensible defaults:

```cpp
struct Velocity {
    float dx = 0.f;
    float dy = 0.f;
};
```

This keeps components trivially copyable and cache-friendly. EnTT stores
components in contiguous pools, so small types with no heap allocations let the
CPU prefetch efficiently when a system iterates thousands of entities.

Components are grouped by domain with comment banners: Spatial, Rendering,
Collision, Gameplay, Aiming/Shooting, Weapon, Emitter, Pickup/Decay, and Tags.
Tag components are empty structs used purely for filtering:

```cpp
struct OffScreenDespawn {};
struct Piercing {};
struct StabilizerPickup {};
```

### StringId for hot components

Several components reference sprite sheets or pattern names by string. Storing a
`std::string` in a component that is iterated every frame (like `Sprite` or
`BulletEmitter`) adds a heap allocation per entity and breaks trivial
copyability.

`StringId` solves this with string interning. A `StringInterner` (stored in the
registry context via `reg.ctx()`) maps strings to `uint16_t` indices at
setup time. Components store the lightweight `StringId` instead:

```cpp
struct Sprite {
    StringId sheet_id;   // 2 bytes, trivially copyable
    int frame_x = 0;
    // ...
};
```

Copies are O(1) — just a 2-byte integer copy instead of a heap allocation.
When the actual string is needed (e.g. for a sprite sheet lookup), systems
resolve it through the interner:

```cpp
const auto& interner = reg.ctx().get<StringInterner>();
const auto* sheet = sprites.get(interner.resolve(sprite.sheet_id));
```

## System conventions

Systems are free functions in the `raven::systems` namespace, one per file in
`src/ecs/systems/`. Each system takes an `entt::registry&` and whatever
additional context it needs (delta time, input state, a tilemap reference, etc.):

```cpp
void update_movement(entt::registry& reg, float dt);
void update_collision(entt::registry& reg);
void update_emitters(entt::registry& reg, const PatternLibrary& patterns, float dt);
```

Systems create EnTT views to iterate only the entities that have the required
components. This is the fundamental ECS query pattern:

```cpp
auto view = reg.view<Transform2D, Velocity>();
for (auto [entity, tf, vel] : view.each()) {
    tf.x += vel.dx * dt;
    tf.y += vel.dy * dt;
}
```

### Anonymous-namespace helpers

When a system needs internal helper functions, they go in an anonymous namespace
at the top of the `.cpp` file. This keeps them out of the public API while
co-locating them with the system that uses them. Examples:

- `emitter_system.cpp` — `find_player_position()` and `fire_burst()` are
  anonymous-namespace helpers that the public `update_emitters()` function calls.
- `damage_system.cpp` — `tick_invulnerability()`, `handle_player_death()`, and
  `handle_enemy_death()` break the damage system into focused steps.

### Execution order

Scenes own the system execution order. `GameScene::update()` calls systems in a
specific sequence where each system's output feeds the next:

```
update_input              read keyboard/gamepad → target velocity
update_shooting           aim resolution + bullet spawn
update_emitters           enemy bullet pattern firing
animation state logic     velocity → idle/walk switch
update_animation          tick frames
update_movement           velocity → position
update_tile_collision     resolve wall overlaps
update_collision          circle-circle hit tests
update_pickups            weapon + stabilizer collection
update_weapon_decay       tick decay timers, handle explosion
update_damage             apply DamageOnContact, spawn drops
update_cleanup            tick lifetimes, despawn expired entities
```

This ordering is not enforced by the framework — it is the scene's
responsibility to call systems in the right order. Getting it wrong causes
subtle bugs (e.g. if collision runs before movement, hits are always one frame
late).

## Entity destruction

Calling `reg.destroy()` inside an EnTT view iteration is undefined behavior. It
invalidates the internal iterators that the view is traversing. Even if a
`break` immediately follows the destroy, the code is technically UB and will
break if the early exit is ever removed or if multiple entities match.

The project convention (documented in
[ADR-0007](../decisions/0007-deferred-entity-destruction.md)) is
**collect-then-destroy**: gather entities into a vector during iteration, then
destroy them in a second pass:

```cpp
std::vector<entt::entity> to_destroy;

for (auto [ent, ...] : view.each()) {
    if (should_destroy) {
        to_destroy.push_back(ent);
    }
}

for (auto ent : to_destroy) {
    if (reg.valid(ent)) {
        reg.destroy(ent);
    }
}
```

The `reg.valid()` guard handles cases where the same entity is collected more
than once (e.g. a piercing bullet hitting overlapping enemies). This pattern
appears in `collision_system.cpp`, `damage_system.cpp`, and
`cleanup_system.cpp`.

## Shared utilities

When multiple systems need the same geometry helper, it goes in a shared header
rather than being duplicated. `src/ecs/systems/hitbox_math.hpp` provides the
`circles_overlap()` function used by both the collision system and the pickup
system:

```cpp
[[nodiscard]] inline bool circles_overlap(
    float x1, float y1, float r1,
    float x2, float y2, float r2);
```

The function is `inline` in the header so it can be included without linker
issues while keeping the implementation visible for inlining. It uses
squared-distance comparison to avoid a `sqrt` call.

## Performance patterns

### Persistent scratch buffers via `reg.ctx()`

EnTT's registry context (`reg.ctx()`) stores singleton data that is not attached
to any specific entity. Raven uses it for long-lived objects like the
`StringInterner` and the RNG (`std::mt19937`).

It is also used for **persistent scratch buffers** that avoid per-frame heap
allocations. The render system stores its sort buffer in the context:

```cpp
auto& entries = reg.ctx().emplace<std::vector<RenderEntry>>();
entries.clear();  // capacity stays allocated across frames
```

The vector's capacity grows to a high-water mark and stays there. Subsequent
frames reuse the allocation, avoiding thousands of `malloc`/`free` calls per
second in the render loop.

### StringId for O(1) component copies

As described in the component design section, `StringId` replaces `std::string`
in frequently copied components. This matters for operations like
`weapon_from_emitter()` which constructs a `Weapon` from an `EmitterDef` — all
fields including the sprite sheet reference are trivial copies.

## Key files

| File | Role |
| ---- | ---- |
| `src/ecs/components.hpp` | All component structs, grouped by domain |
| `src/ecs/systems/` | One `.cpp`/`.hpp` pair per system |
| `src/core/string_id.hpp` | `StringId` and `StringInterner` |
| `src/ecs/systems/hitbox_math.hpp` | Shared `circles_overlap()` helper |
| `src/scenes/game_scene.cpp` | System execution order |
| `docs/decisions/0007-deferred-entity-destruction.md` | ADR for the collect-then-destroy convention |
