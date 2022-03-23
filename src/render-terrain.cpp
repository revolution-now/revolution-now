/****************************************************************
**render-terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Renders individual terrain squares.
*
*****************************************************************/
#include "render-terrain.hpp"

// Revolution Now
#include "error.hpp"
#include "game-state.hpp"
#include "gs-terrain.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "tiles.hpp"
#include "world-map.hpp"

// render
#include "render/renderer.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

bool g_show_grid = false;

} // namespace

// Pass in the painter as well for efficiency.
void render_terrain_square( rr::Painter& painter, Rect where,
                            Coord world_square ) {
  e_tile tile =
      is_land( world_square ) ? e_tile::land : e_tile::water;
  render_sprite( painter, where, tile );
  if( g_show_grid )
    painter.draw_empty_rect( where,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel{ 0, 0, 0, 30 } );
}

void render_terrain_square( rr::Painter& painter, Coord where,
                            Coord world_square ) {
  render_terrain_square(
      painter, Rect::from( where, g_tile_delta ), world_square );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( toggle_grid, void ) {
  g_show_grid = !g_show_grid;
  lg.debug( "terrain grid is {}.", g_show_grid ? "on" : "off" );
}

} // namespace

} // namespace rn
