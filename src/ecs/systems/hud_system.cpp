#include "ecs/systems/hud_system.hpp"

#include "ecs/components.hpp"

namespace raven::systems {

void render_hud(entt::registry& reg, SDL_Renderer* renderer) {
    constexpr int margin = 4;

    // ── Health bar (top-left) ──────────────────────────────────────
    constexpr int hp_bar_w = 40;
    constexpr int hp_bar_h = 4;
    constexpr int hp_bar_x = margin;
    constexpr int hp_bar_y = margin;

    auto player_view = reg.view<Player, Health>();
    for (auto [entity, player, hp] : player_view.each()) {
        // Background (dark gray)
        SDL_Rect bg{hp_bar_x, hp_bar_y, hp_bar_w, hp_bar_h};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &bg);

        // Fill (red, or white if invulnerable)
        float ratio = (hp.max > 0.f) ? (hp.current / hp.max) : 0.f;
        if (ratio < 0.f)
            ratio = 0.f;
        if (ratio > 1.f)
            ratio = 1.f;
        int fill_w = static_cast<int>(static_cast<float>(hp_bar_w) * ratio);

        bool invulnerable = reg.any_of<Invulnerable>(entity);
        if (invulnerable) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);
        }
        if (fill_w > 0) {
            SDL_Rect fill{hp_bar_x, hp_bar_y, fill_w, hp_bar_h};
            SDL_RenderFillRect(renderer, &fill);
        }

        // ── Lives pips (right of health bar) ───────────────────────
        constexpr int pip_size = 4;
        constexpr int pip_spacing = 2;
        int pip_x = hp_bar_x + hp_bar_w + margin;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < player.lives; ++i) {
            SDL_Rect pip{pip_x + i * (pip_size + pip_spacing), hp_bar_y, pip_size, pip_size};
            SDL_RenderFillRect(renderer, &pip);
        }

        // ── Weapon decay timer (below health bar) ─────────────────
        if (auto* decay = reg.try_get<WeaponDecay>(entity)) {
            constexpr int decay_bar_w = 30;
            constexpr int decay_bar_h = 3;
            int decay_x = hp_bar_x;
            int decay_y = hp_bar_y + hp_bar_h + 2;

            // Background
            SDL_Rect decay_bg{decay_x, decay_y, decay_bar_w, decay_bar_h};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &decay_bg);

            // Yellow fill
            float decay_ratio = decay->remaining / 10.f;
            if (decay_ratio < 0.f)
                decay_ratio = 0.f;
            if (decay_ratio > 1.f)
                decay_ratio = 1.f;
            int decay_fill_w = static_cast<int>(static_cast<float>(decay_bar_w) * decay_ratio);
            if (decay_fill_w > 0) {
                SDL_Rect decay_fill{decay_x, decay_y, decay_fill_w, decay_bar_h};
                SDL_SetRenderDrawColor(renderer, 230, 200, 50, 255);
                SDL_RenderFillRect(renderer, &decay_fill);
            }
        }

        // ── Ability cooldown bar (bottom-left) ──────────────────
        constexpr int ability_bar_w = 30;
        constexpr int ability_bar_h = 3;
        constexpr int ability_bar_x = margin;
        int ability_bar_y = 270 - margin - ability_bar_h; // bottom of 480x270

        float ability_ratio = 0.f;
        bool has_ability = false;
        if (auto* slam_cd = reg.try_get<GroundSlamCooldown>(entity)) {
            has_ability = true;
            ability_ratio =
                (slam_cd->rate > 0.f) ? (1.f - slam_cd->remaining / slam_cd->rate) : 1.f;
        } else if (auto* conc_cd = reg.try_get<ConcussionShotCooldown>(entity)) {
            has_ability = true;
            ability_ratio =
                (conc_cd->rate > 0.f) ? (1.f - conc_cd->remaining / conc_cd->rate) : 1.f;
        }

        if (has_ability) {
            if (ability_ratio < 0.f)
                ability_ratio = 0.f;
            if (ability_ratio > 1.f)
                ability_ratio = 1.f;

            // Background
            SDL_Rect ability_bg{ability_bar_x, ability_bar_y, ability_bar_w, ability_bar_h};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &ability_bg);

            // Cyan fill (ready = full)
            int ability_fill_w =
                static_cast<int>(static_cast<float>(ability_bar_w) * ability_ratio);
            if (ability_fill_w > 0) {
                SDL_Rect ability_fill{ability_bar_x, ability_bar_y, ability_fill_w, ability_bar_h};
                if (ability_ratio >= 1.f) {
                    SDL_SetRenderDrawColor(renderer, 100, 220, 255, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 50, 110, 130, 255);
                }
                SDL_RenderFillRect(renderer, &ability_fill);
            }
        }

        // ── Charge indicator (bottom-left, above ability bar) ───
        if (auto* cs = reg.try_get<ChargedShot>(entity); cs && cs->charging) {
            constexpr int charge_bar_w = 30;
            constexpr int charge_bar_h = 3;
            int charge_bar_y = ability_bar_y - charge_bar_h - 2;

            // Background
            SDL_Rect charge_bg{margin, charge_bar_y, charge_bar_w, charge_bar_h};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &charge_bg);

            // Orange fill
            float charge = cs->charge;
            if (charge > 1.f)
                charge = 1.f;
            int charge_fill_w = static_cast<int>(static_cast<float>(charge_bar_w) * charge);
            if (charge_fill_w > 0) {
                SDL_Rect charge_fill{margin, charge_bar_y, charge_fill_w, charge_bar_h};
                if (cs->charge >= cs->full_charge_threshold) {
                    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 200, 120, 40, 255);
                }
                SDL_RenderFillRect(renderer, &charge_fill);
            }
        }
    }

    // ── Score (top-right) ──────────────────────────────────────────
    auto* state = reg.ctx().find<GameState>();
    if (state) {
        int score = state->score;

        // Extract digits
        int digits[10] = {};
        int num_digits = 0;
        if (score == 0) {
            digits[0] = 0;
            num_digits = 1;
        } else {
            int temp = score;
            while (temp > 0 && num_digits < 10) {
                digits[num_digits++] = temp % 10;
                temp /= 10;
            }
            // Reverse
            for (int i = 0; i < num_digits / 2; ++i) {
                int swap = digits[i];
                digits[i] = digits[num_digits - 1 - i];
                digits[num_digits - 1 - i] = swap;
            }
        }

        // Draw digits as small rectangles (5x7 each, 1px gap)
        constexpr int digit_w = 5;
        constexpr int digit_h = 7;
        constexpr int digit_gap = 1;
        int total_w = num_digits * digit_w + (num_digits - 1) * digit_gap;
        int score_x = 480 - margin - total_w; // Right-aligned (480 = virtual width)
        int score_y = margin;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < num_digits; ++i) {
            SDL_Rect digit_rect{score_x + i * (digit_w + digit_gap), score_y, digit_w, digit_h};
            // Vary brightness slightly by digit value
            int brightness = 180 + digits[i] * 7;
            if (brightness > 255)
                brightness = 255;
            SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(brightness),
                                   static_cast<Uint8>(brightness), static_cast<Uint8>(brightness),
                                   255);
            SDL_RenderFillRect(renderer, &digit_rect);
        }

        // ── Wave indicator (top-center) ────────────────────────────
        int total_waves = state->total_waves;
        if (total_waves > 0) {
            constexpr int dot_size = 3;
            constexpr int dot_gap = 3;
            int dots_total_w = total_waves * dot_size + (total_waves - 1) * dot_gap;
            int dots_x = (480 - dots_total_w) / 2;
            int dots_y = margin;

            for (int i = 0; i < total_waves; ++i) {
                SDL_Rect dot{dots_x + i * (dot_size + dot_gap), dots_y, dot_size, dot_size};
                if (i < state->current_wave) {
                    // Completed wave — filled bright
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                    SDL_RenderFillRect(renderer, &dot);
                } else if (i == state->current_wave && !state->room_cleared) {
                    // Current wave — bright filled
                    SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
                    SDL_RenderFillRect(renderer, &dot);
                } else {
                    // Remaining — hollow
                    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                    SDL_RenderDrawRect(renderer, &dot);
                }
            }
        }
    }
}

} // namespace raven::systems
