#include "scenes/scene.hpp"

namespace raven {

void SceneManager::push(std::unique_ptr<Scene> scene, Game& game) {
    scene->on_enter(game);
    stack_.push_back(std::move(scene));
}

void SceneManager::pop(Game& game) {
    if (!stack_.empty()) {
        stack_.back()->on_exit(game);
        stack_.pop_back();
    }
}

void SceneManager::swap(std::unique_ptr<Scene> scene, Game& game) {
    pop(game);
    push(std::move(scene), game);
}

void SceneManager::update(Game& game, float dt) {
    if (!stack_.empty()) {
        stack_.back()->update(game, dt);
    }
}

void SceneManager::render(Game& game) {
    // Render all scenes bottom-up (for transparency/overlay support)
    for (auto& scene : stack_) {
        scene->render(game);
    }
}

} // namespace raven
