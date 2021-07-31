# === this directory ==============================================

# We need threading.
file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-cl
  STATIC
  ${sources}
)

target_compile_features( rn-cl PUBLIC cxx_std_20 )
set_target_properties( rn-cl PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-cl )

target_link_libraries(
    rn-cl
    PUBLIC
    rn-base
    rn-parz
)

target_include_directories(
    rn-cl
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

clang_tidy( rn-cl )
