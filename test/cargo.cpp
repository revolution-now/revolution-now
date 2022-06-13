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
#include "src/game-state.hpp"
#include "src/rand.hpp"
#include "src/ustate.hpp"

// refl
#include "refl/cdr.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"
#include "base/to-str-tags.hpp"

// Abseil
#include "absl/strings/str_replace.h"

// Must be last.
#include "catch-common.hpp"

#define REQUIRE_BROKEN_INVARIANTS \
  REQUIRE( !ch.validate( GameState::units() ) )

#define REQUIRE_GOOD_INVARIANTS \
  REQUIRE( ch.validate( GameState::units() ) )

namespace rn {
namespace {

using namespace std;
using namespace rn;

using Catch::UnorderedEquals;

// This is so that we can call private members.
struct CargoHoldTester : public CargoHold {
  CargoHoldTester( int slots ) : CargoHold( slots ) {}

  // Methods.
  using CargoHold::clear;
  using CargoHold::remove;
  using CargoHold::try_add;
  using CargoHold::try_add_somewhere;
  using CargoHold::validate;
  using CargoHold::operator[];

  // Data members.
  using CargoHold::o_;

  string debug_string() const {
    string res = absl::StrReplaceAll(
        base::to_str( static_cast<CargoHold const&>( *this ) ),
        { { "CargoSlot::", "" },
          { "slots=", "" },
          { "CargoHold{", "" } } );
    // Remove trailing '}'.
    res.pop_back();
    return res;
  }
};

TEST_CASE( "CargoHold slot bounds zero" ) {
  CargoHoldTester ch0( 0 );
  REQUIRE( ch0.begin() == ch0.end() );

  CargoHoldTester ch6( 6 );
  REQUIRE_NOTHROW( ch6[0] );
  REQUIRE_NOTHROW( ch6[5] );
  REQUIRE( distance( ch0.begin(), ch0.end() ) == 0 );

  ch0.clear();
  ch6.clear();
}

TEST_CASE( "CargoHold slot bounds six" ) {
  CargoHoldTester ch( 6 );
  REQUIRE( distance( ch.begin(), ch.end() ) == 6 );

  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[0] == CargoSlot_t{ CargoSlot::empty{} } );

  REQUIRE( ch.at( -1 ) == nothing );
  REQUIRE( ch.at( 6 ) == nothing );
  REQUIRE( ch.at( 7 ) == nothing );
  REQUIRE( ch.at( 5 ).has_value() );
  REQUIRE( ch.at( 0 ).has_value() );
  REQUIRE( ch.at( 5 ) == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.at( 0 ) == CargoSlot_t{ CargoSlot::empty{} } );

  ch.clear();
}

TEST_CASE( "CargoHold zero size" ) {
  CargoHoldTester ch( 0 );
  REQUIRE_GOOD_INVARIANTS;

  REQUIRE( ch.slots_occupied() == 0 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_total() == 0 );

  REQUIRE( ch.count_items() == 0 );
  REQUIRE( ch.count_items_of_type<Cargo::unit>() == 0 );
  REQUIRE( ch.count_items_of_type<Cargo::commodity>() == 0 );
  REQUIRE( ch.items_of_type<Cargo::unit>().empty() );
  REQUIRE( ch.items_of_type<Cargo::commodity>().empty() );
  REQUIRE( ch.units().empty() );
  REQUIRE( ch.commodities().empty() );

  REQUIRE( ch.debug_string() == "[]" );

  ch.clear();
}

TEST_CASE( "CargoHold has slots but empty" ) {
  CargoHoldTester ch( 6 );

  REQUIRE( ch.slots_occupied() == 0 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_total() == 6 );

  REQUIRE( ch.count_items() == 0 );
  REQUIRE( ch.count_items_of_type<Cargo::unit>() == 0 );
  REQUIRE( ch.count_items_of_type<Cargo::commodity>() == 0 );
  REQUIRE( ch.items_of_type<Cargo::unit>().empty() );
  REQUIRE( ch.items_of_type<Cargo::commodity>().empty() );

  for( auto i : { 0, 1, 2, 3, 4, 5 } )
    REQUIRE( ch[i] == CargoSlot_t{ CargoSlot::empty{} } );

  REQUIRE( ch.units().empty() );
  REQUIRE( ch.commodities().empty() );

  REQUIRE( ch.debug_string() ==
           "[empty,empty,empty,empty,empty,empty]" );

  ch.clear();
}

TEST_CASE( "CargoHold add/remove to/from size-0 cargo hold" ) {
  CargoHoldTester ch( 0 );

  auto unit_id = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto comm1 = Commodity{ /*type=*/e_commodity::food,
                          /*quantity=*/100 };

  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id } ) );
  REQUIRE_FALSE(
      ch.try_add_somewhere( Cargo::unit{ unit_id } ) );
  REQUIRE_FALSE(
      ch.fits_somewhere( Cargo::commodity{ comm1 } ) );
  REQUIRE_FALSE(
      ch.try_add_somewhere( Cargo::commodity{ comm1 } ) );

  REQUIRE( ch.count_items() == 0 );

  ch.clear();
}

