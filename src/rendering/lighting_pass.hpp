#pragma once

#ifdef RAVEN_ENABLE_SHADER_LIGHTING

#include <SDL3/SDL.h>

#include <string>
#include <vector>

namespace raven {

/// @brief Tunable values for the shader lighting prototype (ADR-0028).
///
/// Lives in the registry context so the debug overlay can change it.
struct LightingSettings {
    bool enabled = true;    ///< Run the lighting pass at all.
    int ambient = -4;       ///< Ramp steps added to every lit pixel; negative darkens.
    int dither_band = 8;    ///< 0 flat rings, 8 fully dithered falloff.
    int player_radius = 80; ///< Radius of the light a player carries, in pixels.
    int player_zones = 7;   ///< Ramp steps that light adds at its centre.
    int bullet_radius = 20; ///< Radius of the light a bullet carries, in pixels.
    int bullet_zones = 3;   ///< Ramp steps that light adds at its centre.
};

/// @brief One light for one frame, in virtual-resolution pixels.
struct Light {
    float x = 0.f;  ///< Centre X.
    float y = 0.f;  ///< Centre Y.
    int radius = 0; ///< Reach in pixels.
    int zones = 0;  ///< Ramp steps added at the centre.
};

/// @brief Resolves the drawn world through the lighting shader.
///
/// A prototype behind RAVEN_ENABLE_SHADER_LIGHTING. It needs SDL's "gpu"
/// renderer; without one, init() fails and the game draws as it always has.
class LightingPass {
  public:
    static constexpr int MAX_LIGHTS = 32; ///< Matches lighting.frag.hlsl.

    /// @brief Create the shader, load the lookup tables and build the render state.
    /// @param renderer The SDL renderer; must be the "gpu" renderer.
    /// @param ramps_path PNG of the ramps, one row per ramp.
    /// @param reverse_path PNG mapping colour to ramp and step.
    /// @return True when the pass is ready to use.
    bool init(SDL_Renderer* renderer, const std::string& ramps_path,
              const std::string& reverse_path);

    /// @brief Release everything init() created.
    void shutdown();

    /// @brief Whether init() succeeded.
    /// @return True if apply() will do anything.
    [[nodiscard]] bool ready() const { return state_ != nullptr; }

    /// @brief Draw `source` into `target` through the lighting shader.
    /// @param source The world as drawn, in colour.
    /// @param target The render target that receives the lit frame.
    /// @param settings Ambient shift and dither band.
    /// @param lights This frame's lights; those past MAX_LIGHTS are ignored.
    void apply(SDL_Texture* source, SDL_Texture* target, const LightingSettings& settings,
               const std::vector<Light>& lights);

  private:
    SDL_Renderer* renderer_ = nullptr;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUShader* shader_ = nullptr;
    SDL_GPUSampler* sampler_ = nullptr;
    SDL_GPURenderState* state_ = nullptr;
    SDL_Texture* ramps_ = nullptr;
    SDL_Texture* reverse_ = nullptr;
    int steps_ = 0; ///< Width of the ramps table.

    /// @brief Load a PNG as a nearest-sampled texture with its bytes unchanged.
    SDL_Texture* load_table(const std::string& path);
};

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
