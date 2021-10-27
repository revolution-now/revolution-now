# === this directory ==============================================

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-gl
  STATIC
  ${sources}
)

target_compile_features( rn-gl PUBLIC cxx_std_20 )
set_target_properties( rn-gl PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-gl )

target_link_libraries(
  rn-gl
  PRIVATE
  glad::glad
  rn-base
)

target_include_directories(
  rn-gl
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  # This is so that we can include stuff from other folders, such
  # as "base/xxx.hpp".
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-gl )
