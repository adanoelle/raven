#include "core/input.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/input_system.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace raven;

namespace {

constexpr float DT = 1.f / 120.f;

entt::entity spawn_player(entt::registry& reg, float speed = 200.f) {
    auto ent = reg.create();
    reg.emplace<Player>(ent, speed, 3);
    reg.emplace<Velocity>(ent);
    return ent;
}

/// Run enough ticks for the exponential approach to fully converge.
void settle(entt::registry& reg, const InputState& input) {
    for (int i = 0; i < 200; ++i) {
        systems::update_input(reg, input, DT);
    }
}

} // namespace

TEST_CASE("Input system movement", "[input]") {
    entt::registry reg;

    SECTION("Diagonal input is normalised to unit length") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 1.f;
        input.move_y = 1.f;

        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        float mag = std::sqrt(vel.dx * vel.dx + vel.dy * vel.dy);
        REQUIRE(mag == Catch::Approx(200.f).margin(0.1f));
        REQUIRE(vel.dx == Catch::Approx(200.f / std::sqrt(2.f)).margin(0.1f));
        REQUIRE(vel.dy == Catch::Approx(200.f / std::sqrt(2.f)).margin(0.1f));
    }

    SECTION("Cardinal input reaches full speed") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 1.f;

        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(200.f).margin(0.1f));
        REQUIRE(vel.dy == Catch::Approx(0.f).margin(0.001f));
    }

    SECTION("Sub-unit stick input is not boosted to full speed") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 0.5f;

        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(100.f).margin(0.1f));
    }

    SECTION("Single tick applies the exponential approach factor") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 1.f;

        systems::update_input(reg, input, DT);

        float expected = 200.f * (1.f - std::exp(-60.f * DT));
        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(expected).margin(0.001f));
    }

    SECTION("Releasing input decays velocity back to zero") {
        auto ent = spawn_player(reg);
        InputState moving;
        moving.move_x = 1.f;
        settle(reg, moving);

        InputState idle;
        settle(reg, idle);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(0.f).margin(0.1f));
    }

    SECTION("Dash overrides input-driven movement") {
        auto ent = spawn_player(reg);
        reg.get<Velocity>(ent) = {400.f, 0.f}; // dash burst velocity
        reg.emplace<Dash>(ent);

        InputState input;
        input.move_x = -1.f;
        systems::update_input(reg, input, DT);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(400.f)); // untouched
    }

    SECTION("Charging a shot applies the move penalty") {
        auto ent = spawn_player(reg);
        ChargedShot cs;
        cs.charging = true;
        cs.move_penalty = 0.5f;
        reg.emplace<ChargedShot>(ent, cs);

        InputState input;
        input.move_x = 1.f;
        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(100.f).margin(0.1f));
    }
}

TEST_CASE("Menu direction edges only fire when an axis crosses half-way", "[input]") {
    // A fresh push past half-way is a press
    CHECK(axis_pressed(0.f, 1.f, 1.f));
    CHECK(axis_pressed(0.f, -1.f, -1.f));
    CHECK(axis_pressed(0.4f, 0.6f, 1.f));

    // Holding a direction is not a press: a menu opened while the player
    // is already moving must not move its cursor
    CHECK_FALSE(axis_pressed(1.f, 1.f, 1.f));
    CHECK_FALSE(axis_pressed(-1.f, -1.f, -1.f));

    // Wrong direction, or not far enough
    CHECK_FALSE(axis_pressed(0.f, 1.f, -1.f));
    CHECK_FALSE(axis_pressed(0.f, 0.5f, 1.f));
}

// ── Device input (Input class) ─────────────────────────────────────

namespace {

/// @brief A virtual gamepad that Input::init() picks up. Only this pad
/// counts as a gamepad while it exists, so a real controller plugged into
/// the test machine can't interfere.
struct VirtualPad {
    static constexpr Uint16 VENDOR = 0x1234;
    static constexpr Uint16 PRODUCT = 0x5678;

    SDL_JoystickID id = 0;
    SDL_Joystick* joystick = nullptr;

    VirtualPad() {
        SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT, "0x1234/0x5678");
        REQUIRE(SDL_Init(SDL_INIT_GAMEPAD));

        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.vendor_id = VENDOR;
        desc.product_id = PRODUCT;
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        desc.name = "Raven test pad";
        id = SDL_AttachVirtualJoystick(&desc);
        REQUIRE(id != 0);
        joystick = SDL_OpenJoystick(id);
        REQUIRE(joystick != nullptr);

