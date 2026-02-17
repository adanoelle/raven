#include "rendering/tilemap.hpp"

#include <algorithm>
#include <utility>

namespace raven {

Tilemap::~Tilemap() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
}

Tilemap::Tilemap(Tilemap&& other) noexcept
    : texture_(std::exchange(other.texture_, nullptr)), tiles_(std::move(other.tiles_)),
      collision_grid_(std::move(other.collision_grid_)), spawns_(std::move(other.spawns_)),
      width_px_(other.width_px_), height_px_(other.height_px_), cell_size_(other.cell_size_),
      grid_w_(other.grid_w_), grid_h_(other.grid_h_), loaded_(other.loaded_) {
    other.loaded_ = false;
}

Tilemap& Tilemap::operator=(Tilemap&& other) noexcept {
    if (this != &other) {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
        texture_ = std::exchange(other.texture_, nullptr);
        tiles_ = std::move(other.tiles_);
        collision_grid_ = std::move(other.collision_grid_);
        spawns_ = std::move(other.spawns_);
        width_px_ = other.width_px_;
        height_px_ = other.height_px_;
        cell_size_ = other.cell_size_;
        grid_w_ = other.grid_w_;
        grid_h_ = other.grid_h_;
        loaded_ = other.loaded_;
        other.loaded_ = false;
    }
    return *this;
}

void Tilemap::init_collision(int w, int h, int cell, std::vector<bool> grid) {
    grid_w_ = w;
    grid_h_ = h;
    cell_size_ = cell;
    collision_grid_ = std::move(grid);
    width_px_ = w * cell;
    height_px_ = h * cell;
    loaded_ = true;
}

bool Tilemap::is_solid(float x, float y, float w, float h) const {
    if (cell_size_ <= 0 || grid_w_ <= 0 || grid_h_ <= 0) {
        return false;
    }

    // Early out if entirely outside grid bounds
    float right = x + w - 1.f;
    float bottom = y + h - 1.f;
    if (right < 0.f || bottom < 0.f || x >= static_cast<float>(width_px_) ||
        y >= static_cast<float>(height_px_)) {
        return false;
    }

    // Floor-divide to get grid coordinates, then clamp to valid range
    auto floor_div = [](int a, int b) -> int { return (a >= 0) ? a / b : (a - b + 1) / b; };
    int min_gx = std::max(0, floor_div(static_cast<int>(x), cell_size_));
    int min_gy = std::max(0, floor_div(static_cast<int>(y), cell_size_));
    int max_gx = std::min(grid_w_ - 1, floor_div(static_cast<int>(right), cell_size_));
    int max_gy = std::min(grid_h_ - 1, floor_div(static_cast<int>(bottom), cell_size_));

    for (int gy = min_gy; gy <= max_gy; ++gy) {
        for (int gx = min_gx; gx <= max_gx; ++gx) {
            if (collision_grid_[static_cast<size_t>(gy * grid_w_ + gx)]) {
                return true;
            }
        }
    }
    return false;
}

bool Tilemap::is_cell_solid(int grid_x, int grid_y) const {
    if (grid_x < 0 || grid_x >= grid_w_ || grid_y < 0 || grid_y >= grid_h_) {
        return false;
    }
    return collision_grid_[static_cast<size_t>(grid_y * grid_w_ + grid_x)];
}

const SpawnPoint* Tilemap::find_spawn(const std::string& name) const {
    for (const auto& sp : spawns_) {
        if (sp.name == name) {
            return &sp;
        }
    }
    return nullptr;
}

std::vector<const SpawnPoint*> Tilemap::find_all_spawns(const std::string& name) const {
    std::vector<const SpawnPoint*> result;
    for (const auto& sp : spawns_) {
        if (sp.name == name) {
            result.push_back(&sp);
        }
    }
    return result;
}

} // namespace raven
