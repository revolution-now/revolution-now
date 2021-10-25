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

// SDL
#include "SDL.h"

// GLAD (OpenGL Loader)
#include "glad/glad.h"

using namespace std;

namespace rn {

namespace {

constexpr int  kSpriteScale      = 8;
constexpr long kVertexSizeFloats = 5;
constexpr long kVertexSizeBytes =
    sizeof( float ) * kVertexSizeFloats;

void check_gl_errors() {
  GLenum err_code;
  bool   error_found = false;
  while( true ) {
    err_code = glGetError();
    if( err_code == GL_NO_ERROR ) break;
    lg.error( "OpenGL error: {}", err_code );
    error_found = true;
  }
  if( error_found ) {
    FATAL(
        "Terminating after one or more OpenGL errors "
        "occurred." );
  }
}

GLuint load_texture( fs::path const& p ) {
  auto           img = Surface::load_image( p.string().c_str() );
  ::SDL_Surface* surface = (::SDL_Surface*)img.get();
  // Make sure we have RGBA.
  CHECK( surface->format->BytesPerPixel == 4 );

  GLuint opengl_texture = 0;
  glGenTextures( 1, &opengl_texture );
  glBindTexture( GL_TEXTURE_2D, opengl_texture );

  // Configure how OpenGL maps coordinate to texture pixel.
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                   GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                   GL_NEAREST );

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                   GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                   GL_CLAMP_TO_EDGE );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, surface->w,
                surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                surface->pixels );
  return opengl_texture;
}

GLuint load_shader_pgrm( fs::path const& vert,
                         fs::path const& frag ) {
  int              success;
  constexpr size_t kErrorLength = 512;
  char             errors[kErrorLength];

  // == Vertex Shader ===========================================

  GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  UNWRAP_CHECK( vertex_shader_source,
                base::read_text_file_as_string( vert ) );
  char const* p_vertex_shader_source =
      vertex_shader_source.c_str();
  glShaderSource( vertex_shader, 1, &p_vertex_shader_source,
                  nullptr );
  glCompileShader( vertex_shader ); // check errors?
  // Check for compiler errors.
  glGetShaderiv( vertex_shader, GL_COMPILE_STATUS, &success );
  if( !success ) {
    glGetShaderInfoLog( vertex_shader, kErrorLength, NULL,
                        errors );
    FATAL( "Vertex shader compilation failed: {}", errors );
  }

  // == Fragment Shader =========================================

  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  UNWRAP_CHECK( fragment_shader_source,
                base::read_text_file_as_string( frag ) );
  char const* p_fragment_shader_source =
      fragment_shader_source.c_str();
  glShaderSource( fragment_shader, 1, &p_fragment_shader_source,
                  nullptr );
  glCompileShader( fragment_shader ); // check errors?
  // Check for compiler errors.
  glGetShaderiv( fragment_shader, GL_COMPILE_STATUS, &success );
  if( !success ) {
    glGetShaderInfoLog( fragment_shader, kErrorLength, NULL,
                        errors );
    FATAL( "Fragment shader compilation failed: {}", errors );
  }

  // == Shader Program ==========================================

  GLuint shader_program = glCreateProgram();

  glAttachShader( shader_program, vertex_shader );
  glAttachShader( shader_program, fragment_shader );
  glLinkProgram( shader_program );
  // Check for linking errors.
  glGetProgramiv( shader_program, GL_LINK_STATUS, &success );
  if( !success ) {
    glGetProgramInfoLog( shader_program, kErrorLength, NULL,
                         errors );
    FATAL( "Shader program linking failed: {}", errors );
  }

  glValidateProgram( shader_program );
  GLint validation_successful;
  glGetProgramiv( shader_program, GL_VALIDATE_STATUS,
                  &validation_successful );
  GLint out_size;
  glGetProgramInfoLog( shader_program,
                       /*maxlength=*/kErrorLength, &out_size,
                       errors );
  string_view info_log( errors, out_size );
  lg.debug( "shader progrma info log: {}\n", info_log );
  CHECK( validation_successful == GL_TRUE,
         "shader program failed validation: check above info "
         "log." );

  // TODO: Consider calling glDetachShader here.

  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  return shader_program;
}