TEST_CASE( "CargoHold add/remove from size-1 cargo hold" ) {
  CargoHoldTester ch( 1 );

  auto cargo = GENERATE(
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ create_unit(
          GameState::units(), e_nation::english,
          UnitType::create( e_unit_type::free_colonist ) ) } },
      CargoSlot::cargo{ /*contents=*/Cargo::commodity{ Commodity{
          /*type=*/e_commodity::food, /*quantity=*/100 } } } );

  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 1 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.fits( cargo.contents, 0 ) );

  REQUIRE( ch.debug_string() == "[empty]" );

  SECTION( "specify slot" ) {
    REQUIRE( ch.try_add( cargo.contents, 0 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 0 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.o_.slots[0] == CargoSlot_t{ cargo } );
    if( auto* u = get_if<Cargo::unit>( &( cargo.contents ) ) ) {
      UnitId unit_id = u->id;
      REQUIRE( ch.find_unit( unit_id ) == 0 );
      REQUIRE_THAT(
          ch.units(),
          UnorderedEquals( vector<UnitId>{ unit_id } ) );
      REQUIRE( ch.debug_string() ==
               fmt::format(
                   "[cargo{{contents=Cargo::unit{{id={}}}}}]",
                   unit_id ) );
    }
    if( auto* comm =
            get_if<Cargo::commodity>( &( cargo.contents ) ) ) {
      REQUIRE_THAT(
          ch.commodities(),
          UnorderedEquals( vector<pair<Commodity, int>>{
              { comm->obj, 0 } } ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Cargo::commodity{obj=Commodity{"
               "type=food,quantity=100}}}]" );
    }
    REQUIRE_NOTHROW( ch.remove( 0 ) );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 1 );
    REQUIRE( ch.slots_occupied() == 0 );
  }

  SECTION( "first available" ) {
    REQUIRE( ch.fits_somewhere( cargo.contents ) );
    REQUIRE( ch.try_add_somewhere( cargo.contents ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 0 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.o_.slots[0] == CargoSlot_t{ cargo } );
    if( auto* u = get_if<Cargo::unit>( &( cargo.contents ) ) ) {
      UnitId unit_id = u->id;
      REQUIRE( ch.find_unit( unit_id ) == 0 );
      REQUIRE_THAT(
          ch.units(),
          UnorderedEquals( vector<UnitId>{ unit_id } ) );
      REQUIRE( ch.debug_string() ==
               fmt::format(
                   "[cargo{{contents=Cargo::unit{{id={}}}}}]",
                   unit_id ) );
    }
    if( auto* comm =
            get_if<Cargo::commodity>( &( cargo.contents ) ) ) {
      REQUIRE_THAT(
          ch.commodities(),
          UnorderedEquals( vector<pair<Commodity, int>>{
              { comm->obj, 0 } } ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Cargo::commodity{obj=Commodity{"
               "type=food,quantity=100}}}]" );
    }
    REQUIRE_NOTHROW( ch.remove( 0 ) );
    REQUIRE( ch.slots_total() == 1 );
    REQUIRE( ch.slots_remaining() == 1 );
    REQUIRE( ch.slots_occupied() == 0 );
  }

  ch.clear();
}

TEST_CASE(
    "CargoHold add/remove size-1 cargo from size-6 cargo "
    "hold" ) {
  CargoHoldTester ch( 6 );

  auto cargo = GENERATE(
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ create_unit(
          GameState::units(), e_nation::english,
          UnitType::create( e_unit_type::free_colonist ) ) } },
      CargoSlot::cargo{ /*contents=*/Cargo::commodity{ Commodity{
          /*type=*/e_commodity::food, /*quantity=*/100 } } } );

  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.fits( cargo.contents, 0 ) );
  REQUIRE( ch.fits( cargo.contents, 3 ) );

  SECTION( "specify zero slot" ) {
    REQUIRE( ch.try_add( cargo.contents, 0 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.o_.slots[0] == CargoSlot_t{ cargo } );
    if( auto* u = get_if<Cargo::unit>( &( cargo.contents ) ) ) {
      UnitId unit_id = u->id;
      REQUIRE( ch.find_unit( unit_id ) == 0 );
      REQUIRE_THAT(
          ch.units(),
          UnorderedEquals( vector<UnitId>{ unit_id } ) );
      REQUIRE(
          ch.debug_string() ==
          fmt::format( "[cargo{{contents=Cargo::unit{{id={}}}}},"
                       "empty,empty,empty,empty,empty]",
                       unit_id ) );
    }
    if( auto* comm =
            get_if<Cargo::commodity>( &( cargo.contents ) ) ) {
      REQUIRE_THAT(
          ch.commodities(),
          UnorderedEquals( vector<pair<Commodity, int>>{
              { comm->obj, 0 } } ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Cargo::commodity{obj=Commodity{"
               "type=food,quantity=100}}},empty,empty,empty,"
               "empty,empty]" );
    }
    REQUIRE_NOTHROW( ch.remove( 0 ) );
  }

  SECTION( "specify middle slot" ) {
    REQUIRE( ch.try_add( cargo.contents, 3 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.o_.slots[3] == CargoSlot_t{ cargo } );
    if( auto* u = get_if<Cargo::unit>( &( cargo.contents ) ) ) {
      UnitId unit_id = u->id;
      REQUIRE( ch.find_unit( unit_id ) == 3 );
      REQUIRE_THAT(
          ch.units(),
          UnorderedEquals( vector<UnitId>{ unit_id } ) );
      REQUIRE(
          ch.debug_string() ==
          fmt::format( "[empty,empty,empty,cargo{{contents="
                       "Cargo::unit{{id={}}}}},empty,empty]",
                       unit_id ) );
    }
    if( auto* comm =
            get_if<Cargo::commodity>( &( cargo.contents ) ) ) {
      REQUIRE_THAT(
          ch.commodities(),
          UnorderedEquals( vector<pair<Commodity, int>>{
              { comm->obj, 3 } } ) );
      REQUIRE( ch.debug_string() ==
               "[empty,empty,empty,cargo{contents=Cargo::"
               "commodity{obj=Commodity{type=food,quantity=100}"
               "}},empty,empty]" );
    }
    REQUIRE_NOTHROW( ch.remove( 3 ) );
  }

  SECTION( "first available" ) {
    REQUIRE( ch.fits_somewhere( cargo.contents ) );
    REQUIRE( ch.try_add_somewhere( cargo.contents ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.o_.slots[0] == CargoSlot_t{ cargo } );
    if( auto* u = get_if<Cargo::unit>( &( cargo.contents ) ) ) {
      UnitId unit_id = u->id;
      REQUIRE( ch.find_unit( unit_id ) == 0 );
      REQUIRE_THAT(
          ch.units(),
          UnorderedEquals( vector<UnitId>{ unit_id } ) );
      REQUIRE(
          ch.debug_string() ==
          fmt::format( "[cargo{{contents=Cargo::unit{{id={}}}}},"
                       "empty,empty,empty,empty,empty]",
                       unit_id ) );
    }
    if( auto* comm =
            get_if<Cargo::commodity>( &( cargo.contents ) ) ) {
      REQUIRE_THAT(
          ch.commodities(),
          UnorderedEquals( vector<pair<Commodity, int>>{
              { comm->obj, 0 } } ) );
      REQUIRE( ch.debug_string() ==
               "[cargo{contents=Cargo::commodity{obj=Commodity{"
               "type=food,quantity=100}}},empty,empty,empty,"
               "empty,empty]" );
    }
    REQUIRE_NOTHROW( ch.remove( 0 ) );
  }

  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  ch.clear();
}

TEST_CASE(
    "CargoHold add/remove size-4 cargo from size-6 cargo "
    "hold" ) {
  CargoHoldTester ch( 6 );

  auto unit_id = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );

  REQUIRE( ch.fits( Cargo::unit{ unit_id }, 0 ) );
  REQUIRE( ch.fits( Cargo::unit{ unit_id }, 1 ) );
  REQUIRE( ch.fits( Cargo::unit{ unit_id }, 2 ) );
  REQUIRE_FALSE( ch.fits( Cargo::unit{ unit_id }, 3 ) );
  REQUIRE_FALSE( ch.fits( Cargo::unit{ unit_id }, 4 ) );
  REQUIRE_FALSE( ch.fits( Cargo::unit{ unit_id }, 5 ) );

  SECTION( "specify middle slot" ) {
    REQUIRE( ch.try_add( Cargo::unit{ unit_id }, 1 ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE( ch.slots_occupied() == 4 );
    REQUIRE( ch.o_.slots[0] ==
             CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.o_.slots[1] ==
             CargoSlot_t{ CargoSlot::cargo{
                 /*contents=*/Cargo::unit{ unit_id } } } );
    REQUIRE( ch.o_.slots[2] ==
             CargoSlot_t{ CargoSlot::overflow{} } );
    REQUIRE( ch.o_.slots[3] ==
             CargoSlot_t{ CargoSlot::overflow{} } );
    REQUIRE( ch.o_.slots[4] ==
             CargoSlot_t{ CargoSlot::overflow{} } );
    REQUIRE( ch.o_.slots[5] ==
             CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.find_unit( unit_id ) == 1 );
    REQUIRE_THAT( ch.units(),
                  UnorderedEquals( vector<UnitId>{ unit_id } ) );
    REQUIRE( ch.commodities().empty() );
    REQUIRE(
        ch.debug_string() ==
        fmt::format( "[empty,cargo{{contents=Cargo::unit{{id={}}"
                     "}}},overflow,overflow,overflow,empty]",
                     unit_id ) );
    REQUIRE_NOTHROW( ch.remove( 1 ) );
  }

  SECTION( "first available" ) {
    REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id } ) );
    REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id } ) );
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_total() == 6 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE( ch.slots_occupied() == 4 );
    REQUIRE( ch.o_.slots[0] ==
             CargoSlot_t{ CargoSlot::cargo{
                 /*contents=*/Cargo::unit{ unit_id } } } );
    REQUIRE( ch.o_.slots[1] ==
             CargoSlot_t{ CargoSlot::overflow{} } );
    REQUIRE( ch.o_.slots[2] ==
             CargoSlot_t{ CargoSlot::overflow{} } );
    REQUIRE( ch.o_.slots[3] ==
             CargoSlot_t{ CargoSlot::overflow{} } );
    REQUIRE( ch.o_.slots[4] ==
             CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.o_.slots[5] ==
             CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.find_unit( unit_id ) == 0 );
    REQUIRE_THAT( ch.units(),
                  UnorderedEquals( vector<UnitId>{ unit_id } ) );
    REQUIRE( ch.commodities().empty() );
    REQUIRE(
        ch.debug_string() ==
        fmt::format( "[cargo{{contents=Cargo::unit{{id={}}}}},"
                     "overflow,overflow,overflow,empty,empty]",
                     unit_id ) );
    REQUIRE_NOTHROW( ch.remove( 0 ) );
  }

  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  ch.clear();
}

TEST_CASE( "CargoHold try to add same unit twice" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );

  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 1 ) );
  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id2 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id2 } ) );

  ch.clear();
  REQUIRE( ch.count_items() == 0 );
}

