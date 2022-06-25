/****************************************************************
**sdl-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Utilities for working with SDL.
*
*****************************************************************/
#include "sdl-util.hpp"

// Revolution Now
#include "error.hpp"
#include "init.hpp"
#include "logger.hpp"

// config
#include "config/gfx.rds.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

void check_SDL_compile_link_version(
    string_view module_name, ::SDL_version const& link_version,
    ::SDL_version const& compiled_version ) {
  lg.info( "SDL {}: compiled with version: {}.{}.{}",
           module_name, compiled_version.major,
           compiled_version.minor, compiled_version.patch );
  lg.info( "SDL {}:  running with version: {}.{}.{}",
           module_name, link_version.major, link_version.minor,
           link_version.patch );
  CHECK( compiled_version.major == link_version.major,
         "This game was compiled with a version of SDL {} whose "
         "major version number ({}) is different from the major "
         "version number of the runtime library ({})",
         module_name, compiled_version.major,
         link_version.major );

  if( compiled_version.minor != link_version.minor ) {
    lg.warn(
        "This game was compiled with a version of SDL {} whose "
        "minor version number ({}) is different from the minor "
        "version number of the runtime library ({})",
        module_name, compiled_version.minor,
        link_version.minor );
  }
}

::SDL_Rect to_SDL( Rect const& rect ) {
  ::SDL_Rect res;
  res.x = rect.x;
  res.y = rect.y;
  res.w = rect.w;
  res.h = rect.h;
  return res;
}

::SDL_Point to_SDL( Coord const& coord ) {
  ::SDL_Point p;
  p.x = coord.x;
  p.y = coord.y;
  return p;
}

Coord from_SDL( ::SDL_Point const& p ) {
  Coord res;
  res.x = p.x;
  res.y = p.y;
  return res;
}

Rect from_SDL( ::SDL_Rect const& rect ) {
  Rect res;
  res.x = rect.x;
  res.y = rect.y;
  res.w = rect.w;
  res.h = rect.h;
  return res;
}

void init_sdl() {
  ::SDL_version compiled, linked;
  SDL_VERSION( &compiled );
  ::SDL_GetVersion( &linked );
  check_SDL_compile_link_version( "base", linked, compiled );

  CHECK( compiled.major == 2,
         "This program must be run with SDL version 2." );
  // See:
  // https://discourse.libsdl.org/t/a-couple-of-questions-regarding-batching-in-sdl-2-0-10/26453/3
  if( linked.minor == 0 && linked.patch < 10 )
    lg.warn(
        "SDL versions prior to 2.0.10 may not have GPU batching "
        "support; game may run slow." );

  CHECK( ::SDL_Init( SDL_INIT_EVERYTHING ) >= 0,
         "sdl could not initialize" );
  ::SDL_ShowCursor( SDL_DISABLE );
}

void cleanup_sdl() { ::SDL_Quit(); }

//
//
//
REGISTER_INIT_ROUTINE( sdl );

::SDL_Color color_from_pixel( SDL_PixelFormat* fmt,
                              uint32_t         pixel ) {
  CHECK( fmt->BitsPerPixel == 32, "bits per pixel: {}",
         fmt->BitsPerPixel );
  ::SDL_Color color{};

  /* Get Red component */
  auto temp = pixel & fmt->Rmask;  /* Isolate red component */
  temp      = temp >> fmt->Rshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Rloss; /* Expand to a full 8-bit number */
  color.r = (Uint8)temp;

  /* Get Green component */
  temp = pixel & fmt->Gmask;  /* Isolate green component */
  temp = temp >> fmt->Gshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Gloss;  /* Expand to a full 8-bit number */
  color.g = (Uint8)temp;

  /* Get Blue component */
  temp = pixel & fmt->Bmask;  /* Isolate blue component */
  temp = temp >> fmt->Bshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Bloss;  /* Expand to a full 8-bit number */
  color.b = (Uint8)temp;

  /* Get Alpha component */
  temp = pixel & fmt->Amask;  /* Isolate alpha component */
  temp = temp >> fmt->Ashift; /* Shift it down to 8-bit */
  temp = temp << fmt->Aloss;  /* Expand to a full 8-bit number */
  color.a = (Uint8)temp;

  return color;
}

::SDL_Color to_SDL( gfx::pixel color ) {
  return { color.r, color.g, color.b, color.a };
}

gfx::pixel from_SDL( ::SDL_Color color ) {
  return { color.r, color.g, color.b, color.a };
}

/****************************************************************
** OpenGL Specific.
*****************************************************************/
::SDL_GLContext init_SDL_for_OpenGL( ::SDL_Window* window ) {
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

  bool wait_for_vsync = config_gfx.wait_for_vsync;

  if( ::SDL_GL_SetSwapInterval( wait_for_vsync ? 1 : 0 ) != 0 )
    lg.warn( "setting swap interval is not supported." );
  return opengl_context;
}

void sdl_gl_swap_window( ::SDL_Window* window ) {
  ::SDL_GL_SwapWindow( window );
}

void close_SDL_for_OpenGL( ::SDL_GLContext context ) {
  ::SDL_GL_DeleteContext( context );
  ::SDL_GL_UnloadLibrary();
}

} // namespace rn
