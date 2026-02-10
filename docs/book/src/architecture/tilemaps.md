# Tilemaps and LDtk Integration

Raven uses [LDtk](https://ldtk.io) for level authoring and the
[LDtkLoader](https://github.com/Madour/LDtkLoader) C++ library to
parse level data at runtime.  Levels define tile visuals, a collision
grid, and named spawn points — the building blocks for hand-crafted
room templates that will feed into roguelike room-graph generation.

## Data flow

```
assets/maps/raven.ldtk          LDtk project file (JSON)
        │
        ▼
Tilemap::load()                  tilemap_loader.cpp, uses LDtkLoader
        │
        ├─ Tiles / AutoLayer  →  vector<TileData>     pre-baked src/dest rects
        ├─ IntGrid            →  vector<bool>         collision grid (row-major)
        └─ Entities           →  vector<SpawnPoint>   named positions
        │
        ▼
GameScene                        owns Tilemap, wires systems
        │
        ├─ render_tilemap()      draws tiles before sprites
        └─ update_tile_collision()  AABB-vs-grid push-out
```

## The Tilemap class

`Tilemap` is a non-ECS class defined in `src/rendering/tilemap.hpp`.
It is owned directly by `GameScene`, not stored as an ECS entity.
Individual tiles are not entities either — they are pre-baked render
data iterated in a tight loop.

### Members

| Member | Type | Purpose |
|--------|------|---------|
| `tiles_` | `vector<TileData>` | Pre-computed source rects, dest positions, flip flags |
| `collision_grid_` | `vector<bool>` | Row-major flat grid. `true` = solid |
| `spawns_` | `vector<SpawnPoint>` | Named positions from the LDtk entity layer |
| `texture_` | `SDL_Texture*` | Tileset texture loaded from the PNG |
| `grid_w_`, `grid_h_` | `int` | Grid dimensions in cells |
| `cell_size_` | `int` | Cell size in pixels (16) |
| `width_px_`, `height_px_` | `int` | Level dimensions in pixels |

### TileData

```cpp
struct TileData {
    SDL_Rect src;   // Source rect in the tileset texture.
    int dest_x;     // Destination X in world pixels.
    int dest_y;     // Destination Y in world pixels.
    bool flip_x;    // Flip horizontally.
    bool flip_y;    // Flip vertically.
};
```

### SpawnPoint

```cpp
struct SpawnPoint {
    std::string name;   // e.g. "PlayerStart", "EnemySpawn"
    float x, y;         // World position in pixels.
};
```

## File split

The `Tilemap` implementation is split across two `.cpp` files for
testability:

| File | Content | SDL required? |
|------|---------|---------------|
| `tilemap.cpp` | `is_solid()`, `is_cell_solid()`, `find_spawn()`, `init_collision()`, destructor, move ops | Link only |
| `tilemap_loader.cpp` | `Tilemap::load()` — LDtk parsing, texture loading | Yes (LDtkLoader + SDL_image) |

Tests link `tilemap.cpp` (and SDL2 for the destructor) but not
`tilemap_loader.cpp`.  The `init_collision()` method lets tests inject
collision grids directly without needing `.ldtk` files on disk.

## Loading a level

`Tilemap::load()` takes an SDL renderer, a path to the `.ldtk` project
file, and a level name:

```cpp
tilemap_.load(game.renderer().sdl_renderer(),
              "assets/maps/raven.ldtk", "Test_Room");
```

### Parsing steps

1. `ldtk::Project::loadFromFile()` parses the JSON.
2. Get the default world, then the named level.
3. Read `level->size` for pixel dimensions.
4. Iterate `level->allLayers()` in **reverse** order.  LDtk stores
   layers front-to-back; reversing gives back-to-front render order.
5. For each layer, branch on type:

**Tiles / AutoLayer:**
- Load the tileset PNG if not already loaded.  The path is resolved
  relative to the `.ldtk` file using string manipulation (no
  `std::filesystem`, per project convention).
- For each tile, pre-compute a `TileData` from `tile.getTextureRect()`
  and `tile.getPosition()`.

**IntGrid:**
- Read grid dimensions from `layer.getGridSize()`.
- For each cell, `getIntGridVal(gx, gy).value > 0` marks it solid.
- Also picks up auto-tiles if the layer has an associated tileset.

**Entities:**
- Store each entity's `getName()` and `getPosition()` as a
  `SpawnPoint`.

### Path resolution

Tileset PNG paths in the `.ldtk` file are relative to the project
file itself.  The loader resolves them with:

```cpp
std::string base_dir;
auto last_slash = ldtk_path.find_last_of('/');
if (last_slash != std::string::npos) {
    base_dir = ldtk_path.substr(0, last_slash + 1);
}
std::string tex_path = base_dir + tileset.path;
```

### Single tileset assumption

The initial implementation loads only the first tileset encountered.
All tile layers should reference the same tileset.  Multi-tileset
support is deferred.

### Graceful fallback

If the `.ldtk` file is missing or fails to parse, `load()` logs an
error and returns `false`.  `is_loaded()` remains `false`, and the
game continues with a dark background — the same behavior as before
tilemaps were added.

## Tile rendering

`render_tilemap()` in `src/ecs/systems/tilemap_render_system.cpp` is a
free function called in `GameScene::render()` before `render_sprites()`:

```cpp
void render_tilemap(const Tilemap& tilemap, SDL_Renderer* renderer);
```

The implementation iterates the pre-baked `tiles_` vector and calls
`SDL_RenderCopyEx` for each tile, with flip flags applied via
`SDL_RendererFlip`.  No interpolation is needed because tiles are
static.

### Render order in GameScene

```
SDL_RenderClear()       dark background (8, 8, 24)
render_tilemap()        tiles as background
render_sprites()        entities on top (layer-sorted, interpolated)
```

## Tile collision

`update_tile_collision()` in `src/ecs/systems/tile_collision_system.cpp`
resolves entity-vs-tilemap collision:

```cpp
void update_tile_collision(entt::registry& reg, const Tilemap& tilemap);
```

### Which entities collide?

The system views entities with all four of:
- `Transform2D` — current position
- `PreviousTransform` — position before movement
- `Velocity` — zeroed on collision axis
- `RectHitbox` — AABB dimensions and offset

The player gets a `RectHitbox{12, 14, 0, 2}` alongside its existing
`CircleHitbox`.  The circle hitbox is for bullet collision; the rect
hitbox is for tile collision.  The offset of `(0, 2)` shifts the
collision box down slightly for foot-based feel.

### AABB-vs-grid test

`Tilemap::is_solid()` converts a world-space AABB to a grid cell range
and checks each cell:

```cpp
bool Tilemap::is_solid(float x, float y, float w, float h) const {
    // Early out if entirely outside grid bounds
    if (right < 0 || bottom < 0 || x >= width_px || y >= height_px)
        return false;

    // Floor-divide for correct negative coordinate handling
    // Clamp to valid grid range
    // Check each cell in the overlapping range
    for (gy ...) for (gx ...)
        if (collision_grid_[gy * grid_w_ + gx]) return true;
    return false;
}
```

Negative coordinates use floor-division rather than C++ truncation
toward zero, which would incorrectly map `-1` to cell `0`.

### Axis-separated resolution

When an entity overlaps a solid cell after movement, the system
resolves with axis separation:

1. Try keeping X movement, revert Y to `PreviousTransform`.
   If the result is clear: accept, zero `vel.dy`.
2. Otherwise try keeping Y movement, revert X.
   If clear: accept, zero `vel.dx`.
3. If both axes are blocked: revert to previous position entirely,
   zero both velocity components.

This produces natural sliding along walls.  Moving diagonally into a
wall lets the entity continue along the unblocked axis rather than
stopping dead.

### System execution order

Tile collision runs immediately after movement, before entity-vs-entity
collision:

```
update_input             input → velocity
animation state          velocity → idle/walk
update_animation         tick frames
update_movement          velocity → position (snapshots PreviousTransform)
update_tile_collision    push out of solid tiles, zero blocked velocity
update_projectiles       bullet lifetime
update_collision         entity-vs-entity (circle-circle)
update_damage            apply damage
update_cleanup           despawn off-screen
```

## LDtk project structure

The project file at `assets/maps/raven.ldtk` uses LDtk 1.5.3 format.

### Test room

| Property | Value |
|----------|-------|
| Level name | `Test_Room` |
| Size | 480x272 px (30x17 cells) |
| Cell size | 16 px |

### Layers (top to bottom in LDtk = front to back)

| Layer | Type | Content |
|-------|------|---------|
| Entities | Entity | `PlayerStart` at (248, 136) |
| Tiles | Tiles | Floor/wall visuals from tileset |
| Collision | IntGrid | Value 1 on border cells, 0 interior |

### Tileset

A minimal 64x16 PNG at `assets/tilesets/dungeon.png` with four 16x16
tiles: floor, wall top, wall side, wall corner.  This is a placeholder
for bootstrapping; final art will replace it.

## Spawn points

`GameScene::spawn_player()` uses `Tilemap::find_spawn()` to position
the player at the LDtk-defined `PlayerStart` entity, with a fallback
to screen center if the tilemap is not loaded or the spawn point is
missing:

```cpp
float spawn_x = Renderer::VIRTUAL_WIDTH / 2.f;
float spawn_y = Renderer::VIRTUAL_HEIGHT / 2.f;
if (tilemap_.is_loaded()) {
    if (const auto* start = tilemap_.find_spawn("PlayerStart")) {
        spawn_x = start->x;
        spawn_y = start->y;
    }
}
```

## Configuration

`assets/data/config.json` has a `tilemap` section for reference:

```json
"tilemap": {
    "ldtk_project": "assets/maps/raven.ldtk",
    "default_level": "Test_Room"
}
```

The path and level name are currently hardcoded in `GameScene`.  Future
work will read these from config to support multiple rooms.

## Adding a new room

1. Open `assets/maps/raven.ldtk` in the LDtk editor.
2. Create a new level with the desired dimensions.
3. Paint the **Collision** IntGrid layer: value 1 for solid cells.
4. Paint the **Tiles** layer with visual tiles from the tileset.
5. Place entities in the **Entities** layer (`PlayerStart`, enemy
   spawns, doors, etc.).
6. Save.  The game loads the `.ldtk` file at runtime — no export step.

## Tests

`tests/test_tilemap.cpp` covers the collision and spawn logic without
requiring SDL or `.ldtk` files.  Tests use `init_collision()` to
inject grids directly:

```cpp
Tilemap tm;
std::vector<bool> grid = {
    true,  true,  true,  true,
    true,  false, false, true,
    true,  false, false, true,
    true,  true,  true,  true,
};
tm.init_collision(4, 4, 16, std::move(grid));
```

### Test cases

| Test | What it verifies |
|------|------------------|
| Tilemap collision grid | `is_solid()` detects solid cells, returns false for empty, handles out-of-bounds, detects partial overlap |
| Tilemap cell queries | `is_cell_solid()` returns correct values, out-of-bounds returns false |
| Tilemap spawn points | `find_spawn()` returns nullptr for unknown names |
| Tilemap properties | `width_px()`, `height_px()`, `cell_size()`, `is_loaded()` |
| Tile collision resolution | Entity pushed out of solid tile, velocity zeroed on collision axis, free movement in open space |

## Key files

| File | Role |
|------|------|
| `src/rendering/tilemap.hpp` | `Tilemap`, `TileData`, `SpawnPoint` declarations |
| `src/rendering/tilemap.cpp` | Collision queries, spawn lookup, grid init |
| `src/rendering/tilemap_loader.cpp` | `Tilemap::load()` — LDtk parsing + texture loading |
| `src/ecs/systems/tilemap_render_system.cpp` | `render_tilemap()` — tile blitting |
| `src/ecs/systems/tile_collision_system.cpp` | `update_tile_collision()` — AABB-vs-grid resolution |
| `src/scenes/game_scene.cpp` | Loads tilemap, wires render and collision systems |
| `assets/maps/raven.ldtk` | LDtk project with Test_Room level |
| `assets/tilesets/dungeon.png` | Placeholder tileset (4 tiles, 16x16) |
| `tests/test_tilemap.cpp` | Collision grid and resolution tests |
| `cmake/Dependencies.cmake` | LDtkLoader CPM dependency |
