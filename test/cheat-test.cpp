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
#include "test/mocks/igui.hpp"
#include "test/mocks/imap-updater.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/plane-stack.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/natives.hpp"
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

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

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
    cheat_upgrade_unit_expertise( W.ss(), W.ts(), unit );
  };

  auto down = [&]( Unit& unit ) {
    cheat_downgrade_unit_expertise( W.ss(), W.ts(), unit );
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

TEST_CASE( "[cheat] kill_natives" ) {
  World             W;
  MockLandViewPlane mock_land_view;
  W.planes().back().land_view = &mock_land_view;

  auto f = [&] {
    co_await_test( kill_natives( W.ss(), W.ts() ) );
  };

  SECTION( "no tribes" ) {
    W.gui().EXPECT__message_box(
        "All native tribes have been wiped out." );
    f();
    REQUIRE( !W.natives().tribe_exists( e_tribe::apache ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::sioux ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::arawak ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::cherokee ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::iroquois ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::aztec ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::inca ) );
  }

  SECTION( "one tribe, no dwellings, none selected" ) {
    REQUIRE( !W.natives().tribe_exists( e_tribe::tupi ) );
    W.add_tribe( e_tribe::tupi );
    REQUIRE( W.natives().tribe_exists( e_tribe::tupi ) );
    W.gui().EXPECT__check_box_selector( _, _ ).returns(
        wait( unordered_map<int, bool>{} ) );
    f();
    REQUIRE( W.natives().tribe_exists( e_tribe::tupi ) );
  }

  SECTION( "one tribe, no dwellings" ) {
    REQUIRE( !W.natives().tribe_exists( e_tribe::tupi ) );
    W.add_tribe( e_tribe::tupi );
    REQUIRE( W.natives().tribe_exists( e_tribe::tupi ) );
    W.gui().EXPECT__check_box_selector( _, _ ).returns(
        wait( unordered_map<int, bool>{
            { static_cast<int>( e_tribe::tupi ), true } } ) );
    mock_land_view.EXPECT__animate( _ ); // sound.
    mock_land_view.EXPECT__animate( _ );
    W.gui().EXPECT__message_box(
        "The [Tupi] tribe has been wiped out." );
    f();
    REQUIRE( !W.natives().tribe_exists( e_tribe::tupi ) );
  }

  SECTION( "two tribes, some dwellings" ) {
    REQUIRE( !W.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( !W.natives().tribe_exists( e_tribe::iroquois ) );
    W.add_tribe( e_tribe::tupi );
    W.add_tribe( e_tribe::iroquois );
    REQUIRE( W.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( W.natives().tribe_exists( e_tribe::iroquois ) );
    DwellingId const dwelling_id_1 =
        W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_2 =
        W.add_dwelling( { .x = 0, .y = 1 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_3 =
        W.add_dwelling( { .x = 1, .y = 2 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_4 =
        W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_5 =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois )
            .id;
    W.square( { .x = 1, .y = 0 } ).road = true;
    W.square( { .x = 0, .y = 1 } ).road = true;
    W.square( { .x = 1, .y = 2 } ).road = true;
    W.square( { .x = 2, .y = 1 } ).road = true;
    W.square( { .x = 1, .y = 1 } ).road = true;
    W.map_updater().make_squares_visible(
        W.default_nation(), { { .x = 1, .y = 0 } } );
    W.map_updater().make_squares_visible(
        W.default_nation(), { { .x = 0, .y = 1 } } );
    W.map_updater().make_squares_fogged(
        W.default_nation(), { { .x = 0, .y = 1 } } );
    W.gui().EXPECT__check_box_selector( _, _ ).returns(
        wait( unordered_map<int, bool>{
            { static_cast<int>( e_tribe::tupi ), true } } ) );
    mock_land_view.EXPECT__animate( _ ); // sound.
    mock_land_view.EXPECT__animate( _ );
    W.gui().EXPECT__message_box(
        "The [Tupi] tribe has been wiped out." );
    REQUIRE( W.player_square( { .x = 1, .y = 0 } ).has_value() );
    REQUIRE( W.player_square( { .x = 0, .y = 1 } ).has_value() );
    REQUIRE(
        !W.player_square( { .x = 1, .y = 2 } ).has_value() );
    REQUIRE(
        !W.player_square( { .x = 2, .y = 1 } ).has_value() );
    REQUIRE(
        !W.player_square( { .x = 1, .y = 1 } ).has_value() );
    // Only the fogged one got its fog square updated, because it
    // was flipped from visible to fogged.
    REQUIRE( !W.player_square( { .x = 1, .y = 0 } )
                  ->dwelling.has_value() );
    REQUIRE( W.player_square( { .x = 0, .y = 1 } )
                 ->dwelling.has_value() );
    REQUIRE(
        !W.player_square( { .x = 1, .y = 0 } )->square.road );
    REQUIRE(
        W.player_square( { .x = 0, .y = 1 } )->square.road );
    f();
    REQUIRE( !W.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( W.natives().tribe_exists( e_tribe::iroquois ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_1 ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_2 ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_3 ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_4 ) );
    REQUIRE( W.natives().dwelling_exists( dwelling_id_5 ) );
    Visibility const viz( W.ss(), W.default_nation() );
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::visible_with_fog );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 2, .y = 1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( W.player_square( { .x = 1, .y = 0 } ).has_value() );
    REQUIRE( W.player_square( { .x = 0, .y = 1 } ).has_value() );
    REQUIRE(
        !W.player_square( { .x = 1, .y = 2 } ).has_value() );
    REQUIRE(
        !W.player_square( { .x = 2, .y = 1 } ).has_value() );
    REQUIRE(
        !W.player_square( { .x = 1, .y = 1 } ).has_value() );
    // Only the fogged one got its fog square updated.
    REQUIRE( !W.player_square( { .x = 1, .y = 0 } )
                  ->dwelling.has_value() );
    REQUIRE( !W.player_square( { .x = 0, .y = 1 } )
                  ->dwelling.has_value() );
    REQUIRE(
        !W.player_square( { .x = 1, .y = 0 } )->square.road );
    REQUIRE(
        !W.player_square( { .x = 0, .y = 1 } )->square.road );
  }
}

} // namespace
} // namespace rn
