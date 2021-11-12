file( GLOB src_sources       "[a-zA-Z]*.cpp"       )
file( GLOB src_base_sources  "base/[a-zA-Z]*.cpp"  )
file( GLOB src_rcl_sources   "rcl/[a-zA-Z]*.cpp"   )
file( GLOB src_luapp_sources "luapp/[a-zA-Z]*.cpp" )
file( GLOB src_gl_sources    "gl/[a-zA-Z]*.cpp"    )
file( GLOB src_mock_sources  "mock/[a-zA-Z]*.cpp"  )

add_executable(
  unittest
  ${src_sources}
  ${src_base_sources}
  ${src_rcl_sources}
  ${src_luapp_sources}
  ${src_gl_sources}
  ${src_mock_sources}
)

set_warning_options( unittest )

target_link_libraries(
    unittest
    PRIVATE
    rn
    rn-mock
    Catch2
)
