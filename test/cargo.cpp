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
#include "testing.hpp"

// Revolution Now
#include "src/cargo.hpp"
#include "src/ownership.hpp"

// base-util
#include "base-util/variant.hpp"

// Catch2
#include "catch2/catch.hpp"

#define REQUIRE_BROKEN_INVARIANTS \
  REQUIRE_THROWS_AS_RN( ch.check_invariants() )

#define REQUIRE_GOOD_INVARIANTS \
  REQUIRE_NOTHROW( ch.check_invariants() )

namespace {

using namespace std;
using namespace rn;

using Catch::UnorderedEquals;

// This is so that we can call private members.
struct CargoHoldTester : public CargoHold {
  CargoHoldTester( int slots ) : CargoHold( slots ) {}

  // Methods.
  using CargoHold::check_invariants;
  using CargoHold::remove;
  using CargoHold::try_add;
  using CargoHold::try_add_as_available;

  // Data members.
  using CargoHold::slots_;
};

TEST_CASE( "CargoHold slot bounds" ) {
  CargoHoldTester ch0( 0 );
  REQUIRE_THROWS_AS_RN( ch0[0] );
  REQUIRE( ch0.begin() == ch0.end() );

  CargoHoldTester ch6( 6 );
  REQUIRE_NOTHROW( ch6[0] );
  REQUIRE_NOTHROW( ch6[5] );
  REQUIRE_THROWS_AS_RN( ch6[6] );
  REQUIRE_THROWS_AS_RN( ch6[7] );
  REQUIRE( distance( ch0.begin(), ch0.end() ) == 0 );
}

TEST_CASE( "CargoHold zero size" ) {
  CargoHoldTester ch( 0 );
  REQUIRE_GOOD_INVARIANTS;

  REQUIRE( ch.slots_occupied() == 0 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_total() == 0 );

  REQUIRE( ch.count_items() == 0 );
  REQUIRE( ch.count_items_of_type<UnitId>() == 0 );
  REQUIRE( ch.count_items_of_type<Commodity>() == 0 );
  REQUIRE( ch.items_of_type<UnitId>().empty() );
  REQUIRE( ch.items_of_type<Commodity>().empty() );
  REQUIRE( ch.units().empty() );
  REQUIRE( ch.commodities().empty() );

  REQUIRE( ch.debug_string() == "[]" );
}

TEST_CASE( "CargoHold has slots but empty" ) {
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

TEST_CASE( "CargoHold add/remove to/from size-0 cargo hold" ) {
  CargoHoldTester ch( 0 );

  auto unit_id = create_unit( e_nation::english,
                              e_unit_type::free_colonist )
                     .id();
  auto comm1 = Commodity{/*type=*/e_commodity::food,
                         /*quantity=*/100};
  REQUIRE_THROWS_AS_RN( ch.fits( unit_id, 0 ) );
  REQUIRE_THROWS_AS_RN( ch.fits( unit_id, 1 ) );

  REQUIRE( ch.find_fit( comm1 ).empty() );

  REQUIRE_FALSE( ch.try_add_as_available( unit_id ) );
  REQUIRE_FALSE( ch.try_add_as_available( comm1 ) );

  REQUIRE( ch.count_items() == 0 );

  REQUIRE_THROWS_AS_RN( ch.try_add( unit_id, 0 ) );
  REQUIRE_THROWS_AS_RN( ch.try_add( comm1, 1 ) );

  REQUIRE_THROWS_AS_RN( ch.remove( 0 ) );
  REQUIRE_THROWS_AS_RN( ch.remove( 1 ) );
}

TEST_CASE( "CargoHold add/remove from size-1 cargo hold" ) {
  CargoHoldTester ch( 1 );

  auto cargo = GENERATE(
      CargoSlot::cargo{
          /*contents=*/create_unit( e_nation::english,
                                    e_unit_type::free_colonist )
              .id()},
      CargoSlot::cargo{/*contents=*/Commodity{
          /*type=*/e_commodity::food, /*quantity=*/100}} );

  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 1 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.fits( cargo.contents, 0 ) );
  REQUIRE_THROWS_AS_RN( ch.fits( cargo.contents, 1 ) );
  REQUIRE( ch.find_fit( cargo.contents ) == Vec<int>{0} );
  REQUIRE_THROWS_AS_RN( ch.try_add( cargo.contents, 1 ) );

  REQUIRE( ch.debug_string() == "[empty]" );

  SECTION( "specify slot" ) {
    REQUIRE( ch.try_add( cargo.contents, 0 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 0 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.slots_[0] == CargoSlot_t{cargo} );
    if_v( cargo.contents, UnitId, unit_id ) {
      REQUIRE( ch.find_unit( *unit_id ) == 0 );
      REQUIRE_THAT( ch.units(),
                    UnorderedEquals( Vec<UnitId>{*unit_id} ) );
      REQUIRE( ch.debug_string() == "[cargo{contents=1_id}]" );
    }
    if_v( cargo.contents, Commodity, comm ) {
      REQUIRE_THAT( ch.commodities(),
                    UnorderedEquals( Vec<Pair<Commodity, int>>{
                        {*comm, 0}} ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Commodity{type=food,quantity="
               "100}}]" );
    }
    REQUIRE_NOTHROW( ch.remove( 0 ) );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 1 );
    REQUIRE( ch.slots_occupied() == 0 );
  }

  SECTION( "first available" ) {
    REQUIRE( ch.try_add_as_available( cargo.contents ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 0 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.slots_[0] == CargoSlot_t{cargo} );
    if_v( cargo.contents, UnitId, unit_id ) {
      REQUIRE( ch.find_unit( *unit_id ) == 0 );
      REQUIRE_THAT( ch.units(),
                    UnorderedEquals( Vec<UnitId>{*unit_id} ) );
      REQUIRE( ch.debug_string() == "[cargo{contents=1_id}]" );
    }
    if_v( cargo.contents, Commodity, comm ) {
      REQUIRE_THAT( ch.commodities(),
                    UnorderedEquals( Vec<Pair<Commodity, int>>{
                        {*comm, 0}} ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Commodity{type=food,quantity="
               "100}}]" );
    }
    REQUIRE_NOTHROW( ch.remove( 0 ) );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 1 );
    REQUIRE( ch.slots_occupied() == 0 );
  }
}

TEST_CASE(
    "CargoHold add/remove size-1 cargo from size-6 cargo "
    "hold" ) {
  CargoHoldTester ch( 6 );

  auto cargo = GENERATE(
      CargoSlot::cargo{
          /*contents=*/create_unit( e_nation::english,
                                    e_unit_type::free_colonist )
              .id()},
      CargoSlot::cargo{/*contents=*/Commodity{
          /*type=*/e_commodity::food, /*quantity=*/100}} );

  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.fits( cargo.contents, 0 ) );
  REQUIRE( ch.fits( cargo.contents, 3 ) );
  REQUIRE_THROWS_AS_RN( ch.fits( cargo.contents, 6 ) );
  REQUIRE( ch.find_fit( cargo.contents ) ==
           Vec<int>{0, 1, 2, 3, 4, 5} );
  REQUIRE_THROWS_AS_RN( ch.try_add( cargo.contents, 6 ) );

  SECTION( "specify zero slot" ) {
    REQUIRE( ch.try_add( cargo.contents, 0 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.slots_[0] == CargoSlot_t{cargo} );
    if_v( cargo.contents, UnitId, unit_id ) {
      REQUIRE( ch.find_unit( *unit_id ) == 0 );
      REQUIRE_THAT( ch.units(),
                    UnorderedEquals( Vec<UnitId>{*unit_id} ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=2_id},empty,empty,empty,empty,"
               "empty]" );
    }
    if_v( cargo.contents, Commodity, comm ) {
      REQUIRE_THAT( ch.commodities(),
                    UnorderedEquals( Vec<Pair<Commodity, int>>{
                        {*comm, 0}} ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Commodity{type=food,quantity="
               "100}},empty,empty,empty,empty,empty]" );
    }
    REQUIRE_NOTHROW( ch.remove( 0 ) );
  }

  SECTION( "specify middle slot" ) {
    REQUIRE( ch.try_add( cargo.contents, 3 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.slots_[3] == CargoSlot_t{cargo} );
    if_v( cargo.contents, UnitId, unit_id ) {
      REQUIRE( ch.find_unit( *unit_id ) == 3 );
      REQUIRE_THAT( ch.units(),
                    UnorderedEquals( Vec<UnitId>{*unit_id} ) );
      REQUIRE( ch.debug_string() ==
               "[empty,empty,empty,cargo{contents=2_id},empty,"
               "empty]" );
    }
    if_v( cargo.contents, Commodity, comm ) {
      REQUIRE_THAT( ch.commodities(),
                    UnorderedEquals( Vec<Pair<Commodity, int>>{
                        {*comm, 3}} ) );
      REQUIRE( ch.debug_string() ==
               "[empty,empty,empty,cargo{contents=Commodity{"
               "type=food,quantity=100}},empty,empty]" );
    }
    REQUIRE_THROWS_AS_RN( ch.remove( 0 ) );
    REQUIRE_NOTHROW( ch.remove( 3 ) );
  }

  SECTION( "first available" ) {
    REQUIRE( ch.try_add_as_available( cargo.contents ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.slots_[0] == CargoSlot_t{cargo} );
    if_v( cargo.contents, UnitId, unit_id ) {
      REQUIRE( ch.find_unit( *unit_id ) == 0 );
      REQUIRE_THAT( ch.units(),
                    UnorderedEquals( Vec<UnitId>{*unit_id} ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=2_id},empty,empty,empty,empty,"
               "empty]" );
    }
    if_v( cargo.contents, Commodity, comm ) {
      REQUIRE_THAT( ch.commodities(),
                    UnorderedEquals( Vec<Pair<Commodity, int>>{
                        {*comm, 0}} ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Commodity{type=food,quantity="
               "100}},empty,empty,empty,empty,empty]" );
    }
    REQUIRE_THROWS_AS_RN( ch.remove( 1 ) );
    REQUIRE_NOTHROW( ch.remove( 0 ) );
  }

  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );
}

TEST_CASE(
    "CargoHold add/remove size-4 cargo from size-6 cargo "
    "hold" ) {
  CargoHoldTester ch( 6 );

  auto unit_id = create_unit( e_nation::english,
                              e_unit_type::small_treasure )
                     .id();

  REQUIRE( ch.fits( unit_id, 0 ) );
  REQUIRE( ch.fits( unit_id, 1 ) );
  REQUIRE( ch.fits( unit_id, 2 ) );
  REQUIRE_FALSE( ch.fits( unit_id, 3 ) );
  REQUIRE_FALSE( ch.fits( unit_id, 4 ) );
  REQUIRE_FALSE( ch.fits( unit_id, 5 ) );
  REQUIRE_THROWS_AS_RN( ch.fits( unit_id, 6 ) );

  REQUIRE( ch.find_fit( unit_id ) == Vec<int>{0, 1, 2} );

  SECTION( "specify middle slot" ) {
    REQUIRE( ch.try_add( unit_id, 1 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE( ch.slots_occupied() == 4 );
    REQUIRE( ch.slots_[0] == CargoSlot_t{CargoSlot::empty{}} );
    REQUIRE( ch.slots_[1] == CargoSlot_t{CargoSlot::cargo{
                                 /*contents=*/unit_id}} );
    REQUIRE( ch.slots_[2] ==
             CargoSlot_t{CargoSlot::overflow{}} );
    REQUIRE( ch.slots_[3] ==
             CargoSlot_t{CargoSlot::overflow{}} );
    REQUIRE( ch.slots_[4] ==
             CargoSlot_t{CargoSlot::overflow{}} );
    REQUIRE( ch.slots_[5] == CargoSlot_t{CargoSlot::empty{}} );
    REQUIRE( ch.find_unit( unit_id ) == 1 );
    REQUIRE_THAT( ch.units(),
                  UnorderedEquals( Vec<UnitId>{unit_id} ) );
    REQUIRE( ch.commodities().empty() );
    REQUIRE( ch.debug_string() ==
             "[empty,cargo{contents=3_id},overflow,overflow,"
             "overflow,empty]" );
    REQUIRE_THROWS_AS_RN( ch.remove( 0 ) );
    REQUIRE_THROWS_AS_RN( ch.remove( 2 ) );
    REQUIRE_NOTHROW( ch.remove( 1 ) );
  }

  SECTION( "first available" ) {
    REQUIRE( ch.try_add_as_available( unit_id ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE( ch.slots_occupied() == 4 );
    REQUIRE( ch.slots_[0] == CargoSlot_t{CargoSlot::cargo{
                                 /*contents=*/unit_id}} );
    REQUIRE( ch.slots_[1] ==
             CargoSlot_t{CargoSlot::overflow{}} );
    REQUIRE( ch.slots_[2] ==
             CargoSlot_t{CargoSlot::overflow{}} );
    REQUIRE( ch.slots_[3] ==
             CargoSlot_t{CargoSlot::overflow{}} );
    REQUIRE( ch.slots_[4] == CargoSlot_t{CargoSlot::empty{}} );
    REQUIRE( ch.slots_[5] == CargoSlot_t{CargoSlot::empty{}} );
    REQUIRE( ch.find_unit( unit_id ) == 0 );
    REQUIRE_THAT( ch.units(),
                  UnorderedEquals( Vec<UnitId>{unit_id} ) );
    REQUIRE( ch.commodities().empty() );
    REQUIRE( ch.debug_string() ==
             "[cargo{contents=4_id},overflow,overflow,overflow,"
             "empty,empty]" );
    REQUIRE_THROWS_AS_RN( ch.remove( 1 ) );
    REQUIRE_THROWS_AS_RN( ch.remove( 5 ) );
    REQUIRE_NOTHROW( ch.remove( 0 ) );
  }

  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );
}

TEST_CASE( "CargoHold try to add same unit twice" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();
  auto unit_id2 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();

  REQUIRE( ch.try_add( unit_id1, 1 ) );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id1 ) );
  REQUIRE( ch.try_add_as_available( unit_id2 ) );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id2 ) );
}

TEST_CASE( "CargoHold add item too large for cargo hold" ) {
  CargoHoldTester ch( 4 );
  auto            unit_id1 = create_unit( e_nation::english,
                               e_unit_type::large_treasure )
                      .id();
  REQUIRE_FALSE( ch.fits( unit_id1, 0 ) );
  REQUIRE_FALSE( ch.fits( unit_id1, 1 ) );
  REQUIRE_FALSE( ch.fits( unit_id1, 2 ) );
  REQUIRE( ch.find_fit( unit_id1 ) == Vec<int>{} );
  REQUIRE_FALSE( ch.try_add( unit_id1, 0 ) );
  REQUIRE_FALSE( ch.try_add_as_available( unit_id1 ) );
  REQUIRE( ch.count_items() == 0 );
}

TEST_CASE( "CargoHold try to add too many things" ) {
  CargoHoldTester ch( 5 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();
  auto unit_id2 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  auto unit_id3 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  auto comm1 = Commodity{/*type=*/e_commodity::food,
                         /*quantity=*/100};

  REQUIRE( ch.try_add( unit_id2, 1 ) );
  REQUIRE_FALSE( ch.try_add_as_available( unit_id3 ) );
  REQUIRE( ch.try_add_as_available( comm1 ) );
  REQUIRE( ch.find_fit( unit_id1 ) == Vec<int>{} );
  REQUIRE_FALSE( ch.try_add_as_available( unit_id1 ) );
  REQUIRE_FALSE( ch.try_add_as_available( comm1 ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.debug_string() ==
           "[cargo{contents=Commodity{type=food,quantity=100}},"
           "cargo{contents=9_id},overflow,overflow,overflow]" );
}

TEST_CASE( "CargoHold add multiple units" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();
  auto unit_id2 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  auto unit_id3 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();

  REQUIRE( ch.try_add_as_available( unit_id1 ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.count_items_of_type<UnitId>() == 1 );
  REQUIRE_THAT( ch.units(),
                UnorderedEquals( Vec<UnitId>{unit_id1} ) );
  REQUIRE_THAT( ch.items_of_type<UnitId>(),
                UnorderedEquals( Vec<UnitId>{unit_id1} ) );
  REQUIRE( ch.try_add_as_available( unit_id2 ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.count_items_of_type<UnitId>() == 2 );
  REQUIRE_THAT( ch.units(), UnorderedEquals( Vec<UnitId>{
                                unit_id1, unit_id2} ) );
  REQUIRE_THAT(
      ch.items_of_type<UnitId>(),
      UnorderedEquals( Vec<UnitId>{unit_id1, unit_id2} ) );
  REQUIRE( ch.try_add_as_available( unit_id3 ) );
  REQUIRE( ch.count_items() == 3 );
  REQUIRE( ch.count_items_of_type<UnitId>() == 3 );
  REQUIRE_THAT( ch.units(),
                UnorderedEquals( Vec<UnitId>{unit_id1, unit_id2,
                                             unit_id3} ) );
  REQUIRE_THAT( ch.items_of_type<UnitId>(),
                UnorderedEquals( Vec<UnitId>{unit_id1, unit_id2,
                                             unit_id3} ) );
  REQUIRE( ch.slots_occupied() == 6 );
  REQUIRE( ch.slots_remaining() == 0 );

  REQUIRE( ch.find_unit( unit_id1 ) == 0 );
  REQUIRE( ch.find_unit( unit_id2 ) == 1 );
  REQUIRE( ch.find_unit( unit_id3 ) == 5 );

  REQUIRE(
      ch.debug_string() ==
      "[cargo{contents=11_id},cargo{contents=12_id},overflow,"
      "overflow,overflow,cargo{contents=13_id}]" );

  REQUIRE( ch.commodities().empty() );
}

TEST_CASE( "CargoHold remove from empty slot" ) {
  CargoHoldTester ch( 6 );
  REQUIRE_THROWS_AS_RN( ch.remove( 0 ) );
  REQUIRE_THROWS_AS_RN( ch.remove( 1 ) );
  REQUIRE_THROWS_AS_RN( ch.remove( 5 ) );
}

TEST_CASE( "CargoHold remove from overflow slot" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  REQUIRE( ch.try_add_as_available( unit_id1 ) );
  REQUIRE( ch.slots_[1] == CargoSlot_t{CargoSlot::overflow{}} );
  REQUIRE_THROWS_AS_RN( ch.remove( 1 ) );
  REQUIRE_NOTHROW( ch.remove( 0 ) );
  REQUIRE_THROWS_AS_RN( ch.remove( 0 ) );
  REQUIRE( ch.debug_string() ==
           "[empty,empty,empty,empty,empty,empty]" );
}

TEST_CASE( "CargoHold remove large cargo" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  REQUIRE( ch.try_add( unit_id1, 1 ) );
  REQUIRE( ch.slots_[0] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE( ch.slots_[1] == CargoSlot_t{CargoSlot::cargo{
                               /*contents=*/unit_id1}} );
  REQUIRE( ch.slots_[2] == CargoSlot_t{CargoSlot::overflow{}} );
  REQUIRE( ch.slots_[3] == CargoSlot_t{CargoSlot::overflow{}} );
  REQUIRE( ch.slots_[4] == CargoSlot_t{CargoSlot::overflow{}} );
  REQUIRE( ch.slots_[5] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE_THROWS_AS_RN( ch.remove( 0 ) );
  REQUIRE_THROWS_AS_RN( ch.remove( 2 ) );
  REQUIRE_THROWS_AS_RN( ch.remove( 5 ) );
  REQUIRE( ch.slots_occupied() == 4 );
  REQUIRE( ch.slots_remaining() == 2 );
  REQUIRE_NOTHROW( ch.remove( 1 ) );
  REQUIRE( ch.slots_[0] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE( ch.slots_[1] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE( ch.slots_[2] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE( ch.slots_[3] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE( ch.slots_[4] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE( ch.slots_[5] == CargoSlot_t{CargoSlot::empty{}} );
  REQUIRE( ch.slots_occupied() == 0 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE_THROWS_AS_RN( ch.remove( 1 ) );
  REQUIRE( ch.debug_string() ==
           "[empty,empty,empty,empty,empty,empty]" );
}

TEST_CASE( "CargoHold try add as available from invalid" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();

  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id1, 6 ) );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id1, 7 ) );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id1, 8 ) );
  REQUIRE( ch.try_add_as_available( unit_id1, 0 ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.slots_occupied() == 1 );
}

TEST_CASE(
    "CargoHold try add as available from beginning (units)" ) {
  CargoHoldTester ch( 12 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();
  auto unit_id2 = create_unit( e_nation::english,
                               e_unit_type::large_treasure )
                      .id();
  auto unit_id3 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  auto unit_id4 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();
  auto unit_id5 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();

  Vec<CargoSlot_t> cmp_slots( 12 );

  REQUIRE( ch.try_add_as_available( unit_id1 ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.slots_occupied() == 1 );
  cmp_slots[0] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id1}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id1 ) );

  REQUIRE( ch.try_add_as_available( unit_id2 ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.slots_occupied() == 7 );
  cmp_slots[1] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id2}};
  for( int i = 2; i <= 6; ++i )
    cmp_slots[i] = CargoSlot_t{CargoSlot::overflow{}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id2 ) );

  REQUIRE( ch.try_add_as_available( unit_id3 ) );
  REQUIRE( ch.count_items() == 3 );
  REQUIRE( ch.slots_occupied() == 11 );
  cmp_slots[7] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id3}};
  for( int i = 8; i <= 10; ++i )
    cmp_slots[i] = CargoSlot_t{CargoSlot::overflow{}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id3 ) );

  REQUIRE( ch.try_add_as_available( unit_id4 ) );
  REQUIRE( ch.count_items() == 4 );
  REQUIRE( ch.slots_occupied() == 12 );
  cmp_slots[11] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id4}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id4 ) );

  REQUIRE_FALSE( ch.try_add_as_available( unit_id5 ) );
  REQUIRE( ch.count_items() == 4 );
  REQUIRE( ch.slots_occupied() == 12 );
}

TEST_CASE(
    "CargoHold try add as available from middle (units)" ) {
  CargoHoldTester ch( 12 );

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();
  auto unit_id2 = create_unit( e_nation::english,
                               e_unit_type::large_treasure )
                      .id();
  auto unit_id3 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  auto unit_id4 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();
  auto unit_id5 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();
  auto unit_id6 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();
  auto unit_id7 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();
  auto unit_id8 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();
  auto unit_id9 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();

  Vec<CargoSlot_t> cmp_slots( 12 );

  REQUIRE(
      ch.try_add_as_available( unit_id1, /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.slots_occupied() == 1 );
  cmp_slots[2] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id1}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id1 ) );

  REQUIRE(
      ch.try_add_as_available( unit_id2, /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.slots_occupied() == 7 );
  cmp_slots[3] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id2}};
  for( int i = 4; i <= 8; ++i )
    cmp_slots[i] = CargoSlot_t{CargoSlot::overflow{}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id2 ) );

  REQUIRE_FALSE(
      ch.try_add_as_available( unit_id3, /*starting_slot=*/2 ) );
  REQUIRE( ch.find_fit( unit_id3 ) == Vec<int>{} );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.slots_occupied() == 7 );

  REQUIRE(
      ch.try_add_as_available( unit_id4, /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 3 );
  REQUIRE( ch.slots_occupied() == 8 );
  cmp_slots[9] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id4}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id4 ) );

