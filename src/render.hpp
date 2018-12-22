/****************************************************************
**render.hpp
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

#include <chrono>
#include <functional>
#include <optional>

namespace rn {

// A rendering function must satisfy a few requirements:
//
//   1) Should not clear the texture since it may be drawing
//      on top of something.
//   2) Should not call SDL_RenderPresent since there may
//      be more things that need drawing.
//   3) They should not mutate themselves upon execution.
//
using RenderFunc = std::function<void()>;

void render_world_viewport( OptUnitId blink = std::nullopt );

void render_world_viewport_mv_unit( UnitId       mv_id,
                                    Coord const& target,
                                    double       percent );

ND RenderFunc render_fade_to_dark(
    std::chrono::milliseconds wait,
    std::chrono::milliseconds fade, uint8_t target_alpha );

// Run through all the renderers and run them in order.
void render_all();

// RAII helper for pushing rendering functions onto the stack. It
// will automatically pop them on scope exit.
struct RenderStacker {
  RenderStacker( RenderFunc const& func );
  ~RenderStacker();
};

} // namespace rn