TEST_CASE( "CargoHold add item too large for cargo hold" ) {
  CargoHoldTester ch( 4 );
  auto            unit_id1 = create_unit(
                 GameState::units(), e_nation::english,
                 UnitType::create( e_unit_type::large_treasure ) );
  REQUIRE_FALSE( ch.fits( Cargo::unit{ unit_id1 }, 0 ) );
  REQUIRE_FALSE( ch.fits( Cargo::unit{ unit_id1 }, 1 ) );
  REQUIRE_FALSE( ch.fits( Cargo::unit{ unit_id1 }, 2 ) );
  REQUIRE_FALSE( ch.try_add( Cargo::unit{ unit_id1 }, 0 ) );
  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE_FALSE(
      ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.count_items() == 0 );

  ch.clear();
}

TEST_CASE( "CargoHold try to add too many things" ) {
  CargoHoldTester ch( 5 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id3 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto comm1 = Commodity{ /*type=*/e_commodity::food,
                          /*quantity=*/100 };

  REQUIRE( ch.try_add( Cargo::unit{ unit_id2 }, 1 ) );
  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id3 } ) );
  REQUIRE_FALSE(
      ch.try_add_somewhere( Cargo::unit{ unit_id3 } ) );
  REQUIRE( ch.fits_somewhere( Cargo::commodity{ comm1 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::commodity{ comm1 } ) );
  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE_FALSE(
      ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE_FALSE(
      ch.fits_somewhere( Cargo::commodity{ comm1 } ) );
  REQUIRE_FALSE(
      ch.try_add_somewhere( Cargo::commodity{ comm1 } ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE(
      ch.debug_string() ==
      fmt::format(
          "[cargo{{contents=Cargo::commodity{{obj=Commodity{{"
          "type=food,quantity=100}}}}}},cargo{{contents=Cargo::"
          "unit{{id={}}}}},overflow,overflow,overflow]",
          unit_id2 ) );

  ch.clear();
}

TEST_CASE( "CargoHold add multiple units" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id3 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.count_items_of_type<Cargo::unit>() == 1 );
  REQUIRE_THAT( ch.units(),
                UnorderedEquals( vector<UnitId>{ unit_id1 } ) );
  REQUIRE_THAT( ch.items_of_type<Cargo::unit>(),
                UnorderedEquals( vector<Cargo::unit>{
                    Cargo::unit{ unit_id1 } } ) );
  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id2 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id2 } ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.count_items_of_type<Cargo::unit>() == 2 );
  REQUIRE_THAT( ch.units(), UnorderedEquals( vector<UnitId>{
                                unit_id1, unit_id2 } ) );
  REQUIRE_THAT(
      ch.items_of_type<Cargo::unit>(),
      UnorderedEquals( vector<Cargo::unit>{
          Cargo::unit{ unit_id1 }, Cargo::unit{ unit_id2 } } ) );
  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id3 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id3 } ) );
  REQUIRE( ch.count_items() == 3 );
  REQUIRE( ch.count_items_of_type<Cargo::unit>() == 3 );
  REQUIRE_THAT( ch.units(),
                UnorderedEquals( vector<UnitId>{
                    unit_id1, unit_id2, unit_id3 } ) );
  REQUIRE_THAT(
      ch.items_of_type<Cargo::unit>(),
      UnorderedEquals( vector<Cargo::unit>{
          Cargo::unit{ unit_id1 }, Cargo::unit{ unit_id2 },
          Cargo::unit{ unit_id3 } } ) );
  REQUIRE( ch.slots_occupied() == 6 );
  REQUIRE( ch.slots_remaining() == 0 );

  REQUIRE( ch.find_unit( unit_id1 ) == 0 );
  REQUIRE( ch.find_unit( unit_id2 ) == 1 );
  REQUIRE( ch.find_unit( unit_id3 ) == 5 );

  REQUIRE(
      ch.debug_string() ==
      fmt::format(
          "[cargo{{contents=Cargo::unit{{id={}}}}},cargo{{"
          "contents=Cargo::unit{{id={}}}}},overflow,overflow,"
          "overflow,cargo{{contents=Cargo::unit{{id={}}}}}]",
          unit_id1, unit_id2, unit_id3 ) );

  REQUIRE( ch.commodities().empty() );

  ch.clear();
}

TEST_CASE( "CargoHold remove from empty slot" ) {
  CargoHoldTester ch( 6 );

  ch.clear();
}

TEST_CASE( "CargoHold remove from overflow slot" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.o_.slots[1] ==
           CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE_NOTHROW( ch.remove( 0 ) );
  REQUIRE( ch.debug_string() ==
           "[empty,empty,empty,empty,empty,empty]" );

  ch.clear();
}

TEST_CASE( "CargoHold remove large cargo" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 1 ) );
  REQUIRE( ch.o_.slots[0] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.o_.slots[1] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch.o_.slots[2] ==
           CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch.o_.slots[3] ==
           CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch.o_.slots[4] ==
           CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch.o_.slots[5] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.slots_occupied() == 4 );
  REQUIRE( ch.slots_remaining() == 2 );
  REQUIRE_NOTHROW( ch.remove( 1 ) );
  REQUIRE( ch.o_.slots[0] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.o_.slots[1] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.o_.slots[2] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.o_.slots[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.o_.slots[4] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.o_.slots[5] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch.slots_occupied() == 0 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.debug_string() ==
           "[empty,empty,empty,empty,empty,empty]" );

  ch.clear();
}

TEST_CASE( "CargoHold clear" ) {
  CargoHoldTester ch( 12 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id2 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto comm =
      Commodity{ /*type=*/e_commodity::food, /*quantity=*/100 };

  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 0 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id2 }, 4 ) );
  REQUIRE( ch.try_add( Cargo::commodity{ comm }, 5 ) );

  REQUIRE( ch.count_items() == 3 );
  REQUIRE( ch.slots_occupied() == 6 );
  REQUIRE( ch.slots_total() == 12 );
  REQUIRE( ch.slots_remaining() == 6 );

  ch.clear();

  REQUIRE( ch.count_items() == 0 );
  REQUIRE( ch.slots_occupied() == 0 );
  REQUIRE( ch.slots_total() == 12 );
  REQUIRE( ch.slots_remaining() == 12 );

  for( auto const& slot : ch ) {
    REQUIRE( slot == CargoSlot_t{ CargoSlot::empty{} } );
  }

  ch.clear();
}

TEST_CASE( "CargoHold try add as available from invalid" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id1 }, 0 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 }, 0 ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.slots_occupied() == 1 );

  ch.clear();
}

