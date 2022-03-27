/****************************************************************
**map-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#include "map-square.hpp"

// Revolution Now
#include "config-files.hpp"
#include "config-terrain.hpp"
#include "terrain.hpp"

using namespace std;

namespace rn {

bool is_land( MapSquare const& square ) {
  return is_land( square.terrain );
}

bool is_water( MapSquare const& square ) {
  return is_water( square.terrain );
}

e_surface surface_type( MapSquare const& square ) {
  return surface_type( square.terrain );
}

MovementPoints movement_points_required(
    MapSquare const& src_square, MapSquare const& dst_square ) {
  if( is_water( src_square ) || is_water( dst_square ) )
    return MovementPoints( 1 );
  // Both squares are land.

  bool src_road_or_river = src_square.road || src_square.river;
  bool dst_road_or_river = dst_square.road || dst_square.river;
  bool road_or_river = src_road_or_river && dst_road_or_river;
  if( road_or_river ) return MovementPoints::_1_3();

  // We're moving from land to land without a road/river on both
  // squares, so it comes down to the terrain. In the game, the
  // only terrain that is relevant in this situation is that of
  // the terrain in the target square.
  return MovementPoints(
      config_terrain.terrain.types[dst_square.terrain]
          .mv_points_required );
}

} // namespace rn
