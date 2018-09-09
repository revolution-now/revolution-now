/****************************************************************
* globals.hpp
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

#include <SDL.h>
#include <SDL_image.h>

namespace rn {

extern ::SDL_Window* g_window;
extern ::SDL_Renderer* g_renderer;
extern ::SDL_Texture* g_texture_world;

} // namespace rn

