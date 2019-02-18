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
void render_unit( Texture const& tx, UnitId id,
                  Coord texture_square );

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
  UnitId      id;
  Opt<Orders> orders{};
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
  double percent{0.0};
  // Note that mag_acceleration is not relevant here.
  DissipativeVelocity percent_vel{
      /*min_velocity=*/min_velocity,
      /*max_velocity=*/max_velocity,
      /*initial_velocity=*/initial_velocity,
      /*mag_acceleration=*/mag_acceleration,
      /*mag_drag_acceleration=*/mag_drag_acceleration};
};

struct depixelate_unit {
  depixelate_unit( UnitId id_, Opt<e_unit_type> demote_to_ );
  UnitId             id{};
  std::vector<Coord> all_pixels{};
  bool               finished{false};
  Texture            tx_from;
  // If the unit is being depixelated to a different tile then
  // this will contain the unit that should be pixelated in.
  Opt<e_unit_type>   demote_to{};
  Opt<Matrix<Color>> demote_pixels{};
};

} // namespace viewport_state

using ViewportState = std::variant<
    viewport_state::none,           // for end-of-turn
    viewport_state::blink_unit,     // waiting for orders
    viewport_state::slide_unit,     // unit moving on the map
    viewport_state::depixelate_unit // unit moving on the map
    >;

ViewportState& viewport_rendering_state();

Plane* viewport_plane();

/****************************************************************
** Panel Rendering
*****************************************************************/
Plane* panel_plane();

/****************************************************************
** Miscellaneous Rendering
*****************************************************************/
Plane* effects_plane();
void   effects_plane_enable( bool enable );

void reset_fade_to_dark( std::chrono::milliseconds wait,
                         std::chrono::milliseconds fade,
                         uint8_t target_alpha );

} // namespace rn
