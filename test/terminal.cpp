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
#include "terminal.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

using Catch::Contains;
using Catch::Equals;

TEST_CASE( "[terminal] autocomplete" ) {
  using term::autocomplete;

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
  out = { "unit" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "id";
  out = { "id." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.";
  out = { "e.nation", "e.unit_type", "e.unit_orders" };
  REQUIRE_THAT( autocomplete( in ), Contains( out ) );

  in  = "e.nat";
  out = { "e.nation" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.nation";
  out = { "e.nation." };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.nation_xxx";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.nation.";
  out = {
      "e.nation.dutch",
      "e.nation.english",
      "e.nation.french",
      "e.nation.spanish",
  };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.nation.engli";
  out = { "e.nation.english" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.nation.english";
  out = { "e.nation.english" };
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.nation.englishx";
  out = {};
  REQUIRE_THAT( autocomplete( in ), Equals( out ) );

  in  = "e.nation.english.";
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

  REQUIRE( term::run_cmd( "my_type = MyType.new()" ).valid() );

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
  using term::autocomplete_iterative;

  auto empty = vector<string>{};

  string         in;
  vector<string> out;

  auto ac_i = []( auto& in ) {
    return autocomplete_iterative( in );
  };

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
  out = { "unit" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "id";
  out = { "id.last_" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "id.last_";
  out = { "id.last_colony_id", "id.last_unit_id" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "id.last_u";
  out = { "id.last_unit_id(" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "e.";
  out = { "e.nation", "e.unit_type", "e.unit_orders" };
  REQUIRE_THAT( ac_i( in ), Contains( out ) );

  in  = "e.nat";
  out = { "e.nation." };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "e.nation_xxx";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "e.nation.";
  out = {
      "e.nation.dutch",
      "e.nation.english",
      "e.nation.french",
      "e.nation.spanish",
  };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "e.nation.engli";
  out = { "e.nation.english" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "e.nation.english";
  out = { "e.nation.english" };
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "e.nation.englishx";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

  in  = "e.nation.english.";
  out = {};
  REQUIRE_THAT( ac_i( in ), Equals( out ) );

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

  REQUIRE( term::run_cmd( "my_type = MyType.new()" ).valid() );

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
