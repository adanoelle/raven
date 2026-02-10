#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <vector>

namespace raven {

/// @brief Pre-baked tile render data referencing a source rect in a tileset texture.
struct TileData {
    SDL_Rect src; ///< Source rect in the tileset texture.
    int dest_x;   ///< Destination X position in world pixels.
    int dest_y;   ///< Destination Y position in world pixels.
    bool flip_x;  ///< Flip tile horizontally.
    bool flip_y;  ///< Flip tile vertically.
};

/// @brief Named spawn point extracted from an LDtk entity layer.
struct SpawnPoint {
    std::string name; ///< Entity identifier (e.g. "PlayerStart", "EnemySpawn").
    float x;          ///< World X position in pixels.
    float y;          ///< World Y position in pixels.
};

/// @brief Tilemap loaded from an LDtk project. Holds pre-baked render data,
/// a collision grid, and spawn points. Owned by GameScene, not an ECS entity.
class Tilemap {
  public:
    Tilemap() = default;
    ~Tilemap();

    Tilemap(const Tilemap&) = delete;
    Tilemap& operator=(const Tilemap&) = delete;
    Tilemap(Tilemap&& other) noexcept;
    Tilemap& operator=(Tilemap&& other) noexcept;

    /// @brief Load a level from an LDtk project file.
    /// @param renderer SDL renderer for texture creation.
    /// @param ldtk_path Path to the .ldtk project file.
    /// @param level_name Name of the level to load.
    /// @return True on success.
    bool load(SDL_Renderer* renderer, const std::string& ldtk_path, const std::string& level_name);

    /// @brief Initialise the collision grid directly (for tests and procedural gen).
    /// @param w Grid width in cells.
    /// @param h Grid height in cells.
    /// @param cell Cell size in pixels.
    /// @param grid Row-major solid flags (true = solid).
    void init_collision(int w, int h, int cell, std::vector<bool> grid);

    /// @brief Test if an AABB overlaps any solid cell.
    /// @param x Left edge in world pixels.
    /// @param y Top edge in world pixels.
    /// @param w Width in pixels.
    /// @param h Height in pixels.
    /// @return True if any overlapping cell is solid.
    [[nodiscard]] bool is_solid(float x, float y, float w, float h) const;

    /// @brief Test if a specific grid cell is solid.
    /// @param grid_x Column index.
    /// @param grid_y Row index.
    /// @return True if the cell is solid, false if empty or out of bounds.
    [[nodiscard]] bool is_cell_solid(int grid_x, int grid_y) const;

    /// @brief Find a spawn point by name.
    /// @param name Spawn point identifier.
    /// @return Pointer to the spawn point, or nullptr if not found.
    [[nodiscard]] const SpawnPoint* find_spawn(const std::string& name) const;

    /// @brief Level width in pixels.
    [[nodiscard]] int width_px() const { return width_px_; }

    /// @brief Level height in pixels.
    [[nodiscard]] int height_px() const { return height_px_; }

    /// @brief Grid cell size in pixels.
    [[nodiscard]] int cell_size() const { return cell_size_; }

    /// @brief Whether a level has been successfully loaded.
    [[nodiscard]] bool is_loaded() const { return loaded_; }

    /// @brief Pre-baked tile render data.
    [[nodiscard]] const std::vector<TileData>& tiles() const { return tiles_; }

    /// @brief Tileset texture (may be nullptr if not loaded via load()).
    [[nodiscard]] SDL_Texture* texture() const { return texture_; }

  private:
    SDL_Texture* texture_ = nullptr;
    std::vector<TileData> tiles_;
    std::vector<bool> collision_grid_; ///< Row-major, true = solid.
    std::vector<SpawnPoint> spawns_;
    int width_px_ = 0;
    int height_px_ = 0;
    int cell_size_ = 0;
    int grid_w_ = 0;
    int grid_h_ = 0;
    bool loaded_ = false;
};

} // namespace raven
