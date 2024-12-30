/****************************************************************
**init.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: SDL initialization things.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <string_view>

struct SDL_version;

namespace sdl {

// Will throw an error is the game is being run with a different
// major version of the given SDL library (e.g., mixer, ttf,
// etc.) than it was built with. A warning will be issued if the
// minor version is different.
void check_SDL_compile_link_version(
    std::string_view module_name,
    ::SDL_version const& link_version,
    ::SDL_version const& compiled_version );

void init_sdl_base();

void deinit_sdl_base();

} // namespace sdl
