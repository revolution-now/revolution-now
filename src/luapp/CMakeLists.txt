# === this directory ==============================================

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-luapp
  STATIC
  ${sources}
)

target_compile_features( rn-luapp PUBLIC cxx_std_20 )
set_target_properties( rn-luapp PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-luapp )

target_link_libraries(
  rn-luapp
  PRIVATE
  lua::lua
  rn-base
)

target_include_directories(
  rn-luapp
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-luapp )
