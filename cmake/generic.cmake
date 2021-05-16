# === ccache ======================================================

find_program( CCACHE_PROGRAM ccache )
if( CCACHE_PROGRAM )
    set( CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" )
else()
    message( STATUS "ccache not found." )
endif()

# === compiler warnings ===========================================

# Enable all warnings and treat warnings as errors.
# TODO: remove -Wno-unused-local-typedef after these are fixed:
#       https://bugs.llvm.org/show_bug.cgi?id=24883
#       https://bugs.llvm.org/show_bug.cgi?id=33298
function( set_warning_options target )
    target_compile_options(
        ${target} PRIVATE
        # clang
        $<$<CXX_COMPILER_ID:Clang>:
           -Wall
           -Wextra
           -Wno-unused-local-typedef
           -Wno-unused-parameter
         >
        # gcc
        $<$<CXX_COMPILER_ID:GNU>:
            -Wall
            -Wextra
            # For some reason gcc warns us when we reach the end
            # of a function without a return statement even if it
            # the function is guaranteed to return early due to
            # covering all of the possibilities in a switch
            # statement, each of which has a return statement.
            # This seems a bit too aggressive (clang doesn't do
            # it) so we will turn it off. Clang will warn us
            # properly if we are realistically in danger of
            # falling of the end of a function (assuming that a
            # switch statement whose cases are comprehensive will
            # indeed catch every case at runtime), so it is ok to
            # turn this off for gcc. As background, it is techni-
            # cally the case that e.g. an enum value can be as-
            # signed any integer using casts, and so the compiler
            # theoretically cannot guarantee that any switch
            # statement will actually cover all cases at runtime.
            # But in practice, as long as we don't violate the
            # type system, this shouldn't be a concern.
            -Wno-return-type
            -fcoroutines
         >
        # msvc
        $<$<CXX_COMPILER_ID:MSVC>:
            /Wall /WX > )
endfunction( set_warning_options )

# === build type ==================================================

set( default_build_type "Debug" )
if( NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES )
    message( STATUS "Setting build type to '${default_build_type}'." )
    set( CMAKE_BUILD_TYPE "${default_build_type}" CACHE
         STRING "Choose the type of build." FORCE )
    # Set the possible values of build type for cmake-gui
    set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "RelWithDebInfo")
endif()
message( STATUS "Build type: ${CMAKE_BUILD_TYPE}" )

# === clang-tidy ==================================================

if( USE_CLANG_TIDY )
    find_program( CLANG_TIDY_EXE NAMES "clang-tidy" )
    if( NOT CLANG_TIDY_EXE )
        message( FATAL_ERROR "clang-tidy not found." )
    endif()
    set( DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-quiet" )
endif()

function( clang_tidy target )
    if( USE_CLANG_TIDY )
        set_target_properties( ${target} PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}" )
    endif()
endfunction()

# This is used not only for clang-tidy but also for YCM.
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# === sanitizers ==================================================
set( SANITIZER_FLAGS
  # When one of the checks triggers, this option will cause it to
  # abort the program immediately with an error code instead of
  # trying to recover and continuing to run the program. Not all
  # sanitizer options support this; the ones that don't will
  # simply print an error to the console and the program will
  # keep running.
  -fno-sanitize-recover=all
  # ASan (Address Sanitizer).
  -fsanitize=address
  # This enables all of the UBSan (undefined behavior) checks.
  -fsanitize=undefined
  # FIXME: Re-enable this check when sol2 is fixed:
  #   https://github.com/ThePhD/sol2/issues/1071.
  -fno-sanitize=pointer-overflow
)

function( enable_sanitizers )
  message( STATUS "Enabling Sanitizers." )
  string( JOIN " " SANITIZER_FLAGS_STRING ${SANITIZER_FLAGS} )
  set( CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS_STRING}" PARENT_SCOPE )
  set( CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS_STRING}" PARENT_SCOPE )
endfunction()

# === colors ======================================================

function( force_compiler_color_diagnostics )
	if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
		# using Clang (either linux or apple)
    set( flag "-fcolor-diagnostics" )
	elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
		# using GCC
    set( flag "-fdiagnostics-color=always" )
	elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" )
		# using Intel C++
	elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" )
		# using Visual Studio C++
	endif()
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE )
endfunction()
