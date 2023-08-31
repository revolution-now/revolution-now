/****************************************************************
**colonies-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-08-27.
*
* Description: Unit tests for the ss/colonies module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/colonies.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ss/colonies] last_colony_id" ) {
  ColoniesState colonies_state;

  REQUIRE( ColoniesState::kFirstColonyId == ColonyId{ 1 } );

  REQUIRE( colonies_state.last_colony_id() == base::nothing );

  REQUIRE( colonies_state.add_colony(
               Colony{ .name = "x", .location = { .x = 0 } } ) ==
           ColonyId{ 1 } );
  REQUIRE( colonies_state.last_colony_id() == 1 );
  REQUIRE( colonies_state.add_colony(
               Colony{ .name = "y", .location = { .x = 1 } } ) ==
           ColonyId{ 2 } );
  REQUIRE( colonies_state.last_colony_id() == 2 );
}

} // namespace
} // namespace rn
