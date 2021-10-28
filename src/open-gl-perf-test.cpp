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
#include "gl/vertex-array.hpp"
#include "gl/vertex-buffer.hpp"

// SDL
#include "SDL.h"

// GLAD (OpenGL Loader)
#include "glad/glad.h"

using namespace std;

namespace rn {

namespace {

constexpr int kSpriteScale = 8;

GLuint load_texture( fs::path const& p ) {
  auto           img = Surface::load_image( p.string().c_str() );
  ::SDL_Surface* surface = (::SDL_Surface*)img.get();
  // Make sure we have RGBA.
  CHECK( surface->format->BytesPerPixel == 4 );

  GLuint opengl_texture = 0;
  GL_CHECK( glGenTextures( 1, &opengl_texture ) );
  GL_CHECK( glBindTexture( GL_TEXTURE_2D, opengl_texture ) );

  // Configure how OpenGL maps coordinate to texture pixel.
  GL_CHECK( glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
  GL_CHECK( glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );

  GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE ) );
  GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE ) );

  GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, surface->w,
                          surface->h, 0, GL_RGBA,
                          GL_UNSIGNED_BYTE, surface->pixels ) );
  return opengl_texture;
}

GLuint load_shader_pgrm( fs::path const& vert,
                         fs::path const& frag ) {
  int              success;
  constexpr size_t kErrorLength = 512;
  char             errors[kErrorLength];

  // == Vertex Shader ===========================================

  GLuint vertex_shader =
      GL_CHECK( glCreateShader( GL_VERTEX_SHADER ) );
  UNWRAP_CHECK( vertex_shader_source,
                base::read_text_file_as_string( vert ) );
  char const* p_vertex_shader_source =
      vertex_shader_source.c_str();
  GL_CHECK( glShaderSource( vertex_shader, 1,
                            &p_vertex_shader_source, nullptr ) );
  GL_CHECK( glCompileShader( vertex_shader ) ); // check errors?
  // Check for compiler errors.
  GL_CHECK( glGetShaderiv( vertex_shader, GL_COMPILE_STATUS,
                           &success ) );
  if( !success ) {
    GL_CHECK( glGetShaderInfoLog( vertex_shader, kErrorLength,
                                  NULL, errors ) );
    FATAL( "Vertex shader compilation failed: {}", errors );
  }

  // == Fragment Shader =========================================

  GLuint fragment_shader =
      GL_CHECK( glCreateShader( GL_FRAGMENT_SHADER ) );
  UNWRAP_CHECK( fragment_shader_source,
                base::read_text_file_as_string( frag ) );
  char const* p_fragment_shader_source =
      fragment_shader_source.c_str();
  GL_CHECK( glShaderSource(
      fragment_shader, 1, &p_fragment_shader_source, nullptr ) );
  GL_CHECK(
      glCompileShader( fragment_shader ) ); // check errors?
  // Check for compiler errors.
  GL_CHECK( glGetShaderiv( fragment_shader, GL_COMPILE_STATUS,
                           &success ) );
  if( !success ) {
    GL_CHECK( glGetShaderInfoLog( fragment_shader, kErrorLength,
                                  NULL, errors ) );
    FATAL( "Fragment shader compilation failed: {}", errors );
  }

  // == Shader Program ==========================================

  GLuint shader_program = GL_CHECK( glCreateProgram() );

  GL_CHECK( glAttachShader( shader_program, vertex_shader ) );
  GL_CHECK( glAttachShader( shader_program, fragment_shader ) );
  GL_CHECK( glLinkProgram( shader_program ) );
  // Check for linking errors.
  GL_CHECK( glGetProgramiv( shader_program, GL_LINK_STATUS,
                            &success ) );
  if( !success ) {
    GL_CHECK( glGetProgramInfoLog( shader_program, kErrorLength,
                                   NULL, errors ) );
    FATAL( "Shader program linking failed: {}", errors );
  }

  GL_CHECK( glValidateProgram( shader_program ) );
  GLint validation_successful;
  GL_CHECK( glGetProgramiv( shader_program, GL_VALIDATE_STATUS,
                            &validation_successful ) );
  GLint out_size;
  GL_CHECK( glGetProgramInfoLog( shader_program,
                                 /*maxlength=*/kErrorLength,
                                 &out_size, errors ) );
  string_view info_log( errors, out_size );
  CHECK( validation_successful == GL_TRUE,
         "shader program failed validation: info "
         "log: {}",
         info_log );

  // TODO: Consider calling glDetachShader here.

  GL_CHECK( glDeleteShader( vertex_shader ) );
  GL_CHECK( glDeleteShader( fragment_shader ) );

  return shader_program;
}

struct Vertex {
  gl::vec2 pos;
  gl::vec2 tx_pos;

  static consteval auto attributes() {
    return tuple{ VERTEX_ATTRIB_HOLDER( Vertex, pos ),
                  VERTEX_ATTRIB_HOLDER( Vertex, tx_pos ) };
  }
};

struct OpenGLObjects {
  GLuint shader_program;
  GLuint screen_size_location;
  GLuint tick_location;
  GLuint tx_location;
  // The order of these matters.
  gl::VertexArray<gl::VertexBuffer<Vertex>> vertex_array;
  GLuint                                    opengl_texture;
};

