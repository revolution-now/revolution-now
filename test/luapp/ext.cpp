/****************************************************************
**ext.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Unit tests for the src/luapp/ext.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/ext.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/c-api.hpp"
#include "src/luapp/thing.hpp"

// Must be last.
#include "test/catch-common.hpp"

// FMT_TO_CATCH( ::rn::UnitId );

FMT_TO_CATCH( ::lua::type );

namespace my_ns {

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
    CHECK( C.type_of( idx ) == lua::type::table );
    C.getfield( idx, "x" );
    UNWRAP_RETURN( x, C.get<int>( -1 ) );
    C.pop();
    C.getfield( idx, "y" );
    UNWRAP_RETURN( y, C.get<int>( -1 ) );
    C.pop();
    return Point{ .x = x, .y = y };
  }
};

} // namespace my_ns

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::my_ns::Point;

LUA_TEST_CASE( "[ext] Point" ) {
  C.openlibs();

  // push.
  Point p{ .x = 4, .y = 5 };
  push( L, p );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::table );
  C.setglobal( "my_point" );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( st["my_point"]["x"] == 4 );
  REQUIRE( st["my_point"]["y"] == 5 );

  thing th( L, Point{ .x = 1, .y = 2 } );
  REQUIRE( th.as<table>()["x"] == 1 );
  REQUIRE( th.as<table>()["y"] == 2 );

  // get.
  table lua_point = st.table.create();
  lua_point["x"]  = 7;
  lua_point["y"]  = 8;
  push( L, lua_point );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::table );
  maybe<Point> m = get<Point>( L, -1 );
  REQUIRE( m.has_value() );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( m->x == 7 );
  REQUIRE( m->y == 8 );
}

} // namespace
} // namespace lua
