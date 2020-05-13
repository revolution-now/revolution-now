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

struct VertexData {
  float x = 0.0f;
  float y = 0.0f;
  union {
    float r = 0.0f;
    float x_tx;
  };
  union {
    float g = 0.0f;
    float y_tx;
  };
  float b = 0.0f;
  float a = 0.0f; // -1 means texture.
};
// Need to ensure this for proper data packing in array.
static_assert( sizeof( VertexData ) == sizeof( float ) * 6 );
// Seems sensible.
static_assert( sizeof( VertexData ) % 8 == 0 );

void draw_square_line( vector<VertexData>* vertices, Coord start,
                       Coord end, Color c ) {
  if( start.x > end.x ) std::swap( start.x, end.x );
  if( start.y > end.y ) std::swap( start.y, end.y );

  Delta d;
  if( start.y == end.y )
    d = Delta{ 0_w, 1_h };
  else if( start.x == end.x )
    d = Delta{ 1_w, 0_h };
  else {
    FATAL(
        "Only horizontal and vertical lines supported "
        "(start={}, "
        "end={}).",
        start, end );
  }

  auto push_coord = [&]( Coord const& coord ) {
    vertices->push_back( {
        .x = float( coord.x._ ), //
        .y = float( coord.y._ ), //
        .r = float( c.r ),       //
        .g = float( c.g ),       //
        .b = float( c.b ),       //
        .a = float( c.a ),       //
    } );
  };
  push_coord( start );
  push_coord( end );
  push_coord( end + d );
  push_coord( start );
  push_coord( start + d );
  push_coord( end + d );
}

void draw_box( vector<VertexData>* vertices, Coord corner,
               Coord opposite_corner, Color c ) {
  auto rect = Rect::from( corner, opposite_corner );
  draw_square_line( vertices, rect.upper_left(),
                    rect.upper_right(), c );
  draw_square_line( vertices, rect.upper_right(),
                    rect.lower_right() + 1_h, c );
  draw_square_line( vertices, rect.lower_left(),
                    rect.lower_right() + 1_w, c );
  draw_square_line( vertices, rect.upper_left(),
                    rect.lower_left(), c );
}

void draw_box_inside( vector<VertexData>* vertices,
                      Rect const& rect, Color c ) {
  if( rect.w == 0_w || rect.h == 0_h ) return;
  // Rect is expected to be normalized here.
  draw_box( vertices, rect.upper_left(),
            rect.lower_right() - 1_w - 1_h, c );
}

void draw_box_outside( vector<VertexData>* vertices,
                       Rect const& rect, Color c ) {
  auto upper_left  = rect.upper_left();
  auto lower_right = rect.lower_right();
  upper_left -= 1_w;
  upper_left -= 1_h;
  // if( rect.w > 0_w )
  //  lower_right += 1_w;
  // if( rect.h > 0_h )
  //  lower_right += 1_h;
  draw_box( vertices, upper_left, lower_right, c );
}

