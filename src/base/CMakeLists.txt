set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

add_rn_library(
  rn-base
  # Dependencies
  base-util
  fmt::fmt
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
