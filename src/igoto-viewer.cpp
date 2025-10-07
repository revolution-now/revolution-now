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
      if( east_hidden && can_enter_tile( east ) ) return e;
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
  // Normally we are not supposed to ever overestimate the cost
  // of travel in our heuristic function in order to maintain the
  // property of Admissibility which the A* algo requires for op-
  // timal behavior. However, technically this method
  // (travel_cost) is not the heuristic function, instead it is
  // the method that is used to get the actual cost when moving
  // along the path by single tile. If it is wrong then it won't
  // make the A* algo any worse per se, it just means that we are
  // working with limited information given that some tiles are
  // hidden, and so that is the best that we can do.
  static MovementPoints const kHiddenTilePoints( 1 );
  res = movement_points_required( src, d ).value_or(
      kHiddenTilePoints );
  point const dst = src.moved( d );
  if( has_lcr( dst ).value_or( false ) ) {
    // Try to steer away from LCRs if possible by making it a
    // heavy tile. Here we make it 4 movement points which is
    // just more than a mountain tile.
    static MovementPoints const kLcrPoints( 4 );
    res += kLcrPoints;
  }
  return res.atoms();
}

int IGotoMapViewer::heuristic_cost( point const src,
                                    point const dst ) const {
  return ( dst - src ).chessboard_distance() *
         minimum_heuristic_tile_cost().atoms();
}

} // namespace rn
