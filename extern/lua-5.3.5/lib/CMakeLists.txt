# Note: restrictive pattern.
file( GLOB sources "*.c" )

# Need to compile Lua as C++ so that it can throw exceptions.
set_source_files_properties(
  ${sources}
  PROPERTIES
  LANGUAGE CXX
)

add_library( lua STATIC ${sources} )
add_library( lua::lua ALIAS lua )
set_target_properties( lua PROPERTIES CXX_EXTENSIONS OFF )

target_include_directories( lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries(
  lua
  PUBLIC
  ${CMAKE_DL_LIBS}
)

target_compile_features( lua PUBLIC cxx_std_20 )

set_lua_warning_options( lua )

if( CMAKE_SYSTEM_NAME STREQUAL Linux )
  set( LUA_USE LUA_USE_LINUX )
elseif( CMAKE_SYSTEM_NAME STREQUAL Darwin )
  set( LUA_USE LUA_USE_MACOSX )
endif()

if( CMAKE_BUILD_TYPE STREQUAL Debug )
  message( STATUS "Enabling Lua library assertions." )
  set( LUA_ASSERT "lua_assert=assert" )
else()
  set( LUA_ASSERT "" )
endif()

target_compile_definitions(
  lua
  PUBLIC
  ${LUA_USE}
  LUA_COMPAT_5_2
  ${LUA_ASSERT}
)
