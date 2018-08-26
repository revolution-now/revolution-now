/****************************************************************
* sdl-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Interface for calling SDL functions
*
*****************************************************************/

#pragma once

#include <SDL.h>
#include <SDL_image.h>

namespace rn {

void init_sdl();

void create_window();

void print_video_stats();

void create_renderer();

void cleanup();

SDL_Texture* load_texture( const char* file );

void render_texture( SDL_Texture* texture, SDL_Rect source, SDL_Rect dest,
                     double angle, SDL_RendererFlip flip );

} // namespace rn