TEST_CASE(
    "CargoHold try add as available from beginning (units)" ) {
  CargoHoldTester ch( 12 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::large_treasure ) );
  auto unit_id3 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id4 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto unit_id5 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );

  vector<CargoSlot_t> cmp_slots( 12 );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.slots_occupied() == 1 );
  cmp_slots[0] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id1 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id2 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id2 } ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.slots_occupied() == 7 );
  cmp_slots[1] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id2 } } };
  for( int i = 2; i <= 6; ++i )
    cmp_slots[i] = CargoSlot_t{ CargoSlot::overflow{} };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id3 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id3 } ) );
  REQUIRE( ch.count_items() == 3 );
  REQUIRE( ch.slots_occupied() == 11 );
  cmp_slots[7] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id3 } } };
  for( int i = 8; i <= 10; ++i )
    cmp_slots[i] = CargoSlot_t{ CargoSlot::overflow{} };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id4 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id4 } ) );
  REQUIRE( ch.count_items() == 4 );
  REQUIRE( ch.slots_occupied() == 12 );
  cmp_slots[11] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id4 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id5 } ) );
  REQUIRE_FALSE(
      ch.try_add_somewhere( Cargo::unit{ unit_id5 } ) );
  REQUIRE( ch.count_items() == 4 );
  REQUIRE( ch.slots_occupied() == 12 );

  ch.clear();
}

TEST_CASE(
    "CargoHold try add as available from middle (units)" ) {
  CargoHoldTester ch( 12 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::large_treasure ) );
  auto unit_id3 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id4 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto unit_id5 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto unit_id6 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto unit_id7 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto unit_id8 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto unit_id9 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );

  vector<CargoSlot_t> cmp_slots( 12 );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id1 },
                              /*starting_slot=*/2 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 },
                                 /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 1 );
  REQUIRE( ch.slots_occupied() == 1 );
  cmp_slots[2] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id1 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id2 },
                              /*starting_slot=*/2 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id2 },
                                 /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.slots_occupied() == 7 );
  cmp_slots[3] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id2 } } };
  for( int i = 4; i <= 8; ++i )
    cmp_slots[i] = CargoSlot_t{ CargoSlot::overflow{} };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id3 },
                                    /*starting_slot=*/2 ) );
  REQUIRE_FALSE( ch.try_add_somewhere( Cargo::unit{ unit_id3 },
                                       /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 2 );
  REQUIRE( ch.slots_occupied() == 7 );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id4 },
                              /*starting_slot=*/2 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id4 },
                                 /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 3 );
  REQUIRE( ch.slots_occupied() == 8 );
  cmp_slots[9] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id4 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id5 },
                              /*starting_slot=*/2 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id5 },
                                 /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 4 );
  REQUIRE( ch.slots_occupied() == 9 );
  cmp_slots[10] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id5 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id6 },
                              /*starting_slot=*/2 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id6 },
                                 /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 5 );
  REQUIRE( ch.slots_occupied() == 10 );
  cmp_slots[11] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id6 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id7 },
                              /*starting_slot=*/2 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id7 },
                                 /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 6 );
  REQUIRE( ch.slots_occupied() == 11 );
  cmp_slots[0] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id7 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_id8 },
                              /*starting_slot=*/2 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id8 },
                                 /*starting_slot=*/2 ) );
  REQUIRE( ch.count_items() == 7 );
  REQUIRE( ch.slots_occupied() == 12 );
  cmp_slots[1] = CargoSlot_t{
      CargoSlot::cargo{ /*contents=*/Cargo::unit{ unit_id8 } } };
  REQUIRE( ch.o_.slots == cmp_slots );

  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id9 },
                                    /*starting_slot=*/2 ) );
  REQUIRE_FALSE( ch.try_add_somewhere( Cargo::unit{ unit_id9 },
                                       /*starting_slot=*/2 ) );
  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id9 },
                                    /*starting_slot=*/0 ) );
  REQUIRE_FALSE( ch.try_add_somewhere( Cargo::unit{ unit_id9 },
                                       /*starting_slot=*/0 ) );
  REQUIRE_FALSE( ch.fits_somewhere( Cargo::unit{ unit_id9 },
                                    /*starting_slot=*/6 ) );
  REQUIRE_FALSE( ch.try_add_somewhere( Cargo::unit{ unit_id9 },
                                       /*starting_slot=*/6 ) );

  ch.clear();
}

TEST_CASE( "CargoHold try add as available from all (units)" ) {
  CargoHoldTester ch( 6 );

  auto start = GENERATE( 0, 1, 2, 3, 4, 5 );

  UnitId unit_ids[6];
  for( auto i = 0; i < 6; ++i )
    unit_ids[i] = create_unit(
        GameState::units(), e_nation::english,
        UnitType::create( e_unit_type::free_colonist ) );

  vector<CargoSlot_t> cmp_slots( 6 );

  for( auto i = 0; i < 6; ++i ) {
    REQUIRE( ch.fits_somewhere( Cargo::unit{ unit_ids[i] },
                                /*starting_slot=*/start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_ids[i] },
                                   /*starting_slot=*/start ) );
    REQUIRE( ch.count_items() == i + 1 );
    REQUIRE( ch.slots_occupied() == i + 1 );
    cmp_slots[( i + start ) % 6] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_ids[i] } } };
    REQUIRE( ch.o_.slots == cmp_slots );
  }

  ch.clear();
}

TEST_CASE( "CargoHold check broken invariants" ) {
  CargoHoldTester ch( 6 );
  REQUIRE_GOOD_INVARIANTS;

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto comm1 = Commodity{ /*type=*/e_commodity::food,
                          /*quantity=*/100 };
  auto comm2 = Commodity{ /*type=*/e_commodity::food,
                          /*quantity=*/101 };
  auto comm3 = Commodity{ /*type=*/e_commodity::food,
                          /*quantity=*/0 };

  SECTION( "no overflow in first slot" ) {
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[0] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "no orphaned overflow" ) {
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[3] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "too much overflow after unit 1" ) {
    ch.o_.slots[0] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id1 } } };
    REQUIRE( ch.count_items() == 1 );
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[1] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "too much overflow after unit 2" ) {
    ch.o_.slots[0] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id2 } } };
    ch.o_.slots[1] = CargoSlot_t{ CargoSlot::overflow{} };
    ch.o_.slots[2] = CargoSlot_t{ CargoSlot::overflow{} };
    ch.o_.slots[3] = CargoSlot_t{ CargoSlot::overflow{} };
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[4] = CargoSlot::overflow{};
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "not enough overflow after unit due to empty" ) {
    ch.o_.slots[0] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id2 } } };
    ch.o_.slots[1] = CargoSlot_t{ CargoSlot::overflow{} };
    ch.o_.slots[2] = CargoSlot_t{ CargoSlot::overflow{} };
    ch.o_.slots[3] = CargoSlot_t{ CargoSlot::overflow{} };
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[3] = CargoSlot_t{ CargoSlot::empty{} };
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "not enough overflow after unit due to unit" ) {
    ch.o_.slots[0] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id2 } } };
    ch.o_.slots[1] = CargoSlot_t{ CargoSlot::overflow{} };
    ch.o_.slots[2] = CargoSlot_t{ CargoSlot::overflow{} };
    ch.o_.slots[3] = CargoSlot_t{ CargoSlot::overflow{} };
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 2 );
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[3] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id1 } } };
    REQUIRE( ch.o_.slots[4] ==
             CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "unit with overflow at end 1" ) {
    ch.o_.slots[5] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id1 } } };
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[5] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id2 } } };
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "unit with overflow at end 2" ) {
    ch.o_.slots[4] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id1 } } };
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[4] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::unit{ unit_id2 } } };
    ch.o_.slots[5] = CargoSlot_t{ CargoSlot::overflow{} };
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "no overflow following commodity" ) {
    ch.o_.slots[0] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::commodity{ comm1 } } };
    REQUIRE( ch.count_items() == 1 );
    REQUIRE( ch.slots_remaining() == 5 );
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[1] = CargoSlot_t{ CargoSlot::overflow{} };
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "commodities don't exceed max quantity" ) {
    ch.o_.slots[0] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::commodity{ comm1 } } };
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[1] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::commodity{ comm2 } } };
    REQUIRE_BROKEN_INVARIANTS;
  }

  SECTION( "commodities don't have zero quantity" ) {
    REQUIRE_GOOD_INVARIANTS;
    ch.o_.slots[0] = CargoSlot_t{ CargoSlot::cargo{
        /*contents=*/Cargo::commodity{ comm3 } } };
    REQUIRE_BROKEN_INVARIANTS;
  }

  ch.clear();
}

