#pragma once

#include "scenes/scene.hpp"

namespace raven {

class GameScene : public Scene {
public:
    void on_enter(Game& game) override;
    void on_exit(Game& game) override;
    void update(Game& game, float dt) override;
    void render(Game& game) override;

private:
    void spawn_player(Game& game);
};

} // namespace raven
