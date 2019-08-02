file( GLOB sources "[a-zA-Z]*.cpp" )
add_executable( unittest ${sources} )

set_warning_options( unittest )

target_link_libraries(
    unittest
    PRIVATE
    rn
    Catch2
)

target_include_directories(
    unittest
    PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
