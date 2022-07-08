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

// Revolution Now
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

TEST_CASE( "[terminal] autocomplete" ) {
  lua::state st;
  Terminal   term( st );

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
  out = vector<string>{ "ustate.create_unit_on_map",
                        "ustate.unit_from_id" };
  REQUIRE_THAT( autocomplete( in ), Contains( out ) );

  in  = "ustate.unit_fr";
  out = vector<string>{ "ustate.unit_from_id" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ustate.unit_from_id";
  out = vector<string>{ "ustate.unit_from_id(" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in = "ustate.unit_from_id(";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in = "ustate.unit_from_id(";
  REQUIRE_THAT( autocomplete( in ), Equals( empty ) );

  in  = "ustate.unit_from_id( usta";
  out = vector<string>{ "ustate.unit_from_id( ustate" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "ustate.unit_from_id( ustate";
  out = vector<string>{ "ustate.unit_from_id( ustate." };
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

  REQUIRE( term.run_cmd( "my_type = MyType.new()" ).valid() );

  in  = "my_t";
  out = { "my_type" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type";
  out = { "my_type." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type.";
  out = { "my_type.x", "my_type:get", "my_type:add" };
  REQUIRE_THAT( autocomplete( in ), Contains( out ) );

  in  = "my_type.x";
  out = { "my_type.x" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type:x";
  out = { "my_type.x" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type.xxx";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type.get";
  out = { "my_type:get" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type.add";
  out = { "my_type:add" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "my_type:add";
  out = { "my_type:add(" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "abcabc:";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );
}

TEST_CASE( "[terminal] autocomplete_iterative" ) {
  lua::state st;
  Terminal   term( st );

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
  out = vector<string>{ "ustate.create_unit_on_map",
                        "ustate.unit_from_id" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "ustate.unit_fr";
  out = vector<string>{ "ustate.unit_from_id(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in = "ustate.unit_from_id(";
  REQUIRE_THAT( ac_i( in ), Equals( empty ) );

  in  = "ustate.unit_from_id( usta";
  out = vector<string>{ "ustate.unit_from_id( ustate." };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "uni";
  out = { "unit_" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "unit_c";
  out = { "unit_composer.UnitComposition.create_with_type" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "ustate.u";
  out = { "ustate.unit" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "ustate.unit";
  out = { "ustate.unit_from_id", "ustate.units_from_coord" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "cstate.last_";
  out = { "cstate.last_colony_id(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "ustate.last_u";
  out = { "ustate.last_unit_id(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "che";
  out = { "cheat.reveal_map(" };
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

  REQUIRE( term.run_cmd( "my_type = MyType.new()" ).valid() );

  in  = "my_t";
  out = { "my_type" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type";
  out = { "my_type.x", "my_type:get", "my_type:add" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "my_type.";
  out = { "my_type.x", "my_type:get", "my_type:add" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "my_type.x";
  out = { "my_type.x" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type:x";
  out = { "my_type.x" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type.xxx";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type.get";
  out = { "my_type:get(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type.add";
  out = { "my_type:add(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "my_type:add";
  out = { "my_type:add(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "abcabc:";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );
}

} // namespace
} // namespace rn