        // A virtual trigger's axis rests at the bottom of its range
        trigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0.f);
        trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.f);
        SDL_UpdateJoysticks();
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    }

    ~VirtualPad() {
        SDL_CloseJoystick(joystick);
        SDL_DetachVirtualJoystick(id);
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }

    VirtualPad(const VirtualPad&) = delete;
    VirtualPad& operator=(const VirtualPad&) = delete;
    VirtualPad(VirtualPad&&) = delete;
    VirtualPad& operator=(VirtualPad&&) = delete;

    void button(SDL_GamepadButton b, bool down) const {
        SDL_SetJoystickVirtualButton(joystick, b, down);
        SDL_UpdateJoysticks();
    }

    void stick(SDL_GamepadAxis axis, float value) const {
        SDL_SetJoystickVirtualAxis(joystick, axis, static_cast<Sint16>(value * 32767.f));
        SDL_UpdateJoysticks();
    }

    /// @param amount 0 at rest, 1 fully pulled.
    void trigger(SDL_GamepadAxis axis, float amount) const {
        SDL_SetJoystickVirtualAxis(joystick, axis,
                                   static_cast<Sint16>(-32768.f + amount * 65535.f));
        SDL_UpdateJoysticks();
    }
};

/// @brief One frame of the game loop's input handling, then the tick's view.
const InputState& frame(Input& input) {
    input.begin_frame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        input.process_event(event);
    }
    input.update();
    return input.state();
}

/// @brief Run a frame and let a tick consume its presses.
void tick(Input& input) {
    frame(input);
    input.consume_pressed();
}

} // namespace

TEST_CASE("Default gamepad layout", "[input][gamepad]") {
    VirtualPad pad;
    Input input;
    input.init();

    SECTION("right trigger fires") {
        pad.trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 1.f);
        const auto& s = frame(input);
        CHECK(s.shoot);
        CHECK(s.shoot_pressed);
        CHECK_FALSE(s.melee);
        CHECK_FALSE(s.dash);
        CHECK_FALSE(s.bomb);
    }

    SECTION("shoulders: right melee, left dash; left trigger is the ability") {
        pad.button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
        CHECK(frame(input).melee_pressed);
        pad.button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, false);
        tick(input);

        pad.button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
        CHECK(frame(input).dash_pressed);
        pad.button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false);
        tick(input);

        pad.trigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 1.f);
        CHECK(frame(input).bomb_pressed);
    }

    SECTION("face buttons repeat the shoulders") {
        pad.button(SDL_GAMEPAD_BUTTON_WEST, true);
        CHECK(frame(input).melee);
        pad.button(SDL_GAMEPAD_BUTTON_WEST, false);

        pad.button(SDL_GAMEPAD_BUTTON_SOUTH, true);
        const auto& south = frame(input);
        CHECK(south.dash);
        CHECK(south.confirm); // menus still confirm on A
        pad.button(SDL_GAMEPAD_BUTTON_SOUTH, false);

        pad.button(SDL_GAMEPAD_BUTTON_EAST, true);
        const auto& east = frame(input);
        CHECK(east.bomb);
        CHECK(east.cancel);
        CHECK_FALSE(east.shoot);
    }

    SECTION("start pauses, and back is left free for the playtest marker") {
        pad.button(SDL_GAMEPAD_BUTTON_START, true);
        CHECK(frame(input).pause_pressed);
        pad.button(SDL_GAMEPAD_BUTTON_START, false);
        tick(input);

        pad.button(SDL_GAMEPAD_BUTTON_BACK, true);
        const auto& s = frame(input);
        CHECK_FALSE(s.shoot);
        CHECK_FALSE(s.melee);
        CHECK_FALSE(s.dash);
        CHECK_FALSE(s.bomb);
        CHECK_FALSE(s.pause);
        CHECK_FALSE(s.confirm);
        CHECK_FALSE(s.cancel);
    }
}

TEST_CASE("Triggers have hysteresis", "[input][gamepad]") {
    VirtualPad pad;
    Input input;
    input.init();

    pad.trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.6f);
    CHECK(frame(input).shoot);

    // Easing off a little doesn't release it...
    pad.trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.4f);
    CHECK(frame(input).shoot);

    // ...coming most of the way back does
    pad.trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.2f);
    CHECK_FALSE(frame(input).shoot);

    // And from rest, the same light pull isn't a press
    pad.trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.4f);
    CHECK_FALSE(frame(input).shoot);

    CHECK(trigger_pressed(0.6f, false));
    CHECK_FALSE(trigger_pressed(0.4f, false));
    CHECK(trigger_pressed(0.4f, true));
    CHECK_FALSE(trigger_pressed(0.2f, true));
}

