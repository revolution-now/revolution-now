# Need to compile Lua as C++ so that it can throw exceptions.
set_source_files_properties(
  lua.c
  PROPERTIES
  LANGUAGE CXX
)

add_executable( lua-repl lua.c )

set_lua_warning_options( lua-repl )

target_link_libraries(
  lua-repl
  PUBLIC
  lua::lua
  readline
)
