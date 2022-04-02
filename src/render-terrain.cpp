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

e_tile tile_for_ground_terrain( e_ground_terrain terrain ) {
  switch( terrain ) {
    case e_ground_terrain::arctic: return e_tile::terrain_arctic;
    case e_ground_terrain::desert: return e_tile::terrain_desert;
    case e_ground_terrain::grassland:
      return e_tile::terrain_grassland;
    case e_ground_terrain::marsh: return e_tile::terrain_marsh;
    case e_ground_terrain::plains: return e_tile::terrain_plains;
    case e_ground_terrain::prairie:
      return e_tile::terrain_prairie;
    case e_ground_terrain::savannah:
      return e_tile::terrain_savannah;
    case e_ground_terrain::swamp: return e_tile::terrain_swamp;
    case e_ground_terrain::tundra: return e_tile::terrain_tundra;
  }
}

e_tile overlay_tile( MapSquare const& square ) {
  DCHECK( square.overlay.has_value() );
  switch( *square.overlay ) {
    case e_land_overlay::forest: {
      if( square.ground == e_ground_terrain::desert )
        return e_tile::terrain_forest_scrub_island;
      return e_tile::terrain_forest_island;
    }
    case e_land_overlay::hills:
      return e_tile::terrain_hills_island;
    case e_land_overlay::mountains:
      return e_tile::terrain_mountain_island;
  }
}

void render_terrain_ocean_square( rr::Painter&     painter,
                                  Rect             where,
                                  MapSquare const& square ) {
  DCHECK( square.surface == e_surface::water );
  e_tile tile = square.sea_lane ? e_tile::terrain_ocean_sea_lane
                                : e_tile::terrain_ocean;
  render_sprite( painter, where, tile );
}

// Pass in the painter as well for efficiency.
void render_terrain_land_square( rr::Painter&     painter,
                                 Rect             where,
                                 MapSquare const& square ) {
  DCHECK( square.surface == e_surface::land );
  e_tile tile = tile_for_ground_terrain( square.ground );
  render_sprite( painter, where, tile );
  if( square.overlay.has_value() ) {
    e_tile overlay = overlay_tile( square );
    render_sprite( painter, where, overlay );
  }
  if( g_show_grid )
    painter.draw_empty_rect( where,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel{ 0, 0, 0, 30 } );
}

} // namespace

// Pass in the painter as well for efficiency.
void render_terrain_square( TerrainState const& terrain_state,
                            rr::Painter& painter, Rect where,
                            Coord world_square ) {
  MapSquare const& square =
      square_at( terrain_state, world_square );
  if( square.surface == e_surface::water )
    render_terrain_ocean_square( painter, where, square );
  else
    render_terrain_land_square( painter, where, square );
  if( g_show_grid )
    painter.draw_empty_rect( where,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel{ 0, 0, 0, 30 } );
}

void render_terrain_square( TerrainState const& terrain_state,
                            rr::Painter& painter, Coord where,
                            Coord world_square ) {
  render_terrain_square( terrain_state, painter,
                         Rect::from( where, g_tile_delta ),
                         world_square );
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
