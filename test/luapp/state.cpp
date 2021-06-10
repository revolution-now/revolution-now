/****************************************************************
**state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: Unit tests for the src/luapp/state.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/state.hpp"

// Testing
#include "test/monitoring-types.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Lua
#include "lauxlib.h"
#include "lua.h"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::e_lua_type );

namespace lua {
namespace {

using namespace std;

using ::base::valid;
using ::testing::Tracker;

TEST_CASE( "[state] creation/destruction" ) { state st; }

TEST_CASE( "[state] tables" ) {
  state  st;
  c_api& C = st.api();
  REQUIRE( C.getglobal( "t1" ) == e_lua_type::nil );
  C.pop();
  REQUIRE( C.stack_size() == 0 );

  SECTION( "empty" ) { st.tables( { "" } ); }

  SECTION( "single" ) {
    st.tables( { "t1" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 1 );
    C.pop();
  }

  SECTION( "double" ) {
    st.tables( { "t1", "t2" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 2 );
    C.pop( 2 );
  }

  SECTION( "triple" ) {
    st.tables( { "t1", "t2", "t3" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "tables already present" ) {
    st.tables( { "t1" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::nil );
    REQUIRE( C.stack_size() == 2 );
    C.pop( 2 );
    st.tables( { "t1", "t2" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::nil );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
    st.tables( { "t1", "t2", "t3" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
    st.tables( { "t1", "t2", "t3" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "triple 2" ) {
    st.tables( { "hello_world", "yes123x", "_" } );
    REQUIRE( C.getglobal( "hello_world" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "yes123x" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "_" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "spaces" ) {
    st.tables( { " t1", " t2" } );
    REQUIRE( C.getglobal( " t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, " t2" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 2 );
    C.pop( 2 );
  }

  SECTION( "with reserved" ) {
    st.tables( { "t1", "if", "t3" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "if" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "bad identifier" ) {
    st.tables( { "t1", "x-z", "t3" } );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "x-z" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  REQUIRE( C.stack_size() == 0 );
}

TEST_CASE( "[state] push_function, stateless lua C function" ) {
  state st;
  st.openlibs();
  c_api& C = st.api();

  st.push_function( []( lua_State* L ) -> int {
    c_api C = c_api::view( L );
    int   n = luaL_checkinteger( L, 1 );
    C.push( n + 1 );
    return 1;
  } );
  C.setglobal( "add_one" );

  SECTION( "once" ) {
    REQUIRE( C.dostring( "assert( add_one( 6 ) == 7 )" ) ==
             valid );
  }
  SECTION( "twice" ) {
    REQUIRE( C.dostring( "assert( add_one( 6 ) == 7 )" ) ==
             valid );
  }

  // Make sure that it has no upvalues.
  C.getglobal( "add_one" );
  REQUIRE( C.type_of( -1 ) == e_lua_type::function );
  REQUIRE_FALSE( C.getupvalue( -1, 1 ) );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
}

TEST_CASE( "[state] push_function, stateful lua C function" ) {
  Tracker::reset();

  SECTION( "__gc metamethod is called, twice" ) {
    state st;
    st.openlibs();
    c_api& C = st.api();

    bool created = st.push_function(
        [tracker = Tracker{}]( lua_State* L ) -> int {
          c_api C = c_api::view( L );
          int   n = luaL_checkinteger( L, 1 );
          C.push( n + 1 );
          return 1;
        } );
    REQUIRE( created );
    C.setglobal( "add_one" );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 2 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    REQUIRE( C.dostring( "assert( add_one( 6 ) == 7 )" ) ==
             valid );
    REQUIRE( C.dostring( "assert( add_one( 7 ) == 8 )" ) ==
             valid );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    // Test that the function has an up value and that the upval-
    // ue's metatable has the right name.
    C.getglobal( "add_one" );
    REQUIRE( C.type_of( -1 ) == e_lua_type::function );
    REQUIRE_FALSE( C.getupvalue( -1, 2 ) );
    REQUIRE( C.getupvalue( -1, 1 ) == true );
    REQUIRE( C.type_of( -1 ) == e_lua_type::userdata );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.getmetatable( -1 ) );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.getfield( -1, "__name" ) == e_lua_type::string );
    REQUIRE( C.stack_size() == 4 );
    REQUIRE( C.get<string>( -1 ) ==
             "base::unique_func<int (lua_State*) const>" );
    C.pop( 4 );
    REQUIRE( C.stack_size() == 0 );

    // Now set a second closure and ensure that the metatable
    // gets reused.
    created = st.push_function(
        [tracker = Tracker{}]( lua_State* L ) -> int {
          c_api C = c_api::view( L );
          int   n = luaL_checkinteger( L, 1 );
          C.push( n + 2 );
          return 1;
        } );
    REQUIRE_FALSE( created );
    C.setglobal( "add_two" );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 2 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    REQUIRE( C.dostring( "assert( add_two( 6 ) == 8 )" ) ==
             valid );
    REQUIRE( C.dostring( "assert( add_two( 7 ) == 9 )" ) ==
             valid );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();
  }

  // Ensure that precisely two closures get destroyed (will
  // happen when `st` goes out of scope and Lua calls the final-
  // izers on the userdatas for the two closures that we created
  // above).
  REQUIRE( Tracker::constructed == 0 );
  REQUIRE( Tracker::destructed == 2 );
  REQUIRE( Tracker::copied == 0 );
  REQUIRE( Tracker::move_constructed == 0 );
  REQUIRE( Tracker::move_assigned == 0 );
}

TEST_CASE( "[state] push_function, cpp function has upvalue" ) {
  state st;
  st.openlibs();
  c_api& C = st.api();

  st.push_function( []( int n, string const& s,
                        double d ) -> string {
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );

  // Make sure that it has no upvalues.
  C.getglobal( "go" );
  REQUIRE( C.type_of( -1 ) == e_lua_type::function );
  REQUIRE_FALSE( C.getupvalue( -1, 2 ) );
  REQUIRE( C.getupvalue( -1, 1 ) == true );
  REQUIRE( C.type_of( -1 ) == e_lua_type::userdata );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.getfield( -1, "__name" ) == e_lua_type::string );
  REQUIRE( C.stack_size() == 4 );
  REQUIRE( C.get<string>( -1 ) ==
           "base::unique_func<int (lua_State*) const>" );
  C.pop( 4 );
  REQUIRE( C.stack_size() == 0 );
}

TEST_CASE( "[state] push_function, cpp function, trivial" ) {
  state  st;
  c_api& C = st.api();

  bool called = false;

  st.push_function( [&] { called = !called; } );
  C.setglobal( "go" );
  REQUIRE_FALSE( called );

  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE_FALSE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE_FALSE( called );
}

TEST_CASE( "[state] push_function, cpp function, simple/bool" ) {
  state  st;
  c_api& C = st.api();

  bool called_with = false;

  st.push_function( [&]( bool b ) { called_with = b; } );
  C.setglobal( "go" );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( false )" ) == valid );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( false )" ) == valid );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( true )" ) == valid );
  REQUIRE( called_with );

  REQUIRE( C.dostring( "go( 'hello' )" ) == valid );
  REQUIRE( called_with );

  REQUIRE( C.dostring( "go( nil )" ) == valid );
  REQUIRE_FALSE( called_with );
}

TEST_CASE( "[state] push_function, cpp function, calling" ) {
  state st;
  st.openlibs();
  c_api& C = st.api();

  st.push_function( []( int n, string const& s,
                        double d ) -> string {
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );

  SECTION( "successful call" ) {
    REQUIRE( C.dostring( R"(
      local result =
        go( 6, 'hello this is a very long string', 3.5 )
      local expected =
        "args: n=6, s='hello this is a very long string', d=3.5"
      local err = tostring( result ) .. ' not equal to "' ..
                  tostring( expected ) .. '".'
      assert( result == expected, err )
    )" ) == valid );
  }

  SECTION( "too few args" ) {
    // clang-format off
    char const* err =
      "C++ function expected 3 arguments, but received 2 from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on

    REQUIRE( C.dostring( R"(
      go( 6, 'hello this is a very long string' )
    )" ) == lua_invalid( err ) );
  }

  SECTION( "too many args" ) {
    // clang-format off
    char const* err =
      "C++ function expected 3 arguments, but received 4 from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on

    REQUIRE( C.dostring( R"(
      go( 6, 'hello this is a very long string', 3.5, true )
    )" ) == lua_invalid( err ) );
  }

  SECTION( "wrong arg type" ) {
    // clang-format off
    char const* err =
      "C++ function expected type 'double' for argument 3 "
        "(1-based), but received non-convertible type 'string' "
        "from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on
    REQUIRE( C.dostring( R"(
      go( 6, 'hello this is a very long string', 'world' )
    )" ) == lua_invalid( err ) );
  }

  SECTION( "convertible arg types" ) {
    REQUIRE( C.dostring( R"(
      local result = go( '6', 1.23, '3.5' )
      local expected = "args: n=6, s='1.23', d=3.5"
      local err = tostring( result ) .. ' not equal to "' ..
                  tostring( expected ) .. '".'
      assert( result == expected, err )
    )" ) == valid );
  }
}

TEST_CASE( "[state] call/pcall" ) {
  state st;
  st.openlibs();
  c_api& C = st.api();

  REQUIRE( C.dostring( R"(
    function foo( n, s, d )
      assert( n ~= nil, 'n is nil' )
      assert( s ~= nil, 's is nil' )
      assert( d ~= nil, 'd is nil' )
      local fmt = string.format
      return fmt( "args: n=%s, s='%s', d=%s", n, s, d )
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( st.call( 3, "hello", 3.5 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.5" );
  }

  SECTION( "pcall" ) {
    REQUIRE( st.pcall( 3, "hello", 3.5 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.5" );
  }

  SECTION( "pcall with error" ) {
    // clang-format off
    char const* err =
      "[string \"...\"]:4: s is nil\n"
      "stack traceback:\n"
      "\t[C]: in function 'assert'\n"
      "\t[string \"...\"]:4: in function 'foo'";
    // clang-format on

    REQUIRE( st.pcall( 3, nil, 3.5 ) ==
             lua_unexpected<int>( err ) );
    REQUIRE( C.stack_size() == 0 );
  }
}

TEST_CASE( "[state] call/pcall multret" ) {
  state st;
  st.openlibs();
  c_api& C = st.api();

  REQUIRE( C.dostring( R"(
    function foo( n, s, d )
      return n+1, s .. '!', d+1.5
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( st.call( 3, "hello", 3.5 ) == 3 );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<int>( -3 ) == 4 );
    REQUIRE( C.get<string>( -2 ) == "hello!" );
    REQUIRE( C.get<int>( -1 ) == 5.0 );
  }

  SECTION( "pcall" ) {
    REQUIRE( st.pcall( 3, "hello", 3.5 ) == 3 );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<int>( -3 ) == 4 );
    REQUIRE( C.get<string>( -2 ) == "hello!" );
    REQUIRE( C.get<int>( -1 ) == 5.0 );
  }
}

TEST_CASE( "[state] cpp from cpp via lua" ) {
  state st;
  st.openlibs();
  c_api& C = st.api();

  st.push_function( []( int n, string const& s,
                        double d ) -> string {
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );
  REQUIRE( C.stack_size() == 0 );

  C.getglobal( "go" );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( st.call( 3, "hello", 3.6 ) == 1 );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.get<string>( -1 ) ==
           "args: n=3, s='hello', d=3.6" );
}

TEST_CASE( "[state] cpp->lua->cpp round trip" ) {
  state st;
  st.openlibs();
  c_api& C = st.api();

  st.push_function( [&]( int n, string const& s,
                         double d ) -> string {
    if( n == 4 ) C.error( "n cannot be 4." );
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );

  REQUIRE( C.dostring( R"(
    function foo( n, s, d )
      assert( n ~= nil, 'n is nil' )
      assert( s ~= nil, 's is nil' )
      assert( d ~= nil, 'd is nil' )
      return go( n, s, d )
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( st.call( 3, "hello", 3.6 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.6" );
  }

  SECTION( "call again" ) {
    REQUIRE( st.call( 3, "hello", 3.6 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.6" );
  }

  SECTION( "pcall" ) {
    REQUIRE( st.pcall( 3, "hello", 3.6 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.6" );
  }

  SECTION( "pcall with error" ) {
    // clang-format off
    char const* err =
      "[string \"...\"]:4: s is nil\n"
      "stack traceback:\n"
      "\t[C]: in function 'assert'\n"
      "\t[string \"...\"]:4: in function 'foo'";
    // clang-format on

    REQUIRE( st.pcall( 3, nil, 3.6 ) ==
             lua_unexpected<int>( err ) );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "pcall with from C function" ) {
    // clang-format off
    char const* err =
      "[string \"...\"]:6: n cannot be 4.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:6: in function 'foo'";
    // clang-format on

    REQUIRE( st.pcall( 4, "hello", 3.6 ) ==
             lua_unexpected<int>( err ) );
    REQUIRE( C.stack_size() == 0 );
  }
}

} // namespace
} // namespace lua
