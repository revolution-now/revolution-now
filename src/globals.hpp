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

// Revolution Now
#include "core-config.hpp"
#include "geo-types.hpp"
#include "sdl-util.hpp"
#include "typed-int.hpp"

// SDL
#include "SDL.h"
#include "SDL_image.h"

namespace rn {

extern ::SDL_Window*   g_window;
extern ::SDL_Renderer* g_renderer;
extern Texture         g_texture_viewport;
extern int             g_resolution_scale_factor;
// This origin will be a small distance away from the screen's
// origin and will skip a few pixels that are needed to make the
// integral scalling factor.
extern Delta g_drawing_origin;
// This will begin at the drawing origin above and go down to
// the opposite corner of the drawing area (but not always to the
// edge of the screen, for similiar reasons).
extern Rect g_drawing_region;

W    screen_width_tiles();
H    screen_height_tiles();
void set_screen_width_tiles( W w );
void set_screen_height_tiles( H h );

// At standard zoom, when tile size is (g_tile_width,
// g_tile_height);
W viewport_width_tiles();
H viewport_height_tiles();

} // namespace rn