TEST_CASE( "CargoHold commodity consolidation like types" ) {
  CargoHoldTester ch( 6 );

  auto food_full = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto food_part = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/25 };

  SECTION( "full slot" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.try_add( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
  }

  SECTION( "partial slot" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE( ch.fits( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE( ch.fits( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/75 } } } } );
    REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE( ch.fits( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_part }, 0 ) );
  }

  ch.clear();
}

TEST_CASE( "CargoHold commodity consolidation unlike types" ) {
  CargoHoldTester ch( 6 );

  auto food_full  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto food_part  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/25 };
  auto sugar_full = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/100 };
  auto sugar_part = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/25 };

  SECTION( "full slot" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.try_add( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE_FALSE(
        ch.fits( Cargo::commodity{ sugar_full }, 0 ) );
    REQUIRE_FALSE(
        ch.fits( Cargo::commodity{ sugar_part }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ sugar_full }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ sugar_part }, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
  }

  SECTION( "partial slot" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE( ch.fits( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE_FALSE(
        ch.fits( Cargo::commodity{ sugar_full }, 0 ) );
    REQUIRE_FALSE(
        ch.fits( Cargo::commodity{ sugar_part }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_full }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ sugar_full }, 0 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ sugar_part }, 0 ) );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.try_add( Cargo::commodity{ food_part }, 0 ) );
    REQUIRE( ch[0] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch.try_add( Cargo::commodity{ sugar_part }, 1 ) );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_full }, 1 ) );
    REQUIRE_FALSE( ch.fits( Cargo::commodity{ food_part }, 1 ) );
    REQUIRE_FALSE(
        ch.fits( Cargo::commodity{ sugar_full }, 1 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_full }, 1 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ food_part }, 1 ) );
    REQUIRE_FALSE(
        ch.try_add( Cargo::commodity{ sugar_full }, 1 ) );
    REQUIRE( ch.fits( Cargo::commodity{ sugar_part }, 1 ) );
    REQUIRE( ch.try_add( Cargo::commodity{ sugar_part }, 1 ) );
    REQUIRE( ch[1] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::sugar,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch.slots_occupied() == 2 );
  }

  ch.clear();
}

TEST_CASE(
    "CargoHold commodity consolidation try add as available" ) {
  CargoHoldTester ch( 6 );

  auto food_full  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto food_part  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/25 };
  auto sugar_full = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/100 };
  auto sugar_part = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/25 };

  auto start = GENERATE( 0, 1, 2, 3, 4, 5 );
  auto prev  = ( start + 6 - 1 ) % 6;
  auto next  = ( start + 1 ) % 6;
  auto next2 = ( start + 2 ) % 6;
  auto next3 = ( start + 3 ) % 6;

  SECTION( "same types" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_full },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_full },
                                   start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch[start] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_part },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_part },
                                   start ) );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE( ch.slots_occupied() == 2 );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_part },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_part },
                                   start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch.slots_occupied() == 2 );
    // This should cause overflow.
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_full },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_full },
                                   start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE( ch[next2] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch.slots_occupied() == 3 );
    REQUIRE( ch[next3] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_part },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_part },
                                   start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[next2] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/75 } } } } );
    REQUIRE( ch.slots_occupied() == 3 );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
  }

  SECTION( "unlike types" ) {
    REQUIRE( ch.slots_occupied() == 0 );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_part },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_part },
                                   start ) );
    REQUIRE( ch.slots_occupied() == 1 );
    REQUIRE( ch[start] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ sugar_part },
                                start ) );
    REQUIRE( ch.try_add_somewhere(
        Cargo::commodity{ sugar_part }, start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::sugar,
                                /*quantity=*/25 } } } } );
    REQUIRE( ch.slots_occupied() == 2 );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ sugar_part },
                                start ) );
    REQUIRE( ch.try_add_somewhere(
        Cargo::commodity{ sugar_part }, start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[start] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::sugar,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch.slots_occupied() == 2 );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_full },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_full },
                                   start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[start] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::sugar,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch[next2] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/25 } } } } );
    REQUIRE( ch.slots_occupied() == 3 );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ food_part },
                                start ) );
    REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_part },
                                   start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[start] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::sugar,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch[next2] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch[next3] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch.slots_occupied() == 3 );
    REQUIRE( ch.fits_somewhere( Cargo::commodity{ sugar_full },
                                start ) );
    REQUIRE( ch.try_add_somewhere(
        Cargo::commodity{ sugar_full }, start ) );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( ch[start] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/100 } } } } );
    REQUIRE( ch[next] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::sugar,
                                /*quantity=*/100 } } } } );
    REQUIRE( ch[next2] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::food,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch[next3] ==
             CargoSlot_t{
                 CargoSlot::cargo{ /*contents=*/Cargo::commodity{
                     Commodity{ /*type=*/e_commodity::sugar,
                                /*quantity=*/50 } } } } );
    REQUIRE( ch.slots_occupied() == 4 );
    REQUIRE( ch[prev] == CargoSlot_t{ CargoSlot::empty{} } );
  }

  ch.clear();
}

TEST_CASE( "CargoHold compactify empty" ) {
  CargoHoldTester ch( 0 );
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 0 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_occupied() == 0 );

  ch.clear();
}

TEST_CASE( "CargoHold compactify size-1 with unit" ) {
  CargoHoldTester ch( 1 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 1 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_occupied() == 1 );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );

  ch.clear();
}

TEST_CASE( "CargoHold compactify size-6 with small unit" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 },
                                 /*starting_slot=*/3 ) );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 5 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[0] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[3] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 5 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 5 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );

  ch.clear();
}

TEST_CASE( "CargoHold compactify size-6 with size-4 unit" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE_FALSE(
      ch.try_add( Cargo::unit{ unit_id1 }, /*slot=*/3 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, /*slot=*/2 ) );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 2 );
  REQUIRE( ch.slots_occupied() == 4 );
  REQUIRE( ch[0] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[2] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::overflow{} } );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 2 );
  REQUIRE( ch.slots_occupied() == 4 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 2 );
  REQUIRE( ch.slots_occupied() == 4 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );

  ch.clear();
}

TEST_CASE( "CargoHold compactify size-6 with size-6 unit" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::large_treasure ) );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE_FALSE(
      ch.try_add( Cargo::unit{ unit_id1 }, /*slot=*/1 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, /*slot=*/0 ) );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_occupied() == 6 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::overflow{} } );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_occupied() == 6 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::overflow{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::overflow{} } );

  ch.clear();
}

TEST_CASE( "CargoHold compactify multiple units" ) {
  CargoHoldTester ch( 20 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id3 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id4 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id5 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::large_treasure ) );
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 20 );
  REQUIRE( ch.slots_remaining() == 20 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id3 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id5 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id4 } ) );
  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id2 } ) );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[1] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id3 } } } );
  REQUIRE( ch[5] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id5 } } } );
  REQUIRE( ch[11] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id4 } } } );
  REQUIRE( ch[15] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id2 } } } );
  REQUIRE( ch.slots_total() == 20 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 16 );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 20 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 16 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id5 } } } );
  REQUIRE( ch[6] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id3 } } } );
  REQUIRE( ch[10] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id4 } } } );
  REQUIRE( ch[14] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[15] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id2 } } } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 20 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 16 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id5 } } } );
  REQUIRE( ch[6] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id3 } } } );
  REQUIRE( ch[10] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id4 } } } );
  REQUIRE( ch[14] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[15] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id2 } } } );

  ch.clear();
}