  REQUIRE(
      ch.try_add_as_available( unit_id5, /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 4 );
  REQUIRE( ch.slots_occupied() == 9 );
  cmp_slots[10] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id5}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id5 ) );

  REQUIRE(
      ch.try_add_as_available( unit_id6, /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 5 );
  REQUIRE( ch.slots_occupied() == 10 );
  cmp_slots[11] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id6}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id6 ) );

  REQUIRE(
      ch.try_add_as_available( unit_id7, /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 6 );
  REQUIRE( ch.slots_occupied() == 11 );
  cmp_slots[0] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id7}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id7 ) );

  REQUIRE(
      ch.try_add_as_available( unit_id8, /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 7 );
  REQUIRE( ch.slots_occupied() == 12 );
  cmp_slots[1] =
      CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id8}};
  REQUIRE( ch.slots_ == cmp_slots );
  REQUIRE_THROWS_AS_RN( ch.try_add_as_available( unit_id8 ) );

  REQUIRE_FALSE(
      ch.try_add_as_available( unit_id9, /*starting_slot=*/2 ) );
  REQUIRE_FALSE(
      ch.try_add_as_available( unit_id9, /*starting_slot=*/0 ) );
  REQUIRE_FALSE(
      ch.try_add_as_available( unit_id9, /*starting_slot=*/6 ) );
}

