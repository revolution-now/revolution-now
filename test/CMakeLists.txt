# === subdirectories ==============================================

add_subdirectory( rds )

# === this directory ==============================================

# These are subfolders, but CMake doesn't recurse into them, we
# just take their sources for the target in this folder.
file( GLOB src_base_sources   "base/[a-zA-Z]*.cpp"     )
file( GLOB src_cdr_sources    "cdr/[a-zA-Z]*.cpp"      )
file( GLOB src_gfx_sources    "gfx/[a-zA-Z]*.cpp"      )
file( GLOB src_gl_sources     "gl/[a-zA-Z]*.cpp"       )
file( GLOB src_luapp_sources  "luapp/[a-zA-Z]*.cpp"    )
file( GLOB src_mock_sources   "mock/[a-zA-Z]*.cpp"     )
file( GLOB src_rcl_sources    "rcl/[a-zA-Z]*.cpp"      )
file( GLOB src_refl_sources   "refl/[a-zA-Z]*.cpp"     )
file( GLOB src_sources        "[a-zA-Z]*.cpp"          )
file( GLOB src_stb_sources    "stb/[a-zA-Z]*.cpp"      )
file( GLOB src_render_sources "render/[a-zA-Z]*.cpp"   )
file( GLOB src_fake_sources   "fake/[a-zA-Z]*.cpp"     )
file( GLOB src_ss_sources     "ss/[a-zA-Z]*.cpp"       )

file( GLOB test_mocks_sources "mocks/[a-zA-Z]*.cpp"    )

add_executable(
  unittest
  ${src_sources}
  ${src_base_sources}
  ${src_rcl_sources}
  ${src_luapp_sources}
  ${src_gl_sources}
  ${src_mock_sources}
  ${src_stb_sources}
  ${src_gfx_sources}
  ${src_cdr_sources}
  ${src_refl_sources}
  ${src_render_sources}
  ${src_fake_sources}
  ${src_ss_sources}
  ${test_mocks_sources}
)

set_warning_options( unittest )

target_link_libraries(
  unittest
  PRIVATE
  Catch2
  rn
  rn-mock
)

add_dependencies( unittest rds-test )
get_target_property( RDS_TEST_INCLUDE_DIR rds-test INCLUDE_DIR )

target_include_directories(
  unittest
  PUBLIC
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src/
  ${RDS_TEST_INCLUDE_DIR}/../
)
