#pragma once

#include "scenes/scene.hpp"

namespace raven {

class TitleScene : public Scene {
public:
    void on_enter(Game& game) override;
    void update(Game& game, float dt) override;
    void render(Game& game) override;

private:
    float blink_timer_ = 0.f;
    bool show_prompt_ = true;
};

} // namespace raven
