/****************************************************************
**auto-complete-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-31.
*
* Description: Unit tests for the auto-complete module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/auto-complete.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/luapp/common.hpp"

// Revolution Now
#include "src/lua.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Contains;
using ::Catch::Equals;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,X,_, /*2
      3*/ _,X,X,X,X,X,X,_, /*3
      4*/ _,X,X,X,X,X,X,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,X,X,X,X,X,X,_, /*6
      7*/ _,X,X,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[auto-complete] autocomplete" ) {
  lua::state st;
  // NOTE: this is expensive, but this test currently needs it.
  lua_init( st );

  auto const autocomplete =
      [&] [[clang::noinline]] ( string_view in ) {
        return rn::autocomplete( st, in );
      };

  auto empty = vector<string>{};

  string in;
  vector<string> out;

  in = "";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in = "xgiebg";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in  = "unit_m";
  out = vector<string>{ "unit_mgr" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_mgr";
  out = vector<string>{ "unit_mgr." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_mgr.";
  out = vector<string>{ "unit_mgr.add_unit_to_cargo",
                        "unit_mgr.create_unit_in_cargo",
                        "unit_mgr.create_unit_on_map" };
  REQUIRE_THAT( autocomplete( in ), Contains( out ) );

  in  = "unit_mgr.add_un";
  out = vector<string>{ "unit_mgr.add_unit_to_cargo" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_mgr.create_unit_in_cargo";
  out = vector<string>{ "unit_mgr.create_unit_in_cargo(" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in = "unit_mgr.create_unit_on_map(";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in = "unit_mgr.create_unit_on_map( unit_m";
  out =
      vector<string>{ "unit_mgr.create_unit_on_map( unit_mgr" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in = "unit_mgr.create_unit_on_map( unit_mgr";
  out =
      vector<string>{ "unit_mgr.create_unit_on_map( unit_mgr." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "uni";
  out = { "unit_" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_";
  out = { "unit_composition", "unit_mgr", "unit_type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_t";
  out = { "unit_type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "unit_mgr";
  out = { "unit_mgr." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_";
  out = { "map_gen" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_gen";
  out = { "map_gen." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_gen.cl";
  out = { "map_gen.classic" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_gen.classic";
  out = { "map_gen.classic." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "map_gen.player_xxx";
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

  REQUIRE( st.script.run_safe( R"lua(
      my_type = unit_type.UnitType.create( 'free_colonist' )
  )lua" ) == valid );

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

TEST_CASE( "[auto-complete] autocomplete_iterative" ) {
  lua::state st;
  // NOTE: this is expensive, but this test currently needs it.
  lua_init( st );

  auto const ac_i = [&] [[clang::noinline]] ( string_view in ) {
    return autocomplete_iterative( st, in );
  };

  auto empty = vector<string>{};

  string in;
  vector<string> out;

  in = "";
  REQUIRE_THAT( ac_i( in ), Equals( empty ) );

  in = "xgiebg";
  REQUIRE_THAT( ac_i( in ), Equals( empty ) );

  in  = "unit_m";
  out = vector<string>{ "unit_mgr." };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_mgr.";
  out = vector<string>{ "unit_mgr.add_unit_to_cargo",
                        "unit_mgr.create_unit_in_cargo",
                        "unit_mgr.create_unit_on_map" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "unit_mgr.add_unit_";
  out = vector<string>{ "unit_mgr.add_unit_to_cargo(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in = "unit_mgr.add_unit_to_cargo(";
  REQUIRE_THAT( ac_i( in ), Equals( empty ) );

  in = "unit_mgr.add_unit_to_cargo( unit_m";
  out =
      vector<string>{ "unit_mgr.add_unit_to_cargo( unit_mgr." };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "uni";
  out = { "unit_" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_c";
  out = { "unit_composition.UnitComposition.create_with_type" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_mgr.a";
  out = { "unit_mgr.add_unit_to_cargo(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_mgr.create";
  out = { "unit_mgr.create_" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_mgr.create_";
  out = { "unit_mgr.create_native_unit_on_map",
          "unit_mgr.create_unit_in_cargo",
          "unit_mgr.create_unit_on_map" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_mgr.create_u";
  out = { "unit_mgr.create_unit_" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "map_g";
  out = { "map_gen." };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "map_gen.cl";
  out = { "map_gen.classic." };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "map_gen.classic.r";
  out = { "map_gen.classic.resource_dist.compute_" };
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

  REQUIRE( st.script.run_safe( R"lua(
    my_type = unit_type.UnitType.create( 'free_colonist' )
  )lua" ) == valid );

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

LUA_TEST_CASE(
    "[auto-complete] autocomplete for serializable game state" ) {
  world w;
  // FIXME: we need this so that the autocomplete logic can call
  // meta.all_pairs, which it really shouldn't be doing, but has
  // to because luapp currently has no way to do proper lua-style
  // iteration on the result of the pairs method.
  w.expensive_run_lua_init();

  lua::state& st = w.lua();

  auto const autocomplete =
      [&] [[clang::noinline]] ( string_view in ) {
        return rn::autocomplete( st, in );
      };

  auto const empty = vector<string>{};

  string in;
  vector<string> out;

  in  = "ROO";
  out = { "ROOT" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT";
  out = { "ROOT." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ROOT.";
  out = { "ROOT.colonies",   "ROOT.events",  "ROOT.land_view",
          "ROOT.map",        "ROOT.natives", "ROOT.players",
          "ROOT.settings",   "ROOT.terrain", "ROOT.trade_routes",
          "ROOT.turn",       "ROOT.units",   "ROOT.version",
          "ROOT.zzz_terrain" };
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
