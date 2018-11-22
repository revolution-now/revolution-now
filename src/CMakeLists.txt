find_sdl2_lib( SDL2       )
find_sdl2_lib( SDL2TTF    )
find_sdl2_lib( SDL2_image )
find_sdl2_lib( SDL2_mixer )

add_executable( rn
    cargo.cpp
    fonts.cpp
    globals.cpp
    id.cpp
    loops.cpp
    main.cpp
    movement.cpp
    mv-points.cpp
    nation.cpp
    ownership.cpp
    physics.cpp
    render.cpp
    sdl-util.cpp
    sound.cpp
    tiles.cpp
    turn.cpp
    unit.cpp
    util.cpp
    viewport.cpp
    window.cpp
    world.cpp
)

target_compile_features( rn PUBLIC cxx_std_17 )
set_target_properties( rn PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn )
target_link_libraries( rn PRIVATE
    base-util
    ${SDL2_LIBRARY}
    ${SDL2TTF_LIBRARY}
    ${SDL2_IMAGE_LIBRARY}
    ${SDL2_MIXER_LIBRARY}
)
target_include_directories( rn PRIVATE
    ${SDL2_INCLUDE_DIR}
    ${SDL2TTF_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR}
)
tidy( rn )
