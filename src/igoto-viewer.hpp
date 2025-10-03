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

// ss
#include "ss/mv-points.hpp"

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

  [[nodiscard]] virtual maybe<bool> has_lcr(
      gfx::point tile ) const = 0;

  // Assuming that movement is possible between the two tiles
  // this will return the estimated cost in movement points by
  // the unit, accounting for how many total movement points the
  // unit has at the start of each turn. This won't be perfectly
  // accurate because in general the movement points consumed by
  // a unit when it moves onto expensive terrain will have a
  // random element, but this does a best estimate nevertheless.
  virtual maybe<MovementPoints> movement_points_required(
      gfx::point src, e_direction direction ) const = 0;

  // Must return the best case cost to move between two adjacent
  // tiles. This will differ based on whether we are running for
  // a land unit or sea unit. See comments on the heuristic_cost
  // function for more info.
  [[nodiscard]] virtual MovementPoints
  minimum_heuristic_tile_cost() const = 0;

 public: // for convenience
  // This is the weight used by the goto algorithm to assign the
  // cost of moving from src to the direction d, taking into ac-
  // count terrain type if available.
  [[nodiscard]] virtual int travel_cost(
      gfx::point src, e_direction d ) const final;

  // This is the "heuristic function" for the A* algo, meaning
  // that it makes a quick estimate of the cost of moving between
  // two tiles without computing a path between them. As such, it
  // does not make use of terrain type. It is used to prioritize
  // the ordering that we search tiles.
  //
  // Note that it must satisfy the property of Admissibility in
  // order for optimal path discovery to work properly (given
  // that different tiles have different weights), which means
  // that it should never overestimate the true optimal cost of
  // traveling between two tiles, meaning that it must always re-
  // turn the best case scenario. For example, for a land unit,
  // that means assuming that every tile between the two points
  // is traversible and contains a road.
  [[nodiscard]] virtual int heuristic_cost(
      gfx::point src, gfx::point dst ) const final;

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
