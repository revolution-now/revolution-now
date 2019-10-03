file( GLOB sources "[a-zA-Z]*.cpp" )
add_executable( unittest ${sources} )

set_warning_options( unittest )

get_target_property( FLATBUFF_INCLUDE_DIR rn-flatbuf INCLUDE_DIR )

target_link_libraries(
    unittest
    PRIVATE
    rn
    Catch2
    ${FLATBUF_INCLUDE_DIR}
)
