/****************************************************************
**cheat.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-17.
*
* Description: Unit tests for the src/cheat.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cheat.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/ref.hpp"
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
TEST_CASE( "[cheat] cheat_{up,down}grade_unit_expertise" ) {
  World W;
  W.create_default_map();

  auto up = [&]( Unit& unit ) {
    cheat_upgrade_unit_expertise( W.ss(), W.default_player(),
                                  unit );
  };

  auto down = [&]( Unit& unit ) {
    cheat_downgrade_unit_expertise( W.default_player(), unit );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    Unit&           unit =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::expert_farmer,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::master_carpenter,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "free_colonist fishing" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    Unit& unit = W.add_unit_outdoors( colony.id, e_direction::ne,
                                      e_outdoor_job::fish );
    expected   = UnitComposition( wrapped::UnitComposition{
          .type      = e_unit_type::free_colonist,
          .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::expert_fisherman,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    Unit&           unit =
        W.add_unit_on_map( e_unit_type::expert_farmer, W.kLand );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::expert_farmer,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::expert_farmer,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::free_colonist,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::indentured_servant,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::petty_criminal,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::petty_criminal,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::indentured_servant,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::free_colonist,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::free_colonist,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "master_carpenter / dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::dragoon,
                          e_unit_type::master_carpenter ) );
    Unit& unit = W.add_unit_on_map( initial_ut, W.kLand );
    {
      UNWRAP_CHECK(
          expected_ut,
          UnitType::create( e_unit_type::dragoon,
                            e_unit_type::master_carpenter ) );
      expected = UnitComposition( wrapped::UnitComposition{
          .type      = expected_ut,
          .inventory = {},
      } );
      REQUIRE( unit.composition() == expected );
    }

    up( unit );
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

  SECTION( "dragoon" ) {
    UnitComposition expected;
    Unit&           unit =
        W.add_unit_on_map( e_unit_type::dragoon, W.kLand );
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

    down( unit );
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

    down( unit );
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

    down( unit );
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

    up( unit );
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

    up( unit );
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

    up( unit );
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

    up( unit );
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

    down( unit );
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
  }

  SECTION( "petty_criminal carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    Unit&           unit =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::petty_criminal,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::indentured_servant,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::free_colonist,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::master_carpenter,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    up( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::master_carpenter,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::free_colonist,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::indentured_servant,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::petty_criminal,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );

    down( unit );
    expected = UnitComposition( wrapped::UnitComposition{
        .type      = e_unit_type::petty_criminal,
        .inventory = {},
    } );
    REQUIRE( unit.composition() == expected );
  }

  SECTION( "hardy_pioneer" ) {
    Unit& unit =
        W.add_unit_on_map( e_unit_type::hardy_pioneer, W.kLand );
    unit.consume_20_tools( W.default_player() );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist ) );

    up( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist ) );

    down( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::free_colonist ) );

    down( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE(
        unit.type_obj() ==
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::indentured_servant ) );

    down( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::petty_criminal ) );

    down( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::petty_criminal ) );

    up( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE(
        unit.type_obj() ==
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::indentured_servant ) );

    up( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::free_colonist ) );

    up( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist ) );

    up( unit );
    REQUIRE( unit.composition()
                 .inventory()[e_unit_inventory::tools] == 80 );
    REQUIRE( unit.type_obj() ==
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist ) );
  }
}

TEST_CASE( "[cheat] cheat change commodity quantity" ) {
  World W;
  W.create_default_map();
  Colony& colony = W.add_colony( /*where=*/World::kLand );

  e_commodity const type = e_commodity::horses;

  colony.commodities[type] = 66;

  auto up = [&] {
    cheat_increase_commodity( colony, type );
    return colony.commodities[type];
  };
  auto down = [&] {
    cheat_decrease_commodity( colony, type );
    return colony.commodities[type];
  };

  // Sanity check.
  REQUIRE( colony.commodities[type] == 66 );

  SECTION( "up then down" ) {
    REQUIRE( up() == 100 );
    REQUIRE( up() == 150 );
    REQUIRE( up() == 200 );
    REQUIRE( up() == 250 );
    REQUIRE( down() == 200 );
    REQUIRE( down() == 150 );
    REQUIRE( down() == 100 );
    REQUIRE( down() == 50 );
    REQUIRE( down() == 0 );
    REQUIRE( down() == 0 );
  }
  SECTION( "down then up" ) {
    REQUIRE( down() == 50 );
    REQUIRE( down() == 0 );
    REQUIRE( down() == 0 );
    REQUIRE( up() == 50 );
    REQUIRE( up() == 100 );
  }
}

} // namespace
} // namespace rn
