# === ccache ======================================================

find_program( CCACHE_PROGRAM ccache )
if( CCACHE_PROGRAM )
    set( CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" )
else()
    message( STATUS "ccache not found." )
endif()

# === compiler warnings ===========================================

# Enable all warnings and treat warnings as errors.
function( set_warning_options target )
    target_compile_options(
        ${target} PRIVATE
        # clang
        $<$<CXX_COMPILER_ID:Clang>:
           -Weverything
           -Wno-pre-c++20-compat
           -Wno-c++20-compat
           -Wno-pre-c++17-compat
           -Wno-pre-c++14-compat
           -Wno-c99-extensions
           -Wno-c++98-compat
           -Wno-c++98-compat-pedantic
           -Wno-reserved-macro-identifier
           -Wno-newline-eof
           -Wno-padded
           -Wno-extra-semi-stmt
           -Wno-extra-semi
           -Wno-reserved-identifier
           -Wno-ctad-maybe-unsupported
           -Wno-undefined-func-template
           # TODO: this looks like a new warning that is not
           # ready for primetime yet, and/or it flags too many
           # things. Try re-enabling it at some point in the fu-
           # ture since it may eventually be good.
           -Wno-unsafe-buffer-usage
           # This one gives a warning about missing case state-
           # ments even when there is a default, which we don't
           # want. We still have -Wswitch which will tell us
           # about missing case statements when there is no de-
           # fault (which is important).
           -Wno-switch-enum

           # TODO: consider re-enabling.
           -Wno-shadow
           -Wno-shadow-uncaptured-local
           -Wno-shadow-field
           -Wno-exit-time-destructors
           -Wno-implicit-int-conversion
           -Wno-implicit-float-conversion
           -Wno-sign-conversion
           -Wno-old-style-cast
           -Wno-shorten-64-to-32
           -Wno-global-constructors
           -Wno-weak-vtables
           -Wno-double-promotion
           -Wno-float-equal

           # TODO: remove this after these issues are fixed:
           #       https://bugs.llvm.org/show_bug.cgi?id=24883
           #       https://bugs.llvm.org/show_bug.cgi?id=33298
           -Wno-unused-local-typedef

           # This is optional, should probably be removed eventu-
           # ally. It is convenient when there is a new warning
           # introduced that we want to disable but we don't want
           # to update all clang builds to a version that in-
           # cludes that warning.
           -Wno-unknown-warning-option
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
            # FIXME: gcc seems to have a bug with coroutines
            # where it emits a series of -Wmaybe-uninitialized
            # warnings that appear to refer to something inside
            # the generated coroutine itself that does not appear
            # fixable, so we will suppress that until it is
            # fixed.
            -Wno-maybe-uninitialized
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
)

if( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
  # This is to fix a gcc linker error with asan.
  set( SANITIZER_FLAGS "${SANITIZER_FLAGS} -static-libasan" )
endif()

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
  # This is to fix a clang linker error that happens sometimes
  # when enabling -fsanitize=undefined. See this bug:
  #
  #   https://bugs.llvm.org/show_bug.cgi?id=16404
  #
  # If some day that gets fixed then we can remove these.
  set( SANITIZER_FLAGS "${SANITIZER_FLAGS} -rtlib=compiler-rt" )
  set( SANITIZER_FLAGS "${SANITIZER_FLAGS} -lgcc_s" )
  set( SANITIZER_FLAGS "${SANITIZER_FLAGS} -Wno-unused-command-line-argument" )
endif()

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

# === add_rn_target ===============================================

function( add_rn_target target type )
  file( GLOB sources "[a-zA-Z]*.cpp" )

  if( type STREQUAL "library" )
    add_library( ${target} STATIC ${sources} )
  elseif( type STREQUAL "executable" )
    add_executable( ${target} ${sources} )
  else()
    message( FATAL "unrecognized target type: ${type}" )
  endif()

  target_compile_features( ${target} PUBLIC cxx_std_20 )
  set_target_properties( ${target} PROPERTIES CXX_EXTENSIONS OFF )
  set_warning_options( ${target} )

  target_link_libraries(
      ${target}
      PUBLIC
      ${ARGN}
  )

  # Rds
  set( rds_target "${target}-rds" )
  generate_rds_target( ${rds_target} )
  if( ${has_rds} ) # set in generate_rds_target function.
    add_dependencies( ${target} ${rds_target} )
    get_target_property( RDS_INCLUDE_DIR ${rds_target} INCLUDE_DIR )
  else()
    set( RDS_INCLUDE_DIR "" )
  endif()

  target_include_directories(
      ${target}
      PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${RDS_INCLUDE_DIR}
      ${CMAKE_SOURCE_DIR}/src/
      ${CMAKE_BINARY_DIR}/src/
  )

  clang_tidy( ${target} )
endfunction()

# === add_rn_library ==============================================

function( add_rn_library target )
  add_rn_target( ${target} library ${ARGN} )
endfunction()

# === add_rn_executable ===========================================

function( add_rn_executable target )
  add_rn_target( ${target} executable ${ARGN} )
endfunction()
