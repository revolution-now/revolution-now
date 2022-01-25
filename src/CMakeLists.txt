# === subdirectories ==============================================

add_subdirectory( base )
add_subdirectory( rcl )
add_subdirectory( fb )
add_subdirectory( luapp )
add_subdirectory( rds )
add_subdirectory( gl )
add_subdirectory( mock ) # should not be depended on by `rn`.
add_subdirectory( stb )
add_subdirectory( gfx )
add_subdirectory( cdr )
add_subdirectory( refl )

# === this directory ==============================================

find_sdl2_lib( SDL2       )
find_sdl2_lib( SDL2_ttf   )
find_sdl2_lib( SDL2_image )
find_sdl2_lib( SDL2_mixer )

# We need threading.
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn
  STATIC
  ${sources}
)

target_compile_features( rn PUBLIC cxx_std_20 )
set_target_properties( rn PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn )

target_link_libraries(
    rn
    PUBLIC
    absl::function_ref
    absl::strings
    base-util
    flatbuffers
    glad::glad # TODO: remove/isolate this.
    lua::lua
    midifile
    rn-base
    rn-cdr
    rn-gfx
    rn-gl
    rn-rcl
    rn-luapp
    rn-refl
    rn-stb
    rtmidi
    Threads::Threads
    ${SDL2_LIBRARY}
    ${SDL2_TTF_LIBRARY}
    ${SDL2_IMAGE_LIBRARY}
    ${SDL2_MIXER_LIBRARY} )

add_dependencies( rn rn-flatbuf rn-rds )
get_target_property( FLATBUFF_INCLUDE_DIR rn-flatbuf INCLUDE_DIR )
get_target_property( RDS_INCLUDE_DIR rn-rds INCLUDE_DIR )

target_include_directories(
    rn
    PUBLIC
    ${PROJECT_SOURCE_DIR} # top-level folder
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FLATBUFF_INCLUDE_DIR}
    ${RDS_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${SDL2_TTF_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR} )

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

clang_tidy( rn )
