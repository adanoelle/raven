#include "scenes/scene.hpp"

namespace raven {

void SceneManager::push(std::unique_ptr<Scene> scene, Game& game) {
    if (updating_) {
        pending_.push_back({PendingOp::Kind::Push, std::move(scene)});
        return;
    }
    scene->on_enter(game);
    stack_.push_back(std::move(scene));
}

void SceneManager::pop(Game& game) {
    if (updating_) {
        pending_.push_back({PendingOp::Kind::Pop, nullptr});
        return;
    }
    if (!stack_.empty()) {
        stack_.back()->on_exit(game);
        stack_.pop_back();
    }
}

void SceneManager::swap(std::unique_ptr<Scene> scene, Game& game) {
    if (updating_) {
        pending_.push_back({PendingOp::Kind::Swap, std::move(scene)});
        return;
    }
    pop(game);
    push(std::move(scene), game);
}

void SceneManager::clear(Game& game) {
    while (!stack_.empty()) {
        pop(game);
    }
    pending_.clear();
}

void SceneManager::update(Game& game, float dt) {
    if (!stack_.empty()) {
        updating_ = true;
        stack_.back()->update(game, dt);
        updating_ = false;
    }

    // Apply transitions requested during update. Move the queue out first:
    // on_enter/on_exit may request further transitions, which now apply
    // immediately (updating_ is false) without invalidating this loop.
    std::vector<PendingOp> ops = std::move(pending_);
    pending_.clear();
    for (auto& op : ops) {
        switch (op.kind) {
        case PendingOp::Kind::Push:
            push(std::move(op.scene), game);
            break;
        case PendingOp::Kind::Pop:
            pop(game);
            break;
        case PendingOp::Kind::Swap:
            swap(std::move(op.scene), game);
            break;
        }
    }
}

void SceneManager::render(Game& game) {
    // Render all scenes bottom-up (for transparency/overlay support)
    for (auto& scene : stack_) {
        scene->render(game);
    }
}

} // namespace raven
