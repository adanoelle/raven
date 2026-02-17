#include "ecs/player_class.hpp"

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

} // namespace raven
