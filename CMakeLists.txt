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

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_SOURCE_DIR}/cmake/" )

include( cmake/generic.cmake )
include( cmake/sdl2-helper.cmake )

# If ENABLE_ADDRESS_SANITIZER is on then this function will do so.
if( ENABLE_ADDRESS_SANITIZER )
  enable_sanitizers()
endif()

# Just in case we are using Ninja (which will buffer the output of
# compilers causing them to suppress color output) let's tell the
# compilers to force color output.
force_compiler_color_diagnostics()

# === compiler flags ==============================================

# Might have to turn this one off for producing generic builds.
# FIXME: put this behind a configure variable and include mention
#        of it in the configuration identifier.
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native" )

# === subdirectories ==============================================

add_subdirectory( assets/music )
add_subdirectory( src )
add_subdirectory( exe )
add_subdirectory( test )

# === external dependencies =======================================

# These should not be necessary, but some subdirectories' cmake
# files are not playing nicely and just setting
# CMAKE_CXX_STANDARD, so we have to set it here.
set( CMAKE_CXX_STANDARD 20 )

# We don't want this variable set when we import the externs be-
# cause it will cause trouble; it may be enabled from the pre-
# vious CMake run because we do enable tests us.
set( BUILD_TESTING OFF )

set( SOL2_LUA_VERSION "5.3"  )
set( SOL2_BUILD_LUA    FALSE )

set( FLATBUFFERS_BUILD_TESTS       OFF CACHE BOOL "" )
set( FLATBUFFERS_INSTALL           OFF CACHE BOOL "" )
set( FLATBUFFERS_BUILD_FLATHASH    OFF CACHE BOOL "" )
set( FLATBUFFERS_LIBCXX_WITH_CLANG OFF CACHE BOOL "" )

add_subdirectory( extern/base-util/src EXCLUDE_FROM_ALL )
add_subdirectory( extern/abseil-cpp    EXCLUDE_FROM_ALL )
add_subdirectory( extern/backward-cpp  EXCLUDE_FROM_ALL )
add_subdirectory( extern/spdlog        EXCLUDE_FROM_ALL )
add_subdirectory( extern/fmt           EXCLUDE_FROM_ALL )
add_subdirectory( extern/libucl        EXCLUDE_FROM_ALL )
add_subdirectory( extern/midifile      EXCLUDE_FROM_ALL )
add_subdirectory( extern/rtmidi        EXCLUDE_FROM_ALL )
add_subdirectory( extern/rg-2-midi/src EXCLUDE_FROM_ALL )
add_subdirectory( extern/expected-lite EXCLUDE_FROM_ALL )
add_subdirectory( extern/lua-5.3.5     EXCLUDE_FROM_ALL )
add_subdirectory( extern/sol2          EXCLUDE_FROM_ALL )
add_subdirectory( extern/flatbuffers   EXCLUDE_FROM_ALL )
add_subdirectory( extern/magic_enum    EXCLUDE_FROM_ALL )
add_subdirectory( extern/glad          EXCLUDE_FROM_ALL )
add_subdirectory( extern/cpp-peglib    EXCLUDE_FROM_ALL )

set( BUILD_TESTING ON ) # Catch2 will actually do this as well.
add_subdirectory( extern/Catch2        EXCLUDE_FROM_ALL )

# === global settings =============================================

# This tells fmt headers to include an external version of fmt
# instead of one that comes bundled inside of it.  In particular,
# the external version used will be the one in our extern folder.
target_compile_definitions( fmt INTERFACE SPDLOG_FMT_EXTERNAL )

# === custom targets ==============================================

add_custom_target( run
                   COMMAND exe
                   WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                   USES_TERMINAL )
add_custom_target( test
                   COMMAND unittest --rng-seed=time --order=rand --abort
                   WORKING_DIRECTORY ../../
                   USES_TERMINAL )
