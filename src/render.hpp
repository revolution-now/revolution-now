/****************************************************************
* render.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "unit.hpp"

#include <optional>

namespace rn {

void render_world_viewport( OptUnitId blink = std::nullopt );

} // namespace rn
