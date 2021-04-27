# === this directory ==============================================

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-cpp-lua
  STATIC
  ${sources}
)

target_compile_features( rn-cpp-lua PUBLIC cxx_std_20 )
set_target_properties( rn-cpp-lua PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-cpp-lua )

target_link_libraries(
  rn-cpp-lua
  PRIVATE
  lua::lua
  rn-base
)

target_include_directories(
  rn-cpp-lua
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-cpp-lua )
