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
    bool enabled = true;        ///< Run the lighting pass at all.
    int ambient = -4;           ///< Ramp steps added to every lit pixel; negative darkens.
    int dither_band = 8;        ///< 0 flat rings, 8 fully dithered falloff.
    int player_radius = 80;     ///< Radius of the light a player carries, in pixels.
    int player_zones = 0;       ///< Ramp steps that light adds at its centre; 0 is no light.
    int bullet_radius = 16;     ///< Radius of the light a bullet carries, in pixels.
    int bullet_level = 24;      ///< That light's strength at its centre, sixteenths of a step.
    int light_ceiling = 56;     ///< Most light one pixel takes however many lights
                                ///< overlap, in sixteenths of a step.
    bool light_sprites = false; ///< Light sprites as well as tiles. Off, the pass
                                ///< runs before sprites are drawn: players,
                                ///< enemies and bullets keep their drawn colours.
    int fire_radius = 54;       ///< Radius of the light a placed fire gives, in pixels.
    int fire_level = 44;        ///< That light's strength at its centre, sixteenths of a step.
    int fire_flicker = 6;       ///< How far the strength swells and sinks, sixteenths of a step.
    bool use_loop = false;      ///< Compute lights in the shader loop (at most
                                ///< MAX_LOOP_LIGHTS) instead of the light buffer.
};

/// @brief One light for one frame, in virtual-resolution pixels.
struct Light {
    float x = 0.f;      ///< Centre X.
    float y = 0.f;      ///< Centre Y.
    float radius = 0.f; ///< Reach in pixels.
    int level = 0;      ///< Light added at the centre, in sixteenths of a ramp step.
};

/// @brief Resolves the drawn world through the lighting shader.
///
/// A prototype behind RAVEN_ENABLE_SHADER_LIGHTING. It needs SDL's "gpu"
/// renderer; without one, init() fails and the game draws as it always has.
///
/// Lights are drawn as squares into a light buffer the size of the frame,
/// and the lighting shader reads the sum at each pixel. The cost of a light
/// is its area, and there is no limit on their number. The first version,
/// a loop over lights in the shader, is kept behind
/// LightingSettings::use_loop to compare the two.
class LightingPass {
  public:
    static constexpr int MAX_LOOP_LIGHTS = 32; ///< Matches lighting.frag.hlsl.
    static constexpr int MAX_LEVEL = 255;      ///< Most light one pixel can hold, sixteenths.

    /// @brief Create the shader, load the lookup tables and build the render state.
    /// @param renderer The SDL renderer; must be the "gpu" renderer.
    /// @param ramps_path PNG of the ramps, one row per ramp.
    /// @param reverse_path PNG mapping colour to ramp and step.
    /// @param width Width of the frames apply() will be given, in pixels.
    /// @param height Height of those frames.
    /// @return True when the pass is ready to use.
    bool init(SDL_Renderer* renderer, const std::string& ramps_path,
              const std::string& reverse_path, int width, int height);

    /// @brief Release everything init() created.
    void shutdown();

    /// @brief Whether init() succeeded.
    /// @return True if apply() will do anything.
    [[nodiscard]] bool ready() const { return state_ != nullptr; }

    /// @brief Draw `source` into `target` through the lighting shader.
    /// @param source The world as drawn, in colour.
    /// @param target The render target that receives the lit frame.
    /// @param settings Ambient shift, dither band and which way lights are computed.
    /// @param lights This frame's lights. With LightingSettings::use_loop,
    ///        those past MAX_LOOP_LIGHTS are ignored.
    void apply(SDL_Texture* source, SDL_Texture* target, const LightingSettings& settings,
               const std::vector<Light>& lights);

  private:
    SDL_Renderer* renderer_ = nullptr;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUShader* shader_ = nullptr;       ///< Resolves the frame (lighting.frag).
    SDL_GPUShader* light_shader_ = nullptr; ///< Stamps one light (light.frag).
    SDL_GPUSampler* sampler_ = nullptr;
    SDL_GPURenderState* state_ = nullptr;
    SDL_GPURenderState* light_state_ = nullptr;
    SDL_Texture* ramps_ = nullptr;
    SDL_Texture* reverse_ = nullptr;
    /// Sum of this frame's lights. Bound to state_ when that is created, so
    /// it lives as long as the pass and is never recreated.
    SDL_Texture* light_buffer_ = nullptr;
    SDL_Texture* white_ = nullptr; ///< One white pixel: the texture a light square draws.
    int steps_ = 0;                ///< Width of the ramps table.

    /// @brief Create a fragment shader from whichever format the device takes.
    SDL_GPUShader* create_shader(const unsigned char* spv, size_t spv_size,
                                 const unsigned char* dxil, size_t dxil_size,
                                 const unsigned char* msl, size_t msl_size, Uint32 samplers,
                                 Uint32 uniform_buffers);

    /// @brief Clear the light buffer and draw the lights into it.
    void draw_light_buffer(const std::vector<Light>& lights);

    /// @brief Load a PNG as a nearest-sampled texture with its bytes unchanged.
    SDL_Texture* load_table(const std::string& path);
};

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
