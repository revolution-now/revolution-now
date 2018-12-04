find_sdl2_lib( SDL2       )
find_sdl2_lib( SDL2_ttf   )
find_sdl2_lib( SDL2_image )
find_sdl2_lib( SDL2_mixer )

file( GLOB sources "[a-zA-Z]*.cpp" )
add_executable( rn ${sources} )
target_compile_features( rn PUBLIC cxx_std_17 )
set_target_properties( rn PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn )
target_link_libraries(
    rn
    PRIVATE
    base-util
    absl::container
    absl::hash
    absl::strings
    fmt::fmt
    spdlog::spdlog
    ucl
    ${SDL2_LIBRARY}
    ${SDL2_TTF_LIBRARY}
    ${SDL2_IMAGE_LIBRARY}
    ${SDL2_MIXER_LIBRARY} )
target_include_directories(
    rn
    PRIVATE
    ${SDL2_INCLUDE_DIR}
    ${SDL2_TTF_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR} )

# If this is a debug build then link with the backward-cpp
# library which provides stack traces.
if( CMAKE_BUILD_TYPE STREQUAL Debug )
  find_library( DW dw )
  if( DW STREQUAL "DW-NOTFOUND" )
    message( WARNING "libdw not found; not linking to backward-cpp library" )
  else()
    target_link_libraries( rn PRIVATE dw )
    target_compile_definitions( rn PRIVATE BACKWARD_HAS_DW=1 )
  endif()
  target_link_libraries( rn PRIVATE backward backward_object )
endif()

clang_tidy( rn )
