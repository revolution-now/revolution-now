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
#include "tx.hpp"

// render
#include "render/atlas.hpp"
#include "render/emitter.hpp"
#include "render/painter.hpp"
#include "render/sprite-sheet.hpp"
#include "render/text.hpp"
#include "render/vertex.hpp"

// gl
#include "gl/attribs.hpp"
#include "gl/error.hpp"
#include "gl/iface-glad.hpp"
#include "gl/iface-logger.hpp"
#include "gl/iface.hpp"
#include "gl/init.hpp"
#include "gl/misc.hpp"
#include "gl/shader.hpp"
#include "gl/texture.hpp"
#include "gl/uniform.hpp"
#include "gl/vertex-array.hpp"
#include "gl/vertex-buffer.hpp"

// gfx
#include "gfx/image.hpp"

// stb
#include "stb/image.hpp"

// refl
#include "refl/query-struct.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

// SDL
#include "SDL.h"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

constexpr int kSpriteScale = 128;

using ProgramAttributes =
    refl::member_type_list_t<rr::GenericVertex>;

struct ProgramUniforms {
  static constexpr tuple uniforms{
      gl::UniformSpec<int>( "u_atlas" ),
      gl::UniformSpec<gl::vec2>( "u_atlas_size" ),
      gl::UniformSpec<gl::vec2>( "u_screen_size" ),
  };
};

using ProgramType =
    gl::Program<ProgramAttributes, ProgramUniforms>;

using VertexArray_t =
    gl::VertexArray<gl::VertexBuffer<rr::GenericVertex>>;

struct OpenGLObjects {
  ProgramType                program;
  VertexArray_t              vertex_array;
  rr::AtlasMap               atlas_map;
  size                       atlas_size;
  gl::Texture                atlas_tx;
  unordered_map<string, int> atlas_ids;
  rr::AsciiFont              basic_font;
};

OpenGLObjects init_opengl() {
  fs::path shaders = "src/render";

  UNWRAP_CHECK( vertex_shader_source,
                base::read_text_file_as_string(
                    shaders / "generic.vert" ) );
  UNWRAP_CHECK( fragment_shader_source,
                base::read_text_file_as_string(
                    shaders / "generic.frag" ) );
  UNWRAP_CHECK( vert_shader,
                gl::Shader::create( gl::e_shader_type::vertex,
                                    vertex_shader_source ) );
  UNWRAP_CHECK( frag_shader,
                gl::Shader::create( gl::e_shader_type::fragment,
                                    fragment_shader_source ) );
  UNWRAP_CHECK(
      pgrm, ProgramType::create( vert_shader, frag_shader ) );

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
      .img_path = "assets/art/fonts/basic-6x8.png",
  };

  rr::AtlasBuilder           atlas_builder;
  unordered_map<string, int> atlas_ids;

  CHECK_HAS_VALUE( rr::load_sprite_sheet(
      atlas_builder, world_config, atlas_ids ) );
  UNWRAP_CHECK( ascii_font, load_ascii_font_sheet(
                                atlas_builder, font_config ) );

  UNWRAP_CHECK(
      atlas, atlas_builder.build( size{ .w = 200, .h = 200 } ) );

  size        atlas_size = atlas.img.size_pixels();
  gl::Texture atlas_tx( std::move( atlas.img ) );

  return OpenGLObjects{
      .program      = std::move( pgrm ),
      .vertex_array = {},
      .atlas_map    = std::move( atlas.dict ),
      .atlas_size   = atlas_size,
      .atlas_tx     = std::move( atlas_tx ),
      .atlas_ids    = std::move( atlas_ids ),
      .basic_font   = std::move( ascii_font ),
  };
}

