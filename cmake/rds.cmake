function( generate_rds_target target )
  file( GLOB rdss "[a-z][a-z0-9-]*.rds" )

  # Generate the include files parameters.
  set( rds_generated_files "" )

  # RDS files currently do not include other RDS files, and so
  # there is no need to track dependencies among them during in-
  # cremental building, unlike C++ header files, so the below
  # should be sufficient.
  foreach( rds ${rdss} )
    get_filename_component( stem ${rds} NAME_WE )
    get_filename_component( name ${rds} NAME )
    set( generated_include ${CMAKE_CURRENT_BINARY_DIR}/${stem}.rds.hpp )
    add_custom_command(
      OUTPUT ${generated_include}
      COMMENT "building rds definition ${name}"
      COMMAND rdsc
        ${rds}
        ${generated_include}
      DEPENDS rdsc ${rds}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    list( APPEND rds_generated_files ${generated_include} )
  endforeach()

  # Create a custom target that depends on all the generated
  # files. Depending on this will trigger all these to be built.
  add_custom_target( ${target} DEPENDS ${rds_generated_files} )

  set_property(
    TARGET ${target}
    PROPERTY INCLUDE_DIR
    ${CMAKE_CURRENT_BINARY_DIR}
  )
endfunction()