TEST_CASE( "CargoHold try add as available from all (units)" ) {
  CargoHoldTester ch( 6 );

  auto start = GENERATE( 0, 1, 2, 3, 4, 5 );

  UnitId unit_ids[6];
  for( auto i = 0; i < 6; ++i )
    unit_ids[i] = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();

  Vec<CargoSlot_t> cmp_slots( 6 );

  for( auto i = 0; i < 6; ++i ) {
    REQUIRE( ch.try_add_as_available(
        unit_ids[i], /*starting_slot=*/start ) );
    REQUIRE( ch.count_items() == i + 1 );
    REQUIRE( ch.slots_occupied() == i + 1 );
    cmp_slots[( i + start ) % 6] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_ids[i]}};
    REQUIRE( ch.slots_ == cmp_slots );
    REQUIRE_THROWS_AS_RN(
        ch.try_add_as_available( unit_ids[i] ) );
  }
}

TEST_CASE( "CargoHold check broken invariants" ) {
  CargoHoldTester ch( 6 );
  REQUIRE_GOOD_INVARIANTS;

  auto unit_id1 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();
  auto unit_id2 = create_unit( e_nation::english,
                               e_unit_type::small_treasure )
                      .id();
  auto comm1 = Commodity{/*type=*/e_commodity::food,
                         /*quantity=*/100};
  auto comm2 = Commodity{/*type=*/e_commodity::food,
                         /*quantity=*/101};
  auto comm3 = Commodity{/*type=*/e_commodity::food,
                         /*quantity=*/0};

  SECTION( "no overflow in first slot" ) {
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[0] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "no orphaned overflow" ) {
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[3] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "too much overflow after unit 1" ) {
    ch.slots_[0] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id1}};
    REQUIRE( ch.count_items() == 1 );
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[1] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "too much overflow after unit 2" ) {
    ch.slots_[0] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id2}};
    ch.slots_[1] = CargoSlot_t{CargoSlot::overflow{}};
    ch.slots_[2] = CargoSlot_t{CargoSlot::overflow{}};
    ch.slots_[3] = CargoSlot_t{CargoSlot::overflow{}};
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[4] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "not enough overflow after unit due to empty" ) {
    ch.slots_[0] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id2}};
    ch.slots_[1] = CargoSlot_t{CargoSlot::overflow{}};
    ch.slots_[2] = CargoSlot_t{CargoSlot::overflow{}};
    ch.slots_[3] = CargoSlot_t{CargoSlot::overflow{}};
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[3] = CargoSlot_t{CargoSlot::empty{}};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "not enough overflow after unit due to unit" ) {
    ch.slots_[0] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id2}};
    ch.slots_[1] = CargoSlot_t{CargoSlot::overflow{}};
    ch.slots_[2] = CargoSlot_t{CargoSlot::overflow{}};
    ch.slots_[3] = CargoSlot_t{CargoSlot::overflow{}};
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[3] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id1}};
    REQUIRE( ch.slots_[4] == CargoSlot_t{CargoSlot::empty{}} );
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "unit with overflow at end 1" ) {
    ch.slots_[5] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id1}};
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[5] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id2}};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "unit with overflow at end 2" ) {
    ch.slots_[4] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id1}};
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[4] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/unit_id2}};
    ch.slots_[5] = CargoSlot_t{CargoSlot::overflow{}};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "no overflow following commodity" ) {
    ch.slots_[0] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/comm1}};
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[1] = CargoSlot_t{CargoSlot::overflow{}};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "commodities don't exceed max quantity" ) {
    ch.slots_[0] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/comm1}};
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[1] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/comm2}};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "commodities don't have zero quantity" ) {
    REQUIRE_GOOD_INVARIANTS;
    ch.slots_[0] =
        CargoSlot_t{CargoSlot::cargo{/*contents=*/comm3}};
    REQUIRE_BROKEN_INVARIANTS;
  }
}

