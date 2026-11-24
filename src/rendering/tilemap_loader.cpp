#include "core/fs.hpp"
#include "rendering/tilemap.hpp"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

#include <LDtkLoader/Project.hpp>
#include <algorithm>
#include <cmath>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>

namespace raven {

namespace {

/// @brief LDtkLoader file hook: read project and external level files
/// through the fs seam instead of the library's own std::ifstream.
///
/// A missing file yields an empty stream buffer; the library's JSON parse
/// then throws, which Tilemap::load already catches and reports.
std::unique_ptr<std::streambuf> ldtk_file_loader(const std::string& path) {
    auto text = fs::read_text(path);
    return std::make_unique<std::stringbuf>(text ? std::move(*text) : std::string{});
}

/// @brief Copy one LDtk entity field into a spawn point, as text.
///
/// Null values are left out. Field types with no text form here (colours,
/// points, entity references, arrays) are reported, so a field that the
/// game never sees is not a silent surprise.
void read_field(const ldtk::Entity& entity, const ldtk::FieldDef& def, SpawnPoint& sp) {
    // Store a field's value unless it is null
    auto store = [&](const auto& field, auto to_text) {
        if (!field.is_null()) {
            sp.fields[def.name] = to_text(field.value());
        }
    };
    try {
        switch (def.type) {
        case ldtk::FieldType::String:
            store(entity.getField<std::string>(def.name), [](const std::string& v) { return v; });
            break;
        case ldtk::FieldType::Int:
            store(entity.getField<int>(def.name), [](int v) { return std::to_string(v); });
            break;
        case ldtk::FieldType::Float:
            store(entity.getField<float>(def.name), [](float v) { return std::to_string(v); });
            break;
        case ldtk::FieldType::Bool:
            store(entity.getField<bool>(def.name),
                  [](bool v) { return std::string{v ? "true" : "false"}; });
            break;
        case ldtk::FieldType::Enum:
            store(entity.getField<ldtk::EnumValue>(def.name),
                  [](const ldtk::EnumValue& v) { return v.name; });
            break;
        case ldtk::FieldType::FilePath:
            store(entity.getField<ldtk::FilePath>(def.name),
                  [](const ldtk::FilePath& v) { return std::string{v.c_str()}; });
            break;
        default:
            spdlog::warn("LDtk field '{}' on entity '{}' has a type the game does not read "
                         "(colour, point, entity reference or array), so it is ignored",
                         def.name, sp.name);
            break;
        }
    } catch (const std::exception& e) {
        spdlog::warn("LDtk field '{}' on entity '{}' could not be read: {}", def.name, sp.name,
                     e.what());
    }
}

} // anonymous namespace

bool Tilemap::load(SDL_Renderer* renderer, const std::string& ldtk_path,
                   const std::string& level_name) {
    ldtk::Project project;
    try {
        project.loadFromFile(ldtk_path, ldtk_file_loader);
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

    // One texture per tileset, loaded on first use. Maps tileset uid to an
    // index into textures_, or -1 if it failed to load (so it is reported
    // once, not once per layer).
    std::unordered_map<int, int> texture_index;
    auto texture_for = [&](const ldtk::Tileset& tileset) -> int {
        if (auto it = texture_index.find(tileset.uid); it != texture_index.end()) {
            return it->second;
        }
        int index = -1;
        std::string tex_path = base_dir + tileset.path;
        if (SDL_Surface* surface = IMG_Load(tex_path.c_str())) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
            if (texture) {
                SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
                index = static_cast<int>(textures_.size());
                textures_.push_back(texture);
            } else {
                spdlog::error("Failed to create tileset texture '{}': {}", tex_path,
                              SDL_GetError());
            }
        } else {
            spdlog::error("Failed to load tileset '{}': {}", tex_path, SDL_GetError());
        }
        texture_index.emplace(tileset.uid, index);
        return index;
    };

    auto add_tiles = [&](const ldtk::Layer& layer) {
        if (!layer.hasTileset()) {
            return;
        }
        if (!layer.isVisible()) {
            spdlog::info("Layer '{}' is hidden in LDtk, so its tiles are not drawn",
                         layer.getName());
            return;
        }
        const int texture = texture_for(layer.getTileset());
        if (texture < 0) {
            return;
        }
        const auto alpha =
            static_cast<uint8_t>(std::lround(std::clamp(layer.getOpacity(), 0.f, 1.f) * 255.f));
        if (alpha < 255) {
            // Alpha only shows on a texture that blends; a tileset image
            // with no alpha channel of its own starts out opaque
            SDL_SetTextureBlendMode(textures_[static_cast<size_t>(texture)], SDL_BLENDMODE_BLEND);
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
            td.texture = static_cast<uint16_t>(texture);
            td.alpha = alpha;
            tiles_.push_back(td);
        }
    };

    bool has_collision = false;

    // Iterate layers in reverse (LDtk orders front-to-back; we want back-to-front)
    const auto& layers = level->allLayers();
    for (const auto& layer : std::views::reverse(layers)) {
        // Any layer's cell size will do until the collision layer sets it
        if (cell_size_ == 0) {
            cell_size_ = layer.getCellSize();
        }

        switch (layer.getType()) {
        case ldtk::LayerType::Tiles:
        case ldtk::LayerType::AutoLayer:
            add_tiles(layer);
            break;

        case ldtk::LayerType::IntGrid:
            if (layer.getName() == COLLISION_LAYER) {
                auto grid_size = layer.getGridSize();
                cell_size_ = layer.getCellSize();
                grid_w_ = grid_size.x;
                grid_h_ = grid_size.y;
                collision_grid_.assign(static_cast<size_t>(grid_w_) * static_cast<size_t>(grid_h_),
                                       false);

                for (int gy = 0; gy < grid_h_; ++gy) {
                    for (int gx = 0; gx < grid_w_; ++gx) {
                        if (layer.getIntGridVal(gx, gy).value > 0) {
                            collision_grid_[static_cast<size_t>(gy) * static_cast<size_t>(grid_w_) +
                                            static_cast<size_t>(gx)] = true;
                        }
                    }
                }
                has_collision = true;
            }
            // IntGrid layers can also have auto-tiles
            add_tiles(layer);
            break;

        case ldtk::LayerType::Entities:
            for (const auto& entity : layer.allEntities()) {
                auto pos = entity.getPosition();
                SpawnPoint sp{
                    entity.getName(), static_cast<float>(pos.x), static_cast<float>(pos.y), {}};

                for (const auto& field_def : entity.allFields()) {
                    read_field(entity, field_def, sp);
                }

                spawns_.push_back(std::move(sp));
            }
            break;
        }
    }

    if (cell_size_ <= 0) {
        // Grid queries divide by the cell size
        spdlog::error("LDtk level '{}' has no layers with a cell size", level_name);
        return false;
    }
    if (!has_collision) {
        spdlog::warn("LDtk level '{}' has no IntGrid layer named '{}', so nothing is solid",
                     level_name, COLLISION_LAYER);
    }

    loaded_ = true;
    spdlog::info("Loaded LDtk level '{}': {}x{} px, {} tiles, {} spawns, {}x{} collision grid",
                 level_name, width_px_, height_px_, tiles_.size(), spawns_.size(), grid_w_,
                 grid_h_);
    return true;
}

} // namespace raven
