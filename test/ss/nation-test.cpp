/****************************************************************
**nation-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-21.
*
* Description: Unit tests for the ss/nation module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/nation.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ss/nation] nation_for" ) {
  REQUIRE( nation_for( e_player::english ) ==
           e_nation::english );
  REQUIRE( nation_for( e_player::french ) == e_nation::french );
  REQUIRE( nation_for( e_player::spanish ) ==
           e_nation::spanish );
  REQUIRE( nation_for( e_player::dutch ) == e_nation::dutch );
}

TEST_CASE( "[ss/nation] colonist_player_for" ) {
  REQUIRE( colonist_player_for( e_nation::english ) ==
           e_player::english );
  REQUIRE( colonist_player_for( e_nation::french ) ==
           e_player::french );
  REQUIRE( colonist_player_for( e_nation::spanish ) ==
           e_player::spanish );
  REQUIRE( colonist_player_for( e_nation::dutch ) ==
           e_player::dutch );
}

} // namespace
} // namespace rn
