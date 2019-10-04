file( GLOB fbs "[a-z][a-z0-9-]*.fbs" )

# Generate the include files parameters.
set( generated_files "" )

foreach( fb ${fbs} )
  get_filename_component( stem ${fb} NAME_WE )
  get_filename_component( name ${fb} NAME )
  # This format is chosen by flatbuf.
  set( generated_include ${CMAKE_CURRENT_BINARY_DIR}/${stem}_generated.h )
  add_custom_command(
    OUTPUT ${generated_include}
    COMMENT "Compiling flatbuffer for ${name}"
    COMMAND flatc
    --cpp
    --lua
    --scoped-enums
    --reflect-types
    --reflect-names
    --include-prefix "fb/"
    -o ${CMAKE_CURRENT_BINARY_DIR}
    -I ${CMAKE_CURRENT_SOURCE_DIR}
    ${fb}
    DEPENDS flatc ${fb}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  list( APPEND generated_files ${generated_include} )
endforeach()

# Create a custom target that depends on all the generated files.
# Depending on this will trigger all these to be built.
add_custom_target( rn-flatbuf DEPENDS ${generated_files} )

set_property(
  TARGET rn-flatbuf
  PROPERTY INCLUDE_DIR
  ${CMAKE_CURRENT_BINARY_DIR}/../
)
