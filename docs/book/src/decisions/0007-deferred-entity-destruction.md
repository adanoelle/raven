# 7. Deferred Entity Destruction During EnTT View Iteration

Date: 2026-02-10 Status: Accepted

## Context

EnTT views iterate over internal storage pools. Calling `reg.destroy()` on an
entity while iterating a view that includes that entity is undefined behavior —
it invalidates the underlying iterators and can corrupt the pool. The EnTT
documentation explicitly warns against this.

The collision system (`collision_system.cpp`) had a `reg.destroy(b_ent)` call
inside a `view::each()` loop in the player-vs-enemy-bullets section. The second
half of the same function (player-bullets-vs-enemies) already used the correct
pattern: collecting entities into a `std::vector<entt::entity>` and destroying
them after iteration completes.

The `break` after the destroy meant the bug was unlikely to manifest with a
single `Player` entity, but the code was still technically UB and would cause
real problems if multiple players were ever added or the `break` was removed.

## Decision

**Never call `reg.destroy()` inside an EnTT view iteration.** Always collect
entities to destroy into a `std::vector<entt::entity>` and destroy them after
the iteration loop completes. Guard each destruction with `reg.valid()` in case
the same entity was collected more than once.

Pattern:

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

This convention applies to all systems, not just collision.

## Consequences

**Positive:**

- Eliminates undefined behavior from iterator invalidation
- Consistent pattern across all systems makes the codebase easier to review
- The `reg.valid()` guard handles edge cases where the same entity could be
  collected multiple times (e.g., piercing bullets hitting overlapping enemies)

**Negative:**

- Small allocation for the `std::vector` each frame (mitigated by typical small
  counts; can use a pre-reserved or static vector if profiling shows impact)
- Slightly more code than an immediate `destroy` + `break`
