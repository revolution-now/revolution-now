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

namespace {

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
  // This may be changed later.
  ::SDL_ShowCursor( SDL_DISABLE );
}

void cleanup_sdl() { ::SDL_Quit(); }

REGISTER_INIT_ROUTINE( sdl );

} // namespace

} // namespace rn