struct OpenGLObjects {
  GLuint shader_program;
  GLuint screen_size_location;
  GLuint tick_location;
  GLuint tx_location;
  GLuint vertex_array_object;
  GLuint vertex_buffer_object;
  GLuint opengl_texture;
};

void draw_vertices( OpenGLObjects* gl_objects, Delta const&,
                    float* vertices, int array_size,
                    int num_vertices ) {
  // auto mode = GL_DYNAMIC_DRAW;
  auto        mode  = GL_STATIC_DRAW;
  static bool first = true;
  if( first ) {
    {
      // We must rebind this buffer before setting data, since
      // merely rebinding the vertex array object (although it
      // remembers the buffer ID) does not actually rebind the
      // buffer.
      glBindBuffer( GL_ARRAY_BUFFER,
                    gl_objects->vertex_buffer_object );
      glBufferData( GL_ARRAY_BUFFER,
                    sizeof( float ) * array_size, vertices,
                    mode );
      glBindBuffer( GL_ARRAY_BUFFER, 0 );
    }
    check_gl_errors();
    first = false;
  } else {
    // glBindBuffer( GL_ARRAY_BUFFER,
    //               gl_objects->vertex_buffer_object );
    // glBufferSubData( GL_ARRAY_BUFFER, (GLintptr)0,
    //                  sizeof( float ) * array_size, vertices );
    // glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }

  // for( int i = 0; i < 10; ++i ) {
  //   long offset_bytes  = i * kVertexSizeBytes;
  //   long offset_floats = i * kVertexSizeFloats;
  //   long num_vertices  = 10000;
  //   glBufferSubData( GL_ARRAY_BUFFER, offset_bytes,
  //                    kVertexSizeBytes * num_vertices,
  //                    vertices + offset_floats );
  // }

  // glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * array_size,
  //               vertices, GL_STATIC_DRAW );
  glDrawArrays( GL_TRIANGLES, 0, num_vertices );
  check_gl_errors();
}

#if 0
void draw_sprite( OpenGLObjects* gl_objects,
                  Delta const& screen_delta, int scale,
                  Coord const& coord ) {
  float sheet_w = 256.0;
  float sheet_h = 192.0;

  float tx_ox = 0.0 / sheet_w;
  float tx_oy = 32.0 * 4 / sheet_h;
  float tx_dx = 32.0 / sheet_w;
  float tx_dy = 32.0 / sheet_h;

  float z  = 0.0;
  float sf = float( scale );

  // clang-format off
  float vertices[] = {
    // Coord                                             Tx Coords
    float(coord.x._),    float(coord.y._),    z,   tx_ox,       tx_oy,
    float(coord.x._)+sf, float(coord.y._),    z,   tx_ox+tx_dx, tx_oy,
    float(coord.x._),    float(coord.y._)+sf, z,   tx_ox,       tx_oy+tx_dy,

    float(coord.x._)+sf, float(coord.y._),    z,   tx_ox+tx_dx, tx_oy,
    float(coord.x._)+sf, float(coord.y._)+sf, z,   tx_ox+tx_dx, tx_oy+tx_dy,
    float(coord.x._),    float(coord.y._)+sf, z,   tx_ox,       tx_oy+tx_dy,
  };
  // clang-format on

  constexpr size_t num_columns = kVertexSizeFloats;
  constexpr size_t num_rows    = 6;

  draw_vertices( gl_objects, screen_delta, vertices,
                 num_columns * num_rows, num_rows );
}

void draw_sprites_separate( OpenGLObjects* gl_objects,
                            Delta const&   screen_delta ) {
  glBindVertexArray( gl_objects->vertex_array_object );

  glBindBuffer( GL_ARRAY_BUFFER,
                gl_objects->vertex_buffer_object );
  glUseProgram( gl_objects->shader_program );
  glBindTexture( GL_TEXTURE_2D, gl_objects->opengl_texture );

  glUniform2f( gl_objects->screen_size_location,
               float( screen_delta.w._ ),
               float( screen_delta.h._ ) );
  glUniform2f( gl_objects->tick_location, 0, 0 );

  auto rect  = Rect::from( {}, screen_delta );
  int  scale = kSpriteScale;
  for( auto coord : rect.to_grid_noalign( Scale{ scale } ) )
    draw_sprite( gl_objects, screen_delta, scale, coord );
}
#endif

int draw_sprites_batched( OpenGLObjects* gl_objects,
                          Delta const&   screen_delta ) {
  int scale = kSpriteScale;

  float sheet_w = 256.0;
  float sheet_h = 192.0;

  float tx_ox = 0.0 / sheet_w;
  float tx_oy = 32.0 * 4 / sheet_h;
  float tx_dx = 32.0 / sheet_w;
  float tx_dy = 32.0 / sheet_h;

  constexpr size_t num_columns = kVertexSizeFloats;
  static size_t    num_rows    = 0;

  // Important: should only allocate this once, since allocating
  // a large buffer is apparently expensive.
  static vector<float> s_vertices = [&] {
    vector<float> res;
    int num_sprites = ( screen_delta.w._ + scale ) / scale *
                      ( screen_delta.h._ + scale ) / scale;
    int num_floats = num_sprites * 6 * num_columns;
    if( int( res.size() ) < num_floats )
      res.resize( num_floats );

    auto rect = Rect::from( {}, screen_delta );
    int  i    = 0;
    W    w{ scale };
    H    h{ scale };
    for( auto coord : rect.to_grid_noalign( Scale{ scale } ) ) {
      auto add_vertex = [&]( Coord const& c, float tx_x,
                             float tx_y ) {
        ++num_rows;
        // Coords
        res[i++] = float( c.x._ );
        res[i++] = float( c.y._ );
        res[i++] = 1.0f; // z
        // Texture coords
        res[i++] = tx_x;
        res[i++] = tx_y;
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

  draw_vertices( gl_objects, screen_delta, s_vertices.data(),
                 num_columns * num_rows, num_rows );
  return s_vertices.size() *
         sizeof( decltype( s_vertices )::value_type );
}

OpenGLObjects init_opengl() {
  fs::path shaders = "src/shaders";

  OpenGLObjects gl_objects;

  gl_objects.shader_program = load_shader_pgrm(
      shaders / "perf-test.vert", shaders / "perf-test.frag" );

  gl_objects.screen_size_location = glGetUniformLocation(
      gl_objects.shader_program, "screen_size" );
  gl_objects.tick_location =
      glGetUniformLocation( gl_objects.shader_program, "tick" );
  gl_objects.tx_location =
      glGetUniformLocation( gl_objects.shader_program, "tx" );

  gl_objects.opengl_texture =
      load_texture( "assets/art/tiles/world.png" );

  glGenVertexArrays( 1, &gl_objects.vertex_array_object );
  glGenBuffers( 1, &gl_objects.vertex_buffer_object );

  {
    glBindVertexArray( gl_objects.vertex_array_object );

    glBindBuffer( GL_ARRAY_BUFFER,
                  gl_objects.vertex_buffer_object );

    // Describe to OpenGL how to interpret the bytes in our ver-
    // tices array for feeding into the vertex shader.
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE,
                           kVertexSizeBytes, (void*)0 );
    glEnableVertexAttribArray( 0 );

    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE,
                           kVertexSizeBytes,
                           (void*)( sizeof( float ) * 3 ) );
    glEnableVertexAttribArray( 1 );

    // You can unbind the VAO afterwards so other VAO calls won't
    // accidentally modify this VAO, but this rarely happens.
    // Modifying other VAOs requires a call to glBindVertexArray
    // anyways so we generally don't unbind VAOs (nor VBOs) when
    // it's not directly necessary.
    glBindVertexArray( 0 );
  }

  // Unbind. The call to glVertexAttribPointer registered VBO as
  // the vertex attribute's bound vertex buffer object so after-
  // wards we can safely unbind.
  glBindBuffer( GL_ARRAY_BUFFER, 0 );

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

  check_gl_errors();

  // These next lines are needed on macOS to get the window to
  // appear (???).
  ::SDL_PumpEvents();
  ::SDL_DisplayMode display_mode;
  ::SDL_GetWindowDisplayMode( window, &display_mode );
  ::SDL_SetWindowDisplayMode( window, &display_mode );

  int max_texture_size = 0;
  glGetIntegerv( GL_MAX_TEXTURE_SIZE, &max_texture_size );

  lg.info( "OpenGL loaded:" );
  lg.info( "  * Vendor:      {}.", glGetString( GL_VENDOR ) );
  lg.info( "  * Renderer:    {}.", glGetString( GL_RENDERER ) );
  lg.info( "  * Version:     {}.", glGetString( GL_VERSION ) );
  lg.info( "  * Max Tx Size: {}x{}.", max_texture_size,
           max_texture_size );

  // glEnable( GL_DEPTH_TEST );
  // glDepthFunc( GL_LEQUAL );

  // Without this, alpha blending won't happen.
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glEnable( GL_BLEND );

  int viewport_scale = 1;

#ifdef __APPLE__
  // Ideally need to check if we are >= OSX 10.15 and set this to
  // two.
  viewport_scale = 2;
#endif

  // (0,0) is the lower-left of the rendering region. NOTE: This
  // needs to be re-called when window is resized.
  glViewport( 0, 0, win_size.w._ * viewport_scale,
              win_size.h._ * viewport_scale );

  bool wait_for_vsync = false;

  CHECK( !::SDL_GL_SetSwapInterval( wait_for_vsync ? 1 : 0 ),
         "setting swap interval is not supported." );

  // == Initialization ==========================================

  OpenGLObjects gl_objects = init_opengl();
  check_gl_errors();

  // == Render ==================================================

  auto screen_delta = main_window_logical_size();

  glClearColor( 0.2, 0.3, 0.3, 1.0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  int scale       = kSpriteScale;
  int num_sprites = ( screen_delta.w._ + scale ) / scale *
                    ( screen_delta.h._ + scale ) / scale;

  auto draw_func = draw_sprites_batched;
  // auto draw_func = draw_sprites_separate;

  glBindTexture( GL_TEXTURE_2D, gl_objects.opengl_texture );

  glUseProgram( gl_objects.shader_program );
  glUniform2f( gl_objects.screen_size_location,
               float( screen_delta.w._ ),
               float( screen_delta.h._ ) );
  glUniform2f( gl_objects.tick_location, 0, 0 );
  glUniform1i( gl_objects.tx_location, 0 );

  glBindVertexArray( gl_objects.vertex_array_object );
  int buf_size = draw_func( &gl_objects, screen_delta );
  check_gl_errors();
  ::SDL_GL_SwapWindow( window );

  auto start_time = chrono::system_clock::now();
  long frames     = 0;

  glBindVertexArray( 0 );

  // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

  while( !input::is_q_down() ) {
    // Clear screen.
    glClearColor( 0, 0, 0, 1.0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Set program and uniforms.
    glUseProgram( gl_objects.shader_program );
    glUniform2f( gl_objects.tick_location, frames, frames );
    check_gl_errors();

    // Bind the VAO and draw.
    glBindVertexArray( gl_objects.vertex_array_object );
    draw_func( &gl_objects, screen_delta );
    check_gl_errors();
    ::SDL_GL_SwapWindow( window );

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

  glDeleteTextures( 1, &gl_objects.opengl_texture );
  glDeleteVertexArrays( 1, &gl_objects.vertex_array_object );
  glDeleteBuffers( 1, &gl_objects.vertex_buffer_object );
  glDeleteProgram( gl_objects.shader_program );

  ::SDL_GL_DeleteContext( opengl_context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
