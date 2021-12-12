/****************************************************************
**sdl-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Utilities for working with SDL.
*
*****************************************************************/
#include "sdl-util.hpp"

// Revolution Now
#include "error.hpp"
//#include "fmt-helper.hpp"
//#include "frame.hpp"
#include "init.hpp"
#include "logger.hpp"
//#include "macros.hpp"
//#include "screen.hpp"
//#include "util.hpp"

// base-util
//#include "base-util/algo.hpp"
//#include "base-util/string.hpp"

// C++ standard library
//#include <cmath>
//#include <iomanip>
//#include <unordered_map>
//#include <vector>

using namespace std;

namespace rn {

namespace {} // namespace

void check_SDL_compile_link_version(
    string_view module_name, ::SDL_version const& link_version,
    ::SDL_version const& compiled_version ) {
  lg.info( "SDL {}: compiled with version: {}.{}.{}",
           module_name, compiled_version.major,
           compiled_version.minor, compiled_version.patch );
  lg.info( "SDL {}:  running with version: {}.{}.{}",
           module_name, link_version.major, link_version.minor,
           link_version.patch );
  CHECK( compiled_version.major == link_version.major,
         "This game was compiled with a version of SDL {} whose "
         "major version number ({}) is different from the major "
         "version number of the runtime library ({})",
         module_name, compiled_version.major,
         link_version.major );

  if( compiled_version.minor != link_version.minor ) {
    lg.warn(
        "This game was compiled with a version of SDL {} whose "
        "minor version number ({}) is different from the minor "
        "version number of the runtime library ({})",
        module_name, compiled_version.minor,
        link_version.minor );
  }
}

::SDL_Rect to_SDL( Rect const& rect ) {
  ::SDL_Rect res;
  res.x = rect.x._;
  res.y = rect.y._;
  res.w = rect.w._;
  res.h = rect.h._;
  return res;
}

::SDL_Point to_SDL( Coord const& coord ) {
  ::SDL_Point p;
  p.x = coord.x._;
  p.y = coord.y._;
  return p;
}

Coord from_SDL( ::SDL_Point const& p ) {
  Coord res;
  res.x = p.x;
  res.y = p.y;
  return res;
}

Rect from_SDL( ::SDL_Rect const& rect ) {
  Rect res;
  res.x = rect.x;
  res.y = rect.y;
  res.w = rect.w;
  res.h = rect.h;
  return res;
}

void init_sdl() {
  ::SDL_version compiled, linked;
  SDL_VERSION( &compiled );
  ::SDL_GetVersion( &linked );
  check_SDL_compile_link_version( "base", linked, compiled );

  CHECK( compiled.major == 2,
         "This program must be run with SDL version 2." );
  // See:
  // https://discourse.libsdl.org/t/a-couple-of-questions-regarding-batching-in-sdl-2-0-10/26453/3
  if( linked.minor == 0 && linked.patch < 10 )
    lg.warn(
        "SDL versions prior to 2.0.10 may not have GPU batching "
        "support; game may run slow." );

  CHECK( ::SDL_Init( SDL_INIT_EVERYTHING ) >= 0,
         "sdl could not initialize" );
  ::SDL_ShowCursor( SDL_DISABLE );
}

void cleanup_sdl() { ::SDL_Quit(); }

//
//
REGISTER_INIT_ROUTINE( sdl );

::SDL_Color color_from_pixel( SDL_PixelFormat* fmt,
                              uint32_t         pixel ) {
  CHECK( fmt->BitsPerPixel == 32, "bits per pixel: {}",
         fmt->BitsPerPixel );
  ::SDL_Color color{};

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

::SDL_Color to_SDL( gfx::pixel color ) {
  return { color.r, color.g, color.b, color.a };
}

gfx::pixel from_SDL( ::SDL_Color color ) {
  return { color.r, color.g, color.b, color.a };
}

} // namespace rn
