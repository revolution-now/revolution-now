/****************************************************************
**unit-classes.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-18.
*
* Description: Unit tests for the src/unit-classes.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/unit-classes.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[unit-classes] scout_type" ) {
  REQUIRE( scout_type( e_unit_type::free_colonist ) == nothing );
  REQUIRE( scout_type( e_unit_type::scout ) ==
           e_scout_type::non_seasoned );
  REQUIRE( scout_type( e_unit_type::seasoned_scout ) ==
           e_scout_type::seasoned );
  REQUIRE( scout_type( e_unit_type::dragoon ) == nothing );
}

TEST_CASE( "[unit-classes] pioneer_type" ) {
  REQUIRE( pioneer_type( e_unit_type::free_colonist ) ==
           nothing );
  REQUIRE( pioneer_type( e_unit_type::pioneer ) ==
           e_pioneer_type::non_hardy );
  REQUIRE( pioneer_type( e_unit_type::hardy_pioneer ) ==
           e_pioneer_type::hardy );
  REQUIRE( pioneer_type( e_unit_type::dragoon ) == nothing );
}

TEST_CASE( "[unit-classes] missionary_type" ) {
  UnitType                 in;
  maybe<e_missionary_type> expected = {};

  auto f = [&] { return missionary_type( in ); };

  // petty criminal.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::petty_criminal )
           .value();
  expected = e_missionary_type::criminal;
  REQUIRE( f() == expected );

  // indentured servant.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::indentured_servant )
           .value();
  expected = e_missionary_type::indentured;
  REQUIRE( f() == expected );

  // free colonist.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::free_colonist )
           .value();
  expected = e_missionary_type::normal;
  REQUIRE( f() == expected );

  // expert_farmer.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::expert_farmer )
           .value();
  expected = e_missionary_type::normal;
  REQUIRE( f() == expected );

  // jesuit_missionary.
  in       = e_unit_type::jesuit_missionary;
  expected = e_missionary_type::jesuit;
  REQUIRE( f() == expected );

  // free_colonist non-missionary.
  in       = e_unit_type::free_colonist;
  expected = nothing;
  REQUIRE( f() == expected );

  // petty_criminal non-missionary.
  in       = e_unit_type::petty_criminal;
  expected = nothing;
  REQUIRE( f() == expected );

  // jesuit_colonist non-missionary.
  in       = e_unit_type::jesuit_colonist;
  expected = nothing;
  REQUIRE( f() == expected );

  // artillery.
  in       = e_unit_type::artillery;
  expected = nothing;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
