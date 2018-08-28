#include "base-util.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"
#include "world.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <iostream>

double view_center_x = 0;
double view_center_y = 0;
double zoom = 1.0;
double movement_speed = 8.0;

double world_viewport_width()      { return rn::g_world_viewport_width_tiles*32/zoom; }
double world_viewport_height()     { return rn::g_world_viewport_height_tiles*32/zoom; }

double world_viewport_start_x() { return view_center_x - world_viewport_width()/2.0; }
double world_viewport_start_y() { return view_center_y - world_viewport_height()/2.0; }
double world_viewport_end_x()   { return view_center_x + world_viewport_width()/2.0; }
double world_viewport_end_y()   { return view_center_y + world_viewport_height()/2.0; }

int world_viewport_start_tile_x() { return int( world_viewport_start_x() )/32; }
int world_viewport_start_tile_y() { return int( world_viewport_start_y() )/32; }

int round_up_to_nearest_int_multiple( double d, int m ) {
  int floor = int( d );
  if( floor % m != 0 )
    floor += m;
  return floor/m;
}

int round_down_to_nearest_int_multiple( double d, int m ) {
  int floor = int( d );
  return floor/m;
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double world_viewport_width_tiles() {
  int lower = round_down_to_nearest_int_multiple(
          world_viewport_start_x(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          world_viewport_end_x(), 32 );
  return upper-lower;
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double world_viewport_height_tiles() {
  int lower = round_down_to_nearest_int_multiple(
          world_viewport_start_y(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          world_viewport_end_y(), 32 );
  return upper-lower;
}

void world_viewport_apply_bounds() {
  if( zoom < .5 ) zoom = .5;
  auto [size_y, size_x] = rn::world_size_tiles();
  size_y *= 32;
  size_x *= 32;
  if( world_viewport_start_x() < 0 )
      view_center_x = world_viewport_width()/2.0;
  if( world_viewport_start_y() < 0 )
      view_center_y = world_viewport_height()/2.0;
  if( world_viewport_end_x() > size_x )
      view_center_x = size_x-world_viewport_width()/2.0;
  if( world_viewport_end_y() > size_y )
      view_center_y = size_y-world_viewport_height()/2.0;
  //std::cout << view_center_x << "\n";
}

// [min, max]
int bounds( int what, int min, int max ) {
  if( what < min ) return min;
  if( what > max ) return max;
  return what;
}

void render() {
  ::SDL_SetRenderTarget( rn::g_renderer, rn::g_texture_world );
  SDL_SetRenderDrawColor( rn::g_renderer, 0, 0, 0, 255 );
  SDL_RenderClear( rn::g_renderer );

  int start_tile_x = world_viewport_start_tile_x();
  if( start_tile_x < 0 ) start_tile_x = 0;
  int start_tile_y = world_viewport_start_tile_y();
  if( start_tile_y < 0 ) start_tile_y = 0;

  auto [size_y, size_x] = rn::world_size_tiles();
  int end_tile_x = world_viewport_start_tile_x() + world_viewport_width_tiles();
  if( end_tile_x > size_x ) end_tile_x = size_x;
  int end_tile_y = world_viewport_start_tile_y() + world_viewport_height_tiles();
  if( end_tile_y > size_y ) end_tile_y = size_y;

  for( int i = start_tile_y; i < end_tile_y; ++i ) {
    for( int j = start_tile_x; j < end_tile_x; ++j ) {
      auto s_ = rn::square_at_safe( i, j);
      if( !s_.has_value() ) continue; //ASSERT( s_ );
      rn::Square const& s = *s_;
      rn::g_tile t = s.land ? rn::g_tile::land : rn::g_tile::water;
      rn::render_sprite_grid( t, i-start_tile_y, j-start_tile_x, 0, 0 );
    }
  }

  ::SDL_Rect viewport;
  viewport.x = int( world_viewport_start_x() );
  viewport.y = int( world_viewport_start_y() );
  viewport.w = int( world_viewport_width() );
  viewport.h = int( world_viewport_height() );
  std::cout << viewport << "\n";

  int max_src_width = std::get<1>( rn::world_size_tiles() )*32;
  int max_src_height = std::get<0>( rn::world_size_tiles() )*32;

  ::SDL_Rect src;
  src.x = viewport.x < 0 ? 0 : viewport.x % 32;
  src.y = viewport.y < 0 ? 0 : viewport.y % 32;
  src.w = viewport.w > max_src_width ? max_src_width : viewport.w;
  src.h = viewport.h > max_src_height ? max_src_height : viewport.h;

  ::SDL_SetRenderTarget( rn::g_renderer, NULL );
  SDL_SetRenderDrawColor( rn::g_renderer, 0, 0, 0, 255 );
  SDL_RenderClear( rn::g_renderer );

  ::SDL_Rect dest;
  dest.x = dest.y = 0;
  dest.w = rn::g_tile_width*rn::g_world_viewport_width_tiles;
  dest.h = rn::g_tile_height*rn::g_world_viewport_height_tiles;
  if( viewport.w > max_src_width ) {
    double delta = ((double( viewport.w ) - max_src_width )/viewport.w)*32*rn::g_world_viewport_width_tiles/2.0;
    dest.x += int( delta );
    dest.w -= int( delta*2 );
  }
  if( viewport.h > max_src_height ) {
    double delta = ((double( viewport.h ) - max_src_height )/viewport.h)*32*rn::g_world_viewport_height_tiles/2.0;
    dest.y += int( delta );
    dest.h -= int( delta*2 );
  }
  ::SDL_RenderCopy( rn::g_renderer, rn::g_texture_world, &src, &dest );

  rn::render_tile_map( "panel" );

  SDL_RenderPresent( rn::g_renderer );
}

int main( int, char** ) {
  rn::init_game();
  rn::load_sprites();
  rn::load_tile_maps();

  world_viewport_apply_bounds();
  render();

  // Set a delay before quitting
  //SDL_Delay( 2000 );

  bool running = true;

  while( running ) {
    if( ::SDL_Event event; SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED:
              render();
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case SDLK_q:
            case SDLK_RETURN:
              running = false;
              break;
            case ::SDLK_SPACE:
              rn::toggle_fullscreen();
              break;
            case ::SDLK_LEFT:
              view_center_x -= movement_speed;
              world_viewport_apply_bounds();
              render();
              break;
            case ::SDLK_RIGHT:
              view_center_x += movement_speed;
              world_viewport_apply_bounds();
              render();
              break;
            case ::SDLK_DOWN:
              view_center_y += movement_speed;
              world_viewport_apply_bounds();
              render();
              break;
            case ::SDLK_UP:
              view_center_y -= movement_speed;
              world_viewport_apply_bounds();
              render();
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            zoom *= 0.98;
          if( event.wheel.y > 0 )
            zoom *= 1.02;
          world_viewport_apply_bounds();
          render();
          break;
        default:
          break;
      }
    }
  }

  rn::cleanup();
  return 0;
}
