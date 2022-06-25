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
#include "expect.hpp"
#include "game-state.hpp"
#include "lua.hpp"
#include "map-updater.hpp"

// game-state
#include "gs/root.hpp"

// gfx
#include "gfx/coord.hpp"

// luapp
#include "luapp/as.hpp"
#include "luapp/error.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/rstring.hpp"
#include "luapp/state.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

using Catch::Contains;
using Catch::Equals;

Coord const kSquare{};

// This will preprare a 1x1 map with a grassland tile.
void prepare_world( TerrainState& terrain_state ) {
  NonRenderingMapUpdater map_updater( terrain_state );
  map_updater.modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    m          = Matrix<MapSquare>( Delta{ .w = 1, .h = 1 } );
    m[kSquare] = map_square_for_terrain( e_terrain::grassland );
  } );
}

TEST_CASE( "[lua] run trivial script" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    local x = 5+6
  )";
  REQUIRE( st.script.run_safe( script ) == valid );
}

TEST_CASE( "[lua] syntax error" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    local x =
  )";

  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error(), Contains( "unexpected symbol" ) );
}

TEST_CASE( "[lua] semantic error" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    local a, b
    local x = a + b
  )";

  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error(),
                Contains( "attempt to perform arithmetic" ) );
}

TEST_CASE( "[lua] has base lib" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    return tostring( 5 ) .. type( function() end )
  )";
  REQUIRE( st.script.run_safe<lua::rstring>( script ) ==
           "5function" );
}

TEST_CASE( "[lua] no implicit conversions from double to int" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    return 5+8.5
  )";
  REQUIRE( st.script.run_safe<maybe<int>>( script ) == nothing );
}

TEST_CASE( "[lua] returns double" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    return 5+8.5
  )";
  REQUIRE( st.script.run_safe<double>( script ) == 13.5 );
}

TEST_CASE( "[lua] returns string" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    local function f( s )
      return s .. '!'
    end
    return f( 'hello' )
  )";
  REQUIRE( st.script.run_safe<lua::rstring>( script ) ==
           "hello!" );
}

TEST_CASE( "[lua] has new_game.create" ) {
  lua::state& st = lua_global_state();
  lua_reload( GameState::root() );
  auto script = R"(
    return tostring( new_game.create )
  )";

  auto xp = st.script.run_safe<lua::rstring>( script );
  REQUIRE( xp.has_value() );
  REQUIRE_THAT( xp.value().as_cpp(), Contains( "function" ) );
}

TEST_CASE( "[lua] C++ function binding" ) {
  lua::state& st = lua_global_state();
  prepare_world( GameState::root().zzz_terrain );
  lua_reload( GameState::root() );
  auto script = R"(
    local soldier_type =
        unit_type.UnitType.create( "soldier" )
    local soldier_comp = unit_composer
                        .UnitComposition
                        .create_with_type_obj( soldier_type )
    local unit1 = ustate.create_unit_on_map( "dutch",
                                             soldier_comp,
                                             { x=0, y=0 } )
    local unit2 = ustate.create_unit_on_map( "dutch",
                                             soldier_comp,
                                             { x=0, y=0 } )
    local unit3 = ustate.create_unit_on_map( "dutch",
                                             soldier_comp,
                                             { x=0, y=0 } )
    return unit3:id()-unit1:id()
  )";

  REQUIRE( st.script.run_safe<int>( script ) == 2 );
}

