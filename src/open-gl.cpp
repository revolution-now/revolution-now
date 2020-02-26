/****************************************************************
**open-gl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-09.
*
* Description: OpenGL rendering backend.
*
*****************************************************************/
#include "open-gl.hpp"

// Revolution Now
#include "errors.hpp"
#include "input.hpp"
#include "io.hpp"
#include "logging.hpp"
#include "screen.hpp"
#include "tx.hpp"

// SDL
#include "SDL.h"

// GLAD (OpenGL Loader)
#include "glad/glad.h"

using namespace std;

namespace rn {

namespace {

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
  ::SDL_Surface* surface = ( ::SDL_Surface*)img.get();
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
  constexpr size_t error_length = 512;
  char             errors[error_length];

  // == Vertex Shader ===========================================

  GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  ASSIGN_CHECK_XP( vertex_shader_source,
                   read_file_as_string( vert ) );
  char const* p_vertex_shader_source =
      vertex_shader_source.c_str();
  glShaderSource( vertex_shader, 1, &p_vertex_shader_source,
                  nullptr );
  glCompileShader( vertex_shader ); // check errors?
  // Check for compiler errors.
  glGetShaderiv( vertex_shader, GL_COMPILE_STATUS, &success );
  if( !success ) {
    glGetShaderInfoLog( vertex_shader, error_length, NULL,
                        errors );
    FATAL( "Vertex shader compilation failed: {}", errors );
  }

  // == Fragment Shader =========================================

  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  ASSIGN_CHECK_XP( fragment_shader_source,
                   read_file_as_string( frag ) );
  char const* p_fragment_shader_source =
      fragment_shader_source.c_str();
  glShaderSource( fragment_shader, 1, &p_fragment_shader_source,
                  nullptr );
  glCompileShader( fragment_shader ); // check errors?
  // Check for compiler errors.
  glGetShaderiv( fragment_shader, GL_COMPILE_STATUS, &success );
  if( !success ) {
    glGetShaderInfoLog( fragment_shader, error_length, NULL,
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
    glGetProgramInfoLog( shader_program, error_length, NULL,
                         errors );
    FATAL( "Shader program linking failed: {}", errors );
  }

  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  return shader_program;
}

struct OpenGLObjects {
  GLuint shader_program;
  GLuint screen_size_location;
  GLuint vertex_array_object;
  GLuint vertex_buffer_object;
  GLuint opengl_texture;
};

void draw_sprite( OpenGLObjects* gl_objects,
                  Delta const&   screen_delta,
                  Coord const&   coord ) {
  float sheet_w = 256.0;
  float sheet_h = 192.0;

  float tx_ox = 0.0 / sheet_w;
  float tx_oy = 32.0 * 4 / sheet_h;
  float tx_dx = 32.0 / sheet_w;
  float tx_dy = 32.0 / sheet_h;

  float z = 0.0;

  // clang-format off
  float vertices[] = {
    // Coord                                             Tx Coords
    float(coord.x._),       float(coord.y._),       z,   tx_ox,       tx_oy,
    float(coord.x._)+32.0f, float(coord.y._),       z,   tx_ox+tx_dx, tx_oy,
    float(coord.x._),       float(coord.y._)+32.0f, z,   tx_ox,       tx_oy+tx_dy,

    float(coord.x._)+32.0f, float(coord.y._),       z,   tx_ox+tx_dx, tx_oy,
    float(coord.x._)+32.0f, float(coord.y._)+32.0f, z,   tx_ox+tx_dx, tx_oy+tx_dy,
    float(coord.x._),       float(coord.y._)+32.0f, z,   tx_ox,       tx_oy+tx_dy,
  };
  // clang-format on

  constexpr size_t num_columns = 5;

  glGenVertexArrays( 1, &gl_objects->vertex_array_object );
  glGenBuffers( 1, &gl_objects->vertex_buffer_object );

  glBindVertexArray( gl_objects->vertex_array_object );

  glBindBuffer( GL_ARRAY_BUFFER,
                gl_objects->vertex_buffer_object );
  glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices,
                GL_STATIC_DRAW );
  // Describe to OpenGL how to interpret the bytes in our ver-
  // tices array for feeding into the vertex shader.
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE,
                         num_columns * sizeof( float ),
                         (void*)0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE,
                         num_columns * sizeof( float ),
                         (void*)( sizeof( float ) * 3 ) );
  glEnableVertexAttribArray( 1 );

  // Unbind. The call to glVertexAttribPointer registered VBO as
  // the vertex attribute's bound vertex buffer object so after-
  // wards we can safely unbind.
  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  // You can unbind the VAO afterwards so other VAO calls won't
  // accidentally modify this VAO, but this rarely happens. Modi-
  // fying other VAOs requires a call to glBindVertexArray any-
  // ways so we generally don't unbind VAOs (nor VBOs) when it's
  // not directly necessary.
  glBindVertexArray( 0 );

  // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  glClearColor( 0.2, 0.3, 0.3, 1.0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glUseProgram( gl_objects->shader_program );
  glUniform2f( gl_objects->screen_size_location,
               float( screen_delta.w._ ),
               float( screen_delta.h._ ) );
  glBindTexture( GL_TEXTURE_2D, gl_objects->opengl_texture );
  glBindVertexArray( gl_objects->vertex_array_object );
  size_t num_rows = sizeof( vertices ) / num_columns;
  glDrawArrays( GL_TRIANGLES, 0, num_rows );
  glBindVertexArray( 0 );
}

OpenGLObjects init_opengl() {
  fs::path shaders = "src/shaders";

  OpenGLObjects res;

  res.shader_program =
      load_shader_pgrm( shaders / "experimental.vert",
                        shaders / "experimental.frag" );

  res.screen_size_location =
      glGetUniformLocation( res.shader_program, "screen_size" );

  res.opengl_texture =
      load_texture( "assets/art/tiles/world.png" );

  return res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/

/****************************************************************
** Testing
*****************************************************************/
void test_open_gl() {
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

  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LEQUAL );

  // Without this, alpha blending won't happen.
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glEnable( GL_BLEND );

  /* This makes our buffer swap syncronized with the monitor's
   * vertical refresh */
  ::SDL_GL_SetSwapInterval( 1 );

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

  // Disable wait-for-vsync (FIXME: only for testing).
  CHECK( !::SDL_GL_SetSwapInterval( 0 ),
         "setting swap interval is not supported." );

  // == Render Some Stuff =======================================

  OpenGLObjects gl_objects = init_opengl();
  check_gl_errors();

  // == Render ==================================================

  auto screen_delta = main_window_logical_size();

  long frames = 0;

  auto start_time = Clock_t::now();
  while( !input::is_q_down() ) {
    draw_sprite( &gl_objects, screen_delta, { 0_x, 100_y } );
    ::SDL_GL_SwapWindow( window );
    ++frames;
    //::SDL_Delay( 100 );
  }
  auto end_time   = Clock_t::now();
  auto delta_time = end_time - start_time;
  lg.info( "Total time: {}.", delta_time );
  auto secs =
      chrono::duration_cast<chrono::seconds>( delta_time );
  lg.info( "Average frame rate: {}.",
           frames / double( secs.count() ) );

  // == Cleanup =================================================

  glDeleteTextures( 1, &gl_objects.opengl_texture );
  glDeleteVertexArrays( 1, &gl_objects.vertex_array_object );
  glDeleteBuffers( 1, &gl_objects.vertex_buffer_object );
  glDeleteProgram( gl_objects.shader_program );

  ::SDL_GL_DeleteContext( opengl_context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
