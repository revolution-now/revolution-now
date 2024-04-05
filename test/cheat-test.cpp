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
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/imap-updater.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/icolony-evolve.rds.hpp"
#include "src/plane-stack.hpp"
#include "src/ss/fog-square.rds.hpp"
#include "src/unit-transformation.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/natives.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/turn.rds.hpp"
#include "src/ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

RDS_DEFINE_MOCK( IColonyEvolver );

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;
using ::mock::matchers::Eq;

using unexplored = PlayerSquare::unexplored;
using explored   = PlayerSquare::explored;
using fogged     = FogStatus::fogged;
using clear      = FogStatus::clear;

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
    consume_20_tools( W.ss(), W.ts(), unit );
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
    mock_land_view.EXPECT__animate( _ );
    W.gui().EXPECT__message_box(
        "The [Tupi] tribe has been wiped out." );
    REQUIRE( W.player_square( { .x = 1, .y = 0 } )
                 .inner_if<explored>()
                 .get_if<clear>() );
    REQUIRE( W.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .get_if<fogged>() );
    REQUIRE( W.player_square( { .x = 1, .y = 2 } ) ==
             unexplored{} );
    REQUIRE( W.player_square( { .x = 2, .y = 1 } ) ==
             unexplored{} );
    REQUIRE( W.player_square( { .x = 1, .y = 1 } ) ==
             unexplored{} );
    // Only the fogged one got its fog square updated, because it
    // was flipped from visible to fogged.
    REQUIRE( W.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .inner_if<fogged>()
                 .value()
                 .dwelling.has_value() );
    REQUIRE( W.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .inner_if<fogged>()
                 .value()
                 .square.road );
    f();
    REQUIRE( !W.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( W.natives().tribe_exists( e_tribe::iroquois ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_1 ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_2 ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_3 ) );
    REQUIRE( !W.natives().dwelling_exists( dwelling_id_4 ) );
    REQUIRE( W.natives().dwelling_exists( dwelling_id_5 ) );
    VisibilityForNation const viz( W.ss(), W.default_nation() );
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::fogged );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 2, .y = 1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( W.player_square( { .x = 1, .y = 0 } )
                 .inner_if<explored>()
                 .get_if<clear>() );
    REQUIRE( W.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .get_if<fogged>() );
    REQUIRE( W.player_square( { .x = 1, .y = 2 } ) ==
             unexplored{} );
    REQUIRE( W.player_square( { .x = 2, .y = 1 } ) ==
             unexplored{} );
    REQUIRE( W.player_square( { .x = 1, .y = 1 } ) ==
             unexplored{} );
    // Only the fogged one got its fog square updated.
    REQUIRE( !W.player_square( { .x = 0, .y = 1 } )
                  .inner_if<explored>()
                  .inner_if<fogged>()
                  .value()
                  .dwelling.has_value() );
    REQUIRE( !W.player_square( { .x = 0, .y = 1 } )
                  .inner_if<explored>()
                  .inner_if<fogged>()
                  .value()
                  .square.road );
  }
}

TEST_CASE( "[cheat] cheat_toggle_reveal_full_map" ) {
  World W;
  W.create_default_map();
  MockLandViewPlane mock_land_view;
  W.planes().back().land_view = &mock_land_view;

  auto f = [&] {
    cheat_toggle_reveal_full_map( W.ss(), W.ts() );
  };

  auto& show_indian_moves =
      W.settings()
          .game_options
          .flags[e_game_flag_option::show_indian_moves];
  auto& show_foreign_moves =
      W.settings()
          .game_options
          .flags[e_game_flag_option::show_foreign_moves];
  auto const& map_revealed = W.land_view().map_revealed;

  W.turn().cycle =
      TurnCycle::nation{ .nation = W.default_nation() };

  // Starting state sanity check.
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::no_special_view>() );

  mock_land_view.EXPECT__set_visibility( maybe<e_nation>{} );
  f();
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::entire>() );

  show_indian_moves  = true;
  show_foreign_moves = true;
  mock_land_view.EXPECT__set_visibility( e_nation::dutch );
  f();
  REQUIRE( show_indian_moves );
  REQUIRE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::no_special_view>() );

  show_indian_moves  = true;
  show_foreign_moves = true;
  mock_land_view.EXPECT__set_visibility( maybe<e_nation>{} );
  f();
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::entire>() );

  mock_land_view.EXPECT__set_visibility( e_nation::dutch );
  f();
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::no_special_view>() );
}

TEST_CASE( "[cheat] cheat_advance_colony_one_turn" ) {
  World              W;
  MockIColonyEvolver mock_colony_evolver;

  Colony& colony = W.add_colony( { .x = 1, .y = 1 } );

  auto f = [&] {
    cheat_advance_colony_one_turn( mock_colony_evolver, colony );
  };

  // The Eq-ref trick is to prevent the matcher from storing a
  // copy of the object, which it would still do with only ref.
  mock_colony_evolver.EXPECT__evolve_one_turn(
      Eq( ref( colony ) ) );
  ++colony.id; // make sure the mock is not holding a copy.
  f();
}

} // namespace
} // namespace rn
