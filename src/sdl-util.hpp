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
#include "sdl.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/pixel.hpp"

// C++ standard library
#include <string_view>

namespace rn {

// Will throw an error is the game is being run with a different
// major version of the given SDL library (e.g., mixer, ttf,
// etc.) than it was built with. A warning will be issued if the
// minor version is different.
void check_SDL_compile_link_version(
    std::string_view module_name,
    ::SDL_version const& link_version,
    ::SDL_version const& compiled_version );

} // namespace rn
