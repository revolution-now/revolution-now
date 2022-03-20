/****************************************************************
**renderer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-19.
*
* Description: Holds and initializes the global renderer.
*
*****************************************************************/
#include "renderer.hpp"

// Revolution Now
#include "config-files.hpp"
#include "init.hpp"
#include "maybe.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"

// Revolution Now (config)
#include "../config/rcl/tile-sheet.inl"

// SDL
#include "SDL.h"

using namespace std;

namespace rn {

namespace {

unique_ptr<rr::Renderer> g_renderer;

void init_renderer() {
  ::SDL_Window* window =
      static_cast<::SDL_Window*>( main_os_window_handle() );
  Delta logical_screen_size = main_window_logical_size();

  rr::RendererConfig renderer_config = {
      .logical_screen_size = logical_screen_size,
      .max_atlas_size      = { .w = 200, .h = 200 },
      // These are taken by reference.
      .sprite_sheets = config_tile.sheets.refl().sprite_sheets,
      .font_sheets   = config_tile.sheets.refl().font_sheets,
  };

  // This renderer needs to be released before the SDL context is
  // cleaned up.
  g_renderer = rr::Renderer::create(
      renderer_config, [&] { sdl_gl_swap_window( window ); } );
}

void cleanup_renderer() { g_renderer.reset(); }

} // namespace

REGISTER_INIT_ROUTINE( renderer );

/****************************************************************
** Public API
*****************************************************************/
rr::Renderer& global_renderer_use_only_when_needed() {
  CHECK( g_renderer != nullptr );
  return *g_renderer;
}

} // namespace rn
