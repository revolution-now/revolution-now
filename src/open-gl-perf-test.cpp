/****************************************************************
**open-gl-perf-test.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-22.
*
* Description: OpenGL performance testing.
*
*****************************************************************/
#include "open-gl-perf-test.hpp"

// Revolution Now
#include "base/io.hpp"
#include "error.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "tx.hpp"

// render
#include "render/painter.hpp"
#include "render/renderer.hpp"
#include "render/sprite-sheet.hpp"
#include "render/text.hpp"

// gl
#include "gl/iface-logger.hpp"
#include "gl/init.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

#define OUTSIDE( x__, y__, w__, h__ )         \
  painter.draw_empty_rect(                    \
      rect{ .origin = { .x = x__, .y = y__ }, \
            .size   = { .w = w__, .h = h__ } }, \
      rr::Painter::e_border_mode::outside, pixel::red() );

#define INSIDE( x__, y__, w__, h__ )          \
  painter.draw_empty_rect(                    \
      rect{ .origin = { .x = x__, .y = y__ }, \
            .size   = { .w = w__, .h = h__ } }, \
      rr::Painter::e_border_mode::inside, pixel::white() );

void paint_things( rr::Renderer& renderer ) {
  rr::Painter painter = renderer.painter();

  painter.draw_point(
      point{ .x = 50, .y = 50 },
      pixel{ .r = 255, .g = 0, .b = 0, .a = 255 } );

  OUTSIDE( 100, 100, 0, 0 );
  OUTSIDE( 100, 120, 1, 0 );
  OUTSIDE( 100, 140, 0, 1 );
  OUTSIDE( 100, 160, 1, 1 );
  OUTSIDE( 100, 180, 2, 2 );
  OUTSIDE( 100, 200, 3, 3 );
  OUTSIDE( 100, 220, 4, 4 );
  OUTSIDE( 100, 240, 5, 5 );
  OUTSIDE( 200, 100, 50, 50 );

  INSIDE( 100, 100, 0, 0 );
  INSIDE( 100, 120, 1, 0 );
  INSIDE( 100, 140, 0, 1 );
  INSIDE( 100, 160, 1, 1 );
  INSIDE( 100, 180, 2, 2 );
  INSIDE( 100, 200, 3, 3 );
  INSIDE( 100, 220, 4, 4 );
  INSIDE( 100, 240, 5, 5 );
  INSIDE( 200, 100, 50, 50 );

  painter.draw_solid_rect(
      rect{ .origin = { .x = 300, .y = 100 },
            .size   = { .w = 50, .h = 50 } },
      pixel{ .r = 128, .g = 64, .b = 0, .a = 255 } );
  painter.with_mods( { .depixelate = .5, .alpha = .5 } )
      .draw_solid_rect(
          rect{ .origin = { .x = 325, .y = 125 },
                .size   = { .w = 50, .h = 50 } },
          pixel{ .r = 0, .g = 0, .b = 0, .a = 255 } );

  UNWRAP_CHECK( water_id,
                base::lookup( renderer.atlas_ids(), "water" ) );
  UNWRAP_CHECK( grass_id,
                base::lookup( renderer.atlas_ids(), "grass" ) );
  painter.draw_sprite( water_id, { .x = 300, .y = 200 } );
  painter.draw_sprite( grass_id, { .x = 364, .y = 200 } );

  painter.draw_sprite_scale(
      water_id, rect{ .origin = { .x = 450, .y = 200 },
                      .size   = { .w = 128, .h = 64 } } );

  painter.with_mods( { .depixelate = .5, .alpha = 1.0 } )
      .draw_sprite( water_id, { .x = 300, .y = 250 } )
      .draw_sprite( grass_id, { .x = 364, .y = 250 } );

  pixel     text_color{ .r = 0, .g = 0, .b = 48, .a = 255 };
  rr::Typer typer = renderer.typer(
      "simple", { .x = 300, .y = 300 }, text_color );
  typer.write( "Color of this text is {}.\nThe End.\n\n-David",
               text_color );
}

