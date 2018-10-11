/****************************************************************
**globals.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Globals needed by the game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"
#include "sdl-util.hpp"
#include "typed-int.hpp"

#include <SDL.h>
#include <SDL_image.h>

namespace rn {

extern ::SDL_Window*   g_window;
extern ::SDL_Renderer* g_renderer;
extern Texture         g_texture_world;

W    screen_width_tiles();
H    screen_height_tiles();
void set_screen_width_tiles( W w );
void set_screen_height_tiles( H h );

// At standard zoom, when tile size is (g_tile_width,
// g_tile_height);
W viewport_width_tiles();
H viewport_height_tiles();

} // namespace rn
