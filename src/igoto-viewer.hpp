/****************************************************************
**igoto-viewer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Interface for goto path-finding algorithms to
*              query the map they are searching.
*
*****************************************************************/
#pragma once

// rds
#include "igoto-viewer.rds.hpp"

// Revolution Now
#include "maybe.hpp"

// gfx
#include "gfx/cartesian.hpp"

namespace rn {

/****************************************************************
** IGotoMapViewer
*****************************************************************/
// The A-Star (or similar) algorithms that compute unit travel
// paths will use this interface to query the map and to deter-
// mine if a given tile is traversible. It is good to put this
// behind an interface because there are various factors that go
// into answering that question.
struct IGotoMapViewer {
  virtual ~IGotoMapViewer() = default;

 public: // required
  [[nodiscard]] virtual bool can_enter_tile(
      gfx::point tile ) const = 0;

  [[nodiscard]] virtual e_map_side map_side(
      gfx::point tile ) const = 0;

  [[nodiscard]] virtual e_map_side_edge is_on_map_side_edge(
      gfx::point tile ) const = 0;

  // Nothing is returned if the tile is hidden and we don't know.
  // Otherwise, the sea lane status is returned.
  [[nodiscard]] virtual maybe<bool> is_sea_lane(
      gfx::point tile ) const = 0;

 public: // for convenience
  // In order to sail to the harbor you have to from a source in
  // a given direction where, depending on the location on the
  // map, the source tile may or may not need to contain sea lane
  // itself and the direction may or may not take you off of the
  // map.
  //
  // Given a source tile, this function will return a direction
  // that the ship would have to move to launch it into the sea
  // lane, if that is possible.
  [[nodiscard]] virtual maybe<e_direction>
  is_sea_lane_launch_point( gfx::point tile ) const final;
};

} // namespace rn
