/****************************************************************
* render.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#include "render.hpp"

#include "globals.hpp"
#include "macros.hpp"
#include "movement.hpp"
#include "ownership.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"
#include "world.hpp"

#include <SDL.h>

namespace rn {

namespace {


  
} // namespace

void render_panel() {
  // bottom edge
  for( X i( 0 ); i < 22; ++i )
    render_sprite_grid( g_tile::panel_edge_left, Y( 14 ), i, 1, 0 );
  // left edge
  for( Y i( 0 ); i < 14; ++i )
    render_sprite_grid( g_tile::panel_edge_left, i, X( 22 ), 0, 0 );
  // bottom left corner of main panel
  render_sprite_grid( g_tile::panel, Y( 14 ), X( 22 ), 0, 0 );

  for( Y i( 0 ); i < 15; ++i )
    for( X j( 23 ); j < 28; ++j )
      render_sprite_grid( g_tile::panel, i, j, 0, 0 );
}

void render_world_viewport( OptUnitId blink_id ) {
  ::SDL_SetRenderTarget( g_renderer, g_texture_world );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  ::SDL_RenderClear( g_renderer );

  auto covered = viewport().covered_tiles();

  Coord coords;
  if( blink_id )
    coords = coords_for_unit( *blink_id );

  for( Y i = covered.y; i < covered.y+covered.h; ++i ) {
    for( X j = covered.x; j < covered.x+covered.w; ++j ) {
      auto s_ = square_at_safe( i, j);
      ASSERT( s_ );
      Square const& s = *s_;
      g_tile t = s.land ? g_tile::land : g_tile::water;
      auto sy = Y(0)+(i-covered.y);
      auto sx = X(0)+(j-covered.x);
      render_sprite_grid( t, sy, sx, 0, 0 );
      for( auto id : units_from_coord( i, j ) ) {
        if( blink_id && Coord{i,j} == coords && id != *blink_id )
          // Don't render other units in the same square as
          // the one blinking.  Will have to revisit this once
          // units board ships.
          continue;
        if( blink_id && id == *blink_id ) {
          // Animate blinking
          auto ticks = ::SDL_GetTicks();
          if( ticks % 1000 < 500 )
            continue;
        }
        auto const& unit = unit_from_id( id );
        render_sprite_grid(
            unit.desc().tile, sy, sx, 0, 0 );
      }
    }
  }

  ::SDL_SetRenderTarget( g_renderer, NULL );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  ::SDL_RenderClear( g_renderer );

  ::SDL_Rect src = to_SDL( viewport().get_render_src_rect() );
  ::SDL_Rect dest = to_SDL( viewport().get_render_dest_rect() );
  ::SDL_RenderCopy( g_renderer, g_texture_world, &src, &dest );

  //render_tile_map( "panel" );
  render_panel();

  ::SDL_RenderPresent( g_renderer );
}

} // namespace rn
