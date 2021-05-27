file( GLOB src_sources "[a-zA-Z]*.cpp" )
file( GLOB luapp_sources "luapp/[a-zA-Z]*.cpp" )

add_executable(
  unittest
  ${src_sources}
  ${luapp_sources}
)

set_warning_options( unittest )

target_link_libraries(
    unittest
    PRIVATE
    rn
    Catch2
)
