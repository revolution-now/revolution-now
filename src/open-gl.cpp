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
#include "logging.hpp"
#include "screen.hpp"

// SDL
#include "SDL.h"

// OpenGL
#define GL3_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

using namespace std;

namespace rn {

namespace {

char const* vertex_shader_source = R"(
  #version 330 core
  layout (location = 0) in vec3 aPos;

  void main() {
    gl_Position = vec4( aPos.x, aPos.y, aPos.z, 1.0 );
  }
)";

char const* fragment_shader_source = R"(
  #version 330 core
  out vec4 FragColor;

  void main() {
    FragColor = vec4( 1.0f, 0.5f, 0.2f, 1.0f );
  }
)";

void render_triangle( ::SDL_Window* window ) {
  int              success;
  constexpr size_t error_length = 512;
  char             errors[error_length];

  // == Vertex Shader ===========================================

  unsigned int vertex_shader;
  vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( vertex_shader, 1, &vertex_shader_source,
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

  unsigned int fragment_shader;
  fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fragment_shader, 1, &fragment_shader_source,
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

  unsigned int shader_program;
  shader_program = glCreateProgram();

  glAttachShader( shader_program, vertex_shader );
  glAttachShader( shader_program, fragment_shader );
  glLinkProgram( shader_program );
  // Check for linking errors.
  glGetProgramiv( shader_program, GL_LINK_STATUS, &success );
  if( !success ) {
    glGetProgramInfoLog( shader_program, error_length, NULL,
                         errors );
    FATAL( "Fragment shader linking failed: {}", errors );
  }

  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  // == Vertex Array Object =====================================

  float vertices[] = {
      -0.5f, -0.5f, 0.0f, //
      0.5f,  -0.5f, 0.0f, //
      0.0f,  0.5f,  0.0f  //
  };

  unsigned int vertex_array_object, vertex_buffer_object;
  glGenVertexArrays( 1, &vertex_array_object );
  glGenBuffers( 1, &vertex_buffer_object );

  glBindVertexArray( vertex_array_object );

  glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_object );
  glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices,
                GL_STATIC_DRAW );
  // Describe to OpenGL how to interpret the bytes in our ver-
  // tices array for feeding into the vertex shader.
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE,
                         3 * sizeof( float ), (void*)0 );
  glEnableVertexAttribArray( 0 );

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

  // == Render ==================================================

  glClearColor( 0.2, 0.3, 0.3, 1.0 );
  glClear( GL_COLOR_BUFFER_BIT );
  glUseProgram( shader_program );
  glBindVertexArray( vertex_array_object );
  glDrawArrays( GL_TRIANGLES, 0, 3 );
  glBindVertexArray( 0 );

  // == Present =================================================

  ::SDL_GL_SwapWindow( window );
  ::SDL_Delay( 3000 );

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
  auto flags = ::SDL_WINDOW_SHOWN | ::SDL_WINDOW_OPENGL;

  ::SDL_Window* window = ::SDL_CreateWindow(
      "OpenGL Test", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, 512, 512, flags );

  ::SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
  ::SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );

  /* Turn on double buffering with a 24bit Z buffer.
   * You may need to change this to 16 or 32 for your system */
  ::SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  ::SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

  /* Create our opengl context and attach it to our window */
  ::SDL_GLContext opengl_context =
      ::SDL_GL_CreateContext( window );
  CHECK( opengl_context );

  lg.info( "OpenGL loaded:" );
  lg.info( "  * Vendor:   {}.", glGetString( GL_VENDOR ) );
  lg.info( "  * Renderer: {}.", glGetString( GL_RENDERER ) );
  lg.info( "  * Version:  {}.", glGetString( GL_VERSION ) );

  /* This makes our buffer swap syncronized with the monitor's
   * vertical refresh */
  ::SDL_GL_SetSwapInterval( 1 );

  // Apparently needs to change when window is resized.
  glViewport( 0, 0, 512, 512 );

  render_triangle( window );

  /* Delete our opengl context, destroy our window, and shutdown
   * SDL */
  ::SDL_GL_DeleteContext( opengl_context );
}

} // namespace rn
