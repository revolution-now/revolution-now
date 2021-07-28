file( GLOB sources "[a-zA-Z0-9]*.cpp" )

add_executable( rdsc ${sources} )

target_compile_features( rdsc PUBLIC cxx_std_20 )
set_target_properties( rdsc PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( rdsc )

target_link_libraries(
    rdsc
    PRIVATE
    absl::function_ref
    absl::strings
    base-util
    fmt::fmt
    rn-base
    rn-luapp
)

target_include_directories(
    rdsc
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

clang_tidy( rdsc )
