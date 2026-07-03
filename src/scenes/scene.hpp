#pragma once

#include <memory>
#include <vector>

namespace raven {

class Game; // forward declare

/// @brief Abstract scene interface. Scenes define update/render behaviour
/// and receive lifecycle callbacks when pushed onto or popped from the stack.
class Scene {
  public:
    virtual ~Scene() = default;

    /// @brief Called when the scene is pushed onto the stack.
    /// @param game The Game instance providing access to subsystems.
    virtual void on_enter(Game& game) { (void)game; }

    /// @brief Called when the scene is popped from the stack.
    /// @param game The Game instance providing access to subsystems.
    virtual void on_exit(Game& game) { (void)game; }

    /// @brief Run one fixed-timestep tick of scene logic.
    /// @param game The Game instance providing access to subsystems.
    /// @param dt Fixed timestep delta in seconds.
    virtual void update(Game& game, float dt) = 0;

    /// @brief Render the scene for the current frame.
    /// @param game The Game instance providing access to subsystems.
    virtual void render(Game& game) = 0;
};

/// @brief Stack-based scene manager. The top scene receives updates and renders.
///
/// Supports push (for overlays like pause menus) and swap (for transitions).
///
/// Transitions requested from inside a scene's update() are deferred and
/// applied after the update returns — popping a scene destroys it, so
/// applying immediately would free the scene that is still executing.
class SceneManager {
  public:
    /// @brief Push a scene onto the stack, becoming the active scene.
    /// @param scene Owning pointer to the new scene.
    /// @param game The Game instance passed to on_enter().
    void push(std::unique_ptr<Scene> scene, Game& game);

    /// @brief Pop the top scene, returning control to the one below it.
    /// @param game The Game instance passed to on_exit().
    void pop(Game& game);

    /// @brief Replace the top scene with a new one (pop + push).
    /// @param scene Owning pointer to the replacement scene.
    /// @param game The Game instance passed to on_exit() and on_enter().
    void swap(std::unique_ptr<Scene> scene, Game& game);

    /// @brief Pop all scenes, calling on_exit() on each from top to bottom.
    ///
    /// Must not be called from inside a scene's update(). Use during game
    /// shutdown so scene destructors run while SDL resources still exist.
    /// @param game The Game instance passed to on_exit().
    void clear(Game& game);

    /// @brief Update the top scene.
    /// @param game The Game instance.
    /// @param dt Fixed timestep delta in seconds.
    void update(Game& game, float dt);

    /// @brief Render the top scene.
    /// @param game The Game instance.
    void render(Game& game);

    /// @brief Check whether the scene stack is empty.
    /// @return True if no scenes are on the stack.
    [[nodiscard]] bool empty() const { return stack_.empty(); }

    /// @brief Check whether a scene is the active (top) scene.
    ///
    /// Scenes below an overlay (e.g. GameScene under PauseScene) still
    /// render; they can use this to adapt, such as freezing interpolation.
    /// @param scene The scene to test.
    /// @return True if scene is currently on top of the stack.
    [[nodiscard]] bool is_top(const Scene* scene) const {
        return !stack_.empty() && stack_.back().get() == scene;
    }

  private:
    /// @brief A push/pop/swap requested during update(), applied afterwards.
    struct PendingOp {
        enum class Kind : uint8_t { Push, Pop, Swap };
        Kind kind;
        std::unique_ptr<Scene> scene; ///< Null for Pop.
    };

    std::vector<std::unique_ptr<Scene>> stack_;
    std::vector<PendingOp> pending_;
    bool updating_ = false; ///< True while the top scene's update() runs.
};

} // namespace raven
