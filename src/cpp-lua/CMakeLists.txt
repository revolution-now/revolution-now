# === this directory ==============================================

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  cpp-lua
  STATIC
  ${sources}
)

target_compile_features( cpp-lua PUBLIC cxx_std_20 )
set_target_properties( cpp-lua PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( cpp-lua )

target_link_libraries(
  cpp-lua
  PRIVATE
  lua::lua
  rn-base
)

target_include_directories(
  cpp-lua
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( cpp-lua )
