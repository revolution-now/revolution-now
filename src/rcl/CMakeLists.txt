# === this directory ==============================================

# We need threading.
file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-rcl
  STATIC
  ${sources}
)

target_compile_features( rn-rcl PUBLIC cxx_std_20 )
set_target_properties( rn-rcl PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-rcl )

target_link_libraries(
    rn-rcl
    PUBLIC
    absl::strings
    rn-base
    rn-parz
)

target_include_directories(
    rn-rcl
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-rcl )
