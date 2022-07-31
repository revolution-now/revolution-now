/****************************************************************
**terminal.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-25.
*
* Description: Unit tests for the terminal module.
*
*****************************************************************/
#include "testing.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/lua.hpp"
#include "src/terminal.hpp"

// luapp
#include "src/luapp/state.hpp"

// Must be last.
#include "test/catch-common.hpp"

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
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[terminal] autocomplete" ) {
  lua::state st;
  // NOTE: this is expensive, but this test currently needs it.
  lua_init( st );
  Terminal term( st );

  auto autocomplete = [&]( string_view in ) {
    return term.autocomplete( in );
  };

  auto empty = vector<string>{};

  string         in;
  vector<string> out;

  in = "";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in = "xgiebg";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in  = "usta";
  out = vector<string>{ "ustate" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ustate";
  out = vector<string>{ "ustate." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ustate.";
  out = vector<string>{ "ustate.add_unit_to_cargo",
                        "ustate.create_unit_in_cargo",
                        "ustate.create_unit_on_map" };
  REQUIRE_THAT( autocomplete( in ), Contains( out ) );

  in  = "ustate.add_un";
  out = vector<string>{ "ustate.add_unit_to_cargo" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ustate.create_unit_in_cargo";
  out = vector<string>{ "ustate.create_unit_in_cargo(" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in = "ustate.create_unit_on_map(";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in  = "ustate.create_unit_on_map( usta";
  out = vector<string>{ "ustate.create_unit_on_map( ustate" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ustate.create_unit_on_map( ustate";
  out = vector<string>{ "ustate.create_unit_on_map( ustate." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "uni";
  out = { "unit_" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_";
  out = { "unit_composer", "unit_type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_t";
  out = { "unit_type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ustate";
  out = { "ustate." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_";
  out = { "map_gen" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_gen";
  out = { "map_gen", "map_gen.classic.resource_dist" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_gen.nation_xxx";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = ".";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = ":";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "..";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = ".:";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  REQUIRE( term.run_cmd( "my_type = unit_type.UnitType.create( "
                         "'free_colonist' )" ) == valid );

  in  = "my_t";
  out = { "my_type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type";
  out = { "my_type." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type.";
  out = { "my_type:base_type", "my_type:type" };
  REQUIRE_THAT( autocomplete( in ), Contains( out ) );

  in  = "my_type.xxx";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type.base_type";
  out = { "my_type:base_type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type.type";
  out = { "my_type:type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type:type";
  out = { "my_type:type(" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "abcabc:";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );
}

TEST_CASE( "[terminal] autocomplete_iterative" ) {
  lua::state st;
  // NOTE: this is expensive, but this test currently needs it.
  lua_init( st );
  Terminal term( st );

  auto ac_i = [&]( string_view in ) {
    return term.autocomplete_iterative( in );
  };

  auto empty = vector<string>{};

  string         in;
  vector<string> out;

  in = "";
  REQUIRE_THAT( ac_i( in ), Equals( empty ) );

  in = "xgiebg";
  REQUIRE_THAT( ac_i( in ), Equals( empty ) );

  in  = "usta";
  out = vector<string>{ "ustate." };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "ustate.";
  out = vector<string>{ "ustate.add_unit_to_cargo",
                        "ustate.create_unit_in_cargo",
                        "ustate.create_unit_on_map" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "ustate.add_unit_";
  out = vector<string>{ "ustate.add_unit_to_cargo(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in = "ustate.add_unit_to_cargo(";
  REQUIRE_THAT( ac_i( in ), Equals( empty ) );

  in  = "ustate.add_unit_to_cargo( usta";
  out = vector<string>{ "ustate.add_unit_to_cargo( ustate." };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "uni";
  out = { "unit_" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_c";
  out = { "unit_composer.UnitComposition.create_with_type" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "ustate.a";
  out = { "ustate.add_unit_to_cargo(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "ustate.create";
  out = { "ustate.create_unit_" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "map_g";
  out = { "map_gen" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "map_gen.ge";
  out = { "map_gen.generate(" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = ".";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = ":";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "..";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = ".:";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  REQUIRE( term.run_cmd( "my_type = unit_type.UnitType.create( "
                         "'free_colonist' )" ) == valid );

  in  = "my_t";
  out = { "my_type:" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type";
  out = { "my_type:" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "my_type.";
  out = { "my_type:type", "my_type:base_type" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "my_type.base_type";
  out = { "my_type:base_type(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type.xxx";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type.base_type";
  out = { "my_type:base_type(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type.type";
  out = { "my_type:type(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "abcabc:";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );
}

TEST_CASE(
    "[terminal] autocomplete for serializable game state" ) {
  World W;

  // FIXME: we need this so that the autocomplete logic can call
  // meta.all_pairs, which it really shouldn't be doing, but has
  // to because luapp currently has no way to do proper lua-style
  // iteration on the result of the pairs method.
  W.expensive_run_lua_init();

  lua::state& st = W.lua();
  Terminal    term( st );

  auto autocomplete = [&]( string_view in ) {
    return term.autocomplete( in );
  };

  auto const empty = vector<string>{};

  string         in;
  vector<string> out;

  in  = "ROO";
  out = { "ROOT" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT";
  out = { "ROOT." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT.";
  out = { "ROOT.colonies", "ROOT.land_view", "ROOT.players",
          "ROOT.settings", "ROOT.terrain",   "ROOT.turn",
          "ROOT.units" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT.col";
  out = { "ROOT.colonies" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT.colonies";
  out = { "ROOT.colonies." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT.colonies.";
  out = { "ROOT.colonies:colony_for_id", "ROOT.colonies:exists",
          "ROOT.colonies:last_colony_id" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT.colonies.col";
  out = { "ROOT.colonies:colony_for_id" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT.colonies:colony_for_id";
  out = { "ROOT.colonies:colony_for_id(" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in = "ROOT.colonies:colony_for_id(";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );
}

} // namespace
} // namespace rn
