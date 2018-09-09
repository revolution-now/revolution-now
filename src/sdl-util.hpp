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

#include "core-config.hpp"

#include "base-util.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include <ostream>

namespace rn {

void init_game();

void init_sdl();

void create_window();

void print_video_stats();

void create_renderer();

void cleanup();

ND SDL_Texture* load_texture( const char* file );

void render_texture( SDL_Texture* texture, SDL_Rect source, SDL_Rect dest,
                     double angle, SDL_RendererFlip flip );

ND bool is_window_fullscreen();
void set_fullscreen( bool fullscreen );
void toggle_fullscreen();

ND ::SDL_Rect to_SDL( Rect const& rect );

} // namespace rn

std::ostream& operator<<( std::ostream& out, ::SDL_Rect const& r );