void paint_things( unordered_map<string, int> const& atlas_ids,
                   rr::AsciiFont const&              ascii_font,
                   rr::Painter&                      painter ) {
  painter.draw_point(
      point{ .x = 50, .y = 50 },
      pixel{ .r = 255, .g = 0, .b = 0, .a = 255 } );

  // clang-format off
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=100}, .size={.w=0, .h=0}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=120}, .size={.w=1, .h=0}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=140}, .size={.w=0, .h=1}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=160}, .size={.w=1, .h=1}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=180}, .size={.w=2, .h=2}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=200}, .size={.w=3, .h=3}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=220}, .size={.w=4, .h=4}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=240}, .size={.w=5, .h=5}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );
  painter.draw_empty_rect( rect{ .origin={.x=200, .y=100}, .size={.w=50,.h=50}}, rr::Painter::e_border_mode::outside, gfx::pixel::red() );

  painter.draw_empty_rect( rect{ .origin={.x=100, .y=100}, .size={.w=0, .h=0} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=120}, .size={.w=1, .h=0} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=140}, .size={.w=0, .h=1} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=160}, .size={.w=1, .h=1} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=180}, .size={.w=2, .h=2} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=200}, .size={.w=3, .h=3} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=220}, .size={.w=4, .h=4} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=100, .y=240}, .size={.w=5, .h=5} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  painter.draw_empty_rect( rect{ .origin={.x=200, .y=100}, .size={.w=50, .h=50} }, rr::Painter::e_border_mode::inside, gfx::pixel::white() );
  // clang-format on

  painter.draw_solid_rect(
      rect{ .origin = { .x = 300, .y = 100 },
            .size   = { .w = 50, .h = 50 } },
      pixel{ .r = 128, .g = 64, .b = 0, .a = 255 } );
  painter.with_mods( { .depixelate = .5, .alpha = .5 } )
      .draw_solid_rect(
          rect{ .origin = { .x = 325, .y = 125 },
                .size   = { .w = 50, .h = 50 } },
          pixel{ .r = 0, .g = 0, .b = 0, .a = 255 } );

  UNWRAP_CHECK( water_id, base::lookup( atlas_ids, "water" ) );
  UNWRAP_CHECK( grass_id, base::lookup( atlas_ids, "grass" ) );
  painter.draw_sprite( water_id, { .x = 300, .y = 200 } );
  painter.draw_sprite( grass_id, { .x = 364, .y = 200 } );

  painter.draw_sprite_scale(
      water_id, rect{ .origin = { .x = 450, .y = 200 },
                      .size   = { .w = 128, .h = 64 } } );

  painter.with_mods( { .depixelate = .5, .alpha = 1.0 } )
      .draw_sprite( water_id, { .x = 300, .y = 250 } )
      .draw_sprite( grass_id, { .x = 364, .y = 250 } );

  pixel     text_color{ .r = 0, .g = 0, .b = 48, .a = 255 };
  rr::Typer typer( painter, ascii_font, { .x = 300, .y = 300 },
                   text_color );
  typer.write( "Color of this text is {}.\nThe End.\n\n-David",
               text_color );
}

void upload_vertices(
    rr::AtlasMap const& m, rr::AsciiFont const& ascii_font,
    unordered_map<string, int> const& atlas_ids,
    VertexArray_t&                    vert_arr,
    vector<rr::GenericVertex>&        buffer ) {
  buffer.clear();
  rr::Emitter emitter( buffer );
  rr::Painter painter( m, emitter );
  paint_things( atlas_ids, ascii_font, painter );
  vert_arr.buffer<0>().upload_data_replace(
      buffer, gl::e_draw_mode::stat1c );
}

