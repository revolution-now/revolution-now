# Need to compile Lua as C++ so that it can throw exceptions.
set_source_files_properties(
  luac.c
  PROPERTIES
  LANGUAGE CXX
)

add_executable( luac luac.c )

set_lua_warning_options( luac )

target_link_libraries(
  luac
  PUBLIC
  lua::lua
)
