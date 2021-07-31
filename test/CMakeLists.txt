file( GLOB src_sources       "[a-zA-Z]*.cpp"       )
file( GLOB src_base_sources  "base/[a-zA-Z]*.cpp"  )
file( GLOB src_cl_sources    "cl/[a-zA-Z]*.cpp"    )
file( GLOB src_luapp_sources "luapp/[a-zA-Z]*.cpp" )
file( GLOB src_parz_sources  "parz/[a-zA-Z]*.cpp"  )

add_executable(
  unittest
  ${src_sources}
  ${src_base_sources}
  ${src_cl_sources}
  ${src_luapp_sources}
  ${src_parz_sources}
)

set_warning_options( unittest )

target_link_libraries(
    unittest
    PRIVATE
    rn
    Catch2
)