TEST_CASE("Sticks use a round deadzone", "[input][gamepad]") {
    SECTION("pure function") {
        const StickValue rest = apply_radial_deadzone(0.1f, 0.1f, 0.2f);
        CHECK(rest.x == 0.f);
        CHECK(rest.y == 0.f);

        // A shallow diagonal keeps its small component
        const StickValue shallow = apply_radial_deadzone(0.9f, 0.15f, 0.2f);
        CHECK(shallow.y > 0.f);

        // Full travel still reaches full speed, and starts from zero at the edge
        const StickValue full = apply_radial_deadzone(1.f, 0.f, 0.2f);
        CHECK(full.x == Catch::Approx(1.f));
        const StickValue edge = apply_radial_deadzone(0.21f, 0.f, 0.2f);
        CHECK(edge.x < 0.05f);
    }

    SECTION("left stick movement") {
        VirtualPad pad;
        Input input;
        input.init();

        pad.stick(SDL_GAMEPAD_AXIS_LEFTX, 0.1f);
        CHECK(frame(input).move_x == 0.f);

        pad.stick(SDL_GAMEPAD_AXIS_LEFTX, 0.9f);
        pad.stick(SDL_GAMEPAD_AXIS_LEFTY, 0.15f);
        const auto& s = frame(input);
        CHECK(s.move_x > 0.8f);
        CHECK(s.move_y > 0.f); // a per-axis deadzone would have zeroed this
    }
}

TEST_CASE("A tap released between polls still counts", "[input][gamepad]") {
    VirtualPad pad;
    Input input;
    input.init();

    SECTION("button") {
        pad.button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
        pad.button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false);
        const auto& s = frame(input);
        CHECK_FALSE(s.dash); // up again by the time of the poll
        CHECK(s.dash_pressed);
    }

    SECTION("trigger") {
        pad.trigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 1.f);
        pad.trigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0.f);
        const auto& s = frame(input);
        CHECK_FALSE(s.bomb);
        CHECK(s.bomb_pressed);
    }

    SECTION("only once") {
        pad.button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
        tick(input);
        CHECK_FALSE(frame(input).dash_pressed); // still held: no new press
    }
}

TEST_CASE("Keyboard and mouse events latch presses", "[input]") {
    Input input; // no devices: only events reach it

    auto key = [&](SDL_Scancode code, bool repeat) {
        SDL_Event event{};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.scancode = code;
        event.key.repeat = repeat;
        input.process_event(event);
    };

    SECTION("a key press") {
        input.begin_frame();
        key(SDL_SCANCODE_C, false);
        input.update();
        CHECK(input.state().melee_pressed);

        input.consume_pressed();
        input.begin_frame();
        key(SDL_SCANCODE_C, true); // key repeat is not a new press
        input.update();
        CHECK_FALSE(input.state().melee_pressed);
    }

    SECTION("one key can drive two actions") {
        input.begin_frame();
        key(SDL_SCANCODE_Z, false);
        input.update();
        CHECK(input.state().shoot_pressed);
        CHECK(input.state().confirm_pressed);
    }

    SECTION("only real mouse use makes the mouse the aiming device") {
        input.begin_frame();
        input.update();
        CHECK_FALSE(input.state().mouse_active);

        SDL_Event motion{};
        motion.type = SDL_EVENT_MOUSE_MOTION;
        input.begin_frame();
        input.process_event(motion);
        input.update();
        CHECK(input.state().mouse_active);
    }

    SECTION("a mouse click fires") {
        SDL_Event click{};
        click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        click.button.button = SDL_BUTTON_LEFT;
        input.begin_frame();
        input.process_event(click);
        input.update();
        CHECK(input.state().shoot_pressed);
        CHECK(input.state().mouse_active);
    }
}

TEST_CASE("Combat actions are reachable without leaving the aim stick", "[input]") {
    // ADR-0027: the first control for each combat action is a shoulder or
    // trigger, so the right thumb can stay on the aim stick
    const GamepadLayout layout;
    auto on_shoulder = [](const PadControl& c) {
        return c.kind == PadControl::Kind::Trigger ||
               (c.kind == PadControl::Kind::Button &&
                (c.code == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                 c.code == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
    };
    CHECK(on_shoulder(layout.shoot[0]));
    CHECK(on_shoulder(layout.melee[0]));
    CHECK(on_shoulder(layout.dash[0]));
    CHECK(on_shoulder(layout.bomb[0]));
}