TEST_CASE( "[lua] frozen globals" ) {
  lua::state& st = lua_global_state();
  auto        xp = st.script.run_safe( "new_game = 1" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT(
      xp.error(),
      Contains( "attempt to modify a read-only global" ) );

  xp = st.script.run_safe( "new_game.x = 1" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT(
      xp.error(),
      Contains( "attempt to modify a read-only table:" ) );

  xp = st.script.run_safe( "ustate = 1" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT(
      xp.error(),
      Contains( "attempt to modify a read-only global" ) );

  xp = st.script.run_safe( "ustate.x = 1" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT(
      xp.error(),
      Contains( "attempt to modify a read-only table:" ) );

  REQUIRE( st.script.run_safe<int>( "x = 1; return x" ) == 1 );

  REQUIRE( st.script.run_safe<int>(
               "d = {}; d.x = 1; return d.x" ) == 1 );
}

TEST_CASE( "[lua] rawset is locked down" ) {
  lua::state& st = lua_global_state();
  // `id` is locked down.
  auto xp = st.script.run_safe( "rawset( _ENV, 'id', 3 )" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error(), Contains( "nil value" ) );

  // `xxx` is not locked down, but rawset should fail for any
  // key.
  xp = st.script.run_safe( "rawset( _ENV, 'xxx', 3 )" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error(), Contains( "nil value" ) );
}

LUA_FN( throwing, int, int x ) {
  lua::cthread L = lua_global_state().thread.main().cthread();
  if( x >= 10 )
    lua::throw_lua_error(
        L, "x (which is {}) must be less than 10.", x );
  return x + 1;
};

TEST_CASE( "[lua] throwing" ) {
  lua::state& st     = lua_global_state();
  auto        script = "return testing.throwing( 5 )";
  REQUIRE( st.script.run_safe<int>( script ) == 6 );

  script  = "return testing.throwing( 11 )";
  auto xp = st.script.run_safe<int>( script );
  REQUIRE( !xp.has_value() );
  REQUIRE_THAT(
      xp.error(),
      Contains( "x (which is 11) must be less than 10." ) );
}

LUA_FN( coord_test, Coord, Coord const& coord ) {
  auto new_coord = coord;
  new_coord.x += 1;
  new_coord.y += 1;
  return new_coord;
}

TEST_CASE( "[lua] Coord" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    local coord = Coord{x=2, y=2}
    -- Test equality.
    assert_eq( coord, Coord{x=2,y=2} )
    -- Test tostring.
    assert_eq( tostring( coord ), "Coord{x=2,y=2}" )

    coord = testing.coord_test( coord )
    -- Test equality.
    assert_eq( coord, Coord{x=3,y=3} )
    -- Test tostring.
    assert_eq( tostring( coord ), "Coord{x=3,y=3}" )

    coord.x = coord.x + 1
    coord.y = coord.y + 2
    return coord
  )";
  REQUIRE( st.script.run_safe<Coord>( script ) ==
           Coord{ .x = 4, .y = 5 } );
}

LUA_FN( opt_test, maybe<string>, maybe<int> const& maybe_int ) {
  if( !maybe_int ) return "got nothing";
  int n = *maybe_int;
  if( n < 5 ) return nothing;
  if( n < 10 ) return "less than 10";
  return to_string( n );
}

LUA_FN( opt_test2, maybe<Coord>,
        maybe<Coord> const& maybe_coord ) {
  if( !maybe_coord ) return Coord{ .x = 5, .y = 7 };
  return Coord{ .x = maybe_coord->x + 1,
                .y = maybe_coord->y + 1 };
}

TEST_CASE( "[lua] optional" ) {
  lua::state& st = lua_global_state();
  // string/int
  auto script = R"(
    assert( testing.opt_test( nil ) == "got nothing"  )
    assert( testing.opt_test( 0   ) ==  nil           )
    assert( testing.opt_test( 4   ) ==  nil           )
    assert( testing.opt_test( 5   ) == "less than 10" )
    assert( testing.opt_test( 9   ) == "less than 10" )
    assert( testing.opt_test( 10  ) == "10"           )
    assert( testing.opt_test( 100 ) == "100"          )
  )";
  REQUIRE( st.script.run_safe( script ) == valid );

  REQUIRE( st.script.run_safe<maybe<string>>( "return nil" ) ==
           nothing );
  REQUIRE( st.script.run_safe<maybe<string>>(
               "return 'hello'" ) == "hello" );
  REQUIRE( st.script.run_safe<maybe<int>>( "return 'hello'" ) ==
           nothing );

  // Coord
  auto script2 = R"(
    assert( testing.opt_test2( nil            ) == Coord{x=5,y=7} )
    assert( testing.opt_test2( Coord{x=2,y=3} ) == Coord{x=3,y=4} )
  )";
  REQUIRE( st.script.run_safe( script2 ) == valid );

  REQUIRE( st.script.run_safe<maybe<Coord>>( "return nil" ) ==
           nothing );
  REQUIRE( st.script.run_safe<maybe<Coord>>(
               "return Coord{x=9, y=8}" ) ==
           Coord{ .x = 9, .y = 8 } );
  REQUIRE( st.script.run_safe<maybe<Coord>>(
               "return 'hello'" ) == nothing );
  REQUIRE( st.script.run_safe<maybe<Coord>>( "return 5" ) ==
           nothing );
}

// Test the o.as<maybe<?>>() constructs. This tests the custom
// handlers that we've defined for maybe<>.
TEST_CASE( "[lua] get as maybe" ) {
  lua::state& st = lua_global_state();
  st["func"]     = []( lua::any o ) -> string {
    if( o == lua::nil ) return "nil";
    if( lua::type_of( o ) == lua::type::string ) {
          return lua::as<string>( o ) + "!";
    } else if( auto maybe_double = lua::as<maybe<double>>( o );
               maybe_double.has_value() ) {
          return fmt::format( "a double: {}", *maybe_double );
    } else if( auto maybe_bool = lua::as<maybe<bool>>( o );
               maybe_bool.has_value() ) {
          return fmt::format( "a bool: {}", *maybe_bool );
    } else {
          return "?";
    }
  };
  REQUIRE( lua::as<string>( st["func"]( "hello" ) ) ==
           "hello!" );
  REQUIRE( lua::as<string>( st["func"]( 5 ) ) == "a double: 5" );
  REQUIRE( lua::as<string>( st["func"]( true ) ) ==
           "a bool: true" );

  REQUIRE( lua::as<maybe<string>>( st["func"]( false ) ) ==
           "a bool: false" );
}

TEST_CASE( "[lua] new_usertype" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    u = MyType.new()
    assert( u.x == 5 )
    assert( u:get() == "c" )
    assert( u:add( 4, 5 ) == 9+5 )
  )";
  REQUIRE( st.script.run_safe( script ) == valid );
}

TEST_CASE( "[lua] run lua tests" ) {
  // This needs to be a standalone clean state since we don't
  // want e.g. globals to be frozen or other things that the game
  // normally does to the Lua environment. Also, these tests are
  // intended to be standalone tests that don't need the game en-
  // vironment to be run; in fact they can be run using the
  // normal `lua` command line tool. We're running them here just
  // to make sure that they get run regularly.
  lua::state st;
  st.lib.open_all();
  auto script = R"(
    package.path = 'src/lua/?.lua'
    local main = require( 'test.runner' )
    main( true )
  )";
  REQUIRE( st.script.run_safe( script ) == valid );
}

} // namespace
