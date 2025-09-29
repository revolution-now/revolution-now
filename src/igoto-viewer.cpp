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
    case atlantic:
      if( !enterable_sea_lane( tile ) ) break;
      for( e_direction const d : { e, ne, se } )
        if( enterable_sea_lane( tile.moved( d ) ) ) //
          return d;
      break;
    case pacific:
      for( e_direction const d : { w, nw, sw } )
        if( enterable_sea_lane( tile.moved( d ) ) ) //
          return d;
      break;
  }
  return nothing;
}

} // namespace rn
