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
#include "src/expect.hpp"
#include "src/imap-updater.hpp"
#include "src/lua.hpp"

// ss
#include "src/ss/root.hpp"

// gfx
#include "src/gfx/coord.hpp"

// luapp
#include "src/luapp/as.hpp"
#include "src/luapp/error.hpp"
#include "src/luapp/ext-base.hpp"
#include "src/luapp/register.hpp"
#include "src/luapp/rstring.hpp"
#include "src/luapp/state.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::Catch::Contains;
using ::Catch::Equals;
using ::Catch::Matches;
using ::lua::lua_expect;
using ::lua::unexpected;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_player::dutch );
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[lua] run trivial script" ) {
  lua::state st;
  auto script = R"lua(
    local x = 5+6
  )lua";
  REQUIRE( st.script.run_safe( script ) == valid );
}

TEST_CASE( "[lua] syntax error" ) {
  lua::state st;
  auto script = R"lua(
    local x =
  )lua";

  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error().msg,
                Contains( "unexpected symbol" ) );
}

TEST_CASE( "[lua] semantic error" ) {
  lua::state st;
  auto script = R"lua(
    local a, b
    local x = a + b
  )lua";

  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error().msg,
                Contains( "attempt to perform arithmetic" ) );
}

TEST_CASE( "[lua] has base lib" ) {
  lua::state st;
  st.lib.open_all();
  auto script           = R"lua(
    return tostring( 5 ) .. type( function() end )
  )lua";
  lua::rstring const rs = st.string.create( "5function" );
  REQUIRE( st.script.run_safe<lua::rstring>( script ) == rs );
}

TEST_CASE( "[lua] no implicit conversions from double to int" ) {
  lua::state st;
  auto script = R"lua(
    return 5+8.5
  )lua";
  REQUIRE( st.script.run_safe<int>( script ) ==
           unexpected{
             .msg = "native code expected type `int' as a "
                    "return value (which requires 1 Lua value), "
                    "but the values returned by Lua were not "
                    "convertible to that native type.  The Lua "
                    "values received were: [number]." } );
}

TEST_CASE( "[lua] returns double" ) {
  lua::state st;
  auto script = R"lua(
    return 5+8.5
  )lua";
  REQUIRE( st.script.run_safe<double>( script ) == 13.5 );
}

TEST_CASE( "[lua] returns string" ) {
  lua::state st;
  auto script = R"lua(
    local function f( s )
      return s .. '!'
    end
    return f( 'hello' )
  )lua";
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

LUA_FN( opt_test, lua_expect<string>,
        lua_expect<int> const& maybe_int ) {
  if( !maybe_int ) return "got nothing";
  int n = *maybe_int;
  if( n < 5 ) return unexpected{};
  if( n < 10 ) return "less than 10";
  return to_string( n );
}

LUA_FN( opt_test2, lua_expect<Coord>,
        lua_expect<Coord> const& maybe_coord ) {
  if( !maybe_coord ) return Coord{ .x = 5, .y = 7 };
  return Coord{ .x = maybe_coord->x + 1,
                .y = maybe_coord->y + 1 };
}

