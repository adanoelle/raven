#include "ecs/systems/dash_system.hpp"

#include "ecs/components.hpp"

#include <algorithm>
#include <cmath>

namespace raven::systems {

void update_dash(entt::registry& reg, const InputState& input, float dt) {
    // Tick dash cooldowns
    auto cd_view = reg.view<DashCooldown>();
    for (auto [entity, cooldown] : cd_view.each()) {
        cooldown.remaining -= dt;
    }

    // Tick follow-up windows; unspent follow-ups expire
    auto fu_view = reg.view<DashFollowUp>();
    for (auto [entity, followup] : fu_view.each()) {
        followup.remaining -= dt;
        if (followup.remaining <= 0.f) {
            reg.remove<DashFollowUp>(entity);
        }
    }

    // Initiate dash on press
    auto player_view = reg.view<Player, Velocity, AimDirection, DashCooldown>();
    for (auto [entity, player, vel, aim, cooldown] : player_view.each()) {
        if (!input.dash_pressed) {
            continue;
        }
        // A follow-up dash bypasses the cooldown started by the first dash.
        const bool is_follow_up = reg.any_of<DashFollowUp>(entity);
        if (cooldown.remaining > 0.f && !is_follow_up) {
            continue;
        }
        if (reg.any_of<Dash>(entity)) {
            continue;
        }

        // Direction: movement input if significant, else aim direction
        float dir_x = input.move_x;
        float dir_y = input.move_y;
        if (dir_x * dir_x + dir_y * dir_y < 0.01f) {
            dir_x = aim.x;
            dir_y = aim.y;
        }

        // Normalize direction
        float len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
        if (len > 0.f) {
            dir_x /= len;
            dir_y /= len;
        }

        Dash dash;
        dash.dir_x = dir_x;
        dash.dir_y = dir_y;
        reg.emplace<Dash>(entity, dash);
        cooldown.remaining = cooldown.rate;

        // Dash-chain prototype (Knight talent): a dash from neutral grants
        // one follow-up token, spendable on a second dash (here) or the
        // 360-degree spin (melee_system). A follow-up dash consumes it;
        // the chain is over.
        if (is_follow_up) {
            reg.remove<DashFollowUp>(entity);
        } else if (reg.any_of<DashChainTalent>(entity)) {
            reg.emplace<DashFollowUp>(entity);
        }
        push_sfx(reg, Sfx::Dash);

        // Grant invulnerability (slightly longer than dash for grace period).
        // Never shorten i-frames already granted, e.g. by a recent hit.
        auto& inv = reg.get_or_emplace<Invulnerable>(entity, 0.f);
        inv.remaining = std::max(inv.remaining, 0.18f);
    }

    // Process active dashes: override velocity
    auto dash_view = reg.view<Velocity, Dash>();
    for (auto [entity, vel, dash] : dash_view.each()) {
        vel.dx = dash.dir_x * dash.speed;
        vel.dy = dash.dir_y * dash.speed;

        dash.remaining -= dt;
        if (dash.remaining <= 0.f) {
            reg.remove<Dash>(entity);
        }
    }
}

} // namespace raven::systems