void draw_lines( OpenGLObjects* gl_objects,
                 Delta const&   screen_delta ) {
  glUniform2f( gl_objects->screen_size_location,
               float( screen_delta.w._ ),
               float( screen_delta.h._ ) );

  vector<VertexData> vertices;

  draw_box_outside( &vertices, { 100_x, 100_y, 0_w, 0_h },
                    Color::red() );
  draw_box_outside( &vertices, { 100_x, 120_y, 1_w, 0_h },
                    Color::red() );
  draw_box_outside( &vertices, { 100_x, 140_y, 0_w, 1_h },
                    Color::red() );
  draw_box_outside( &vertices, { 100_x, 160_y, 1_w, 1_h },
                    Color::red() );
  draw_box_outside( &vertices, { 100_x, 180_y, 2_w, 2_h },
                    Color::red() );
  draw_box_outside( &vertices, { 100_x, 200_y, 3_w, 3_h },
                    Color::red() );
  draw_box_outside( &vertices, { 100_x, 220_y, 4_w, 4_h },
                    Color::red() );
  draw_box_outside( &vertices, { 100_x, 240_y, 5_w, 5_h },
                    Color::red() );
  draw_box_outside( &vertices, { 200_x, 100_y, 50_w, 50_h },
                    Color::red() );

  draw_box_inside( &vertices, { 100_x, 100_y, 0_w, 0_h },
                   Color::white() );
  draw_box_inside( &vertices, { 100_x, 120_y, 1_w, 0_h },
                   Color::white() );
  draw_box_inside( &vertices, { 100_x, 140_y, 0_w, 1_h },
                   Color::white() );
  draw_box_inside( &vertices, { 100_x, 160_y, 1_w, 1_h },
                   Color::white() );
  draw_box_inside( &vertices, { 100_x, 180_y, 2_w, 2_h },
                   Color::white() );
  draw_box_inside( &vertices, { 100_x, 200_y, 3_w, 3_h },
                   Color::white() );
  draw_box_inside( &vertices, { 100_x, 220_y, 4_w, 4_h },
                   Color::white() );
  draw_box_inside( &vertices, { 100_x, 240_y, 5_w, 5_h },
                   Color::white() );
  draw_box_inside( &vertices, { 200_x, 100_y, 50_w, 50_h },
                   Color::white() );

  glBufferData( GL_ARRAY_BUFFER,
                sizeof( VertexData ) * vertices.size(),
                vertices.data(), GL_STATIC_DRAW );
  glDrawArrays( GL_TRIANGLES, 0, vertices.size() );
}

OpenGLObjects init_opengl() {
  fs::path shaders = "src/shaders";

  OpenGLObjects gl_objects;

  gl_objects.shader_program =
      load_shader_pgrm( shaders / "experimental.vert",
                        shaders / "experimental.frag" );

  gl_objects.screen_size_location = glGetUniformLocation(
      gl_objects.shader_program, "screen_size" );

  gl_objects.opengl_texture =
      load_texture( "assets/art/tiles/world.png" );

  glGenVertexArrays( 1, &gl_objects.vertex_array_object );
  glGenBuffers( 1, &gl_objects.vertex_buffer_object );

  glBindVertexArray( gl_objects.vertex_array_object );
  glBindBuffer( GL_ARRAY_BUFFER,
                gl_objects.vertex_buffer_object );

  int vtx_idx = 0;
  glVertexAttribPointer( vtx_idx, 2, GL_FLOAT, GL_FALSE,
                         sizeof( VertexData ), (void*)0 );
  glEnableVertexAttribArray( vtx_idx++ );
  glVertexAttribPointer( vtx_idx, 4, GL_FLOAT, GL_FALSE,
                         sizeof( VertexData ),
                         (void*)( 2 * sizeof( float ) ) );
  glEnableVertexAttribArray( vtx_idx++ );

  glUseProgram( gl_objects.shader_program );
  glBindTexture( GL_TEXTURE_2D, gl_objects.opengl_texture );
  return gl_objects;
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

  bool wait_for_vsync = true;

  CHECK( !::SDL_GL_SetSwapInterval( wait_for_vsync ? 1 : 0 ),
         "setting swap interval is not supported." );

  // == Initialization ==========================================

  OpenGLObjects gl_objects = init_opengl();
  check_gl_errors();

  // == Render ==================================================

  auto screen_delta = main_window_logical_size() / Scale{ 1 };

  check_gl_errors();

  while( !input::is_q_down() ) {
    glClearColor( 0.2, 0.3, 0.3, 1.0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    draw_lines( &gl_objects, screen_delta );
    ::SDL_GL_SwapWindow( window );
    ::SDL_Delay( 17 );
  }

  // == Cleanup =================================================

  glDeleteTextures( 1, &gl_objects.opengl_texture );
  glDeleteVertexArrays( 1, &gl_objects.vertex_array_object );
  glDeleteBuffers( 1, &gl_objects.vertex_buffer_object );
  glDeleteProgram( gl_objects.shader_program );

  ::SDL_GL_DeleteContext( opengl_context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
