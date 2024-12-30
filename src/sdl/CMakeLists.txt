add_rn_library(
  rn-sdl
  # Dependencies
  rn-base
  ${SDL2_LIBRARY}
  ${SDL2_MIXER_LIBRARY}
)

target_include_directories(
  rn-sdl
  PUBLIC
  ${SDL2_INCLUDE_DIR}
  ${SDL2_MIXER_INCLUDE_DIR}
)
