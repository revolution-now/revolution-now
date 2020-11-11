# === subdirectories ==============================================

add_subdirectory( rnlc )

# === this directory ==============================================

file( GLOB rnls "[a-z][a-z0-9-]*.rnl" )

# Generate the include files parameters.
set( generated_files "" )

set( rnl_peg "${CMAKE_CURRENT_SOURCE_DIR}/rnlc/rnl.peg" )

# FIXME: this currently does not support proper incremental
# builds in that it does not track dependencies among the rnl
# files via their include statements. scripts/rnl-invalidator.sh
# is being used as a pre-build step outside of the CMake files to
# fix this, but ideally it should be done with CMake somehow.
foreach( rnl ${rnls} )
  get_filename_component( stem ${rnl} NAME_WE )
  get_filename_component( name ${rnl} NAME )
  set( generated_include ${CMAKE_CURRENT_BINARY_DIR}/${stem}.hpp )
  add_custom_command(
    OUTPUT ${generated_include}
    COMMENT "Building rnl definition ${name}"
    COMMAND rnlc
      ${rnl}
      ${generated_include}
      ${rnl_peg}
      #-o ${CMAKE_CURRENT_BINARY_DIR}
      #-I ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS rnlc ${rnl} ${rnl_peg}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  list( APPEND generated_files ${generated_include} )
endforeach()

# Create a custom target that depends on all the generated files.
# Depending on this will trigger all these to be built.
add_custom_target( rn-rnls DEPENDS ${generated_files} )

set_property(
  TARGET rn-rnls
  PROPERTY INCLUDE_DIR
  ${CMAKE_CURRENT_BINARY_DIR}/../
)
