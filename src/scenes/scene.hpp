#pragma once

#include <memory>
#include <vector>

namespace raven {

class Game; // forward declare

/// Abstract scene interface.
class Scene {
public:
    virtual ~Scene() = default;

    virtual void on_enter(Game& game) { (void)game; }
    virtual void on_exit(Game& game) { (void)game; }
    virtual void update(Game& game, float dt) = 0;
    virtual void render(Game& game) = 0;
};

/// Stack-based scene manager. Top scene gets updates/renders.
/// Supports push (overlay, e.g. pause) and swap (transition).
class SceneManager {
public:
    void push(std::unique_ptr<Scene> scene, Game& game);
    void pop(Game& game);
    void swap(std::unique_ptr<Scene> scene, Game& game);

    void update(Game& game, float dt);
    void render(Game& game);

    [[nodiscard]] bool empty() const { return stack_.empty(); }

private:
    std::vector<std::unique_ptr<Scene>> stack_;
};

} // namespace raven
