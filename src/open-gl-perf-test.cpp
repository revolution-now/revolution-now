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

// gl
#include "gl/attribs.hpp"
#include "gl/error.hpp"
#include "gl/iface-glad.hpp"
#include "gl/iface-logger.hpp"
#include "gl/iface.hpp"
#include "gl/shader.hpp"
#include "gl/texture.hpp"
#include "gl/uniform.hpp"
#include "gl/vertex-array.hpp"
#include "gl/vertex-buffer.hpp"

// gfx
#include "gfx/image.hpp"

// stb
#include "stb/image.hpp"

// base
#include "base/to-str-ext-std.hpp"

// SDL
#include "SDL.h"

using namespace std;

namespace rn {

namespace {

constexpr int kSpriteScale = 128;

struct Vertex {
  gl::vec2 pos;
  gl::vec2 tx_pos;
  gl::vec3 shading_color;
  gl::vec2 center;

  static consteval auto attributes() {
    return tuple{ VERTEX_ATTRIB_HOLDER( Vertex, pos ),
                  VERTEX_ATTRIB_HOLDER( Vertex, tx_pos ),
                  VERTEX_ATTRIB_HOLDER( Vertex, shading_color ),
                  VERTEX_ATTRIB_HOLDER( Vertex, center ) };
  }
};

using ProgramAttributes =
    mp::list<gl::vec2, gl::vec2, gl::vec3, gl::vec2>;

struct ProgramUniforms {
  static constexpr tuple uniforms{
      gl::UniformSpec<gl::vec2>( "screen_size" ),
      gl::UniformSpec<int>( "tick" ),
      gl::UniformSpec<int>( "tx" ),
  };
};

using ProgramType =
    gl::Program<ProgramAttributes, ProgramUniforms>;

using VertexArray_t = gl::VertexArray<gl::VertexBuffer<Vertex>>;

struct OpenGLObjects {
  ProgramType   program;
  VertexArray_t vertex_array;
  gl::Texture   tx;
};

int upload_sprites_buffer( OpenGLObjects* gl_objects,
                           Delta const&   screen_delta ) {
  Scale sprite_scale = Scale{ kSpriteScale };
  Delta sprite_delta = Delta{ 1_w, 1_h } * sprite_scale;

  float sheet_w = 256.0;
  float sheet_h = 192.0;

  float tx_ox = 0.0 / sheet_w;
  float tx_oy = 32.0 * 4 / sheet_h;
  float tx_dx = 32.0 / sheet_w;
  float tx_dy = 32.0 / sheet_h;

  // Important: should only allocate this once, since allocating
  // a large buffer is apparently expensive.
  static vector<Vertex> s_vertices = [&] {
    vector<Vertex> res;
    int            num_sprites =
        ( screen_delta.w._ + kSpriteScale ) / kSpriteScale *
        ( screen_delta.h._ + kSpriteScale ) / kSpriteScale;
    int num_vertices = num_sprites * 6;
    res.resize( num_vertices );

    auto rect = Rect::from( {}, screen_delta );
    int  i    = 0;
    W    w    = sprite_delta.w;
    H    h    = sprite_delta.h;
    for( auto coord : rect.to_grid_noalign( sprite_delta ) ) {
      Rect  sprite     = Rect::from( coord, sprite_delta );
      Coord center     = sprite.center();
      auto  add_vertex = [&]( Coord const& c, float tx_x,
                             float tx_y, float red ) {
        res[i++] = {
            // Coords
            { .x = float( c.x._ ), .y = float( c.y._ ) },
            // Texture coords
            { .x = tx_x, .y = tx_y },
            // Shading color
            { .x = red, .y = 0.0, .z = 0.0 },
            // Sprite Center
            { .x = float( center.x._ ),
               .y = float( center.y._ ) },
        };
      };

      add_vertex( coord, tx_ox, tx_oy, /*red=*/1.0 );
      add_vertex( coord + w, tx_ox + tx_dx, tx_oy, /*red=*/0.0 );
      add_vertex( coord + h, tx_ox, tx_oy + tx_dy, /*red=*/0.0 );
      add_vertex( coord + w, tx_ox + tx_dx, tx_oy, /*red=*/0.0 );
      add_vertex( coord + w + h, tx_ox + tx_dx, tx_oy + tx_dy,
                  /*red=*/0.0 );
      add_vertex( coord + h, tx_ox, tx_oy + tx_dy, /*red=*/0.0 );
    };
    return res;
  }();

  static auto once [[maybe_unused]] = [&] {
    gl_objects->vertex_array.buffer<0>().upload_data_replace(
        s_vertices, gl::e_draw_mode::stat1c );
    return monostate{};
  }();

#if 0 // to disable re-uploading.
  gl_objects->vertex_array.buffer<0>().upload_data_modify(
      s_vertices, 0 );

  for( int i = 0; i < 10; ++i )
    gl_objects->vertex_array.buffer<0>().upload_data_modify(
        span<Vertex const>{ s_vertices }.subspan( i, 1 ), i );
#endif

  return s_vertices.size();
}

OpenGLObjects init_opengl() {
  fs::path shaders = "src/shaders";

  UNWRAP_CHECK( vertex_shader_source,
                base::read_text_file_as_string(
                    shaders / "perf-test.vert" ) );
  UNWRAP_CHECK( fragment_shader_source,
                base::read_text_file_as_string(
                    shaders / "perf-test.frag" ) );
  UNWRAP_CHECK( vert_shader,
                gl::Shader::create( gl::e_shader_type::vertex,
                                    vertex_shader_source ) );
  UNWRAP_CHECK( frag_shader,
                gl::Shader::create( gl::e_shader_type::fragment,
                                    fragment_shader_source ) );
  UNWRAP_CHECK(
      pgrm, ProgramType::create( vert_shader, frag_shader ) );

  gl::Texture tx(
      stb::load_image( "assets/art/tiles/world.png" ) );

  return OpenGLObjects{ .program      = std::move( pgrm ),
                        .vertex_array = {},
                        .tx           = std::move( tx ) };
}

} // namespace

/****************************************************************
** Testing
*****************************************************************/
void render_loop( ::SDL_Window*         window,
                  gl::OpenGLWithLogger* opengl_with_logger ) {
  // == Initialization ==========================================

  OpenGLObjects gl_objects = init_opengl();

  // == Render ==================================================

  auto screen_delta = main_window_logical_size();

  GL_CHECK( glClearColor( 0.2, 0.3, 0.3, 1.0 ) );
  GL_CHECK(
      glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

  int scale       = kSpriteScale;
  int num_sprites = ( screen_delta.w._ + scale ) / scale *
                    ( screen_delta.h._ + scale ) / scale;

  auto& program    = gl_objects.program;
  auto& vert_array = gl_objects.vertex_array;

  auto tx_binder = gl_objects.tx.bind();

  using namespace ::base::literals;

  program["screen_size"_t] = gl::vec2{
      float( screen_delta.w._ ), float( screen_delta.h._ ) };
  program["tx"_t]   = 0;
  program["tick"_t] = 0;

  int num_vertices =
      upload_sprites_buffer( &gl_objects, screen_delta );
  program.run( vert_array, num_vertices );
  ::SDL_GL_SwapWindow( window );

  auto start_time = chrono::system_clock::now();
  long frames     = 0;

  // GL_CHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));

  opengl_with_logger->enable_logging( false );

  while( !input::is_q_down() ) {
    if( frames == 0 ) {
      fmt::print( "=== frame 0 ===\n" );
      opengl_with_logger->enable_logging( true );
    }
    if( frames == 5 ) {
      fmt::print( "=== frame 5 ===\n" );
      opengl_with_logger->enable_logging( true );
    }

    // Clear screen.
    GL_CHECK( glClearColor( 0, 0, 0, 1.0 ) );
    GL_CHECK(
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

    program["tick"_t] = frames;

    upload_sprites_buffer( &gl_objects, screen_delta );
    program.run( vert_array, num_vertices );
    ::SDL_GL_SwapWindow( window );

    ++frames;
    opengl_with_logger->enable_logging( false );
  }

  opengl_with_logger->enable_logging( false );
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
  lg.info( "Buffer Size:    {:.2}MB", double( num_vertices ) *
                                          sizeof( Vertex ) /
                                          ( 1024 * 1024 ) );
  lg.info( "Sprite count:   {}k", num_sprites / 1000 );
  lg.info( "=================================================" );
}

void open_gl_perf_test() {
  gl::OpenGLGlad       opengl_glad;
  gl::OpenGLWithLogger opengl_with_logger( &opengl_glad );
  gl::set_global_gl_implementation( &opengl_with_logger );

  opengl_with_logger.enable_logging( true );

  CHECK( ::SDL_GL_LoadLibrary( nullptr ) == 0,
         "Failed to load OpenGL library." );

  ::SDL_Window* window =
      static_cast<::SDL_Window*>( main_os_window_handle() );
  auto win_size = main_window_physical_size();

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

  // Doing this any earlier in the process doesn't seem to work.
  CHECK( gladLoadGLLoader(
             ( GLADloadproc )::SDL_GL_GetProcAddress ),
         "Failed to initialize GLAD." );

  // These next lines are needed on macOS to get the window to
  // appear (???).
  ::SDL_PumpEvents();
  ::SDL_DisplayMode display_mode;
  ::SDL_GetWindowDisplayMode( window, &display_mode );
  ::SDL_SetWindowDisplayMode( window, &display_mode );

  int max_texture_size = 0;
  GL_CHECK(
      glGetIntegerv( GL_MAX_TEXTURE_SIZE, &max_texture_size ) );

  lg.info( "OpenGL loaded:" );
  lg.info( "  * Vendor:      {}.",
           GL_CHECK( glGetString( GL_VENDOR ) ) );
  lg.info( "  * Renderer:    {}.",
           GL_CHECK( glGetString( GL_RENDERER ) ) );
  lg.info( "  * Version:     {}.",
           GL_CHECK( glGetString( GL_VERSION ) ) );
  lg.info( "  * Max Tx Size: {}x{}.", max_texture_size,
           max_texture_size );

  // GL_CHECK(glEnable( GL_DEPTH_TEST ));
  // GL_CHECK(glDepthFunc( GL_LEQUAL ));

  // Without this, alpha blending won't happen.
  GL_CHECK(
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
  GL_CHECK( glEnable( GL_BLEND ) );

  int viewport_scale = 1;

#ifdef __APPLE__
  // Ideally need to check if we are >= OSX 10.15 and set this to
  // two.
  viewport_scale = 2;
#endif

  // (0,0) is the lower-left of the rendering region. NOTE: This
  // needs to be re-called when window is resized.
  GL_CHECK( glViewport( 0, 0, win_size.w._ * viewport_scale,
                        win_size.h._ * viewport_scale ) );

  static constexpr bool wait_for_vsync = true;

  if( ::SDL_GL_SetSwapInterval( wait_for_vsync ? 1 : 0 ) != 0 )
    lg.warn( "setting swap interval is not supported." );

  render_loop( window, &opengl_with_logger );

  ::SDL_GL_DeleteContext( opengl_context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
