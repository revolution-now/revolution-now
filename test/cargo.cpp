/****************************************************************
**cargo.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-01.
*
* Description: Unit tests for cargo module.
*
*****************************************************************/
// Revolution Now
#include "src/cargo.hpp"
#include "src/ownership.hpp"

// base-util
#include "base-util/variant.hpp"

// Catch2
#include "catch2/catch.hpp"

using namespace rn;

// This is so that we can call private members.
struct CargoHoldTester : public CargoHold {
  CargoHoldTester( int slots ) : CargoHold( slots ) {}

  using CargoHold::remove;
  using CargoHold::try_add;
  using CargoHold::try_add_first_available;
};

namespace {

TEST_CASE( "CargoHold empty state" ) {
  CargoHoldTester ch( 6 );

  REQUIRE( ch.slots_occupied() == 0 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_total() == 6 );

  REQUIRE( ch.count_items() == 0 );
  REQUIRE( ch.count_items_of_type<UnitId>() == 0 );
  REQUIRE( ch.count_items_of_type<Commodity>() == 0 );
  REQUIRE( ch.items_of_type<UnitId>().empty() );
  REQUIRE( ch.items_of_type<Commodity>().empty() );

  for( auto i : {0, 1, 2, 3, 4, 5} )
    REQUIRE( ch[i] == CargoSlot_t{CargoSlot::empty{}} );

  REQUIRE( ch.units().empty() );
  REQUIRE( ch.commodities().empty() );

  REQUIRE( ch.debug_string() ==
           "[empty,empty,empty,empty,empty,empty]" );
}

TEST_CASE( "add unit" ) {
  CargoHoldTester ch( 6 );

  auto unit_id = create_unit( e_nation::english,
                              e_unit_type::free_colonist )
                     .id();

  SECTION( "" ) {
    REQUIRE( ch.count_items() == 0 );
    REQUIRE( ch.try_add( unit_id, 0 ) );
  }

  SECTION( "" ) {
    REQUIRE( ch.count_items() == 0 ); //
  }
}

} // namespace
