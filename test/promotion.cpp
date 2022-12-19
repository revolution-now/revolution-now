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

// Testing
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/unit-composer.hpp"
#include "src/ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::Approx;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_default_player(); }

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
    return try_promote_unit_for_current_activity(
        W.ss(), W.default_player(), unit );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    Unit&           unit =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer );
    expected = UnitComposition( wrapped::UnitComposition{
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
    Unit&           unit =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal );
    expected = UnitComposition( wrapped::UnitComposition{
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
    Unit&           unit   = W.add_unit_outdoors(
        colony.id, e_direction::w, e_outdoor_job::food,
        e_unit_type::petty_criminal );
    expected = UnitComposition( wrapped::UnitComposition{
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
    Unit& unit = W.add_unit_on_map( e_unit_type::petty_criminal,
                                    W.kLand );
    expected   = UnitComposition( wrapped::UnitComposition{
          .type = UnitType::create( e_unit_type::petty_criminal ),
          .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    REQUIRE( up( unit ) == false );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    Unit&           unit =
        W.add_unit_on_map( e_unit_type::expert_farmer, W.kLand );
    expected = UnitComposition( wrapped::UnitComposition{
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
    Unit& unit = W.add_unit_on_map( initial_ut, W.kLand );
    expected   = UnitComposition( wrapped::UnitComposition{
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
    Unit& unit = W.add_unit_on_map( initial_ut, W.kLand );
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
    Unit& unit = W.add_unit_on_map( initial_ut, W.kLand );
    unit.consume_20_tools( W.default_player() );
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

TEST_CASE( "[unit-type] promoted_from_activity" ) {
  auto f = []( UnitType ut, e_unit_activity activity ) {
    return promoted_from_activity( UnitComposition::create( ut ),
                                   activity );
  };
  auto g = []( UnitComposition uc, e_unit_activity activity ) {
    return promoted_from_activity( uc, activity );
  };
  using UT  = e_unit_type;
  using Act = e_unit_activity;
  UnitType        ut;
  UnitComposition uc;
  UnitComposition expected;
  e_unit_activity act;

  SECTION( "base types" ) {
    // petty_criminal.
    ut       = UnitType::create( UT::petty_criminal );
    act      = Act::farming;
    expected = UnitComposition::create( UT::indentured_servant );
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    // indentured_servant.
    ut       = UnitType::create( UT::indentured_servant );
    act      = Act::farming;
    expected = UnitComposition::create( UT::free_colonist );
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    // free_colonist.
    ut       = UnitType::create( UT::free_colonist );
    act      = Act::farming;
    expected = UnitComposition::create( UT::expert_farmer );
    REQUIRE( f( ut, act ) == expected );
    act      = Act::bell_ringing;
    expected = UnitComposition::create( UT::elder_statesman );
    REQUIRE( f( ut, act ) == expected );
    act      = Act::fighting;
    expected = UnitComposition::create( UT::veteran_colonist );
    REQUIRE( f( ut, act ) == expected );
    act      = Act::scouting;
    expected = UnitComposition::create( UT::seasoned_colonist );
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    expected =
        UnitComposition::create( UT::expert_cotton_planter );
    REQUIRE( f( ut, act ) == expected );
    // expert_cotton_planer.
    ut  = UnitType::create( UT::expert_cotton_planter );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // veteran_colonist.
    ut  = UnitType::create( UT::veteran_colonist );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // cavalry.
    ut  = UnitType::create( UT::cavalry );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // damaged_artillery.
    ut  = UnitType::create( UT::damaged_artillery );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // native_convert.
    ut  = UnitType::create( UT::native_convert );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // privateer.
    ut  = UnitType::create( UT::privateer );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // wagon_train.
    ut  = UnitType::create( UT::wagon_train );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // treasure.
    ut  = UnitType::create( UT::treasure );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
  }
  SECTION( "non-expert modified colonists" ) {
    // pioneer/petty_criminal.
    ut = UnitType::create( UT::pioneer, UT::petty_criminal )
             .value();
    expected = UnitComposition::create(
        UnitType::create( UT::pioneer, UT::indentured_servant )
            .value() );
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // pioneer/indentured_servant.
    ut = UnitType::create( UT::pioneer, UT::indentured_servant )
             .value();
    expected = UnitComposition::create(
        UnitType::create( UT::pioneer, UT::free_colonist )
            .value() );
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // pioneer/free_colonist.
    ut = UnitType::create( UT::pioneer, UT::free_colonist )
             .value();
    expected = UnitComposition::create(
        UnitType::create( UT::hardy_pioneer, UT::hardy_colonist )
            .value() );
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // pioneer 20 tools/free_colonist.
    uc = UnitComposition::create(
             UnitType::create( UT::pioneer, UT::free_colonist )
                 .value(),
             { { e_unit_inventory::tools, 20 } } )
             .value();
    expected = UnitComposition::create(
                   UnitType::create( UT::hardy_pioneer,
                                     UT::hardy_colonist )
                       .value(),
                   { { e_unit_inventory::tools, 20 } } )
                   .value();
    act = Act::farming;
    REQUIRE( g( uc, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( g( uc, act ) == expected );
    act = Act::fighting;
    REQUIRE( g( uc, act ) == expected );
    act = Act::scouting;
    REQUIRE( g( uc, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( g( uc, act ) == expected );
    act = Act::pioneering;
    REQUIRE( g( uc, act ) == expected );
    // pioneer/expert_farmer.
    ut = UnitType::create( UT::pioneer, UT::expert_farmer )
             .value();
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::pioneering;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // pioneer 20 tools/expert_farmer.
    uc = UnitComposition::create(
             UnitType::create( UT::pioneer, UT::expert_farmer )
                 .value(),
             { { e_unit_inventory::tools, 20 } } )
             .value();
    expected = UnitComposition::create(
                   UnitType::create( UT::hardy_pioneer,
                                     UT::hardy_colonist )
                       .value(),
                   { { e_unit_inventory::tools, 20 } } )
                   .value();
    act = Act::farming;
    REQUIRE_FALSE( g( uc, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( g( uc, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( g( uc, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( g( uc, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( g( uc, act ).has_value() );
    act = Act::pioneering;
    REQUIRE_FALSE( g( uc, act ).has_value() );
    // dragoon/petty_criminal.
    ut = UnitType::create( UT::dragoon, UT::petty_criminal )
             .value();
    expected = UnitComposition::create(
        UnitType::create( UT::dragoon, UT::indentured_servant )
            .value() );
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // dragoon/free_colonist.
    ut = UnitType::create( UT::dragoon, UT::free_colonist )
             .value();
    expected = UnitComposition::create(
        UnitType::create( UT::veteran_dragoon,
                          UT::veteran_colonist )
            .value() );
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // dragoon/expert_farmer.
    ut = UnitType::create( UT::dragoon, UT::expert_farmer )
             .value();
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::pioneering;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // scout/free_colonist.
    ut =
        UnitType::create( UT::scout, UT::free_colonist ).value();
    expected = UnitComposition::create(
        UnitType::create( UT::seasoned_scout,
                          UT::seasoned_colonist )
            .value() );
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
  }
  SECTION( "expert modified colonists" ) {
    // veteran_soldier
    ut       = UnitType::create( UT::veteran_soldier );
    expected = UnitComposition::create( UT::continental_army );
    act      = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // veteran_dragoon
    ut = UnitType::create( UT::veteran_dragoon );
    expected =
        UnitComposition::create( UT::continental_cavalry );
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // continental_army
    ut  = UnitType::create( UT::continental_army );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::pioneering;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // continental_cavalry
    ut  = UnitType::create( UT::continental_cavalry );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::pioneering;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    // hardy_pioneer
    ut  = UnitType::create( UT::hardy_pioneer );
    act = Act::farming;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::bell_ringing;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::fighting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::scouting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::cotton_planting;
    REQUIRE_FALSE( f( ut, act ).has_value() );
    act = Act::pioneering;
    REQUIRE_FALSE( f( ut, act ).has_value() );
  }
}

TEST_CASE( "[unit-type] cleared_expertise" ) {
  auto* f  = cleared_expertise;
  using UT = e_unit_type;
  UnitType ut;
  UnitType expected;

  SECTION( "base types" ) {
    // petty_criminal.
    ut = UnitType::create( UT::petty_criminal );
    REQUIRE( f( ut ) == nothing );
    // indentured_servant.
    ut = UnitType::create( UT::indentured_servant );
    REQUIRE( f( ut ) == nothing );
    // free_colonist.
    ut = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == nothing );
    // native_convert.
    ut = UnitType::create( UT::native_convert );
    REQUIRE( f( ut ) == nothing );
    // expert_farmer.
    ut       = UnitType::create( UT::expert_farmer );
    expected = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == expected );
    // veteran_colonist.
    ut       = UnitType::create( UT::veteran_colonist );
    expected = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == expected );
    // jesuit_missionary.
    ut       = UnitType::create( UT::jesuit_colonist );
    expected = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == expected );
    // caravel.
    ut = UnitType::create( UT::caravel );
    REQUIRE( f( ut ) == nothing );
    // artillery.
    ut = UnitType::create( UT::artillery );
    REQUIRE( f( ut ) == nothing );
  }
  SECTION( "non-expert modified colonists" ) {
    // dragoon/petty_criminal.
    ut = UnitType::create( UT::dragoon, UT::petty_criminal )
             .value();
    REQUIRE( f( ut ) == nothing );
    // dragoon/indentured_servant.
    ut = UnitType::create( UT::dragoon, UT::indentured_servant )
             .value();
    REQUIRE( f( ut ) == nothing );
    // dragoon/free_colonist.
    ut = UnitType::create( UT::dragoon, UT::free_colonist )
             .value();
    REQUIRE( f( ut ) == nothing );
    // scout/expert_farmer.
    ut =
        UnitType::create( UT::scout, UT::expert_farmer ).value();
    expected =
        UnitType::create( UT::scout, UT::free_colonist ).value();
    REQUIRE( f( ut ) == expected );
    // scout/veteran_colonist.
    ut = UnitType::create( UT::scout, UT::veteran_colonist )
             .value();
    expected =
        UnitType::create( UT::scout, UT::free_colonist ).value();
    REQUIRE( f( ut ) == expected );
    // pioneer/jesuit_colonist.
    ut = UnitType::create( UT::pioneer, UT::jesuit_colonist )
             .value();
    expected = UnitType::create( UT::pioneer, UT::free_colonist )
                   .value();
    REQUIRE( f( ut ) == expected );
  }
  SECTION( "expert modified colonists" ) {
    // veteran_dragoon.
    ut       = UnitType::create( UT::veteran_dragoon );
    expected = UnitType::create( UT::dragoon );
    REQUIRE( f( ut ) == expected );
    // continental_cavalry.
    ut       = UnitType::create( UT::continental_cavalry );
    expected = UnitType::create( UT::dragoon );
    REQUIRE( f( ut ) == expected );
    // continental_army.
    ut       = UnitType::create( UT::continental_army );
    expected = UnitType::create( UT::soldier );
    REQUIRE( f( ut ) == expected );
    // seasoned_scout.
    ut       = UnitType::create( UT::seasoned_scout );
    expected = UnitType::create( UT::scout );
    REQUIRE( f( ut ) == expected );
    // hardy_pioneer.
    ut       = UnitType::create( UT::hardy_pioneer );
    expected = UnitType::create( UT::pioneer );
    REQUIRE( f( ut ) == expected );
    // veteran_soldier.
    ut       = UnitType::create( UT::veteran_soldier );
    expected = UnitType::create( UT::soldier );
    REQUIRE( f( ut ) == expected );
    // jesuit_missionary..
    ut       = UnitType::create( UT::jesuit_missionary );
    expected = UnitType::create( UT::missionary );
    REQUIRE( f( ut ) == expected );
  }
}

TEST_CASE( "[unit-type] on_death_demoted_type" ) {
  auto* f  = on_death_demoted_type;
  using UT = e_unit_type;
  // No demoting.
  REQUIRE( f( UnitType::create( UT::free_colonist ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::native_convert ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::indentured_servant ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::expert_sugar_planter ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::damaged_artillery ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::caravel ) ) == nothing );
  // Demoting.
  REQUIRE( f( UnitType::create( UT::soldier ) ) ==
           UnitType::create( UT::free_colonist ) );
  REQUIRE(
      f( UnitType::create( UT::soldier, UT::indentured_servant )
             .value() ) ==
      UnitType::create( UT::indentured_servant ) );
  REQUIRE( f( UnitType::create( UT::dragoon ) ) ==
           UnitType::create( UT::soldier ) );
  REQUIRE(
      f( UnitType::create( UT::dragoon, UT::indentured_servant )
             .value() ) ==
      UnitType::create( UT::soldier, UT::indentured_servant )
          .value() );
  REQUIRE( f( UnitType::create( UT::veteran_soldier ) ) ==
           UnitType::create( UT::veteran_colonist ) );
  REQUIRE( f( UnitType::create( UT::veteran_dragoon ) ) ==
           UnitType::create( UT::veteran_soldier ) );
  REQUIRE( f( UnitType::create( UT::cavalry ) ) ==
           UnitType::create( UT::regular ) );
  REQUIRE( f( UnitType::create( UT::continental_army ) ) ==
           UnitType::create( UT::veteran_colonist ) );
  REQUIRE( f( UnitType::create( UT::continental_cavalry ) ) ==
           UnitType::create( UT::continental_army ) );
  REQUIRE( f( UnitType::create( UT::artillery ) ) ==
           UnitType::create( UT::damaged_artillery ) );
}

TEST_CASE( "[unit-type] on_capture_demoted_type" ) {
  auto* f  = on_capture_demoted_type;
  using UT = e_unit_type;
  // No demoting.
  REQUIRE( f( UnitType::create( UT::free_colonist ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::native_convert ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::indentured_servant ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::expert_sugar_planter ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::damaged_artillery ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::caravel ) ) == nothing );
  REQUIRE( f( UnitType::create( UT::cavalry ) ) == nothing );
  // Demoting.
  REQUIRE( f( UnitType::create( UT::veteran_colonist ) ) ==
           UT::free_colonist );
}

TEST_CASE( "[unit-type] on the job promotion" ) {
  World W;
  W.create_default_map();
  Colony& colony = W.add_colony( W.kLand );

  // Add indoor units. The default config says that none of these
  // units will ever be promoted since they are indoor.
  W.add_unit_indoors( colony.id, e_indoor_job::bells,
                      e_unit_type::petty_criminal );
  W.add_unit_indoors( colony.id, e_indoor_job::tools,
                      e_unit_type::indentured_servant );
  W.add_unit_indoors( colony.id, e_indoor_job::coats,
                      e_unit_type::free_colonist );
  W.add_unit_indoors( colony.id, e_indoor_job::rum,
                      e_unit_type::native_convert );
  W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                      e_unit_type::expert_sugar_planter );
  W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                      e_unit_type::master_weaver );

  // Add outdoor units. The default config says that in order to
  // be promoted you have to be either a petty criminal, inden-
  // tured servant, or free colonist, and you must be working at
  // planting one of the trade crops or fur trapping.

  // tries=yes, succeeds=yes.
  W.add_unit_outdoors( colony.id, e_direction::nw,
                       e_outdoor_job::sugar,
                       e_unit_type::petty_criminal );
  EXPECT_CALL( W.rand(),
               bernoulli( Approx( 0.00333, 0.00001 ) ) )
      .returns( true );

  // tries=no.
  W.add_unit_outdoors( colony.id, e_direction::n,
                       e_outdoor_job::lumber,
                       e_unit_type::petty_criminal );

  // tries=yes, succeeds=no.
  W.add_unit_outdoors( colony.id, e_direction::ne,
                       e_outdoor_job::tobacco,
                       e_unit_type::indentured_servant );
  EXPECT_CALL( W.rand(), bernoulli( Approx( 0.005, 0.00001 ) ) )
      .returns( false );

  // tries=no.
  W.add_unit_outdoors( colony.id, e_direction::w,
                       e_outdoor_job::food,
                       e_unit_type::indentured_servant );

  // tries=yes, succeeds=yes.
  W.add_unit_outdoors( colony.id, e_direction::e,
                       e_outdoor_job::cotton,
                       e_unit_type::free_colonist );
  EXPECT_CALL( W.rand(), bernoulli( Approx( 0.01, 0.00001 ) ) )
      .returns( true );

  // tries=no.
  W.add_unit_outdoors( colony.id, e_direction::sw,
                       e_outdoor_job::fish,
                       e_unit_type::free_colonist );

  // tries=no.
  W.add_unit_outdoors( colony.id, e_direction::s,
                       e_outdoor_job::sugar,
                       e_unit_type::native_convert );

  // tries=no.
  W.add_unit_outdoors( colony.id, e_direction::se,
                       e_outdoor_job::tobacco,
                       e_unit_type::expert_farmer );

  vector<OnTheJobPromotionResult> const res =
      workers_to_promote_for_on_the_job_training( W.ss(), W.ts(),
                                                  colony );

  vector<OnTheJobPromotionResult> const expected{
      { .unit_id     = UnitId{ 7 },
        .promoted_to = e_unit_type::expert_sugar_planter },
      { .unit_id     = UnitId{ 11 },
        .promoted_to = e_unit_type::expert_cotton_planter } };

  REQUIRE( res == expected );
}

TEST_CASE( "[unit-type] expert_for_activity" ) {
  auto f = [&]( e_unit_activity activity ) {
    return expert_for_activity( activity );
  };

  REQUIRE( f( e_unit_activity::fishing ) ==
           e_unit_type::expert_fisherman );
  REQUIRE( f( e_unit_activity::pioneering ) ==
           e_unit_type::hardy_colonist );
  REQUIRE( f( e_unit_activity::fighting ) ==
           e_unit_type::veteran_colonist );
}

TEST_CASE( "[unit-type] promoted_by_natives" ) {
  UnitComposition comp, expected;
  e_native_skill  skill = {};

  auto f = [&] { return promoted_by_natives( comp, skill ); };

  skill = e_native_skill::fishing;
  comp  = UnitComposition::create(
             UnitType::create( e_unit_type::pioneer,
                                e_unit_type::indentured_servant )
                 .value(),
             { { e_unit_inventory::tools, 80 } } )
             .value();
  expected =
      UnitComposition::create(
          UnitType::create( e_unit_type::pioneer,
                            e_unit_type::expert_fisherman )
              .value(),
          { { e_unit_inventory::tools, 80 } } )
          .value();
  REQUIRE( f() == expected );

  skill = e_native_skill::farming;
  comp =
      UnitComposition::create( e_unit_type::indentured_servant );
  expected =
      UnitComposition::create( e_unit_type::expert_farmer );
  REQUIRE( f() == expected );

  skill = e_native_skill::fur_trading;
  comp  = UnitComposition::create( e_unit_type::free_colonist );
  expected =
      UnitComposition::create( e_unit_type::master_fur_trader );
  REQUIRE( f() == expected );

  // The natives won't teach a petty criminal by living among the
  // natives, but they can teach a petty criminal to become a
  // seasoned scout when "speaking with the chief", so we need to
  // support that.
  skill = e_native_skill::scouting;
  comp  = UnitComposition::create( e_unit_type::petty_criminal );
  expected =
      UnitComposition::create( e_unit_type::seasoned_colonist );
  REQUIRE( f() == expected );

  // Make sure that we don't succeed in promoting a colonist that
  // is already an expert in something else.
  skill = e_native_skill::farming;
  comp  = UnitComposition::create(
      UnitType::create( e_unit_type::pioneer,
                         e_unit_type::expert_fisherman )
          .value() );
  REQUIRE( f() == nothing );

  // Make sure that we don't succeed in promoting a colonist that
  // is already an expert in the given skill.
  skill = e_native_skill::fishing;
  comp  = UnitComposition::create(
      UnitType::create( e_unit_type::pioneer,
                         e_unit_type::expert_fisherman )
          .value() );
  REQUIRE( f() == nothing );

  // Make sure that we can promote a scout to a seasoned scout as
  // is sometimes done when a scout speaks to the chief.
  skill = e_native_skill::scouting;
  comp  = UnitComposition::create(
      UnitType::create( e_unit_type::scout ) );
  expected =
      UnitComposition::create( e_unit_type::seasoned_scout );
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
