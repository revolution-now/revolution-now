cmake_minimum_required( VERSION 3.0 )

# Compile as C++.
project( lua VERSION 5.3.5 LANGUAGES CXX )

function( set_lua_warning_options target )
  target_compile_options(
    ${target}
    PRIVATE
    # clang/GCC warnings
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
      -Wall
      -Wextra
      -Wno-deprecated
      -Wno-string-plus-int
    >
    # MSVC warnings
    $<$<CXX_COMPILER_ID:MSVC>: /Wall /WX >
  )
endfunction( set_lua_warning_options )

enable_language( C )

add_subdirectory( lib )
add_subdirectory( lua-repl )
add_subdirectory( luac )