TEST_CASE( "CargoHold commodity consolidation like types" ) {
  CargoHoldTester ch( 6 );

  auto food_full = Commodity{/*type=*/e_commodity::food,
                             /*quantity=*/100};
  auto food_part = Commodity{/*type=*/e_commodity::food,
                             /*quantity=*/25};

  SECTION( "full slot" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.try_add( food_full, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE(
        ch[0] ==
        CargoSlot_t{CargoSlot::cargo{/*contents=*/Commodity{
            /*type=*/e_commodity::food, /*quantity=*/100}}} );
    REQUIRE_FALSE( ch.fits( food_full, 0 ) );
    REQUIRE_FALSE( ch.fits( food_part, 0 ) );
    REQUIRE_FALSE( ch.try_add( food_full, 0 ) );
    REQUIRE_FALSE( ch.try_add( food_part, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
  }

  SECTION( "partial slot" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.try_add( food_part, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE(
        ch[0] ==
        CargoSlot_t{CargoSlot::cargo{/*contents=*/Commodity{
            /*type=*/e_commodity::food, /*quantity=*/25}}} );
    REQUIRE_FALSE( ch.fits( food_full, 0 ) );
    REQUIRE( ch.fits( food_part, 0 ) );
    REQUIRE_FALSE( ch.try_add( food_full, 0 ) );
    REQUIRE(
        ch[0] ==
        CargoSlot_t{CargoSlot::cargo{/*contents=*/Commodity{
            /*type=*/e_commodity::food, /*quantity=*/25}}} );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.try_add( food_part, 0 ) );
    REQUIRE(
        ch[0] ==
        CargoSlot_t{CargoSlot::cargo{/*contents=*/Commodity{
            /*type=*/e_commodity::food, /*quantity=*/50}}} );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE_FALSE( ch.fits( food_full, 0 ) );
    REQUIRE( ch.fits( food_part, 0 ) );
    REQUIRE( ch.try_add( food_part, 0 ) );
    REQUIRE(
        ch[0] ==
        CargoSlot_t{CargoSlot::cargo{/*contents=*/Commodity{
            /*type=*/e_commodity::food, /*quantity=*/75}}} );
    REQUIRE( ch[1] == CargoSlot_t{CargoSlot::empty{}} );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE_FALSE( ch.fits( food_full, 0 ) );
    REQUIRE( ch.fits( food_part, 0 ) );
    REQUIRE( ch.try_add( food_part, 0 ) );
    REQUIRE(
        ch[0] ==
        CargoSlot_t{CargoSlot::cargo{/*contents=*/Commodity{
            /*type=*/e_commodity::food, /*quantity=*/100}}} );
    REQUIRE( ch[1] == CargoSlot_t{CargoSlot::empty{}} );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE_FALSE( ch.fits( food_part, 0 ) );
    REQUIRE_FALSE( ch.fits( food_full, 0 ) );
    REQUIRE_FALSE( ch.try_add( food_part, 0 ) );
  }
}

TEST_CASE( "CargoHold commodity consolidation unlike types" ) {
  SECTION( "full slot" ) {
    //
  }

  SECTION( "partial slot" ) {
    //
  }
}

TEST_CASE(
    "CargoHold commodity consolidation try add as available" ) {
  //
}

TEST_CASE( "CargoHold commodity consolidation max quantities" ) {
  //
}

TEST_CASE( "CargoHold compactify empty" ) {
  //
}

TEST_CASE( "CargoHold compactify single size-1 unit" ) {
  //
}

TEST_CASE( "CargoHold compactify single size-4 unit" ) {
  //
}

TEST_CASE( "CargoHold compactify multiple units" ) {
  //
}

TEST_CASE( "CargoHold compactify single commodity" ) {
  //
}

TEST_CASE(
    "CargoHold compactify multiple commodities different "
    "types" ) {
  //
}

TEST_CASE(
    "CargoHold compactify multiple commodities same type" ) {
  //
}

TEST_CASE( "CargoHold compactify multiple commodities" ) {
  //
}

TEST_CASE(
    "CargoHold compactify units and commodities units first" ) {
  //
}

TEST_CASE(
    "CargoHold compactify units and commodities large unit" ) {
  //
}

} // namespace
