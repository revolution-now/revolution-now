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

# === external dependencies =======================================

add_subdirectory( extern/base-util/src EXCLUDE_FROM_ALL )
add_subdirectory( extern/abseil-cpp    EXCLUDE_FROM_ALL )
add_subdirectory( extern/spdlog        EXCLUDE_FROM_ALL )
add_subdirectory( extern/fmt           EXCLUDE_FROM_ALL )
add_subdirectory( extern/libucl        EXCLUDE_FROM_ALL )

# === global settings =============================================

# This tells fmt headers to include an external version of fmt
# instead of one that comes bundled inside of it.  In particular,
# the external version used will be the one in our extern folder.
target_compile_definitions( fmt INTERFACE SPDLOG_FMT_EXTERNAL )

# === custom targets ==============================================

add_custom_target( run
                   COMMAND rn
                   WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                   USES_TERMINAL )
