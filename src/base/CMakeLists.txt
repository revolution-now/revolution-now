# === this directory ==============================================

# We need threading.
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  rn-base
  STATIC
  ${sources}
)

target_compile_features( rn-base PUBLIC cxx_std_20 )
set_target_properties( rn-base PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rn-base )

target_link_libraries(
    rn-base
    PUBLIC
    base-util
    function-ref
)

target_include_directories(
    rn-base
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(
  rn-base
  PUBLIC
  # To use these defines, they must be surrounded in a TOSTRING
  # macro to stringify them. Also, they won't work if the path
  # has spaces in it.
  "RN_BUILD_OUTPUT_ROOT_DIR=${CMAKE_BINARY_DIR}"
  "RN_SOURCE_TREE_ROOT=${CMAKE_SOURCE_DIR}"
  )

clang_tidy( rn-base )
