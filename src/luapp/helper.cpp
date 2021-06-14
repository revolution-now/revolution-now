/****************************************************************
**helper.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua helper object.
*
*****************************************************************/
#include "helper.hpp"

// luapp
#include "c-api.hpp"
#include "userdata.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"

// Lua
#include "lauxlib.h"

using namespace std;

namespace lua {

namespace {} // namespace

/****************************************************************
** helper
*****************************************************************/
helper::helper( cthread L ) : C( c_api( L ) ) {}

void helper::openlibs() noexcept { C.openlibs(); }

int helper::noref() { return LUA_NOREF; }

// TODO: use this as a model for loading a piece of code once
// into the registry. Can remove eventually.
void helper::tables_slow( std::string_view path ) noexcept {
  if( tables_func_ref == LUA_NOREF ) {
    static string const code = R"lua(
      local path = ...
      local tab = _G
      for p in string.gmatch( path, '[^.]+' ) do
        tab[p] = tab[p] or {{}}
        tab = tab[p]
      end
    )lua";
    CHECK_HAS_VALUE( C.loadstring( code.c_str() ) );
    tables_func_ref = C.ref_registry();
  }

  CHECK( C.registry_get( tables_func_ref ) == type::function );
  C.push( string( path ).c_str() );
  CHECK_HAS_VALUE( C.pcall( /*nargs=*/1, /*nresults=*/0 ) );
}

void helper::tables( c_string_list const& path ) noexcept {
  C.pushglobaltable();
  for( char const* elem : path ) {
    lua::type type = C.getfield( /*table_idx=*/-1, elem );
    switch( type ) {
      case type::nil: {
        C.pop(); // nil
        C.newtable();
        C.setfield( /*table_idx=*/-2, elem );
        C.getfield( /*table_idx=*/-1, elem );
        break;
      }
      case type::table: {
        break;
      }
      default: {
        FATAL( "field {} is not a table, instead is of type {}.",
               elem, type );
      }
    }
  }
  C.pop( path.size() + 1 ); // + global table
}

void helper::traverse_and_push_table_and_key(
    c_string_list const& path ) noexcept {
  CHECK( path.size() > 0 );
  C.pushglobaltable();
  auto i = path.begin();
  while( true ) {
    char const* elem = *i;
    if( i == path.end() - 1 ) {
      C.push( elem );
      return;
    }
    lua::type type = C.getfield( /*table_idx=*/-1, elem );
    CHECK( type == type::table, "field '{}' is not a table.",
           elem );
    ++i;
    C.swap_top();
    C.pop();
  }
}

type helper::push_path( c_string_list const& path ) noexcept {
  C.pushglobaltable();
  char const* elem = "_G";
  lua::type   type = type::table;
  auto        i    = path.begin();
  while( i != path.end() ) {
    CHECK( C.type_of( -1 ) == type::table,
           "field '{}' is not a table.", elem );
    elem = *i;
    type = C.getfield( /*table_idx=*/-1, elem );
    ++i;
    C.swap_top();
    C.pop();
  }
  return type;
}

void helper::push_stateless_lua_c_function(
    LuaCFunction* func ) noexcept {
  C.push( func );
}

void helper::push_stateful_lua_c_function(
    base::unique_func<int( lua_State* ) const> func ) noexcept {
  // Pushes new userdata onto stack.
  push_userdata_by_value( C.this_cthread(), std::move( func ) );

  // 1. Create the closure with one upvalue (the userdata).
  static auto closure_caller = []( lua_State* L ) -> int {
    using Closure_t = remove_cvref_t<decltype( func )>;
    static string const type_name =
        base::demangled_typename<Closure_t>();
    // Use `testudata` instead of `checkudata` so that we throw a
    // C++ error instead of a Lua error.
    void* ud = luaL_testudata( L, lua_upvalueindex( 1 ),
                               type_name.c_str() );
    CHECK(
        ud != nullptr,
        "closure_caller expected type {} but did not find it.",
        type_name );
    Closure_t const& closure =
        *static_cast<Closure_t const*>( ud );
    return closure( L );
  };

  C.push( closure_caller, /*upvalues=*/1 );
}

} // namespace lua
