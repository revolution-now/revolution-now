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
#include "init.hpp"
#include "logger.hpp"
#include "maybe.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "sdl.hpp"

// config
#include "config/tile-sheet.rds.hpp"

// gl
#include "gl/init.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Global State
*****************************************************************/
unique_ptr<rr::Renderer> g_renderer;
::SDL_Window*            g_window = nullptr;
gl::InitResult           g_gl_iface;
::SDL_GLContext          g_gl_context = nullptr;

/****************************************************************
** Initialization
*****************************************************************/
void init_renderer() {
  // =========== SDL Stuff

  g_window =
      static_cast<::SDL_Window*>( main_os_window_handle() );
  Delta logical_screen_size  = main_window_logical_size();
  Delta physical_screen_size = main_window_physical_size();

  g_gl_context = init_SDL_for_OpenGL( g_window );
  CHECK( g_gl_context != nullptr );

  // =========== gl/iface

  // The window and context must have been created first.
  g_gl_iface = gl::init_opengl( gl::InitOptions{
      .include_glfunc_logging             = false,
      .initial_window_physical_pixel_size = physical_screen_size,
  } );

  lg.info( "{}", g_gl_iface.driver_info.pretty_print() );

  // =========== Renderer Config

  rr::RendererConfig renderer_config = {
      .logical_screen_size = logical_screen_size,
      .max_atlas_size      = { .w = 3000, .h = 2000 },
      // These are taken by reference.
      .sprite_sheets  = config_tile_sheet.sheets.sprite_sheets,
      .font_sheets    = config_tile_sheet.sheets.font_sheets,
      .dump_atlas_png = config_tile_sheet.dump_texture_atlas_to,
  };

  // This renderer needs to be released before the SDL context is
  // cleaned up.
  g_renderer = rr::Renderer::create(
      renderer_config, [] { sdl_gl_swap_window( g_window ); } );

  lg.info( "texture atlas size: {}.",
           g_renderer->atlas_img_size() );
}

void cleanup_renderer() {
  // These must be done in this order.

  // =========== Renderer Cleanup
  g_renderer.reset();

  // =========== gl/iface Cleanup
  g_gl_iface = {};

  // =========== SDL Cleanup
  close_SDL_for_OpenGL( g_gl_context );
  g_gl_context = nullptr;
  g_window     = nullptr;
}

REGISTER_INIT_ROUTINE( renderer );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
bool is_renderer_loaded() { return g_renderer != nullptr; }

rr::Renderer& global_renderer_use_only_when_needed() {
  CHECK( is_renderer_loaded(),
         "The renderer has not yet been initialized." );
  return *g_renderer;
}

} // namespace rn
