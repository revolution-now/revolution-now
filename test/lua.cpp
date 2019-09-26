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

#define LUA_MODULE_NAME_OVERRIDE "testing"

// Revolution Now
#include "lua.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

using Catch::Contains;
using Catch::Equals;

TEST_CASE( "[lua] run trivial script" ) {
  auto script = R"(
    local x = 5+6
  )";
  REQUIRE( lua::run<void>( script ) == monostate{} );
}

TEST_CASE( "[lua] syntax error" ) {
  auto script = R"(
    local x =
  )";

  auto xp = lua::run<void>( script );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT( xp.error().what,
                Contains( "unexpected symbol" ) );
}

TEST_CASE( "[lua] semantic error" ) {
  auto script = R"(
    local a, b
    local x = a + b
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
    local function f( s )
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
  REQUIRE_THAT( xp.error().what,
                Contains( "modify a read-only table" ) );
}

TEST_CASE( "[lua] enums from string" ) {
  auto script = R"(
    return e.nation.dutch == e.nation_from_string( "dutch" )
  )";

  REQUIRE( lua::run<bool>( script ) == true );
}

TEST_CASE( "[lua] has startup.run" ) {
  lua::reload();
  auto script = R"(
    return tostring( startup.main )
  )";

  auto xp = lua::run<string>( script );
  REQUIRE( xp.has_value() );
  REQUIRE_THAT( xp.value(), Contains( "function" ) );
}

TEST_CASE( "[lua] C++ function binding" ) {
  lua::reload();
  auto script = R"(
    local id1 = europort.create_unit_in_port( e.nation.dutch, e.unit_type.soldier )
    local id2 = europort.create_unit_in_port( e.nation.dutch, e.unit_type.soldier )
    local id3 = europort.create_unit_in_port( e.nation.dutch, e.unit_type.soldier )
    return id3-id1
  )";

  REQUIRE( lua::run<int>( script ) == 2 );
}

TEST_CASE( "[lua] frozen globals" ) {
  auto xp = lua::run<void>( "e = 1" );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT(
      xp.error().what,
      Contains( "attempt to modify a read-only global." ) );

  xp = lua::run<void>( "startup = 1" );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT(
      xp.error().what,
      Contains( "attempt to modify a read-only global." ) );

  xp = lua::run<void>( "startup.x = 1" );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT(
      xp.error().what,
      Contains( "attempt to modify a read-only table." ) );

  xp = lua::run<void>( "id = 1" );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT(
      xp.error().what,
      Contains( "attempt to modify a read-only global." ) );

  xp = lua::run<void>( "id.x = 1" );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT(
      xp.error().what,
      Contains( "attempt to modify a read-only table." ) );

  REQUIRE( lua::run<int>( "x = 1; return x" ) == 1 );

  REQUIRE( lua::run<int>( "d = {}; d.x = 1; return d.x" ) == 1 );
}

TEST_CASE( "[lua] has modules" ) {
  auto script = R"lua(
    assert( modules['startup'] ~= nil )
    assert( modules['util']    ~= nil )
    assert( modules['meta']    ~= nil )
    assert( modules['utype']   ~= nil )
  )lua";
  REQUIRE( lua::run<void>( script ) == monostate{} );
}

LUA_FN( check_failure, int, int x ) {
  RN_CHECK( x < 10, "x (which is {}) must be less than 10.", x );
  return x + 1;
};

TEST_CASE( "[lua] check-failure" ) {
  auto script = "return testing.check_failure( 5 )";
  REQUIRE( lua::run<int>( script ) == 6 );

  script  = "return testing.check_failure( 11 )";
  auto xp = lua::run<int>( script );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT(
      xp.error().what,
      Contains( "x (which is 11) must be less than 10." ) );
}

LUA_FN( coord_test, Coord, Coord const& coord ) {
  auto new_coord = coord;
  new_coord.x += 1_w;
  new_coord.y += 1_h;
  return new_coord;
}

TEST_CASE( "[lua] Coord" ) {
  auto script = R"(
    local coord = Coord{x=2, y=2}
    coord = testing.coord_test( coord )
    coord.x = coord.x + 1
    coord.y = coord.y + 2
    return coord
  )";
  REQUIRE( lua::run<Coord>( script ) == Coord{4_x, 5_y} );
}

LUA_FN( opt_test, Opt<string>, Opt<int> const& maybe_int ) {
  if( !maybe_int ) return "got nullopt";
  int n = *maybe_int;
  if( n < 5 ) return nullopt;
  if( n < 10 ) return "less than 10";
  return to_string( n );
}

TEST_CASE( "[lua] optional" ) {
  auto script = R"(
    assert( testing.opt_test( nil ) == "got nullopt" )
    assert( testing.opt_test( 0 ) == nil )
    assert( testing.opt_test( 4 ) == nil )
    assert( testing.opt_test( 5 ) == "less than 10" )
    assert( testing.opt_test( 9 ) == "less than 10" )
    assert( testing.opt_test( 10 ) == "10" )
    assert( testing.opt_test( 100 ) == "100" )
  )";
  REQUIRE( lua::run<void>( script ) == monostate{} );

  REQUIRE( lua::run<Opt<string>>( "return nil" ) == nullopt );
  REQUIRE( lua::run<Opt<string>>( "return 'hello'" ) ==
           "hello" );
}

TEST_CASE( "[lua] new_usertype" ) {
  auto script = R"(
    u = MyType.new()
    assert( u.x == 5 )
    assert( u:get() == "c" )
    assert( u:add( 4, 5 ) == 9 )
  )";
  REQUIRE( lua::run<void>( script ) == monostate{} );
}

} // namespace
