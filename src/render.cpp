/****************************************************************
**render.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#include "render.hpp"

#include "errors.hpp"
#include "globals.hpp"
#include "movement.hpp"
#include "ownership.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"
#include "world.hpp"

#include <SDL.h>

namespace rn {

namespace {} // namespace

void render_panel() {
  constexpr int panel_width{6};
  auto          bottom_bar = 0_y + screen_height_tiles() - 1;
  auto left_side = 0_x + screen_width_tiles() - panel_width;
  // bottom edge
  for( X i( 0 ); i - 0_x < screen_width_tiles() - panel_width;
       ++i )
    render_sprite_grid( g_tile::panel_edge_left, bottom_bar, i,
                        1, 0 );
  // left edge
  for( Y i( 0 ); i - 0_y < screen_height_tiles() - 1; ++i )
    render_sprite_grid( g_tile::panel_edge_left, i, left_side, 0,
                        0 );
  // bottom left corner of main panel
  render_sprite_grid( g_tile::panel, bottom_bar, left_side, 0,
                      0 );

  for( Y i( 0 ); i - 0_y < screen_height_tiles(); ++i )
    for( X j( left_side + 1 ); j - 0_x < screen_width_tiles();
         ++j )
      render_sprite_grid( g_tile::panel, i, j, 0, 0 );
}

void render_world_viewport( OptUnitId blink_id ) {
  ::SDL_SetRenderTarget( g_renderer, g_texture_world );
  constexpr uint8_t opaque{255};
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, opaque );
  ::SDL_RenderClear( g_renderer );

  auto covered = viewport().covered_tiles();

  Coord coords;
  if( blink_id ) coords = coords_for_unit( *blink_id );

  for( Y i = covered.y; i < covered.y + covered.h; ++i ) {
    for( X j = covered.x; j < covered.x + covered.w; ++j ) {
      auto s_ = square_at_safe( i, j );
      CHECK( s_ );
      Square const& s  = *s_;
      g_tile        t  = s.land ? g_tile::land : g_tile::water;
      auto          sy = 0_y + ( i - covered.y );
      auto          sx = 0_x + ( j - covered.x );
      render_sprite_grid( t, sy, sx, 0, 0 );
      // Don't render any units on the square of the blinkingone,
      // including the blinking one itself.
      if( !blink_id || Coord{i, j} != coords ) {
        for( auto id : units_from_coord( i, j ) ) {
          auto const& unit = unit_from_id( id );
          render_sprite_grid( unit.desc().tile, sy, sx, 0, 0 );
        }
      }
      // Render blinker last.
      if( blink_id && Coord{i, j} == coords ) {
        // Animate blinking
        auto ticks = ::SDL_GetTicks();
        // TODO: use chrono here.
        constexpr unsigned int one_second{1000};
        constexpr unsigned int half_second{500};
        if( ticks % one_second > half_second ) {
          auto const& unit = unit_from_id( *blink_id );
          render_sprite_grid( unit.desc().tile, sy, sx, 0, 0 );
        }
      }
    }
  }

  ::SDL_SetRenderTarget( g_renderer, nullptr );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, opaque );
  ::SDL_RenderClear( g_renderer );

  ::SDL_Rect src  = to_SDL( viewport().get_render_src_rect() );
  ::SDL_Rect dest = to_SDL( viewport().get_render_dest_rect() );
  ::SDL_RenderCopy( g_renderer, g_texture_world, &src, &dest );

  // render_tile_map( "panel" );
  render_panel();

  ::SDL_RenderPresent( g_renderer );
}

void render_world_viewport_mv_unit( UnitId       mv_id,
                                    Coord const& target,
                                    double       percent ) {
  constexpr uint8_t opaque{255};
  ::SDL_SetRenderTarget( g_renderer, g_texture_world );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, opaque );
  ::SDL_RenderClear( g_renderer );

  auto covered = viewport().covered_tiles();

  for( Y i = covered.y; i < covered.y + covered.h; ++i ) {
    for( X j = covered.x; j < covered.x + covered.w; ++j ) {
      auto s_ = square_at_safe( i, j );
      CHECK( s_ );
      Square const& s  = *s_;
      g_tile        t  = s.land ? g_tile::land : g_tile::water;
      auto          sy = 0_y + ( i - covered.y );
      auto          sx = 0_x + ( j - covered.x );
      render_sprite_grid( t, sy, sx, 0, 0 );
      for( auto id : units_from_coord( i, j ) ) {
        if( id == mv_id ) continue;
        auto const& unit = unit_from_id( id );
        render_sprite_grid( unit.desc().tile, sy, sx, 0, 0 );
      }
    }
  }

  Coord coords  = coords_for_unit( mv_id );
  W     delta_x = target.x - coords.x;
  H     delta_y = target.y - coords.y;
  CHECK( -1 <= delta_x && delta_x <= 1 );
  CHECK( -1 <= delta_y && delta_y <= 1 );
  W pixel_delta_x =
      W( int( ( delta_x * g_tile_width )._ * percent ) );
  H pixel_delta_y =
      H( int( ( delta_y * g_tile_height )._ * percent ) );

  auto        sy      = 0_y + ( coords.y - covered.y );
  auto        sx      = 0_x + ( coords.x - covered.x );
  auto const& unit    = unit_from_id( mv_id );
  X           pixel_x = sx * g_tile_width._ + pixel_delta_x;
  Y           pixel_y = sy * g_tile_height._ + pixel_delta_y;
  render_sprite( unit.desc().tile, pixel_y, pixel_x, 0, 0 );

  ::SDL_SetRenderTarget( g_renderer, nullptr );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, opaque );
  ::SDL_RenderClear( g_renderer );

  ::SDL_Rect src  = to_SDL( viewport().get_render_src_rect() );
  ::SDL_Rect dest = to_SDL( viewport().get_render_dest_rect() );
  ::SDL_RenderCopy( g_renderer, g_texture_world, &src, &dest );

  // render_tile_map( "panel" );
  render_panel();

  ::SDL_RenderPresent( g_renderer );
}
} // namespace rn
