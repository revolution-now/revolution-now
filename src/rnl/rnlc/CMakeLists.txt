add_executable( rnlc rnlc.cpp )

target_compile_features( rnlc PUBLIC cxx_std_20 )
set_target_properties( rnlc PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rnlc )

target_link_libraries(
    rnlc
    #PRIVATE
    #peglib # ???
)

clang_tidy( rnlc )
