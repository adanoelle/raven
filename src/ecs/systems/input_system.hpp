#pragma once

#include "core/input.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

void update_input(entt::registry& reg, const InputState& input);

} // namespace raven::systems
