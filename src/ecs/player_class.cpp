#include "ecs/player_class.hpp"

#include "core/string_id.hpp"
#include "ecs/components.hpp"

namespace raven {

void apply_brawler(entt::registry& reg, entt::entity entity) {
    auto& player = reg.get<Player>(entity);
    player.speed = 100.f;
    player.lives = 3;

    auto& hp = reg.get<Health>(entity);
    hp.current = 150.f;
    hp.max = 150.f;

    reg.emplace<ClassId>(entity, ClassId::Id::Brawler);
    reg.emplace<MeleeStats>(entity, MeleeStats{3.f, 35.f, 0.785f, 300.f, 0.12f});
    reg.emplace<GroundSlamCooldown>(entity);

    // Weapon: default stats (bullet_damage=1)
}

void apply_sharpshooter(entt::registry& reg, entt::entity entity) {
    auto& player = reg.get<Player>(entity);
    player.speed = 150.f;
    player.lives = 2;

    auto& hp = reg.get<Health>(entity);
    hp.current = 60.f;
    hp.max = 60.f;

    reg.emplace<ClassId>(entity, ClassId::Id::Sharpshooter);
    reg.emplace<MeleeStats>(entity, MeleeStats{1.f, 20.f, 0.4f, 150.f, 0.08f});
    reg.emplace<ChargedShot>(entity);
    reg.emplace<ConcussionShotCooldown>(entity);

    auto& weapon = reg.get<Weapon>(entity);
    weapon.bullet_damage = 1.5f;
    weapon.fire_rate = 0.3f;
}

void apply_knight(entt::registry& reg, entt::entity entity) {
    auto& player = reg.get<Player>(entity);
    player.speed = 125.f;
    player.lives = 3;

    auto& hp = reg.get<Health>(entity);
    hp.current = 100.f;
    hp.max = 100.f;

    reg.emplace<ClassId>(entity, ClassId::Id::Knight);
    reg.emplace<MeleeStats>(entity, MeleeStats{2.f, 30.f, 0.785f, 250.f, 0.1f});

    // The knight has its own sheet (assets/sprites/knight.png, same row
    // layout as the placeholder); other classes keep the "player" sheet.
    if (auto* sprite = reg.try_get<Sprite>(entity)) {
        if (auto* interner = reg.ctx().find<StringInterner>()) {
            sprite->sheet_id = interner->intern("knight");
        }
    }

    // Signature: the dash chain. Each dash from neutral grants one
    // follow-up token, spendable on a second dash or the 360-degree
    // dash-spin (see dash_system / melee_system).
    reg.emplace<DashChainTalent>(entity);

    // Weapon: default stats (bullet_damage=1)
}

} // namespace raven
