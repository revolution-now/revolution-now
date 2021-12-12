# === this directory ==============================================

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-stb
  STATIC
  ${sources}
)

target_compile_features( rn-stb PUBLIC cxx_std_20 )
set_target_properties( rn-stb PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-stb )

target_link_libraries(
  rn-stb
  PRIVATE
  rn-gfx
  rn-base
  stb::stb
)

target_include_directories(
  rn-stb
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-stb )
