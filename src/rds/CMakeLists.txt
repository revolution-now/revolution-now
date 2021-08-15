# === subdirectories ==============================================

add_subdirectory( rdsc )

# === this directory ==============================================

file( GLOB rdss "[a-z][a-z0-9-]*.rds" )

# Generate the include files parameters.
set( generated_files "" )

# RDS files currently do not include other RDS files, and so
# there is no need to track dependencies among them during incre-
# mental building, unlike C++ header files, so the below should
# be sufficient.
foreach( rds ${rdss} )
  get_filename_component( stem ${rds} NAME_WE )
  get_filename_component( name ${rds} NAME )
  set( generated_include ${CMAKE_CURRENT_BINARY_DIR}/${stem}.hpp )
  add_custom_command(
    OUTPUT ${generated_include}
    COMMENT "Building rds definition ${name}"
    COMMAND rdsc
      ${rds}
      ${generated_include}
    DEPENDS rdsc ${rds}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  list( APPEND generated_files ${generated_include} )
endforeach()

# Create a custom target that depends on all the generated files.
# Depending on this will trigger all these to be built.
add_custom_target( rn-rds DEPENDS ${generated_files} )

set_property(
  TARGET rn-rds
  PROPERTY INCLUDE_DIR
  ${CMAKE_CURRENT_BINARY_DIR}/../
)
