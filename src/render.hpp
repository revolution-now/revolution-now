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
#include "aliases.hpp"
#include "macros.hpp"
#include "matrix.hpp"
#include "orders.hpp"
#include "physics.hpp"
#include "unit.hpp"

// Abseil
#include "absl/container/flat_hash_set.h"

// C+ standard library
#include <chrono>
#include <functional>
#include <optional>

namespace rn {

struct Plane;

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
void render_unit( Texture& tx, UnitId id, Coord pixel_coord,
                  bool with_icon );
void render_unit( Texture& tx, e_unit_type unit_type,
                  Coord pixel_coord );

void render_nationality_icon( Texture& dest, e_unit_type type,
                              e_nation      nation,
                              e_unit_orders orders,
                              Coord         pixel_coord );

/****************************************************************
** Viewport Rendering
*****************************************************************/

// The viewport rendering states are not really states of the
// world, they are mainly just animation or rendering states.
// Each state is represented by a struct which may contain data
// members.  The data members of the struct's will be mutated in
// in order to change/advance the state of animation, although
// the rendering functions themselves will never mutate them.
namespace viewport_state {

struct none {};

struct blink_unit {
  /**************************************************************
  ** Input
  ***************************************************************/
  UnitId id;
  /**************************************************************
  ** Output
  ***************************************************************/
  // Orders given to `id` by player.
  Opt<orders_t> orders{};
  // Units that the player has asked to prioritize (i.e., bring
  // them forward in the queue of units waiting for orders).
  Vec<UnitId> prioritize{};
  // Units that the player has asked to add to the orders queue
  // but at the end. This is useful if a unit that is sentry'd
  // has already been removed from the queue (without asking for
  // orders) and later in the same turn had its orders cleared by
  // the player (but not prioritized), this will allow it to ask
  // for orders this turn.
  Vec<UnitId> add_to_back{};
};

struct slide_unit {
  slide_unit( UnitId id_, Coord target_ )
    : id( id_ ), target( target_ ) {}

  static constexpr auto min_velocity          = 0;
  static constexpr auto max_velocity          = .07;
  static constexpr auto initial_velocity      = .1;
  static constexpr auto mag_acceleration      = 1;
  static constexpr auto mag_drag_acceleration = .002;

  UnitId id;
  Coord  target;
  double percent{ 0.0 };
  // Note that mag_acceleration is not relevant here.
  DissipativeVelocity percent_vel{
      /*min_velocity=*/min_velocity,
      /*max_velocity=*/max_velocity,
      /*initial_velocity=*/initial_velocity,
      /*mag_acceleration=*/mag_acceleration,
      /*mag_drag_acceleration=*/mag_drag_acceleration };
};

struct depixelate_unit {
  depixelate_unit( UnitId id_, Opt<e_unit_type> demote_to_ );
  UnitId             id{};
  std::vector<Coord> all_pixels{};
  bool               finished{ false };
  Texture            tx_from;
  // If the unit is being depixelated to a different tile then
  // this will contain the unit that should be pixelated in.
  Opt<e_unit_type>   demote_to{};
  Opt<Matrix<Color>> demote_pixels{};
};

} // namespace viewport_state

// FIXME: use ADT here
using ViewportState = std::variant<
    viewport_state::none,           // for end-of-turn
    viewport_state::blink_unit,     // waiting for orders
    viewport_state::slide_unit,     // unit moving on the map
    viewport_state::depixelate_unit // unit moving on the map
    >;
NOTHROW_MOVE( ViewportState );

ViewportState& viewport_rendering_state();

Plane* viewport_plane();

/****************************************************************
** Panel Rendering
*****************************************************************/
// TODO: move panel into is own module
Plane* panel_plane();

// FIXME: temporary.
void mark_end_of_turn();
bool was_next_turn_button_clicked();

/****************************************************************
** Miscellaneous Rendering
*****************************************************************/
Plane* effects_plane();

void reset_fade_to_dark( std::chrono::milliseconds wait,
                         std::chrono::milliseconds fade,
                         uint8_t target_alpha );

} // namespace rn
