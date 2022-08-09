/****************************************************************
**as.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-19.
*
* Description: Unit tests for the src/luapp/as.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/as.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/ext-base.hpp"
#include "src/luapp/func-push.hpp"
#include "src/luapp/ruserdata.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::Catch::Matches;

struct A {};

struct Point {
  int x = 0;
  int y = 0;

  bool operator==( Point const& ) const = default;

  friend void lua_push( lua::cthread L, Point const& p ) {
    lua::c_api C( L );
    C.newtable();
    C.push( p.x );
    C.setfield( -2, "x" );
    C.push( p.y );
    C.setfield( -2, "y" );
  }

  friend base::maybe<Point> lua_get( lua::cthread L, int idx,
                                     lua::tag<Point> ) {
    lua::c_api C( L );
    if( C.type_of( idx ) != lua::type::table ) return nothing;
    C.getfield( idx, "x" );
    UNWRAP_RETURN( x, C.get<int>( -1 ) );
    C.pop();
    C.getfield( idx, "y" );
    UNWRAP_RETURN( y, C.get<int>( -1 ) );
    C.pop();
    return Point{ .x = x, .y = y };
  }
};

static_assert( nvalues_for<Point>() == 1 );

LUA_TEST_CASE( "[as] castability test" ) {
  using indexer_t = decltype( st["foo"] );

  // Things that should not be castable.
  static_assert( !Castable<indexer_t, string_view> );
  static_assert( !Castable<indexer_t, char const*> );
  static_assert( !Castable<indexer_t, A> );

  static_assert( !Castable<indexer_t, maybe<string_view>> );
  static_assert( !Castable<indexer_t, maybe<char const*>> );
  static_assert( !Castable<indexer_t, maybe<A>> );

  // Things that should be castable.
  static_assert( Castable<indexer_t, int> );
  static_assert( Castable<indexer_t, bool> );
  static_assert( Castable<indexer_t, double> );
  static_assert( Castable<indexer_t, lightuserdata> );
  static_assert( Castable<indexer_t, string> );
  static_assert( Castable<indexer_t, table> );
  static_assert( Castable<indexer_t, rfunction> );
  static_assert( Castable<indexer_t, userdata> );
  static_assert( Castable<indexer_t, rthread> );
  static_assert( Castable<indexer_t, Point> );

  static_assert( Castable<int, any> );
  static_assert( Castable<string, any> );
  static_assert( Castable<bool, any> );
  static_assert( Castable<double, any> );
  static_assert( Castable<lightuserdata, any> );
  static_assert( Castable<Point, any> );

  static_assert( Castable<indexer_t, maybe<int>> );
  static_assert( Castable<indexer_t, maybe<bool>> );
  static_assert( Castable<indexer_t, maybe<double>> );
  static_assert( Castable<indexer_t, maybe<lightuserdata>> );
  static_assert( Castable<indexer_t, maybe<string>> );
  static_assert( Castable<indexer_t, maybe<table>> );
  static_assert( Castable<indexer_t, maybe<rfunction>> );
  static_assert( Castable<indexer_t, maybe<userdata>> );
  static_assert( Castable<indexer_t, maybe<rthread>> );
  static_assert( Castable<indexer_t, maybe<Point>> );
}

LUA_TEST_CASE( "[as] cast to _" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    (void)as<int>( st["x"] );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    (void)as<string>( st["x"] );
    st["x"] = 5;
    (void)as<string>( st["x"] );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    (void)as<table>( st["x"] );
  }
}

LUA_TEST_CASE( "[as] cast with L" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    REQUIRE( as<int>( L, st["x"] ) == 5 );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    REQUIRE( as<string>( L, st["x"] ) == "hello" );
    st["x"] = 5;
    REQUIRE( as<string>( L, st["x"] ) == "5" );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    REQUIRE( as<table>( L, st["x"] ) == st["x"] );
  }
}

LUA_TEST_CASE( "[as] cast to maybe<_>" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    REQUIRE( as<maybe<int>>( st["x"] ) == 5 );
    REQUIRE( as<maybe<string>>( st["x"] ) == "5" );
    REQUIRE( as<maybe<table>>( st["x"] ) == nothing );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    REQUIRE( as<maybe<int>>( st["x"] ) == nothing );
    REQUIRE( as<maybe<string>>( st["x"] ) == "hello" );
    REQUIRE( as<maybe<table>>( st["x"] ) == nothing );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    table t = as<table>( st["x"] );
    REQUIRE( as<maybe<int>>( st["x"] ) == nothing );
    REQUIRE( as<maybe<string>>( st["x"] ) == nothing );
    REQUIRE( as<maybe<table>>( st["x"] ) == st["x"] );
    REQUIRE( as<maybe<table>>( st["x"] ) == t );
  }
}

LUA_TEST_CASE( "[as] safe_as" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    REQUIRE( safe_as<int>( st["x"] ) == 5 );
    REQUIRE( safe_as<string>( st["x"] ) == "5" );
    REQUIRE( safe_as<table>( st["x"] ) == nothing );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    REQUIRE( safe_as<int>( st["x"] ) == nothing );
    REQUIRE( safe_as<string>( st["x"] ) == "hello" );
    REQUIRE( safe_as<table>( st["x"] ) == nothing );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    table t = as<table>( st["x"] );
    REQUIRE( safe_as<int>( st["x"] ) == nothing );
    REQUIRE( safe_as<string>( st["x"] ) == nothing );
    REQUIRE( safe_as<table>( st["x"] ) == st["x"] );
    REQUIRE( safe_as<table>( st["x"] ) == t );
  }
}

LUA_TEST_CASE( "[as] Point" ) {
  st["point"] = Point{ .x = 3, .y = 5 };
  table t     = as<table>( st["point"] );
  REQUIRE( as<maybe<int>>( st["point"] ) == nothing );
  REQUIRE( as<maybe<string>>( st["point"] ) == nothing );
  REQUIRE( as<maybe<table>>( st["point"] ) == st["point"] );
  REQUIRE( as<maybe<table>>( st["point"] ) == t );

  // The test.
  REQUIRE( as<maybe<Point>>( st["point"] ) ==
           Point{ .x = 3, .y = 5 } );
  Point p = as<Point>( st["point"] );
  REQUIRE( p.x == 3 );
  REQUIRE( p.y == 5 );

  st["non-point"] = 5;
  REQUIRE( as<maybe<Point>>( st["non-point"] ) == nothing );
}

LUA_TEST_CASE( "[as] failed cast" ) {
  st["point"] = "hello";
  st["foo"]   = [&] { return as<Point>( st["point"] ); };

  lua_expect<Point> xp = st["foo"].pcall<Point>();
  REQUIRE( xp.has_error() );
  REQUIRE( C.stack_size() == 0 );

  static string regex = fmt::format(
      ".*:error: failed to convert Lua type `string' to native "
      "type `lua::\\(anonymous namespace\\)::Point'.*\n.*\n.*" );

  REQUIRE_THAT( xp.error(), Matches( regex ) );
}

} // namespace
} // namespace lua
