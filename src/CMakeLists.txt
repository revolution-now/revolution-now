find_sdl2_lib( SDL2       )
find_sdl2_lib( SDL2_ttf   )
find_sdl2_lib( SDL2_image )
find_sdl2_lib( SDL2_mixer )

file( GLOB sources "[a-zA-Z]*.cpp" )
add_executable( rn ${sources} )
target_compile_features( rn PUBLIC cxx_std_20 )
set_target_properties( rn PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn )
target_link_libraries(
    rn
    PRIVATE
    absl::container
    absl::hash
    absl::strings
    base-util
    better-enums::better-enums
    fmt::fmt
    observer-ptr::observer-ptr
    range-v3
    scelta::headers
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
  if( CMAKE_SYSTEM_NAME STREQUAL Linux )
    find_library( DW dw )
    if( DW STREQUAL "DW-NOTFOUND" )
      message( WARNING "libdw not found; not linking to backward-cpp library" )
    else()
      message( STATUS "RN Found libdw." )
      target_link_libraries( rn PRIVATE dw )
      target_compile_definitions(
        rn
        PRIVATE
        BACKWARD_HAS_DW=1 )
    endif()
  endif()
  target_link_libraries( rn PRIVATE backward backward_object )
endif()

if( DEBUG_TRACE )
  message( STATUS "Enabling trace-level logging" )
  target_compile_definitions( rn PRIVATE RN_TRACE=1 )
endif()

target_compile_definitions(
  rn
  PRIVATE
  FMT_STRING_ALIAS=1
  BETTER_ENUMS_STRICT_CONVERSION=1
  )

clang_tidy( rn )
