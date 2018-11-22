cmake_minimum_required( VERSION 3.12...3.12 )

if( ${CMAKE_VERSION} VERSION_LESS 3.12 )
    cmake_policy(
      VERSION
      ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} )
endif()

project(
    revolution-now
    VERSION 0.1.0
    DESCRIPTION "Revolution | Now"
    LANGUAGES CXX
)

set( CMAKE_MODULE_PATH
   ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" )

include( cmake/generic.cmake )
include( cmake/sdl2-helper.cmake )

# === subdirectories ==============================================

add_subdirectory( src )
add_subdirectory( extern/base-util/src )

# === custom targets ==============================================

add_custom_target( run
                   COMMAND rn
                   WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                   USES_TERMINAL )
