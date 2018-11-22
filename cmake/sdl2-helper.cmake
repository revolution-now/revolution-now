# This function sets <name>_LIBRARY and <name>_INCLUDE_DIR.
function( find_sdl2_lib name )
    find_package( ${name} REQUIRED )
    if( NOT "${${name}_FOUND}" )
        message( FATAL_ERROR "Cannot find ${name}." )
    endif()
    string( TOUPPER ${name} UPPER_NAME )
    message( STATUS
      "${UPPER_NAME}_INCLUDE_DIR: ${${UPPER_NAME}_INCLUDE_DIR}" )
    message( STATUS
      "${UPPER_NAME}_LIBRARY: ${${UPPER_NAME}_LIBRARY}" )
endfunction()
