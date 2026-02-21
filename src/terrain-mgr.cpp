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
int num_surrounding_land_tiles( MapMatrix const& map,
                                point const tile ) {
  int total        = 0;
  rect const world = map.rect();
  for( e_direction const d : enum_values<e_direction> ) {
    point const moved = tile.moved( d );
    if( !moved.is_inside( world ) ) continue;
    MapSquare const& square = map[moved];
    if( is_water( square ) ) continue;
    ++total;
  }
  return total;
}

int num_surrounding_land_tiles( RealTerrain const& terrain,
                                point const tile ) {
  return num_surrounding_land_tiles( terrain.map, tile );
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
  return on_all_tiles( ss.terrain.real_terrain().map, fn );
}

void on_all_tiles( MapMatrix const& m,
                   base::function_ref<void(
                       gfx::point, MapSquare const& square )>
                       fn ) {
  rect const world = m.rect();
  for( point const p : rect_iterator( world ) ) fn( p, m[p] );
}

void on_surrounding(
    MapMatrix const& m, point const tile,
    base::function_ref<
        void( gfx::point, MapSquare const& square )> const fn ) {
  rect const r = m.rect();
  for( e_direction const d : enum_values<e_direction> ) {
    point const p = tile.moved( d );
    if( !p.is_inside( r ) ) continue;
    fn( p, m[p] );
  }
}

void on_surrounding(
    SSConst const& ss, point const tile,
    base::function_ref<
        void( gfx::point, MapSquare const& square )> const fn ) {
  return on_surrounding( ss.terrain.real_terrain().map, tile,
                         fn );
}

} // namespace rn
