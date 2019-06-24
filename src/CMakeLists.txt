find_sdl2_lib( SDL2       )
find_sdl2_lib( SDL2_ttf   )
find_sdl2_lib( SDL2_image )
find_sdl2_lib( SDL2_mixer )

# We need threading.
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

file( GLOB sources "[a-zA-Z]*.cpp" )
add_executable( rn ${sources} )
target_compile_features( rn PUBLIC cxx_std_20 )
set_target_properties( rn PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn )
target_link_libraries(
    rn
    PRIVATE
    absl::container
    absl::flat_hash_map
    absl::flat_hash_set
    absl::hash
    absl::strings
    base-util
    better-enums::better-enums
    expected-lite
    fmt::fmt
    function_ref
    midifile
    observer-ptr::observer-ptr
    range-v3
    rtmidi
    scelta::headers
    spdlog::spdlog
    Threads::Threads
    ucl
    ${SDL2_LIBRARY}
    ${SDL2_TTF_LIBRARY}
    ${SDL2_IMAGE_LIBRARY}
    ${SDL2_MIXER_LIBRARY} )
target_include_directories(
    rn
    PRIVATE
    ${PROJECT_SOURCE_DIR} # top-level folder
    ${SDL2_INCLUDE_DIR}
    ${SDL2_TTF_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR} )

# The `midi-convert` target will convert (incrementally) any .rg
# (Rosegarden) files to MIDI.  The `ogg-convert` will render the
# midi to OGG.
add_dependencies( rn midi-convert )
add_dependencies( rn ogg-convert )

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
  SCELTA_SUPPORT_VARIANT_BOOST_DISABLE
  SCELTA_SUPPORT_OPTIONAL_BOOST_DISABLE
  )

clang_tidy( rn )
