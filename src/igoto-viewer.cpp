/****************************************************************
**igoto-viewer.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Interface for goto path-finding algorithms to
*              query the map they are searching.
*
*****************************************************************/
#include "igoto-viewer.hpp"

using namespace std;

namespace rn {

using ::gfx::point;

/****************************************************************
** Public API.
*****************************************************************/
maybe<e_direction> IGotoMapViewer::is_sea_lane_launch_point(
    point const tile ) const {
  switch( is_on_map_side_edge( tile ) ) {
    using enum e_map_side_edge;
    using enum e_direction;
    case none:
      break;
    case atlantic:
      return e;
    case pacific:
      return w;
  }
  auto const enterable_sea_lane = [&]( point const tile ) {
    // NOTE: The can_enter_tile will test that the tile exists.
    return can_enter_tile( tile ) &&
           is_sea_lane( tile ).value_or( false );
  };
  switch( map_side( tile ) ) {
    using enum e_map_side;
    using enum e_direction;
    case atlantic: {
      if( !enterable_sea_lane( tile ) ) break;
      for( e_direction const d : { e, ne, se } )
        if( enterable_sea_lane( tile.moved( d ) ) ) //
          return d;
      // We just return "east" here if the tile to the east is
      // hidden, because on a standard game map, any atlantic sea
      // lane tile is a launch point, whether it is on the map
      // edge or not, since sea lane tiles always have sea lane
      // to the right of them. This corrects an issue where a
      // ship that is travelling diagonally detects a sea lane
      // tile X to its east as it is traveling but doesn't recog-
      // nize it as a launch point because the sea lane tile to
      // the east of X is hidden, which leads to strange
      // non-optimal behavior in the dynamic path adjustment.
      // Even in the case that the map is modded and there is no
      // sea lane tile to the east of X, it will be ok because
      // when the ship goes there then the tile to the east of X
      // will become visible and it will see that it is not on a
      // launch point and so it will re-route.
      point const east       = tile.moved( e );
      bool const east_hidden = !is_sea_lane( east ).has_value();
      if( can_enter_tile( east ) && east_hidden ) return e;
      break;
    }
    case pacific:
      for( e_direction const d : { w, nw, sw } )
        if( enterable_sea_lane( tile.moved( d ) ) ) //
          return d;
      break;
  }
  return nothing;
}

int IGotoMapViewer::travel_cost( point const src,
                                 e_direction const d ) const {
  MovementPoints res;
  res             = movement_points_required( src, d );
  point const dst = src.moved( d );
  if( has_lcr( dst ).value_or( false ) )
    // Try to steer away from LCRs if possible.
    res += MovementPoints( 3 );
  return res.atoms();
}

int IGotoMapViewer::heuristic_cost( point const src,
                                    point const dst ) const {
  // Even though we generally measure movement cost in atoms
  // (three per movement point) we don't assign 1 movement point
  // to compute distance in the heuristic function, instead we
  // assume the cost of a road (1 atom). This is so that our
  // heuristic function satisfies the property of "admissibility"
  // which means that it will never overestimate the cost of
  // moving between two tiles. In this game, the lowest possible
  // cost in moving between two tiles would be the "chessboard
  // distance" where every tile has a road on it. Even though
  // statistically it might be more accurate to assume a cost of
  // 1 or 2 movement points (3 and 6 atoms, respectively) to move
  // between tiles, that would break the admissibility property
  // which would then break guarantees of the A* algo in finding
  // the optimal path given that the costs of tiles are
  // non-uniform (due to differing terrain types, roads, etc.).
  static constexpr int kCostOfRoadInAtoms = 1;
  return ( dst - src ).chessboard_distance() *
         kCostOfRoadInAtoms;
}

} // namespace rn
