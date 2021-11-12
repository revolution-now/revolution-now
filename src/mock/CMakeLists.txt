# === this directory ==============================================

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-mock
  STATIC
  ${sources}
)

target_compile_features( rn-mock PUBLIC cxx_std_20 )
set_target_properties( rn-mock PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-mock )

target_link_libraries(
  rn-mock
  PRIVATE
  rn-base
)

target_include_directories(
  rn-mock
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-mock )
