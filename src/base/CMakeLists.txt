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
)

target_include_directories(
    rn-base
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

clang_tidy( rn-base )
