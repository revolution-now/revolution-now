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

// Revolution Now
#include "unit.hpp"

// Abseil
#include "absl/container/flat_hash_set.h"

// C+ standard library
#include <chrono>
#include <functional>
#include <optional>

namespace rn {

/****************************************************************
** Rendering Stack
*****************************************************************/
// A rendering function must satisfy a few requirements:
//
//   1) Should not clear the texture since it may be drawing
//      on top of something.
//   2) Should not call SDL_RenderPresent since there may
//      be more things that need drawing.
//   3) They should not mutate themselves upon execution.
//
using RenderFunc = std::function<void()>;

// RAII helper for pushing rendering functions onto the stack.
// It will automatically pop them on scope exit.
// TODO: we need to be able to push a `wall` item into the
// rendering stack that signifies that no renderers previous
// to it should be called.
struct RenderStacker {
  RenderStacker( RenderFunc const& func );
  ~RenderStacker();
};

// Run through all the renderers and run them in order.
void render_all();

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
// Expects the rendering target to already be set. This will
// fully render a lang square with no units or colonies on it.
void render_landscape( Coord world_square,
                       Coord texture_square );

// Expects the rendering target to already be set.
void render_unit( UnitId id, Coord texture_square );

// Copies the viewport texture onto the main texture. All view-
// port rendering should be first done to the viewport texture,
// then this function called at the end so that the right posi-
// tion/scaling can be applied to account for e.g. zoom.
void render_copy_viewport_texture();

/****************************************************************
** Viewport Rendering: these render to the viewport texture.
*****************************************************************/
// Options for rendering the viewport. All fields must be ini-
// tialized.
struct ViewportRenderOptions {
  ViewportRenderOptions() = default;
  absl::flat_hash_set<Coord>  squares_with_no_units{};
  absl::flat_hash_set<UnitId> units_to_skip{};
  Opt<UnitId>                 unit_to_blink{std::nullopt};

  void assert_invariants() const;
};

// This function renders the complete static viewport view to the
// viewport texture. This includes land, units, colonies, etc,
// barring anything omitted from the options.
void render_world_viewport(
    ViewportRenderOptions const& options = {} );

// This function renders the unit-sliding animation (only) to the
// viewport texture.
void render_mv_unit( UnitId mv_id, Coord const& target,
                     double percent );

/****************************************************************
** Miscellaneous Rendering
*****************************************************************/
void render_panel();

ND RenderFunc render_fade_to_dark(
    std::chrono::milliseconds wait,
    std::chrono::milliseconds fade, uint8_t target_alpha );

} // namespace rn
