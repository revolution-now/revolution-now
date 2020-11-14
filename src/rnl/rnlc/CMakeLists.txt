# We need threading according to cpp-peglib docs.
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

file( GLOB sources "[a-zA-Z0-9]*.cpp" )

add_executable( rnlc ${sources} )

target_compile_features( rnlc PUBLIC cxx_std_20 )
set_target_properties( rnlc PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rnlc )

target_link_libraries(
    rnlc
    PRIVATE
    absl::strings
    base-util
    cpp-peglib::cpp-peglib
    fmt::fmt
    rn-base
)

target_include_directories(
    rnlc
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

clang_tidy( rnlc )
