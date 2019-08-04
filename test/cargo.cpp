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

// Catch2
#include "catch2/catch.hpp"

using namespace rn;

// This is so that we can call private members.
struct CargoHoldTester : public CargoHold {
  CargoHoldTester( int slots ) : CargoHold( slots ) {}

  CargoHold& parent() {
    return static_cast<CargoHold&>( *this );
  }

  ND bool try_add_first_available( rn::Cargo const& cargo ) {
    return parent().try_add_first_available( cargo );
  }
  ND bool try_add( Cargo const& cargo, int idx ) {
    return parent().try_add( cargo, idx );
  }
  void remove( int slot_idx ) { parent().remove( slot_idx ); }
};

namespace {

TEST_CASE( "cargo initially empty" ) {
  CargoHoldTester hold( 6 );

  auto unit_id = create_unit( e_nation::english,
                              e_unit_type::free_colonist )
                     .id();

  SECTION( "" ) {
    REQUIRE( hold.count_items() == 0 );
    REQUIRE( hold.try_add( unit_id, 0 ) );
  }

  SECTION( "" ) {
    REQUIRE( hold.count_items() == 0 ); //
  }
}

} // namespace