/****************************************************************
** Testing
*****************************************************************/
void render_loop(
    rr::Renderer&                    renderer,
    base::function_ref<void( bool )> enable_opengl_logging,
    base::function_ref<void()>       swap ) {
  int  num_vertices = 0;
  auto start_time   = chrono::system_clock::now();
  long frames       = 0;

  enable_opengl_logging( false );

  while( !input::is_q_down() ) {
    if( frames == 0 ) {
      fmt::print( "=== frame 0 ===\n" );
      enable_opengl_logging( true );
    }

    renderer.begin_pass();
    renderer.clear_screen( pixel{ .r = uint8_t( 0.2 * 255 ),
                                  .g = uint8_t( 0.3 * 255 ),
                                  .b = uint8_t( 0.3 * 255 ),
                                  .a = uint8_t( 255 ) } );
    paint_things( renderer );
    num_vertices = renderer.end_pass();
    swap();

    ++frames;
    enable_opengl_logging( false );
  }

  enable_opengl_logging( false );
  fmt::print( "=== end frames ===\n" );

  auto end_time   = chrono::system_clock::now();
  auto delta_time = end_time - start_time;
  auto millis =
      chrono::duration_cast<chrono::milliseconds>( delta_time );
  double max_fpms = frames / double( millis.count() );
  double max_fps  = max_fpms * 1000.0;
  lg.info( "=================================================" );
  lg.info( "OpenGL Performance Test" );
  lg.info( "=================================================" );
  lg.info( "Total time:     {}.", delta_time );
  lg.info( "Avg frame rate: {}.", max_fps );
  lg.info( "Buffer Size:    {:.2}MB",
           double( num_vertices ) * 48 / ( 1024 * 1024 ) );
  lg.info( "Vertex count:   {}k", num_vertices / 1000 );
  lg.info( "=================================================" );
}

} // namespace

void open_gl_perf_test() {
  /**************************************************************
  ** SDL Stuff
  ***************************************************************/
  ::SDL_Window* window =
      static_cast<::SDL_Window*>( main_os_window_handle() );
  auto win_size_delta = main_window_physical_size();
  size win_size       = { .w = win_size_delta.w._,
                          .h = win_size_delta.h._ };

  ::SDL_GLContext opengl_context = init_SDL_for_OpenGL( window );

  /**************************************************************
  ** gl/iface
  ***************************************************************/
  // The window and context must have been created first.
  gl::InitResult opengl_info = init_opengl( gl::InitOptions{
      .enable_glfunc_logging              = true,
      .initial_window_physical_pixel_size = win_size,
  } );

  auto enable_logging = [&]( bool enable_logging ) {
    opengl_info.logging_iface->enable_logging( enable_logging );
  };

  lg.info( "{}", opengl_info.driver_info.pretty_print() );

  /**************************************************************
  ** Renderer Config
  ***************************************************************/
  rr::SpriteSheetConfig world_config{
      .img_path    = "assets/art/tiles/world.png",
      .sprite_size = size{ .w = 32, .h = 32 },
      .sprites =
          {
              { "water", point{ .x = 0, .y = 0 } },
              { "grass", point{ .x = 1, .y = 0 } },
          },
  };

  rr::AsciiFontSheetConfig font_config{
      .img_path  = "assets/art/fonts/basic-6x8.png",
      .font_name = "simple",
  };

  Delta logical_screen_size = main_window_logical_size();

  rr::RendererConfig renderer_config = {
      .logical_screen_size =
          size{ .w = logical_screen_size.w._,
                .h = logical_screen_size.h._ },
      .max_atlas_size = { .w = 200, .h = 200 },
      .sprite_sheets  = { std::move( world_config ) },
      .font_sheets    = { std::move( font_config ) },
  };

  /**************************************************************
  ** Render Loop
  ***************************************************************/
  {
    // This renderer needs to be released before the SDL context
    // is cleaned up.
    rr::Renderer renderer =
        rr::Renderer::create( renderer_config );

    render_loop( renderer, enable_logging,
                 [&] { ::SDL_GL_SwapWindow( window ); } );
  }

  /**************************************************************
  ** SDL Cleanup
  ***************************************************************/
  close_SDL_for_OpenGL( opengl_context );
}

} // namespace rn