TEST_CASE( "CargoHold compactify size-1 with commodity" ) {
  CargoHoldTester ch( 1 );

  auto food_full = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 1 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE(
      ch.try_add_somewhere( Cargo::commodity{ food_full } ) );
  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_occupied() == 1 );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 1 );
  REQUIRE( ch.slots_remaining() == 0 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );

  ch.clear();
}

TEST_CASE(
    "CargoHold compactify size-6 single commodity partial" ) {
  CargoHoldTester ch( 6 );

  auto food_part = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/75 };

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_part },
                                 /*starting_slot=*/3 ) );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 5 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[0] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[3] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_part } } } );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 5 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_part } } } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 5 );
  REQUIRE( ch.slots_occupied() == 1 );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_part } } } );

  ch.clear();
}

TEST_CASE(
    "CargoHold compactify multiple commodities same type "
    "full" ) {
  CargoHoldTester ch( 6 );

  auto food_full = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_full },
                                 /*starting_slot=*/3 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_full },
                                 /*starting_slot=*/3 ) );
  REQUIRE( ch.try_add_somewhere( Cargo::commodity{ food_full },
                                 /*starting_slot=*/3 ) );
  REQUIRE( ch[0] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[3] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[4] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[5] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 3 );
  REQUIRE( ch.slots_occupied() == 3 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[1] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[2] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 3 );
  REQUIRE( ch.slots_occupied() == 3 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[1] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[2] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );

  ch.clear();
}

TEST_CASE(
    "CargoHold compactify multiple commodities same type "
    "partial" ) {
  CargoHoldTester ch( 6 );

  auto food_full        = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto food_almost_full = Commodity{ /*type=*/e_commodity::food,
                                     /*quantity=*/98 };
  auto food_part        = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/66 };

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 6 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE(
      ch.try_add( Cargo::commodity{ food_part }, /*slot=*/3 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_part }, /*slot=*/4 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_part }, /*slot=*/5 ) );
  REQUIRE( ch[0] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[3] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_part } } } );
  REQUIRE( ch[4] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_part } } } );
  REQUIRE( ch[5] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_part } } } );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 2 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::cargo{
                        /*contents=*/Cargo::commodity{
                            food_almost_full } } } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 6 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 2 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[1] == CargoSlot_t{ CargoSlot::cargo{
                        /*contents=*/Cargo::commodity{
                            food_almost_full } } } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[3] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );

  ch.clear();
}

TEST_CASE(
    "CargoHold compactify multiple commodities different "
    "types" ) {
  CargoHoldTester ch( 7 );

  auto food_full      = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto sugar_full     = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/100 };
  auto food_overflow  = Commodity{ /*type=*/e_commodity::food,
                                  /*quantity=*/32 };
  auto sugar_combined = Commodity{ /*type=*/e_commodity::sugar,
                                   /*quantity=*/66 };
  auto food_part      = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/66 };
  auto sugar_part     = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/33 };

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 7 );
  REQUIRE( ch.slots_remaining() == 7 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE(
      ch.try_add( Cargo::commodity{ sugar_part }, /*slot=*/1 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_part }, /*slot=*/2 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ sugar_part }, /*slot=*/3 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ sugar_full }, /*slot=*/4 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_part }, /*slot=*/5 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_full }, /*slot=*/6 ) );
  REQUIRE( ch.slots_total() == 7 );
  REQUIRE( ch.slots_remaining() == 1 );
  REQUIRE( ch.slots_occupied() == 6 );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 7 );
  REQUIRE( ch.slots_remaining() == 2 );
  REQUIRE( ch.slots_occupied() == 5 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[1] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::cargo{
                        /*contents=*/Cargo::commodity{
                            food_overflow } } } );
  REQUIRE( ch[3] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ sugar_full } } } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::cargo{
                        /*contents=*/Cargo::commodity{
                            sugar_combined } } } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[6] == CargoSlot_t{ CargoSlot::empty{} } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 7 );
  REQUIRE( ch.slots_remaining() == 2 );
  REQUIRE( ch.slots_occupied() == 5 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[1] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[2] == CargoSlot_t{ CargoSlot::cargo{
                        /*contents=*/Cargo::commodity{
                            food_overflow } } } );
  REQUIRE( ch[3] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ sugar_full } } } );
  REQUIRE( ch[4] == CargoSlot_t{ CargoSlot::cargo{
                        /*contents=*/Cargo::commodity{
                            sugar_combined } } } );
  REQUIRE( ch[5] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[6] == CargoSlot_t{ CargoSlot::empty{} } );

  ch.clear();
}

TEST_CASE( "CargoHold compactify units and commodities" ) {
  CargoHoldTester ch( 24 );
  auto food_full      = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto food_overflow  = Commodity{ /*type=*/e_commodity::food,
                                  /*quantity=*/32 };
  auto sugar_combined = Commodity{ /*type=*/e_commodity::sugar,
                                   /*quantity=*/66 };
  auto food_part      = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/66 };
  auto sugar_part     = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/33 };
  auto unit_id1       = create_unit(
            GameState::units(), e_nation::english,
            UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id3 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id4 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id5 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::large_treasure ) );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 24 );
  REQUIRE( ch.slots_remaining() == 24 );
  REQUIRE( ch.slots_occupied() == 0 );

  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, /*slot=*/0 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_full }, /*slot=*/1 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id5 }, /*slot=*/2 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ sugar_part }, /*slot=*/8 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_part }, /*slot=*/9 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id4 }, /*slot=*/10 ) );
  REQUIRE( ch.try_add( Cargo::commodity{ sugar_part },
                       /*slot=*/14 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id3 }, /*slot=*/15 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id2 }, /*slot=*/19 ) );
  REQUIRE(
      ch.try_add( Cargo::commodity{ food_part }, /*slot=*/20 ) );
  REQUIRE( ch.slots_total() == 24 );
  REQUIRE( ch.slots_remaining() == 3 );
  REQUIRE( ch.slots_occupied() == 21 );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 24 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 20 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id5 } } } );
  REQUIRE( ch[6] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id3 } } } );
  REQUIRE( ch[10] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id4 } } } );
  REQUIRE( ch[14] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[15] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id2 } } } );
  REQUIRE( ch[16] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[17] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[18] == CargoSlot_t{ CargoSlot::cargo{
                         /*contents=*/Cargo::commodity{
                             food_overflow } } } );
  REQUIRE( ch[19] == CargoSlot_t{ CargoSlot::cargo{
                         /*contents=*/Cargo::commodity{
                             sugar_combined } } } );
  REQUIRE( ch[20] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[21] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[22] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[23] == CargoSlot_t{ CargoSlot::empty{} } );

  ch.clear();
}