/****************************************************************
** Testing
*****************************************************************/
void render_loop( ::SDL_Window*         window,
                  gl::OpenGLWithLogger* opengl ) {
  // == Initialization ==========================================

  OpenGLObjects gl_objects = init_opengl();

  // == Render ==================================================

  auto screen_delta = main_window_logical_size();

  gl::clear(
      gl::color{ .r = 0.2, .g = 0.3, .b = 0.3, .a = 1.0 } );

  int scale       = kSpriteScale;
  int num_sprites = ( screen_delta.w._ + scale ) / scale *
                    ( screen_delta.h._ + scale ) / scale;

  auto& program    = gl_objects.program;
  auto& vert_array = gl_objects.vertex_array;

  // Texture 0 should be the default, but let's just set it
  // anyway to be sure.
  GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );
  auto tx_binder = gl_objects.atlas_tx.bind();

  using namespace ::base::literals;

  program["u_screen_size"_t] = gl::vec2{
      float( screen_delta.w._ ), float( screen_delta.h._ ) };
  program["u_atlas"_t] = 0; // GL_TEXTURE0
  program["u_atlas_size"_t] =
      gl::vec2::from_size( gl_objects.atlas_size );
  // program["tick"_t] = 0;

  vector<rr::GenericVertex> vertices;

  upload_vertices( gl_objects.atlas_map, gl_objects.basic_font,
                   gl_objects.atlas_ids, gl_objects.vertex_array,
                   vertices );
  int num_vertices = vertices.size();
  program.run( vert_array, num_vertices );
  ::SDL_GL_SwapWindow( window );

  auto start_time = chrono::system_clock::now();
  long frames     = 0;

  // GL_CHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));

  opengl->enable_logging( false );

  while( !input::is_q_down() ) {
    if( frames == 0 ) {
      fmt::print( "=== frame 0 ===\n" );
      opengl->enable_logging( true );
    }
    if( frames == 5 ) {
      fmt::print( "=== frame 5 ===\n" );
      opengl->enable_logging( true );
    }

    gl::clear(
        gl::color{ .r = 0.2, .g = 0.3, .b = 0.3, .a = 1.0 } );

    // program["tick"_t] = frames;

    upload_vertices( gl_objects.atlas_map, gl_objects.basic_font,
                     gl_objects.atlas_ids,
                     gl_objects.vertex_array, vertices );
    // FIXME: vertex array/buffer should know their size.
    program.run( vert_array, num_vertices );
    ::SDL_GL_SwapWindow( window );

    ++frames;
    opengl->enable_logging( false );
  }

  opengl->enable_logging( false );
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
           double( num_vertices ) * sizeof( rr::GenericVertex ) /
               ( 1024 * 1024 ) );
  lg.info( "Vertex count:   {}k", num_vertices / 1000 );
  lg.info( "Sprite count:   {}k", num_sprites / 1000 );
  lg.info( "=================================================" );
}

} // namespace

void open_gl_perf_test() {
  /**************************************************************
  ** SDL Stuff
  ***************************************************************/
  ::SDL_Window* window =
      static_cast<::SDL_Window*>( main_os_window_handle() );
  auto      win_size_delta = main_window_physical_size();
  gfx::size win_size       = { .w = win_size_delta.w._,
                               .h = win_size_delta.h._ };

  // These next lines are needed on macOS to get the window to
  // appear (???).
#ifdef __APPLE__
  ::SDL_PumpEvents();
  ::SDL_DisplayMode display_mode;
  ::SDL_GetWindowDisplayMode( window, &display_mode );
  ::SDL_SetWindowDisplayMode( window, &display_mode );
#endif

  ::SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
  ::SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
  ::SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
  ::SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK,
                         SDL_GL_CONTEXT_PROFILE_CORE );

  /* Turn on double buffering with a 24bit Z buffer.
   * You may need to change this to 16 or 32 for your system */
  ::SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  ::SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

  /* Create our opengl context and attach it to our window */
  ::SDL_GLContext opengl_context =
      ::SDL_GL_CreateContext( window );
  CHECK( opengl_context );

  static constexpr bool wait_for_vsync = true;

  if( ::SDL_GL_SetSwapInterval( wait_for_vsync ? 1 : 0 ) != 0 )
    lg.warn( "setting swap interval is not supported." );

  /**************************************************************
  ** gl/iface
  ***************************************************************/
  // The window and context must have been created first.
  gl::InitResult opengl_info = init_opengl( gl::InitOptions{
      .enable_glfunc_logging              = true,
      .initial_window_physical_pixel_size = win_size,
  } );

  lg.info( "{}", opengl_info.driver_info.pretty_print() );

  /**************************************************************
  ** Render Loop
  ***************************************************************/
  render_loop( window, opengl_info.logging_iface.get() );

  /**************************************************************
  ** SDL Cleanup
  ***************************************************************/
  ::SDL_GL_DeleteContext( opengl_context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
