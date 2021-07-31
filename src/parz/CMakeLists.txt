# === this directory ==============================================

# We need threading.
file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-parz
  STATIC
  ${sources}
)

target_compile_features( rn-parz PUBLIC cxx_std_20 )
set_target_properties( rn-parz PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-parz )

target_link_libraries(
    rn-parz
    PUBLIC
    rn-base
)

target_include_directories(
    rn-parz
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-parz )
