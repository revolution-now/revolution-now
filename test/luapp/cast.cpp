/****************************************************************
**cast.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-19.
*
* Description: Unit tests for the src/luapp/cast.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/cast.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/ext-base.hpp"
#include "src/luapp/func-push.hpp"
#include "src/luapp/thing.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

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

  bool operator<=>( Point const& ) const = default;

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

} // namespace
} // namespace lua

DEFINE_FORMAT( ::lua::Point, "Point{{x={},y={}}}", o.x, o.y );
FMT_TO_CATCH( ::lua::Point );

namespace lua {
namespace {

LUA_TEST_CASE( "[cast] castability test" ) {
  using indexer_t = decltype( st["foo"] );

  // Things that should not be castable.
  static_assert( !Castable<indexer_t, any> );
  static_assert( !Castable<indexer_t, thing> );
  static_assert( !Castable<indexer_t, string_view> );
  static_assert( !Castable<indexer_t, char const*> );
  static_assert( !Castable<indexer_t, rthread> );
  static_assert( !Castable<indexer_t, A> );
  static_assert( !Castable<indexer_t, userdata> );

  static_assert( !Castable<indexer_t, maybe<any>> );
  static_assert( !Castable<indexer_t, maybe<thing>> );
  static_assert( !Castable<indexer_t, maybe<string_view>> );
  static_assert( !Castable<indexer_t, maybe<char const*>> );
  static_assert( !Castable<indexer_t, maybe<rthread>> );
  static_assert( !Castable<indexer_t, maybe<A>> );
  static_assert( !Castable<indexer_t, maybe<userdata>> );

  // Things that should be castable.
  static_assert( Castable<indexer_t, int> );
  static_assert( Castable<indexer_t, bool> );
  static_assert( Castable<indexer_t, double> );
  static_assert( Castable<indexer_t, lightuserdata> );
  static_assert( Castable<indexer_t, string> );
  static_assert( Castable<indexer_t, table> );
  static_assert( Castable<indexer_t, rfunction> );
  static_assert( Castable<indexer_t, Point> );

  static_assert( Castable<indexer_t, maybe<int>> );
  static_assert( Castable<indexer_t, maybe<bool>> );
  static_assert( Castable<indexer_t, maybe<double>> );
  static_assert( Castable<indexer_t, maybe<lightuserdata>> );
  static_assert( Castable<indexer_t, maybe<string>> );
  static_assert( Castable<indexer_t, maybe<table>> );
  static_assert( Castable<indexer_t, maybe<rfunction>> );
  static_assert( Castable<indexer_t, maybe<Point>> );
}

LUA_TEST_CASE( "[cast] cast to _" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    (void)cast<int>( st["x"] );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    (void)cast<string>( st["x"] );
    st["x"] = 5;
    (void)cast<string>( st["x"] );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    (void)cast<table>( st["x"] );
  }
}

LUA_TEST_CASE( "[cast] cast to maybe<_>" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    REQUIRE( cast<maybe<int>>( st["x"] ) == 5 );
    REQUIRE( cast<maybe<string>>( st["x"] ) == "5" );
    REQUIRE( cast<maybe<table>>( st["x"] ) == nothing );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    REQUIRE( cast<maybe<int>>( st["x"] ) == nothing );
    REQUIRE( cast<maybe<string>>( st["x"] ) == "hello" );
    REQUIRE( cast<maybe<table>>( st["x"] ) == nothing );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    table t = cast<table>( st["x"] );
    REQUIRE( cast<maybe<int>>( st["x"] ) == nothing );
    REQUIRE( cast<maybe<string>>( st["x"] ) == nothing );
    REQUIRE( cast<maybe<table>>( st["x"] ) == st["x"] );
    REQUIRE( cast<maybe<table>>( st["x"] ) == t );
  }
}

LUA_TEST_CASE( "[cast] safe_cast" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    REQUIRE( safe_cast<int>( st["x"] ) == 5 );
    REQUIRE( safe_cast<string>( st["x"] ) == "5" );
    REQUIRE( safe_cast<table>( st["x"] ) == nothing );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    REQUIRE( safe_cast<int>( st["x"] ) == nothing );
    REQUIRE( safe_cast<string>( st["x"] ) == "hello" );
    REQUIRE( safe_cast<table>( st["x"] ) == nothing );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    table t = cast<table>( st["x"] );
    REQUIRE( safe_cast<int>( st["x"] ) == nothing );
    REQUIRE( safe_cast<string>( st["x"] ) == nothing );
    REQUIRE( safe_cast<table>( st["x"] ) == st["x"] );
    REQUIRE( safe_cast<table>( st["x"] ) == t );
  }
}

LUA_TEST_CASE( "[cast] Point" ) {
  st["point"] = Point{ .x = 3, .y = 5 };
  table t     = cast<table>( st["point"] );
  REQUIRE( cast<maybe<int>>( st["point"] ) == nothing );
  REQUIRE( cast<maybe<string>>( st["point"] ) == nothing );
  REQUIRE( cast<maybe<table>>( st["point"] ) == st["point"] );
  REQUIRE( cast<maybe<table>>( st["point"] ) == t );

  // The test.
  REQUIRE( cast<maybe<Point>>( st["point"] ) ==
           Point{ .x = 3, .y = 5 } );
  Point p = cast<Point>( st["point"] );
  REQUIRE( p.x == 3 );
  REQUIRE( p.y == 5 );

  st["non-point"] = 5;
  REQUIRE( cast<maybe<Point>>( st["non-point"] ) == nothing );
}

LUA_TEST_CASE( "[cast] failed cast" ) {
  st["point"] = "hello";
  st["foo"]   = [&] { return cast<Point>( st["point"] ); };

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
