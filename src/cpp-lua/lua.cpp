/****************************************************************
**lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-06.
*
* Description: C++ wrapper around Lua.
*
*****************************************************************/
#include "lua.hpp"

// base
#include "base/error.hpp"
#include "base/valid.hpp"

// Lua
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <array>

// Is this needed?
// #ifdef L
// #  undef L
// #  include "sol/sol.hpp"
// #  define L( a ) []( auto const& _ ) { return a; }
// #else
// #  include "sol/sol.hpp"
// #endif

using namespace std;

namespace luapp {

using lua_valid = base::valid_or<std::string>;

struct cstate {
  cstate() {
    lua_State* st = luaL_newstate();
    CHECK( st != nullptr );
    L = st;
  }
  ~cstate() { lua_close( L ); }

  /**************************************************************
  ** Lua C Function Wrappers.
  ***************************************************************/
  void openlibs() { luaL_openlibs( L ); }

  lua_valid dofile( char const* file ) {
    int error = luaL_dofile( L, file );
    if( error ) {
      string msg = lua_tostring( L, -1 );
      lua_pop( L, 1 );
      return msg;
    }
    return base::valid;
  }

  int gettop() { return lua_gettop( L ); }

  lua_valid setglobal( char const* s ) {
    enforce_stack_size_ge( 1 );
    ::lua_setglobal( L, s );
    return base::valid;
  }

  lua_valid loadstring( char const* script ) {
    lua_valid res = base::valid;
    if( luaL_loadstring( L, script ) != LUA_OK ) {
      res = lua_tostring( L, -1 );
      lua_pop( L, 1 );
    }
    return res;
  }

  // If this function returns `valid` then `nresults` from the
  // function will be pushed onto the stack. If it returns an
  // error then nothing needs to be popped from the stack. In all
  // cases, the function and arguments will be popped.
  lua_valid pcall( int nargs, int nresults ) {
    // Function object plus args should be on teh stack at least.
    enforce_stack_size_ge( nargs + 1 );
    lua_valid res = base::valid;
    // No matter what happens, lua_pcall will remove the function
    // and arguments from the stack.
    int err = lua_pcall( L, nargs, nresults, /*msgh=*/0 );
    if( err != LUA_OK ) {
      // lua_pcall will have pushed a single value onto the
      // stack, which will be the error object.
      res = lua_tostring( L, -1 );
      lua_pop( L, 1 );
    }
    return res;
  }

  void pop( int n = 1 ) {
    CHECK( stack_size() >= n, "({} >= {})", stack_size(), n );
    lua_pop( L, n );
  }

  /**************************************************************
  ** Helpers
  ***************************************************************/
  string_view lua_type_name( int ntype ) {
    static constexpr array<string_view, 9> type_names{
        /*0=*/"LUA_TNIL",
        /*1=*/"LUA_TBOOLEAN",
        /*2=*/"LUA_TLIGHTUSERDATA",
        /*3=*/"LUA_TNUMBER",
        /*4=*/"LUA_TSTRING",
        /*5=*/"LUA_TTABLE",
        /*6=*/"LUA_TFUNCTION",
        /*7=*/"LUA_TUSERDATA",
        /*8=*/"LUA_TTHREAD",
    };
    // If this static_assert fails then a new type has been
    // likely added.
    static_assert( LUA_NUMTAGS == type_names.size() );
    DCHECK( ntype > 0 );
    DCHECK( ntype < int( type_names.size() ) );
    return type_names[ntype];
  }

  int stack_size() { return gettop(); }

  void enforce_stack_size_eq( int s ) {
    if( stack_size() == s ) return;
    FATAL( "stack size expected to be {} but instead found {}.",
           s, stack_size() );
  }

  void enforce_stack_size_ge( int s ) {
    if( stack_size() >= s ) return;
    FATAL(
        "stack size expected to have size at least {} but "
        "instead found {}.",
        s, stack_size() );
  }

  int type_of( int idx ) { return lua_type( L, idx ); }

  lua_valid enforce_type_of( int idx, int type ) {
    if( type_of( idx ) == type ) return base::valid;
    return "type of element at index " + to_string( idx ) +
           " expected to be " + string( lua_type_name( type ) ) +
           ", but instead is " +
           string( lua_type_name( type_of( idx ) ) );
  }

private:
  lua_State* L;
};

/****************************************************************
** Testing
*****************************************************************/
char const* lua_script = R"(
  for i = 1, 5, 1 do
   my_module.foo( i )
  end
)";

void test_lua_direct() {
  cstate st;
  st.openlibs();
  CHECK_HAS_VALUE( st.dofile( "src/lua/test.lua" ) );
  CHECK( st.stack_size() == 1 );
  CHECK_HAS_VALUE( st.enforce_type_of( -1, LUA_TTABLE ) );
  CHECK_HAS_VALUE( st.setglobal( "my_module" ) );
  CHECK_HAS_VALUE( st.loadstring( lua_script ) );
  CHECK( st.stack_size() == 1 );
  CHECK_HAS_VALUE( st.pcall( /*nargs=*/0, /*nresults=*/0 ) );
  CHECK( st.stack_size() == 0 );
}

} // namespace luapp