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
add_subdirectory( gs )

# === this directory ==============================================

find_sdl2_lib( SDL2       )
find_sdl2_lib( SDL2_mixer )

# We need threading.
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

add_rn_library(
  rn
  # Dependencies
  absl::function_ref
  absl::strings
  base-util
  glad::glad # TODO: remove/isolate this.
  lua::lua
  midifile
  rn-base
  rn-cdr
  rn-config
  rn-gfx
  rn-gl
  rn-gs
  rn-rcl
  rn-luapp
  rn-refl
  rn-render
  rn-rds-util
  rn-stb
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

# If this is a debug build then link with the backward-cpp
# library which provides stack traces.
if( CMAKE_BUILD_TYPE STREQUAL Debug )
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
