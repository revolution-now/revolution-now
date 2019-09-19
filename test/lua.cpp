/****************************************************************
**lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-18.
*
* Description: Unit tests for lua integration.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "lua-ext.hpp"
#include "lua.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

using Catch::Contains;

TEST_CASE( "[lua] run trivial script" ) {
  auto script = R"(
    x = 5+6
  )";
  REQUIRE( lua::run<void>( script ).has_value() );
}

TEST_CASE( "[lua] syntax error" ) {
  auto script = R"(
    x =
  )";

  auto xp = lua::run<void>( script );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT( xp.error().what,
                Contains( "unexpected symbol" ) );
}

TEST_CASE( "[lua] semantic error" ) {
  auto script = R"(
    x = a + b
  )";

  auto xp = lua::run<void>( script );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT( xp.error().what,
                Contains( "attempt to perform arithmetic" ) );
}

TEST_CASE( "[lua] has base lib" ) {
  auto script = R"(
    return tostring( 5 ) .. type( function() end )
  )";
  REQUIRE( lua::run<string>( script ) == "5function" );
}

TEST_CASE( "[lua] returns int" ) {
  auto script = R"(
    return 5+8.5
  )";
  REQUIRE( lua::run<int>( script ) == 13 );
}

TEST_CASE( "[lua] returns double" ) {
  auto script = R"(
    return 5+8.5
  )";
  REQUIRE( lua::run<double>( script ) == 13.5 );
}

TEST_CASE( "[lua] returns string" ) {
  auto script = R"(
    function f( s )
      return s .. '!'
    end
    return f( 'hello' )
  )";
  REQUIRE( lua::run<string>( script ) == "hello!" );
}

TEST_CASE( "[lua] enums exist" ) {
  auto script = R"(
    return tostring( e.nation.dutch ) .. type( e.nation.dutch )
  )";
  REQUIRE( lua::run<string>( script ) == "dutchuserdata" );
}

TEST_CASE( "[lua] enums no assign" ) {
  auto script = R"(
    e.nation.dutch = 3
  )";

  auto xp = lua::run<void>( script );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT( xp.error().what, Contains( "cannot modify" ) );
}

TEST_CASE( "[lua] enums from string" ) {
  auto script = R"(
    return e.nation.dutch == e.nation_from_string( "dutch" )
  )";

  REQUIRE( lua::run<bool>( script ) == true );
}

TEST_CASE( "[lua] load modules" ) {
  lua::reset_state();
  auto script = R"(
    return tostring( startup.run )
  )";

  auto xp = lua::run<string>( script );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT( xp.error().what,
                Contains( "attempt to index a nil value" ) );

  REQUIRE_NOTHROW( lua::load_modules() );

  xp = lua::run<string>( script );
  REQUIRE( xp.has_value() );
  REQUIRE_THAT( xp.value(), Contains( "function" ) );
}

TEST_CASE( "[lua] C++ function binding" ) {
  lua::reset_state();
  auto script = R"(
    id1 = europort.create_unit_in_port( e.nation.dutch, e.unit_type.soldier )
    id2 = europort.create_unit_in_port( e.nation.dutch, e.unit_type.soldier )
    id3 = europort.create_unit_in_port( e.nation.dutch, e.unit_type.soldier )
    return id3-id1
  )";

  REQUIRE( lua::run<int>( script ) == 2 );
}

LUA_FN( testing, coord_test, Coord, Coord const& coord ) {
  auto new_coord = coord;
  new_coord.x += 1_w;
  new_coord.y += 1_h;
  return new_coord;
}

TEST_CASE( "[lua] Coord" ) {
  auto script = R"(
    coord = {x=2, y=2}
    coord = testing.coord_test( coord )
    return coord
  )";
  REQUIRE( lua::run<Coord>( script ) == Coord{3_x, 3_y} );
}

} // namespace
