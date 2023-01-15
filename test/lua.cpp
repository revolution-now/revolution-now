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

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "expect.hpp"
#include "imap-updater.hpp"
#include "lua.hpp"

// ss
#include "ss/root.hpp"

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

namespace rn {
namespace {

using namespace std;

using Catch::Contains;
using Catch::Equals;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[lua] run trivial script" ) {
  lua::state st;
  auto       script = R"(
    local x = 5+6
  )";
  REQUIRE( st.script.run_safe( script ) == valid );
}

TEST_CASE( "[lua] syntax error" ) {
  lua::state st;
  auto       script = R"(
    local x =
  )";

  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error(), Contains( "unexpected symbol" ) );
}

TEST_CASE( "[lua] semantic error" ) {
  lua::state st;
  auto       script = R"(
    local a, b
    local x = a + b
  )";

  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error(),
                Contains( "attempt to perform arithmetic" ) );
}

TEST_CASE( "[lua] has base lib" ) {
  lua::state st;
  st.lib.open_all();
  auto script = R"(
    return tostring( 5 ) .. type( function() end )
  )";
  REQUIRE( st.script.run_safe<lua::rstring>( script ) ==
           "5function" );
}

TEST_CASE( "[lua] no implicit conversions from double to int" ) {
  lua::state st;
  auto       script = R"(
    return 5+8.5
  )";
  REQUIRE( st.script.run_safe<maybe<int>>( script ) == nothing );
}

TEST_CASE( "[lua] returns double" ) {
  lua::state st;
  auto       script = R"(
    return 5+8.5
  )";
  REQUIRE( st.script.run_safe<double>( script ) == 13.5 );
}

TEST_CASE( "[lua] returns string" ) {
  lua::state st;
  auto       script = R"(
    local function f( s )
      return s .. '!'
    end
    return f( 'hello' )
  )";
  REQUIRE( st.script.run_safe<lua::rstring>( script ) ==
           "hello!" );
}

LUA_FN( throwing, int, int x ) {
  lua::cthread L = st.thread.main().cthread();
  if( x >= 10 )
    lua::throw_lua_error(
        L, "x (which is {}) must be less than 10.", x );
  return x + 1;
};

LUA_FN( coord_test, Coord, Coord const& coord ) {
  auto new_coord = coord;
  new_coord.x += 1;
  new_coord.y += 1;
  return new_coord;
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

TEST_CASE( "[lua] after initialization" ) {
  World W;
  W.expensive_run_lua_init();
  // Cause TS to initialize lua since this test happens to need
  // the "TS" global in place.
  W.initialize_ts();
  lua::state& st = W.lua();

  // Function binding.
  {
    auto script = R"(
      assert( type( unit_mgr.create_unit_on_map ) == 'function' )
      local soldier_type =
          unit_type.UnitType.create( "soldier" )
      local soldier_comp = unit_composer
                          .UnitComposition
                          .create_with_type_obj( soldier_type )
      local unit1 = unit_mgr.create_unit_on_map( "dutch",
                                                 soldier_comp,
                                                 { x=0, y=0 } )
      local unit2 = unit_mgr.create_unit_on_map( "dutch",
                                                 soldier_comp,
                                                 { x=0, y=0 } )
      local unit3 = unit_mgr.create_unit_on_map( "dutch",
                                                 soldier_comp,
                                                 { x=0, y=0 } )
      return unit3:id()-unit1:id()
    )";

    REQUIRE( W.lua().script.run_safe<int>( script ) == 2 );
  }

  // FIXME: re-enable this once the TS mechanism is cleaned up.
  // The below snippet will catch when we re-enable the freezing
  // so that we don't forget to re-enable the below.
  {
    auto xp = st.script.run_safe( "new_game = 1" );
    REQUIRE( xp.valid() );
  }
#if 0
  freeze_globals( st );

  // Frozen globals.
  {
    auto xp = st.script.run_safe( "new_game = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error(),
        Contains( "attempt to modify a read-only global" ) );

    xp = st.script.run_safe( "new_game.x = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error(),
        Contains( "attempt to modify a read-only table:" ) );

    xp = st.script.run_safe( "unit_mgr = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error(),
        Contains( "attempt to modify a read-only global" ) );

    xp = st.script.run_safe( "unit_mgr.x = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error(),
        Contains( "attempt to modify a read-only table:" ) );

    REQUIRE( st.script.run_safe<int>( "x = 1; return x" ) == 1 );

    REQUIRE( st.script.run_safe<int>(
                 "d = {}; d.x = 1; return d.x" ) == 1 );
  }
#endif

  // Throwing.
  {
    auto script = "return testing.throwing( 5 )";
    REQUIRE( st.script.run_safe<int>( script ) == 6 );

    script  = "return testing.throwing( 11 )";
    auto xp = st.script.run_safe<int>( script );
    REQUIRE( !xp.has_value() );
    REQUIRE_THAT(
        xp.error(),
        Contains( "x (which is 11) must be less than 10." ) );
  }

  // optional
  {
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
    REQUIRE( st.script.run_safe<maybe<int>>(
                 "return 'hello'" ) == nothing );

    // Coord
    REQUIRE( st.script.run_safe<maybe<Coord>>( "return nil" ) ==
             nothing );
    REQUIRE( st.script.run_safe<maybe<Coord>>(
                 "return {x=9, y=8}" ) ==
             Coord{ .x = 9, .y = 8 } );
    REQUIRE( st.script.run_safe<maybe<Coord>>(
                 "return 'hello'" ) == nothing );
    REQUIRE( st.script.run_safe<maybe<Coord>>( "return 5" ) ==
             nothing );
  }
}

TEST_CASE( "[lua] rawset is locked down" ) {
  lua::state st;
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

// Test the o.as<maybe<?>>() constructs. This tests the custom
// handlers that we've defined for maybe<>.
TEST_CASE( "[lua] get as maybe" ) {
  lua::state st;
  st["func"] = []( lua::any o ) -> string {
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
} // namespace rn