TEST_CASE( "[lua] after initialization" ) {
  World W;
  W.expensive_run_lua_init();
  lua::state& st = W.lua();

  // Configs injected.
  {
    auto script = R"lua(
      assert( config.nation
                .players
                .spanish
                .flag_color
                 == "#FFFE54FF" )
      assert( config.rn
                .power
                .time_till_slow_fps
                 == 60 )
      assert( config.revolution
                .ref_forces
                .initial_forces
                .governor
                .cavalry
                 == 20 )
    )lua";

    REQUIRE( W.lua().script.run_safe( script ) == valid );
  }

  // Configs hardened.
  {
    auto script = R"lua(
      local ok = config.nation.players.spanish
      local bad_get = function()
        return config.nation.players.spanishx
      end
      assert( not pcall( bad_get ),
             'no error thrown on invalid field get.' )
      bad_get = function()
        config.nation.players.spanish = 5
      end
      assert( not pcall( bad_get ),
             'no error thrown on field set.' )
    )lua";

    REQUIRE( W.lua().script.run_safe( script ) == valid );
  }

  // More frozen globals.
  {
    auto xp = st.script.run_safe( "new_game = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only global" ) );

    xp = st.script.run_safe( "new_game.x = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only table" ) );

    xp = st.script.run_safe( "unit_mgr = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only global" ) );

    xp = st.script.run_safe( "unit_mgr.x = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only table" ) );

    REQUIRE( st.script.run_safe<int>( "x = 1; return x" ) == 1 );

    REQUIRE( st.script.run_safe<int>(
                 "d = {}; d.x = 1; return d.x" ) == 1 );

    xp = st.script.run_safe( "unit_composition = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only global" ) );

    xp = st.script.run_safe( "unit_composition.x = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only table" ) );

    xp = st.script.run_safe(
        "unit_composition.UnitComposition = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only table" ) );

    xp = st.script.run_safe(
        "unit_composition.UnitComposition.x = 1" );
    REQUIRE( !xp.valid() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "attempt to modify a read-only table" ) );
  }

  // Function binding.
  {
    auto script = R"lua(
      assert( type( unit_mgr.create_unit_on_map ) == 'function' )
      local soldier_type =
          unit_type.UnitType.create( "soldier" )
      local soldier_comp = unit_composition
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
    )lua";

    REQUIRE( W.lua().script.run_safe<int>( script ) == 2 );
  }

  // Throwing.
  {
    auto script = "return testing.throwing( 5 )";
    REQUIRE( st.script.run_safe<int>( script ) == 6 );

    script  = "return testing.throwing( 11 )";
    auto xp = st.script.run_safe<int>( script );
    REQUIRE( !xp.has_value() );
    REQUIRE_THAT(
        xp.error().msg,
        Contains( "x (which is 11) must be less than 10." ) );
  }

  // optional
  {
    // string/int
    auto script = R"lua(
      assert( testing.opt_test( nil ) == "got nothing"  )
      assert( testing.opt_test( 0   ) ==  nil           )
      assert( testing.opt_test( 4   ) ==  nil           )
      assert( testing.opt_test( 5   ) == "less than 10" )
      assert( testing.opt_test( 9   ) == "less than 10" )
      assert( testing.opt_test( 10  ) == "10"           )
      assert( testing.opt_test( 100 ) == "100"          )
    )lua";
    REQUIRE( st.script.run_safe( script ) == valid );

    REQUIRE_THAT(
        st.script.run_safe<string>( "return nil" ).error().msg,
        Matches( "native code expected type.*string.* as a "
                 "return value \\(which requires 1 Lua "
                 "value\\), but the values returned by Lua were "
                 "not convertible to that native type\\.  The "
                 "Lua values received were: \\[nil\\]\\." ) );

    REQUIRE( st.script.run_safe<lua_expect<string>>(
                 "return 'hello'" ) == "hello" );

    REQUIRE(
        st.script.run_safe<int>( "return 'hello'" ) ==
        unexpected{
          .msg = "native code expected type `int' as a return "
                 "value (which requires 1 Lua value), but the "
                 "values returned by Lua were not convertible "
                 "to that native type.  The Lua values received "
                 "were: [string]." } );

    // Coord
    REQUIRE(
        st.script.run_safe<Coord>( "return nil" ).has_error() );
    REQUIRE( st.script.run_safe<Coord>( "return {x=9, y=8}" ) ==
             Coord{ .x = 9, .y = 8 } );
    REQUIRE( st.script.run_safe<Coord>( "return 'hello'" )
                 .has_error() );
    REQUIRE(
        st.script.run_safe<Coord>( "return 5" ).has_error() );
  }
}

TEST_CASE( "[lua] rawset is locked down" ) {
  lua::state st;
  // `id` is locked down.
  auto xp = st.script.run_safe( "rawset( _ENV, 'id', 3 )" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error().msg, Contains( "nil value" ) );

  // `xxx` is not locked down, but rawset should fail for any
  // key.
  xp = st.script.run_safe( "rawset( _ENV, 'xxx', 3 )" );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error().msg, Contains( "nil value" ) );
}

// Test the o.as<lua_expect<?>>() constructs. This tests the
// custom handlers that we've defined for lua_expect<>.
TEST_CASE( "[lua] get as lua_expect" ) {
  lua::state st;
  st["func"] = []( lua::any o ) -> string {
    if( o == lua::nil ) return "nil";
    if( lua::type_of( o ) == lua::type::string ) {
      return lua::as<string>( o ) + "!";
    } else if( auto maybe_double =
                   lua::as<lua_expect<double>>( o );
               maybe_double.has_value() ) {
      return fmt::format( "a double: {}", *maybe_double );
    } else if( auto maybe_bool = lua::as<lua_expect<bool>>( o );
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

  REQUIRE( lua::as<lua_expect<string>>( st["func"]( false ) ) ==
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
  auto script = R"lua(
    package.path = 'src/lua/?.lua'
    local main = require( 'test.runner' )
    main( true )
  )lua";
  REQUIRE( st.script.run_safe( script ) == valid );
}

} // namespace
} // namespace rn
