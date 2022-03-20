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

// gl
#include "gl/init.hpp"

// Revolution Now (config)
#include "../config/rcl/tile-sheet.inl"

// SDL
#include "SDL.h"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Global State
*****************************************************************/
unique_ptr<rr::Renderer> g_renderer;
::SDL_GLContext          g_gl_context = nullptr;

/****************************************************************
** Initialization
*****************************************************************/
void init_renderer() {
  // =========== SDL Stuff

  ::SDL_Window* window =
      static_cast<::SDL_Window*>( main_os_window_handle() );
  Delta logical_screen_size  = main_window_logical_size();
  Delta physical_screen_size = main_window_physical_size();

  g_gl_context = init_SDL_for_OpenGL( window );
  CHECK( g_gl_context != nullptr );

  // =========== gl/iface

  // The window and context must have been created first.
  gl::InitResult opengl_info = gl::init_opengl( gl::InitOptions{
      .include_glfunc_logging             = false,
      .initial_window_physical_pixel_size = physical_screen_size,
  } );

  lg.info( "{}", opengl_info.driver_info.pretty_print() );

  // =========== Renderer Config

  rr::RendererConfig renderer_config = {
      .logical_screen_size = logical_screen_size,
      .max_atlas_size      = { .w = 200, .h = 200 },
      // These are taken by reference.
      .sprite_sheets =
          config_tile_sheet.sheets.refl().sprite_sheets,
      .font_sheets = config_tile_sheet.sheets.refl().font_sheets,
  };

  // This renderer needs to be released before the SDL context is
  // cleaned up.
  g_renderer = rr::Renderer::create(
      renderer_config, [&] { sdl_gl_swap_window( window ); } );
}

void cleanup_renderer() {
  // These must be done in this order.

  // =========== Renderer Cleanup
  g_renderer.reset();

  // =========== SDL Cleanup
  close_SDL_for_OpenGL( g_gl_context );
  g_gl_context = nullptr;
}

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