TEST_CASE(
    "CargoHold compactify units and commodities shuffle" ) {
  CargoHoldTester ch( 24 );
  auto food_full      = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto food_overflow  = Commodity{ /*type=*/e_commodity::food,
                                  /*quantity=*/32 };
  auto sugar_combined = Commodity{ /*type=*/e_commodity::sugar,
                                   /*quantity=*/66 };
  auto food_part      = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/66 };
  auto sugar_part     = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/33 };
  auto unit_id1       = create_unit(
            GameState::units(), e_nation::english,
            UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id3 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::large_treasure ) );
  auto unit_id4 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id5 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 24 );
  REQUIRE( ch.slots_remaining() == 24 );
  REQUIRE( ch.slots_occupied() == 0 );

  Cargo_t cargos[10] = {
      Cargo::unit{ unit_id1 },        //
      Cargo::unit{ unit_id2 },        //
      Cargo::unit{ unit_id3 },        //
      Cargo::unit{ unit_id4 },        //
      Cargo::unit{ unit_id5 },        //
      Cargo::commodity{ food_part },  //
      Cargo::commodity{ food_part },  //
      Cargo::commodity{ food_full },  //
      Cargo::commodity{ sugar_part }, //
      Cargo::commodity{ sugar_part }  //
  };

  // Catch2 takes its seed on the command line; from this seed we
  // generate some random numbers to reseed our own random gener-
  // ator to get predictable results based on Catch2's seed.
  uint32_t sub_seed =
      GENERATE( take( 100, random( 0, 1000000 ) ) );
  INFO( fmt::format( "sub_seed: {}", sub_seed ) );
  rng::reseed( sub_seed );

  vector<Cargo_t> shuffled( begin( cargos ), end( cargos ) );
  rng::shuffle( shuffled );

  for( auto const& cargo : shuffled ) {
    auto maybe_idx = util::find_index(
        ch.o_.slots, CargoSlot_t{ CargoSlot::empty{} } );
    REQUIRE( maybe_idx.has_value() );
    // Don't use try_add_somewhere here because we don't want
    // to consolidate commodities a priori.
    REQUIRE( ch.try_add( cargo, /*slot=*/*maybe_idx ) );
    INFO( fmt::format( "index {}: {}", *maybe_idx, cargo ) );
  }
  REQUIRE( ch.slots_total() == 24 );
  REQUIRE( ch.slots_remaining() == 3 );
  REQUIRE( ch.slots_occupied() == 21 );

  INFO( fmt::format( "Shuffled cargo: {}",
                     base::FmtJsonStyleList{ shuffled } ) );

  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 24 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 20 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id3 } } } );
  REQUIRE( ch[6] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id2 } } } );
  REQUIRE( ch[10] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id4 } } } );
  REQUIRE( ch[14] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[15] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id5 } } } );
  REQUIRE( ch[16] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[17] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[18] == CargoSlot_t{ CargoSlot::cargo{
                         /*contents=*/Cargo::commodity{
                             food_overflow } } } );
  REQUIRE( ch[19] == CargoSlot_t{ CargoSlot::cargo{
                         /*contents=*/Cargo::commodity{
                             sugar_combined } } } );
  REQUIRE( ch[20] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[21] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[22] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[23] == CargoSlot_t{ CargoSlot::empty{} } );

  // Test idempotency.
  REQUIRE_NOTHROW( ch.compactify() );
  REQUIRE( ch.slots_total() == 24 );
  REQUIRE( ch.slots_remaining() == 4 );
  REQUIRE( ch.slots_occupied() == 20 );
  REQUIRE( ch[0] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id3 } } } );
  REQUIRE( ch[6] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id2 } } } );
  REQUIRE( ch[10] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id4 } } } );
  REQUIRE( ch[14] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id1 } } } );
  REQUIRE( ch[15] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::unit{ unit_id5 } } } );
  REQUIRE( ch[16] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[17] ==
           CargoSlot_t{ CargoSlot::cargo{
               /*contents=*/Cargo::commodity{ food_full } } } );
  REQUIRE( ch[18] == CargoSlot_t{ CargoSlot::cargo{
                         /*contents=*/Cargo::commodity{
                             food_overflow } } } );
  REQUIRE( ch[19] == CargoSlot_t{ CargoSlot::cargo{
                         /*contents=*/Cargo::commodity{
                             sugar_combined } } } );
  REQUIRE( ch[20] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[21] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[22] == CargoSlot_t{ CargoSlot::empty{} } );
  REQUIRE( ch[23] == CargoSlot_t{ CargoSlot::empty{} } );

  REQUIRE_THAT( ch.units(), UnorderedEquals( vector<UnitId>{
                                unit_id1, unit_id2, unit_id3,
                                unit_id4, unit_id5 } ) );

  REQUIRE_THAT( ch.commodities(),
                UnorderedEquals( vector<pair<Commodity, int>>{
                    { food_full, 16 },
                    { food_full, 17 },
                    { food_overflow, 18 },
                    { sugar_combined, 19 } } ) );

  ch.clear();
}

TEST_CASE( "CargoHold find_unit" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id3 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );

  REQUIRE( ch.find_unit( unit_id1 ) == nothing );
  REQUIRE( ch.find_unit( unit_id2 ) == nothing );
  REQUIRE( ch.find_unit( unit_id3 ) == nothing );

  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );

  REQUIRE( ch.find_unit( unit_id1 ) == 0 );
  REQUIRE( ch.find_unit( unit_id2 ) == nothing );
  REQUIRE( ch.find_unit( unit_id3 ) == nothing );

  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id2 } ) );

  REQUIRE( ch.find_unit( unit_id1 ) == 0 );
  REQUIRE( ch.find_unit( unit_id2 ) == 1 );
  REQUIRE( ch.find_unit( unit_id3 ) == nothing );

  REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id3 } ) );

  REQUIRE( ch.find_unit( unit_id1 ) == 0 );
  REQUIRE( ch.find_unit( unit_id2 ) == 1 );
  REQUIRE( ch.find_unit( unit_id3 ) == 5 );

  REQUIRE_THAT( ch.units(),
                UnorderedEquals( vector<UnitId>{
                    unit_id1, unit_id2, unit_id3 } ) );

  ch.clear();
}

TEST_CASE( "CargoHold fits_with_item_removed" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id3 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );

  SECTION( "insert small unit first" ) {
    REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 1 ) );

    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id1 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 0 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id1 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 1 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id1 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 2 } ) );

    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 0 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 1 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 2 } ) );

    REQUIRE( ch.try_add( Cargo::unit{ unit_id3 }, 5 ) );

    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 0 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 1 } ) );
    REQUIRE_FALSE( ch.fits_with_item_removed(
        Cargo::unit{ unit_id2 }, CargoSlotIndex{ 1 },
        CargoSlotIndex{ 2 } ) );
  }

  SECTION( "insert large unit first" ) {
    REQUIRE( ch.try_add( Cargo::unit{ unit_id2 }, 1 ) );

    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id1 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 0 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id1 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 1 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id1 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 2 } ) );

    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 0 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 1 } ) );
    REQUIRE( ch.fits_with_item_removed( Cargo::unit{ unit_id2 },
                                        CargoSlotIndex{ 1 },
                                        CargoSlotIndex{ 2 } ) );
  }

  ch.clear();
}

TEST_CASE( "CargoHold fits_somewhere_with_item_removed" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  auto unit_id3 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::large_treasure ) );
  auto unit_id4 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );
  auto unit_id5 =
      create_unit( GameState::units(), e_nation::english,
                   UnitType::create( e_unit_type::soldier ) );

  SECTION( "insert large unit" ) {
    REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 1 ) );
    REQUIRE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id3 }, /*remove_slot=*/1,
        /*starting_slot=*/0 ) );

    REQUIRE( ch.try_add( Cargo::unit{ unit_id4 }, 5 ) );
    REQUIRE_FALSE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id3 }, /*remove_slot=*/1,
        /*starting_slot=*/0 ) );
  }

  SECTION( "insert samll units" ) {
    REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 1 ) );
    REQUIRE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id2 }, /*remove_slot=*/1,
        /*starting_slot=*/0 ) );
    REQUIRE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id2 }, /*remove_slot=*/1,
        /*starting_slot=*/1 ) );

    REQUIRE( ch.try_add( Cargo::unit{ unit_id4 }, 4 ) );
    REQUIRE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id2 }, /*remove_slot=*/4,
        /*starting_slot=*/0 ) );
    REQUIRE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id2 }, /*remove_slot=*/4,
        /*starting_slot=*/1 ) );

    REQUIRE( ch.try_add( Cargo::unit{ unit_id5 }, 3 ) );
    REQUIRE_FALSE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id2 }, /*remove_slot=*/3,
        /*starting_slot=*/0 ) );
    REQUIRE_FALSE( ch.fits_somewhere_with_item_removed(
        Cargo::unit{ unit_id2 }, /*remove_slot=*/3,
        /*starting_slot=*/1 ) );
  }

  ch.clear();
}

