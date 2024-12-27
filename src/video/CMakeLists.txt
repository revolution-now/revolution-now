add_rn_library(
  rn-video
  # Dependencies
  rn-gfx
  rn-base
  ${SDL2_LIBRARY}
  ${SDL2_MIXER_LIBRARY}
)

target_include_directories(
  rn-video
  PRIVATE
  ${SDL2_INCLUDE_DIR}
  ${SDL2_MIXER_INCLUDE_DIR}
)
