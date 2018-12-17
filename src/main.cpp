#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "geo-types.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "sdl-util.hpp"
#include "sound.hpp"
#include "tiles.hpp"
#include "turn.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "viewport.hpp"
#include "window.hpp"
#include "world.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/io.hpp"
#include "base-util/string.hpp"

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_split.h"
#include "fmt/format.h"
#include "fmt/ostream.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

using namespace rn;
using namespace std;

namespace rn {

Color color_from( SDL_PixelFormat* fmt, Uint32 pixel ) {
  Color color{};

  /* Get Red component */
  auto temp = pixel & fmt->Rmask;  /* Isolate red component */
  temp      = temp >> fmt->Rshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Rloss; /* Expand to a full 8-bit number */
  color.r = (Uint8)temp;

  /* Get Green component */
  temp = pixel & fmt->Gmask;  /* Isolate green component */
  temp = temp >> fmt->Gshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Gloss;  /* Expand to a full 8-bit number */
  color.g = (Uint8)temp;

  /* Get Blue component */
  temp = pixel & fmt->Bmask;  /* Isolate blue component */
  temp = temp >> fmt->Bshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Bloss;  /* Expand to a full 8-bit number */
  color.b = (Uint8)temp;

  /* Get Alpha component */
  temp = pixel & fmt->Amask;  /* Isolate alpha component */
  temp = temp >> fmt->Ashift; /* Shift it down to 8-bit */
  temp = temp << fmt->Aloss;  /* Expand to a full 8-bit number */
  color.a = (Uint8)temp;

  return color;
}

void extract_palette() {
  /* Extracting color components from a 32-bit color value */
  auto files =
      util::wildcard( "assets/art/palette-load/*.png", false );

  absl::flat_hash_set<Color> colors;

  for( auto f : files ) {
    logger->info( "Loading {}", f );
    SDL_Surface* surface = load_surface( f.string().c_str() );

    int              added = 0;
    SDL_PixelFormat* fmt   = surface->format;

    logger->info( "  scanning {} pixels",
                  surface->h * surface->w );

    auto bpp = fmt->BitsPerPixel;
    logger->info( "  bits per pixel: {}", fmt->BitsPerPixel );
    CHECK( bpp == 8 || bpp == 32 || bpp == 24 );

    SDL_LockSurface( surface );

    for( int i = 0; i < surface->h; ++i ) {
      for( int j = 0; j < surface->w; ++j ) {
        Color color;
        if( bpp == 8 ) {
          Uint8 index =
              ( (Uint8*)surface->pixels )[i * surface->w + j];
          color = from_SDL( fmt->palette->colors[index] );

        } else if( bpp == 24 ) {
          Uint8* addr = ( (Uint8*)surface->pixels ) +
                        3 * ( i * surface->w + j );
          Uint32 pixel = *( (Uint32*)addr );
          color        = color_from( fmt, pixel );
        } else if( bpp == 32 ) {
          Uint32 pixel =
              ( (Uint32*)surface->pixels )[i * surface->w + j];
          color = color_from( fmt, pixel );
        }
        color.a = 255;
        if( colors.contains( color ) ) continue;
        colors.insert( color );
        ++added;
        // fmt::print(
        //    "  adding color #{:03d}: #{:02x}{:02x}{:02x}\n",
        //    colors.size(), color.r, color.g, color.b );
      }
    }

    CHECK_( added <= 5024, "file added too many colors" );
    logger->info( "  ==> added {} colors", added );

    SDL_UnlockSurface( surface );
    SDL_FreeSurface( surface );
  }

  clear_texture_black( Texture() );
  int  idx  = 0;
  auto rgb  = vector<Color>( colors.begin(), colors.end() );
  auto hlvs = util::map( to_HlVS, rgb );
  std::sort( hlvs.begin(), hlvs.end(), hlvs_bucketed_cmp );
  auto rgb_sorted_by_hlv = util::map(
      []( ColorHlVS const& hlvs ) { return to_RGB( hlvs ); },
      hlvs );
  for( auto color : rgb_sorted_by_hlv ) {
    X x{( idx % 96 ) * 5};
    Y y{( idx / 96 ) * 5 + 40};
    W w{5};
    H h{5};
    render_fill_rect( nullopt, color, {x, y, w, h} );
    ++idx;
    // if( idx > 512 ) break;
  }
  ::SDL_RenderPresent( g_renderer );

  wait_for_q();
} // namespace rn

void game() {
  init_game();
  load_sprites();
  load_tile_maps();

  // CHECK( play_music_file( "assets/music/bonny-morn.mp3" ) );

  //(void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
  //                          3_x );
  //(void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
  //                          4_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 2_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 1_x );

  // while( turn() != e_turn_result::quit ) {}

  // font_test();
  // auto res =
  //    ui::yes_no( [] {}, "Would you like to make landfall?" );

  // logger->info( "Selected: {}", int( res ) );
  extract_palette();

  cleanup();
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  init_logging( spdlog::level::info );
  load_configs();
  game();
  return 0;

} catch( exception_with_bt const& e ) {
  logger->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    logger->error( "SDL error: {}", sdl_error );
  print_stack_trace( e.st, 4 );
  cleanup();
} catch( exception const& e ) {
  logger->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    logger->error( "SDL error: {}", sdl_error );
  cleanup();
  return 1;
}
