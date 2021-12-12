# === this directory ==============================================

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-gfx
  STATIC
  ${sources}
)

target_compile_features( rn-gfx PUBLIC cxx_std_20 )
set_target_properties( rn-gfx PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-gfx )

target_link_libraries(
  rn-gfx
  PRIVATE
  rn-base
  rn-rcl
)

target_include_directories(
  rn-gfx
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-gfx )
