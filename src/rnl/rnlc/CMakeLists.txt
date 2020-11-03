# We need threading according to cpp-peglib docs.
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

add_executable( rnlc rnlc.cpp )

target_compile_features( rnlc PUBLIC cxx_std_20 )
set_target_properties( rnlc PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rnlc )

target_link_libraries(
    rnlc
    PRIVATE
    cpp-peglib::cpp-peglib
)

clang_tidy( rnlc )