void draw_vertices( OpenGLObjects*     gl_objects, Delta const&,
                    span<Vertex const> vertices ) {
  static auto once [[maybe_unused]] = [&] {
    gl_objects->vertex_array.buffer<0>().upload_data_replace(
        vertices, gl::e_draw_mode::stat1c );
    return monostate{};
  }();

  gl_objects->vertex_array.buffer<0>().upload_data_modify(
      vertices, 0 );

  for( int i = 0; i < 10; ++i )
    gl_objects->vertex_array.buffer<0>().upload_data_modify(
        vertices.subspan( i, 1 ), i );

  GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, vertices.size() ) );
}

int draw_sprites_batched( OpenGLObjects* gl_objects,
                          Delta const&   screen_delta ) {
  int scale = kSpriteScale;

  float sheet_w = 256.0;
  float sheet_h = 192.0;

  float tx_ox = 0.0 / sheet_w;
  float tx_oy = 32.0 * 4 / sheet_h;
  float tx_dx = 32.0 / sheet_w;
  float tx_dy = 32.0 / sheet_h;

  static size_t num_rows = 0;

  // Important: should only allocate this once, since allocating
  // a large buffer is apparently expensive.
  static vector<Vertex> s_vertices = [&] {
    vector<Vertex> res;
    int num_sprites = ( screen_delta.w._ + scale ) / scale *
                      ( screen_delta.h._ + scale ) / scale;
    int num_vertices = num_sprites * 6;
    res.resize( num_vertices );

    auto rect = Rect::from( {}, screen_delta );
    int  i    = 0;
    W    w{ scale };
    H    h{ scale };
    for( auto coord : rect.to_grid_noalign( Scale{ scale } ) ) {
      auto add_vertex = [&]( Coord const& c, float tx_x,
                             float tx_y ) {
        ++num_rows;
        res[i++] = {
            // Coords
            { .x = float( c.x._ ), .y = float( c.y._ ) },
            // Texture coords
            { .x = tx_x, .y = tx_y } };
      };

      add_vertex( coord, tx_ox, tx_oy );
      add_vertex( coord + w, tx_ox + tx_dx, tx_oy );
      add_vertex( coord + h, tx_ox, tx_oy + tx_dy );
      add_vertex( coord + w, tx_ox + tx_dx, tx_oy );
      add_vertex( coord + w + h, tx_ox + tx_dx, tx_oy + tx_dy );
      add_vertex( coord + h, tx_ox, tx_oy + tx_dy );
    };
    return res;
  }();

  draw_vertices( gl_objects, screen_delta, s_vertices );
  return s_vertices.size() *
         sizeof( decltype( s_vertices )::value_type );
}

OpenGLObjects init_opengl() {
  fs::path shaders = "src/shaders";

  OpenGLObjects gl_objects;

  gl_objects.shader_program = load_shader_pgrm(
      shaders / "perf-test.vert", shaders / "perf-test.frag" );

  gl_objects.screen_size_location =
      GL_CHECK( glGetUniformLocation( gl_objects.shader_program,
                                      "screen_size" ) );
  gl_objects.tick_location = GL_CHECK( glGetUniformLocation(
      gl_objects.shader_program, "tick" ) );
  gl_objects.tx_location   = GL_CHECK(
        glGetUniformLocation( gl_objects.shader_program, "tx" ) );

  gl_objects.opengl_texture =
      load_texture( "assets/art/tiles/world.png" );

  return gl_objects;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/

/****************************************************************
** Testing
*****************************************************************/
void open_gl_perf_test() {
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

  bool wait_for_vsync = true;

  CHECK( !::SDL_GL_SetSwapInterval( wait_for_vsync ? 1 : 0 ),
         "setting swap interval is not supported." );

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

  auto draw_func = draw_sprites_batched;
  // auto draw_func = draw_sprites_separate;

  GL_CHECK( glBindTexture( GL_TEXTURE_2D,
                           gl_objects.opengl_texture ) );

  GL_CHECK( glUseProgram( gl_objects.shader_program ) );
  GL_CHECK( glUniform2f( gl_objects.screen_size_location,
                         float( screen_delta.w._ ),
                         float( screen_delta.h._ ) ) );
  GL_CHECK( glUniform2f( gl_objects.tick_location, 0, 0 ) );
  GL_CHECK( glUniform1i( gl_objects.tx_location, 0 ) );

  int buf_size = 0;
  {
    auto binder = gl_objects.vertex_array.bind();
    buf_size    = draw_func( &gl_objects, screen_delta );
    ::SDL_GL_SwapWindow( window );
  }

  auto start_time = chrono::system_clock::now();
  long frames     = 0;

  // GL_CHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));

  while( !input::is_q_down() ) {
    // Clear screen.
    GL_CHECK( glClearColor( 0, 0, 0, 1.0 ) );
    GL_CHECK(
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

    // Set program and uniforms.
    GL_CHECK( glUseProgram( gl_objects.shader_program ) );
    GL_CHECK( glUniform2f( gl_objects.tick_location, frames,
                           frames ) );

    // Bind the VAO and draw.
    {
      auto binder = gl_objects.vertex_array.bind();
      draw_func( &gl_objects, screen_delta );
      ::SDL_GL_SwapWindow( window );
    }

    ++frames;
  }

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
           double( buf_size ) / ( 1024 * 1024 ) );
  lg.info( "Sprite count:   {}k", num_sprites / 1000 );
  lg.info( "=================================================" );

  // == Cleanup =================================================

  GL_CHECK( glDeleteTextures( 1, &gl_objects.opengl_texture ) );
  GL_CHECK( glDeleteProgram( gl_objects.shader_program ) );

  ::SDL_GL_DeleteContext( opengl_context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
