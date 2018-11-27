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
    fmt::fmt
    spdlog::spdlog
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

clang_tidy( rn )
