/****************************************************************
**promotion.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-10.
*
* Description: Unit tests for the src/promotion.* module.
*
*****************************************************************/
#include "test/fake/world.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/promotion.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/unit-composer.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_player( e_nation::dutch ); }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }

  inline static Coord const kLand = Coord{ .x = 1, .y = 1 };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[promotion] try_promote_unit_for_current_activity" ) {
  World W;
  W.create_default_map();

  auto up = [&]( Unit& unit ) {
    return try_promote_unit_for_current_activity( W.ss(), unit );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer );
    Unit& unit = W.units().unit_for( id );
    expected   = UnitComposition( wrapped::UnitComposition{
          .type = UnitType::create( e_unit_type::expert_farmer ),
          .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == false );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "petty_criminal carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal );
    Unit& unit = W.units().unit_for( id );
    expected   = UnitComposition( wrapped::UnitComposition{
          .type = UnitType::create( e_unit_type::petty_criminal ),
          .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == true );
    expected = UnitComposition( wrapped::UnitComposition{
        .type =
            UnitType::create( e_unit_type::indentured_servant ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == true );
    expected = UnitComposition( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::free_colonist ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == true );
    expected = UnitComposition( wrapped::UnitComposition{
        .type =
            UnitType::create( e_unit_type::master_carpenter ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == false );
    expected = UnitComposition( wrapped::UnitComposition{
        .type =
            UnitType::create( e_unit_type::master_carpenter ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "petty_criminal farmer" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id     = W.add_unit_outdoors(
                     colony.id, e_direction::w, e_outdoor_job::food,
                     e_unit_type::petty_criminal );
    Unit& unit = W.units().unit_for( id );
    expected   = UnitComposition( wrapped::UnitComposition{
          .type = UnitType::create( e_unit_type::petty_criminal ),
          .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == true );
    expected = UnitComposition( wrapped::UnitComposition{
        .type =
            UnitType::create( e_unit_type::indentured_servant ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == true );
    expected = UnitComposition( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::free_colonist ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == true );
    expected = UnitComposition( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::expert_farmer ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == false );
    expected = UnitComposition( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::expert_farmer ),
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "petty_criminal no job" ) {
    UnitComposition expected;
    UnitId id   = W.add_unit_on_map( e_unit_type::petty_criminal,
                                     W.kLand );
    Unit&  unit = W.units().unit_for( id );
    expected    = UnitComposition( wrapped::UnitComposition{
           .type = UnitType::create( e_unit_type::petty_criminal ),
           .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == false );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::expert_farmer, W.kLand );
    Unit& unit = W.units().unit_for( id );
    expected   = UnitComposition( wrapped::UnitComposition{
          .type = UnitType::create( e_unit_type::expert_farmer ),
          .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == false );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "expert_farmer dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id   = W.add_unit_on_map( initial_ut, W.kLand );
    Unit&  unit = W.units().unit_for( id );
    expected    = UnitComposition( wrapped::UnitComposition{
           .type      = initial_ut,
           .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == false );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::dragoon,
                          e_unit_type::petty_criminal ) );
    UnitId unit_id = W.add_unit_on_map( initial_ut, W.kLand );
    Unit&  unit    = W.units().unit_for( unit_id );
    {
      UNWRAP_CHECK(
          expected_ut,
          UnitType::create( e_unit_type::dragoon,
                            e_unit_type::petty_criminal ) );
      expected = UnitComposition( wrapped::UnitComposition{
          .type      = expected_ut,
          .inventory = {},
      } );
      REQUIRE( unit.composition() == expected );
    }

    REQUIRE( up( unit ) == true );
    {
      UNWRAP_CHECK(
          expected_ut,
          UnitType::create( e_unit_type::dragoon,
                            e_unit_type::indentured_servant ) );
      expected = UnitComposition( wrapped::UnitComposition{
          .type      = expected_ut,
          .inventory = {},
      } );
      REQUIRE( unit.composition() == expected );
    }

    REQUIRE( up( unit ) == true );
    {
      UNWRAP_CHECK(
          expected_ut,
          UnitType::create( e_unit_type::dragoon,
                            e_unit_type::free_colonist ) );
      expected = UnitComposition( wrapped::UnitComposition{
          .type      = expected_ut,
          .inventory = {},
      } );
      REQUIRE( unit.composition() == expected );
    }

    REQUIRE( up( unit ) == true );
    {
      UNWRAP_CHECK(
          expected_ut,
          UnitType::create( e_unit_type::veteran_dragoon,
                            e_unit_type::veteran_colonist ) );
      expected = UnitComposition( wrapped::UnitComposition{
          .type      = expected_ut,
          .inventory = {},
      } );
      REQUIRE( unit.composition() == expected );
    }

    REQUIRE( up( unit ) == false );
    {
      UNWRAP_CHECK(
          expected_ut,
          UnitType::create( e_unit_type::veteran_dragoon,
                            e_unit_type::veteran_colonist ) );
      expected = UnitComposition( wrapped::UnitComposition{
          .type      = expected_ut,
          .inventory = {},
      } );
      REQUIRE( unit.composition() == expected );
    }
  }

  SECTION( "pioneer" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::petty_criminal ) );
    UnitId id   = W.add_unit_on_map( initial_ut, W.kLand );
    Unit&  unit = W.units().unit_for( id );
    unit.consume_20_tools();
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::petty_criminal ) );

    REQUIRE( up( unit ) == true );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE(
        unit.type_obj() ==
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::indentured_servant ) );

    REQUIRE( up( unit ) == true );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::free_colonist ) );

    REQUIRE( up( unit ) == true );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist ) );

    REQUIRE( up( unit ) == false );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist ) );
  }
}

} // namespace
} // namespace rn
