/****************************************************************
**nation.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Unit tests for the src/nation.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/nation.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[test/nation] some test" ) {
  auto all = all_nations();
  REQUIRE( all.size() == 4 );
  REQUIRE( all[0] == e_nation::dutch );
  REQUIRE( all[1] == e_nation::french );
  REQUIRE( all[2] == e_nation::english );
  REQUIRE( all[3] == e_nation::spanish );

  Nationality const& dutch = nation_obj( e_nation::dutch );
  REQUIRE( dutch.enum_val == e_nation::dutch );
  REQUIRE( dutch.display_name == "Dutch" );
  REQUIRE( dutch.country_name == "The Netherlands" );
  REQUIRE( dutch.new_world_name == "New Netherlands" );
  REQUIRE( dutch.adjective == "Dutch" );
  REQUIRE( dutch.article == "a" );
  REQUIRE(
      dutch.flag_color ==
      gfx::pixel{ .r = 0xff, .g = 0x71, .b = 0x00, .a = 0xff } );

  Nationality const& english = nation_obj( e_nation::english );
  REQUIRE( english.enum_val == e_nation::english );
  REQUIRE( english.display_name == "English" );
  REQUIRE( english.country_name == "England" );
  REQUIRE( english.new_world_name == "New England" );
  REQUIRE( english.adjective == "English" );
  REQUIRE( english.article == "an" );
  REQUIRE(
      dutch.flag_color ==
      gfx::pixel{ .r = 0xff, .g = 0x71, .b = 0x00, .a = 0xff } );
}

} // namespace
} // namespace rn
