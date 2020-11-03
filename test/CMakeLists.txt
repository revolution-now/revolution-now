file( GLOB sources "[a-zA-Z]*.cpp" )
add_executable( unittest ${sources} )

set_warning_options( unittest )

target_link_libraries(
    unittest
    PRIVATE
    rn
    Catch2
)