TEST_CASE( "CargoHold cago_starting_at_slot" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 1 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id2 }, 2 ) );

  {
    maybe<Cargo_t const&> c = ch.cargo_starting_at_slot( 0 );
    REQUIRE_FALSE( c.has_value() );
  }
  {
    maybe<Cargo_t const&> c = ch.cargo_starting_at_slot( 1 );
    REQUIRE( c.has_value() );
    REQUIRE( c == Cargo_t{ Cargo::unit{ unit_id1 } } );
  }
  {
    maybe<Cargo_t const&> c = ch.cargo_starting_at_slot( 2 );
    REQUIRE( c.has_value() );
    REQUIRE( c == Cargo_t{ Cargo::unit{ unit_id2 } } );
  }
  {
    maybe<Cargo_t const&> c = ch.cargo_starting_at_slot( 3 );
    REQUIRE_FALSE( c.has_value() );
  }
  {
    maybe<Cargo_t const&> c = ch.cargo_starting_at_slot( 4 );
    REQUIRE_FALSE( c.has_value() );
  }
  {
    maybe<Cargo_t const&> c = ch.cargo_starting_at_slot( 5 );
    REQUIRE_FALSE( c.has_value() );
  }

  ch.clear();
}

TEST_CASE( "CargoHold cago_covering_slot" ) {
  CargoHoldTester ch( 6 );

  auto unit_id1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
  auto unit_id2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id1 }, 1 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id2 }, 2 ) );

  {
    maybe<pair<Cargo_t const&, int>> c =
        ch.cargo_covering_slot( 0 );
    REQUIRE_FALSE( c.has_value() );
  }
  {
    maybe<pair<Cargo_t const&, int>> c =
        ch.cargo_covering_slot( 1 );
    REQUIRE( c.has_value() );
    REQUIRE( c->second == 1 );
    REQUIRE( c->first == Cargo_t{ Cargo::unit{ unit_id1 } } );
  }
  {
    maybe<pair<Cargo_t const&, int>> c =
        ch.cargo_covering_slot( 2 );
    REQUIRE( c.has_value() );
    REQUIRE( c->second == 2 );
    REQUIRE( c->first == Cargo_t{ Cargo::unit{ unit_id2 } } );
  }
  {
    maybe<pair<Cargo_t const&, int>> c =
        ch.cargo_covering_slot( 3 );
    REQUIRE( c.has_value() );
    REQUIRE( c->second == 2 );
    REQUIRE( c->first == Cargo_t{ Cargo::unit{ unit_id2 } } );
  }
  {
    maybe<pair<Cargo_t const&, int>> c =
        ch.cargo_covering_slot( 4 );
    REQUIRE( c.has_value() );
    REQUIRE( c->second == 2 );
    REQUIRE( c->first == Cargo_t{ Cargo::unit{ unit_id2 } } );
  }
  {
    maybe<pair<Cargo_t const&, int>> c =
        ch.cargo_covering_slot( 5 );
    REQUIRE( c.has_value() );
    REQUIRE( c->second == 2 );
    REQUIRE( c->first == Cargo_t{ Cargo::unit{ unit_id2 } } );
  }

  ch.clear();
}

TEST_CASE( "CargoHold max_commodity_quantity_that_fits" ) {
  auto food_full  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto food_part  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/66 };
  auto sugar_part = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/33 };
  auto unit_id1   = create_unit(
        GameState::units(), e_nation::english,
        UnitType::create( e_unit_type::small_treasure ) );

  SECTION( "size zero" ) {
    CargoHoldTester ch( 0 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 0 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 0 );
    ch.clear();
  }

  SECTION( "size one full" ) {
    CargoHoldTester ch( 1 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 100 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 100 );
    REQUIRE(
        ch.try_add_somewhere( Cargo::commodity{ food_full } ) );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 0 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 0 );
    ch.clear();
  }

  SECTION( "size one partial" ) {
    CargoHoldTester ch( 1 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 100 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 100 );
    REQUIRE(
        ch.try_add_somewhere( Cargo::commodity{ food_part } ) );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 34 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 0 );
    ch.clear();
  }

  SECTION( "size six" ) {
    CargoHoldTester ch( 6 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 600 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 600 );
    REQUIRE(
        ch.try_add_somewhere( Cargo::commodity{ food_full } ) );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 500 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 500 );
    REQUIRE(
        ch.try_add_somewhere( Cargo::commodity{ sugar_part } ) );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 400 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 467 );
    ch.clear();
  }

  SECTION( "size six with units" ) {
    CargoHoldTester ch( 6 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 600 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 600 );
    REQUIRE( ch.try_add_somewhere( Cargo::unit{ unit_id1 } ) );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 200 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 200 );
    REQUIRE(
        ch.try_add_somewhere( Cargo::commodity{ sugar_part } ) );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::food ) == 100 );
    REQUIRE( ch.max_commodity_quantity_that_fits(
                 e_commodity::sugar ) == 167 );
    ch.clear();
  }
}

TEST_CASE( "CargoHold slot_holds_cargo_type" ) {
  CargoHoldTester ch( 8 );

  auto food    = Commodity{ /*type=*/e_commodity::food,
                         /*quantity=*/100 };
  auto unit_id = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::small_treasure ) );

  REQUIRE( ch.try_add( Cargo::commodity{ food }, 1 ) );
  REQUIRE( ch.try_add( Cargo::unit{ unit_id }, 2 ) );

  REQUIRE_FALSE( ch.slot_holds_cargo_type<Cargo::unit>( 0 ) );
  REQUIRE( ch.slot_holds_cargo_type<Cargo::unit>( 2 ) );
  REQUIRE( ch.slot_holds_cargo_type<Cargo::unit>( 2 ).value() ==
           Cargo::unit{ unit_id } );
  REQUIRE_FALSE( ch.slot_holds_cargo_type<Cargo::unit>( 1 ) );
  REQUIRE_FALSE( ch.slot_holds_cargo_type<Cargo::unit>( 6 ) );
  REQUIRE_FALSE( ch.slot_holds_cargo_type<Cargo::unit>( 7 ) );

  REQUIRE_FALSE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 0 ) );
  REQUIRE( ch.slot_holds_cargo_type<Cargo::commodity>( 1 ) );
  REQUIRE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 1 ).value() ==
      Cargo::commodity{ food } );
  REQUIRE_FALSE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 2 ) );
  REQUIRE_FALSE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 3 ) );
  REQUIRE_FALSE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 4 ) );
  REQUIRE_FALSE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 5 ) );
  REQUIRE_FALSE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 6 ) );
  REQUIRE_FALSE(
      ch.slot_holds_cargo_type<Cargo::commodity>( 7 ) );

  ch.clear();
}

TEST_CASE( "CargoHold max_commodity_per_cargo_slot" ) {
  CargoHoldTester ch( 2 );
  REQUIRE( ch.max_commodity_per_cargo_slot() == 100 );
  ch.clear();
}

} // namespace
} // namespace rn
