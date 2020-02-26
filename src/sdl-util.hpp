/****************************************************************
**sdl-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Utilities for working with SDL.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "coord.hpp"

// SDL
#include "SDL.h"

// C++ standard library
#include <string_view>

namespace rn {

// Will throw an error is the game is being run with a different
// major version of the given SDL library (e.g., mixer, ttf,
// etc.) than it was built with. A warning will be issued if the
// minor version is different.
void check_SDL_compile_link_version(
    std::string_view     module_name,
    ::SDL_version const& link_version,
    ::SDL_version const& compiled_version );

::SDL_Color color_from_pixel( SDL_PixelFormat* fmt,
                              uint32_t         pixel );

::SDL_Point to_SDL( Coord const& coord );
::SDL_Rect  to_SDL( Rect const& rect );
::SDL_Color to_SDL( Color color );

Coord from_SDL( ::SDL_Point const& p );
Color from_SDL( ::SDL_Color color );
Rect  from_SDL( ::SDL_Rect const& rect );

} // namespace rn
