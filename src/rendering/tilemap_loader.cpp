#include "rendering/tilemap.hpp"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

#include <LDtkLoader/Project.hpp>

namespace raven {

bool Tilemap::load(SDL_Renderer* renderer, const std::string& ldtk_path,
                   const std::string& level_name) {
    ldtk::Project project;
    try {
        project.loadFromFile(ldtk_path);
    } catch (const std::exception& e) {
        spdlog::error("Failed to load LDtk project '{}': {}", ldtk_path, e.what());
        return false;
    }

    const ldtk::World* world = nullptr;
    try {
        world = &project.getWorld();
    } catch (const std::exception& e) {
        spdlog::error("Failed to get default world: {}", e.what());
        return false;
    }

    const ldtk::Level* level = nullptr;
    try {
        level = &world->getLevel(level_name);
    } catch (const std::exception& e) {
        spdlog::error("Failed to get level '{}': {}", level_name, e.what());
        return false;
    }

    width_px_ = level->size.x;
    height_px_ = level->size.y;

    // Resolve base directory from ldtk_path (no std::filesystem)
    std::string base_dir;
    auto last_slash = ldtk_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        base_dir = ldtk_path.substr(0, last_slash + 1);
    }

    // Iterate layers in reverse (LDtk orders front-to-back; we want back-to-front)
    const auto& layers = level->allLayers();
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        const auto& layer = *it;

        if (!layer.isVisible()) {
            continue;
        }

        auto layer_type = layer.getType();

        if (layer_type == ldtk::LayerType::Tiles || layer_type == ldtk::LayerType::AutoLayer) {
            // Load tileset texture (first time only)
            if (!texture_ && layer.hasTileset()) {
                const auto& tileset = layer.getTileset();
                std::string tex_path = base_dir + tileset.path;

                SDL_Surface* surface = IMG_Load(tex_path.c_str());
                if (!surface) {
                    spdlog::error("Failed to load tileset '{}': {}", tex_path, SDL_GetError());
                    continue;
                }
                texture_ = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_DestroySurface(surface);
                if (!texture_) {
                    spdlog::error("Failed to create tileset texture: {}", SDL_GetError());
                    continue;
                }
                SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_PIXELART);
            }

            int cell = layer.getCellSize();
            for (const auto& tile : layer.allTiles()) {
                auto pos = tile.getPosition();
                auto tex_rect = tile.getTextureRect();

                TileData td{};
                td.src = {tex_rect.x, tex_rect.y, tex_rect.width, tex_rect.height};
                td.dest_x = pos.x;
                td.dest_y = pos.y;
                td.flip_x = tile.flipX;
                td.flip_y = tile.flipY;
                tiles_.push_back(td);
            }

            // Use this layer's cell size if we haven't set one from IntGrid
            if (cell_size_ == 0) {
                cell_size_ = cell;
            }
        } else if (layer_type == ldtk::LayerType::IntGrid) {
            auto grid_size = layer.getGridSize();
            cell_size_ = layer.getCellSize();
            grid_w_ = grid_size.x;
            grid_h_ = grid_size.y;
            collision_grid_.resize(static_cast<size_t>(grid_w_ * grid_h_), false);

            for (int gy = 0; gy < grid_h_; ++gy) {
                for (int gx = 0; gx < grid_w_; ++gx) {
                    const auto& val = layer.getIntGridVal(gx, gy);
                    if (val.value > 0) {
                        collision_grid_[static_cast<size_t>(gy * grid_w_ + gx)] = true;
                    }
                }
            }

            // IntGrid layers can also have auto-tiles
            if (layer.hasTileset()) {
                if (!texture_) {
                    const auto& tileset = layer.getTileset();
                    std::string tex_path = base_dir + tileset.path;

                    SDL_Surface* surface = IMG_Load(tex_path.c_str());
                    if (!surface) {
                        spdlog::error("Failed to load tileset '{}': {}", tex_path, SDL_GetError());
                    } else {
                        texture_ = SDL_CreateTextureFromSurface(renderer, surface);
                        SDL_DestroySurface(surface);
                        if (texture_) {
                            SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_PIXELART);
                        }
                    }
                }

                for (const auto& tile : layer.allTiles()) {
                    auto pos = tile.getPosition();
                    auto tex_rect = tile.getTextureRect();

                    TileData td{};
                    td.src = {tex_rect.x, tex_rect.y, tex_rect.width, tex_rect.height};
                    td.dest_x = pos.x;
                    td.dest_y = pos.y;
                    td.flip_x = tile.flipX;
                    td.flip_y = tile.flipY;
                    tiles_.push_back(td);
                }
            }
        } else if (layer_type == ldtk::LayerType::Entities) {
            for (const auto& entity : layer.allEntities()) {
                auto pos = entity.getPosition();
                SpawnPoint sp{
                    entity.getName(), static_cast<float>(pos.x), static_cast<float>(pos.y), {}};

                // Extract string fields from LDtk entity
                for (const auto& field_def : entity.allFields()) {
                    if (field_def.type == ldtk::FieldType::String) {
                        try {
                            const auto& field = entity.getField<std::string>(field_def.name);
                            if (!field.is_null()) {
                                sp.fields[field_def.name] = field.value();
                            }
                        } catch (...) {
                            // Field access can throw on type mismatch; skip silently
                        }
                    }
                }

                spawns_.push_back(std::move(sp));
            }
        }
    }

    loaded_ = true;
    spdlog::info("Loaded LDtk level '{}': {}x{} px, {} tiles, {} spawns, {}x{} collision grid",
                 level_name, width_px_, height_px_, tiles_.size(), spawns_.size(), grid_w_,
                 grid_h_);
    return true;
}

} // namespace raven
