/****************************************************************
**terrain-mgr.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-14.
*
* Description: Helper methods for dealing with terrain.
*
*****************************************************************/
#include "terrain-mgr.hpp"

// Revolution Now
#include "map-square.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/query-enum.hpp"

namespace rn {

namespace {

using namespace std;

using ::gfx::point;
using ::gfx::rect;
using ::gfx::rect_iterator;
using ::refl::enum_values;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
int num_surrounding_land_tiles( RealTerrain const& terrain,
                                point const tile ) {
  int total        = 0;
  auto const& map  = terrain.map;
  rect const world = map.rect();
  for( e_direction const d : enum_values<e_direction> ) {
    point const moved = tile.moved( d );
    if( !moved.is_inside( world ) ) continue;
    MapSquare const& square = terrain.map[moved];
    if( is_water( square ) ) continue;
    ++total;
  }
  return total;
}

bool is_island( RealTerrain const& terrain, point const tile ) {
  CHECK( tile.is_inside( terrain.map.rect() ) );
  return terrain.map[tile].surface == e_surface::land &&
         num_surrounding_land_tiles( terrain, tile ) == 0;
}

int num_surrounding_land_tiles( SSConst const& ss,
                                point const tile ) {
  int total = 0;
  for( e_direction const d : enum_values<e_direction> ) {
    point const moved = tile.moved( d );
    auto const square = ss.terrain.maybe_square_at( moved );
    if( !square.has_value() ) continue;
    if( is_water( *square ) ) continue;
    ++total;
  }
  return total;
}

bool is_island( SSConst const& ss, point const tile ) {
  return ss.terrain.square_at( tile ).surface ==
             e_surface::land &&
         num_surrounding_land_tiles( ss, tile ) == 0;
}

void on_all_tiles(
    SSConst const& ss,
    base::function_ref<
        void( gfx::point, MapSquare const& square )> const fn ) {
  auto const& map  = ss.terrain.real_terrain().map;
  rect const world = map.rect();
  for( point const p : rect_iterator( world ) ) fn( p, map[p] );
}

} // namespace rn
