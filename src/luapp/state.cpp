/****************************************************************
**state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua state object.
*
*****************************************************************/
#include "state.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

// base-util
#include "absl/strings/str_split.h"

// Lua
#include "lauxlib.h"

using namespace std;

namespace luapp {

namespace {} // namespace

/****************************************************************
** state
*****************************************************************/
state::state( ::lua_State* L )
  : C( new c_api( L, /*own=*/false ) ) {}

state::state() : C( new c_api ) {}

c_api& state::api() noexcept { return *C; }

int state::noref() { return LUA_NOREF; }

// TODO: use this as a model for loading a piece of code once
// into the registry. Can remove eventually.
void state::tables_slow( std::string_view path ) noexcept {
  if( tables_func_ref == LUA_NOREF ) {
    static string code = R"lua(
      local path = ...
      local tab = _G
      for p in string.gmatch( path, '[^.]+' ) do
        tab[p] = tab[p] or {{}}
        tab = tab[p]
      end
    )lua";
    CHECK_HAS_VALUE( C->loadstring( code.c_str() ) );
    tables_func_ref = C->ref_registry();
  }

  CHECK( C->registry_get( tables_func_ref ) ==
         e_lua_type::function );
  C->push( string( path ).c_str() );
  CHECK_HAS_VALUE( C->pcall( /*nargs=*/1, /*nresults=*/0 ) );
}

void state::tables( std::string_view path ) noexcept {
  vector<string> elems = absl::StrSplit( path, '.' );
  C->pushglobaltable();
  for( string const& elem : elems ) {
    e_lua_type type =
        C->getfield( /*table_idx=*/-1, elem.c_str() );
    switch( type ) {
      case e_lua_type::nil: {
        C->pop(); // nil
        C->newtable();
        C->setfield( /*table_idx=*/-2, elem.c_str() );
        C->getfield( /*table_idx=*/-1, elem.c_str() );
        break;
      }
      case e_lua_type::table: {
        break;
      }
      default: {
        FATAL( "field {} is not a table, instead is of type {}.",
               elem, type );
      }
    }
  }
  C->pop( elems.size() + 1 ); // + global table
}

} // namespace luapp
