/****************************************************************
**road.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: All things to do with roads.
*
*****************************************************************/
#include "road.hpp"

// Revolution Now
#include "lua.hpp"
#include "render.hpp"
#include "world-map.hpp"

// render
#include "render/renderer.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Road State
*****************************************************************/
void set_road( Coord tile ) {
  CHECK( is_land( tile ) );
  MapSquare& square = mutable_square_at( tile );
  square.road       = true;
}

void clear_road( Coord tile ) {
  MapSquare& square = mutable_square_at( tile );
  square.road       = false;
}

bool has_road( Coord tile ) {
  MapSquare const& square = square_at( tile );
  return square.road;
}

/****************************************************************
** Rendering
*****************************************************************/
void render_road_if_present( rr::Painter& painter, Coord where,
                             Coord world_tile ) {
  if( !has_road( world_tile ) ) return;

  static vector<pair<e_direction, e_tile>> const road_tiles{
      { { e_direction::nw }, e_tile::road_nw },
      { { e_direction::n }, e_tile::road_n },
      { { e_direction::ne }, e_tile::road_ne },
      { { e_direction::e }, e_tile::road_e },
      { { e_direction::se }, e_tile::road_se },
      { { e_direction::s }, e_tile::road_s },
      { { e_direction::sw }, e_tile::road_sw },
      { { e_direction::w }, e_tile::road_w },
  };

  bool road_in_surroundings = false;
  for( auto [direction, tile] : road_tiles ) {
    Coord shifted = world_tile.moved( direction );
    if( !square_exists( shifted ) || !has_road( shifted ) )
      continue;
    road_in_surroundings = true;
    render_sprite( painter, where, tile );
  }
  if( !road_in_surroundings )
    render_sprite( painter, where, e_tile::road_island );
}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_FN( set_road, void, Coord tile ) {
  if( !is_land( tile ) )
    st.error( "cannot put road on water tile {}.", tile );
  set_road( tile );
}

LUA_FN( clear_road, void, Coord tile ) { clear_road( tile ); }

} // namespace

} // namespace rn
