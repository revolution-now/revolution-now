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
  REQUIRE( nation_for( e_player::ref_english ) ==
           e_nation::english );
  REQUIRE( nation_for( e_player::ref_french ) ==
           e_nation::french );
  REQUIRE( nation_for( e_player::ref_spanish ) ==
           e_nation::spanish );
  REQUIRE( nation_for( e_player::ref_dutch ) ==
           e_nation::dutch );
}

TEST_CASE( "[ss/nation] colonial_player_for" ) {
  REQUIRE( colonial_player_for( e_nation::english ) ==
           e_player::english );
  REQUIRE( colonial_player_for( e_nation::french ) ==
           e_player::french );
  REQUIRE( colonial_player_for( e_nation::spanish ) ==
           e_player::spanish );
  REQUIRE( colonial_player_for( e_nation::dutch ) ==
           e_player::dutch );
}

TEST_CASE( "[ss/nation] ref_player_for" ) {
  REQUIRE( ref_player_for( e_nation::english ) ==
           e_player::ref_english );
  REQUIRE( ref_player_for( e_nation::french ) ==
           e_player::ref_french );
  REQUIRE( ref_player_for( e_nation::spanish ) ==
           e_player::ref_spanish );
  REQUIRE( ref_player_for( e_nation::dutch ) ==
           e_player::ref_dutch );
}

TEST_CASE( "[ss/nation] is_ref" ) {
  REQUIRE( is_ref( e_player::english ) == false );
  REQUIRE( is_ref( e_player::french ) == false );
  REQUIRE( is_ref( e_player::spanish ) == false );
  REQUIRE( is_ref( e_player::dutch ) == false );
  REQUIRE( is_ref( e_player::ref_english ) == true );
  REQUIRE( is_ref( e_player::ref_french ) == true );
  REQUIRE( is_ref( e_player::ref_spanish ) == true );
  REQUIRE( is_ref( e_player::ref_dutch ) == true );
}

TEST_CASE( "[ss/nation] is_colonist" ) {
  REQUIRE( is_colonist( e_player::english ) == true );
  REQUIRE( is_colonist( e_player::french ) == true );
  REQUIRE( is_colonist( e_player::spanish ) == true );
  REQUIRE( is_colonist( e_player::dutch ) == true );
  REQUIRE( is_colonist( e_player::ref_english ) == false );
  REQUIRE( is_colonist( e_player::ref_french ) == false );
  REQUIRE( is_colonist( e_player::ref_spanish ) == false );
  REQUIRE( is_colonist( e_player::ref_dutch ) == false );
}

} // namespace
} // namespace rn
