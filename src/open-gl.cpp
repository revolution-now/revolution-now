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

void init_opengl( Delta const& screen_delta ) {
  fs::path shaders = "src/shaders";

  auto shader_program =
      load_shader_pgrm( shaders / "experimental.vert",
                        shaders / "experimental.frag" );

  // == Uniforms ================================================

  GLuint screen_size_location =
      glGetUniformLocation( shader_program, "screen_size" );

  // == Vertex Array Object =====================================

  float vertices[] = {
      // clang-format off
      // Coord              Color                     Tx Coords
      100,  100,  0.0,   1.0, 0.0, 0.0, 1.0,   0.0, 0.0,
       50,  200,  0.0,   1.0, 0.0, 0.0, 1.0,   0.0, 0.0,
      150,  200,  0.0,   1.0, 0.0, 0.0, 1.0,   0.0, 0.0,

      100,  150,  0.0,   0.0, 0.0, 0.0, 0.0,   0.5, 1.0,
       50,  250,  0.0,   0.0, 0.0, 0.0, 0.0,   1.0, 0.5,
      150,  250,  0.0,   0.0, 0.0, 0.0, 0.0,   0.0, 0.5,

      100,  200,  0.0,   0.0, 0.0, 1.0, 0.5,   0.0, 0.0,
       50,  300,  0.0,   0.0, 0.0, 1.0, 0.5,   0.0, 0.0,
      150,  300,  0.0,   0.0, 0.0, 1.0, 0.5,   0.0, 0.0,
      // clang-format on
  };

  size_t num_columns = 9;
  size_t num_rows    = 9;

  // Flip y coordinates of textures for OpenGL.
  for( size_t row = 0; row < num_rows; ++row ) {
    float& tx_y = vertices[row * num_columns + 8];
    tx_y        = 1.0f - tx_y;
  }

  GLuint vertex_array_object, vertex_buffer_object;
  glGenVertexArrays( 1, &vertex_array_object );
  glGenBuffers( 1, &vertex_buffer_object );

  glBindVertexArray( vertex_array_object );

  glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_object );
  glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices,
                GL_STATIC_DRAW );
  // Describe to OpenGL how to interpret the bytes in our ver-
  // tices array for feeding into the vertex shader.
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE,
                         num_columns * sizeof( float ),
                         (void*)0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE,
                         num_columns * sizeof( float ),
                         (void*)( sizeof( float ) * 3 ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE,
                         num_columns * sizeof( float ),
                         (void*)( sizeof( float ) * 7 ) );
  glEnableVertexAttribArray( 2 );

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

  // == Texture =================================================

  GLuint opengl_texture =
      load_texture( "assets/art/tiles/wood-128x64.png" );

  // == Render ==================================================

  // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  glClearColor( 0.2, 0.3, 0.3, 1.0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glUseProgram( shader_program );
  glUniform2f( screen_size_location, float( screen_delta.w._ ),
               float( screen_delta.h._ ) );
  glBindTexture( GL_TEXTURE_2D, opengl_texture );
  glBindVertexArray( vertex_array_object );
  glDrawArrays( GL_TRIANGLES, 0, num_rows );
  glBindVertexArray( 0 );

  // == Cleanup =================================================

  glDeleteVertexArrays( 1, &vertex_array_object );
  glDeleteBuffers( 1, &vertex_buffer_object );
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
  viewport_scale = 1;
#endif

  // (0,0) is the lower-left of the rendering region. NOTE: This
  // needs to be re-called when window is resized.
  glViewport( 0, 0, win_size.w._ * viewport_scale,
              win_size.h._ * viewport_scale );

  // == Render Some Stuff =======================================

  init_opengl( main_window_logical_size() );
  check_gl_errors();

  // == Present =================================================

  ::SDL_GL_SwapWindow( window );
  while( !input::is_any_key_down() ) { ::SDL_Delay( 100 ); }

  // == Cleanup =================================================

  /* Delete our opengl context, destroy our window, and shutdown
   * SDL */
  ::SDL_GL_DeleteContext( opengl_context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
