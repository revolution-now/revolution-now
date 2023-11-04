/****************************************************************
**sav-struct-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-03.
*
* Description: Unit tests for the sav/sav-struct module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/sav-struct.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace sav {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[sav/sav-struct] construction" ) {
  ColonySAV sav;

  // Test a couple random fields.
  REQUIRE( sav.head.turn == 0 );
  REQUIRE( sav.head.game_options.autosave == false );
  REQUIRE( sav.head.tribe_count == 0 );
  REQUIRE( sav.head.event.the_fountain_of_youth == false );
  REQUIRE( sav.indian[1].tech == tech_type::semi_nomadic );
}

} // namespace
} // namespace sav
