find_sdl2_lib( SDL2       )
find_sdl2_lib( SDL2_mixer )

# === subdirectories ==============================================

add_subdirectory( base )
add_subdirectory( rcl )
add_subdirectory( luapp )
add_subdirectory( rds )
add_subdirectory( gl )
add_subdirectory( mock ) # should not be depended on by `rn`.
add_subdirectory( stb )
add_subdirectory( gfx )
add_subdirectory( cdr )
add_subdirectory( refl )
add_subdirectory( render )
add_subdirectory( config )
add_subdirectory( ss )
add_subdirectory( sav )
add_subdirectory( video )
add_subdirectory( sfx )
add_subdirectory( sdl )

# === this directory ==============================================

# We need threading.
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

add_rn_library(
  rn
  # Dependencies
  base-util
  glad::glad # TODO: remove/isolate this.
  lua::lua
  midifile
  rn-base
  rn-cdr
  rn-config
  rn-gfx
  rn-gl
  rn-ss
  rn-rcl
  rn-luapp
  rn-refl
  rn-render
  rn-rds-util
  rn-sav
  rn-sdl
  rn-sfx
  rn-stb
  rn-video
  rtmidi
  Threads::Threads
  ${SDL2_LIBRARY}
  ${SDL2_MIXER_LIBRARY}
)

target_include_directories(
  rn
  AFTER # append to existing.
  PUBLIC
  ${SDL2_INCLUDE_DIR}
  ${SDL2_MIXER_INCLUDE_DIR}
)

if( DEBUG_TRACE )
  message( STATUS "Enabling trace-level logging" )
  target_compile_definitions( rn PUBLIC RN_TRACE=1 )
endif()

target_compile_definitions(
  rn
  PUBLIC
)
