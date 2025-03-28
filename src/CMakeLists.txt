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

# TODO: When the C++23 standard library is available, consider
# replacing the backward-cpp library with std::stacktrace.

# If this build contains debug symbols then link with the
# backward-cpp library which provides stack traces.
if( CMAKE_BUILD_TYPE STREQUAL Debug OR
    CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo )
  if( CMAKE_SYSTEM_NAME STREQUAL Linux )
    find_library( DW dw )
    if( DW STREQUAL "DW-NOTFOUND" )
      message( WARNING "libdw not found; not linking to backward-cpp library" )
    else()
      message( STATUS "RN Found libdw." )
      target_link_libraries( rn PUBLIC dw )
      target_compile_definitions(
        rn
        PUBLIC
        USE_BACKWARD=1
        BACKWARD_HAS_DW=1 )
    endif()
  endif()
  target_link_libraries( rn PUBLIC backward backward_object )
endif()

if( DEBUG_TRACE )
  message( STATUS "Enabling trace-level logging" )
  target_compile_definitions( rn PUBLIC RN_TRACE=1 )
endif()

target_compile_definitions(
  rn
  PUBLIC
)
