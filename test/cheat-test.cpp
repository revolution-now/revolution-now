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
#include "test/mocks/imenu-server.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/co-runner.hpp"
#include "src/icolony-evolve.rds.hpp"
#include "src/plane-stack.hpp"
#include "src/ss/fog-square.rds.hpp"
#include "src/unit-transformation.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/nation.hpp"
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

using ::gfx::point;
using ::mock::matchers::_;
using ::mock::matchers::Eq;

using unexplored = PlayerSquare::unexplored;
using explored   = PlayerSquare::explored;
using fogged     = FogStatus::fogged;
using clear      = FogStatus::clear;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
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
  world w;
  w.create_default_map();

  auto up = [&]( Unit& unit ) {
    cheat_upgrade_unit_expertise( w.ss(), w.ts(), unit );
  };

  auto down = [&]( Unit& unit ) {
    cheat_downgrade_unit_expertise( w.ss(), w.ts(), unit );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony& colony = w.add_colony( w.kLand );
    Unit& unit =
        w.add_unit_indoors( colony.id, e_indoor_job::hammers,
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
    Colony& colony = w.add_colony( w.kLand );
    Unit& unit = w.add_unit_outdoors( colony.id, e_direction::ne,
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
    Unit& unit =
        w.add_unit_on_map( e_unit_type::expert_farmer, w.kLand );
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
    Unit& unit = w.add_unit_on_map( initial_ut, w.kLand );
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
    Unit& unit =
        w.add_unit_on_map( e_unit_type::dragoon, w.kLand );
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
    Colony& colony = w.add_colony( w.kLand );
    Unit& unit =
        w.add_unit_indoors( colony.id, e_indoor_job::hammers,
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
        w.add_unit_on_map( e_unit_type::hardy_pioneer, w.kLand );
    consume_20_tools( w.ss(), w.ts(), unit );
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
  world w;
  w.create_default_map();
  Colony& colony = w.add_colony( /*where=*/world::kLand );

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
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  auto f = [&] {
    co_await_test( kill_natives( w.ss(), w.ts() ) );
  };

  SECTION( "no tribes" ) {
    w.gui().EXPECT__message_box(
        "All native tribes have been wiped out." );
    f();
    REQUIRE( !w.natives().tribe_exists( e_tribe::apache ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::sioux ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::arawak ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::cherokee ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::iroquois ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::aztec ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::inca ) );
  }

  SECTION( "one tribe, no dwellings, none selected" ) {
    REQUIRE( !w.natives().tribe_exists( e_tribe::tupi ) );
    w.add_tribe( e_tribe::tupi );
    REQUIRE( w.natives().tribe_exists( e_tribe::tupi ) );
    w.gui().EXPECT__check_box_selector( _, _ ).returns(
        wait( unordered_map<int, bool>{} ) );
    f();
    REQUIRE( w.natives().tribe_exists( e_tribe::tupi ) );
  }

  SECTION( "one tribe, no dwellings" ) {
    REQUIRE( !w.natives().tribe_exists( e_tribe::tupi ) );
    w.add_tribe( e_tribe::tupi );
    REQUIRE( w.natives().tribe_exists( e_tribe::tupi ) );
    w.gui().EXPECT__check_box_selector( _, _ ).returns(
        wait( unordered_map<int, bool>{
          { static_cast<int>( e_tribe::tupi ), true } } ) );
    mock_land_view.EXPECT__animate( _ );
    w.gui().EXPECT__message_box(
        "The [Tupi] tribe has been wiped out." );
    f();
    REQUIRE( !w.natives().tribe_exists( e_tribe::tupi ) );
  }

  SECTION( "two tribes, some dwellings" ) {
    REQUIRE( !w.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( !w.natives().tribe_exists( e_tribe::iroquois ) );
    w.add_tribe( e_tribe::tupi );
    w.add_tribe( e_tribe::iroquois );
    REQUIRE( w.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( w.natives().tribe_exists( e_tribe::iroquois ) );
    DwellingId const dwelling_id_1 =
        w.add_dwelling( { .x = 1, .y = 0 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_2 =
        w.add_dwelling( { .x = 0, .y = 1 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_3 =
        w.add_dwelling( { .x = 1, .y = 2 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_4 =
        w.add_dwelling( { .x = 2, .y = 1 }, e_tribe::tupi ).id;
    DwellingId const dwelling_id_5 =
        w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois )
            .id;
    w.square( { .x = 1, .y = 0 } ).road = true;
    w.square( { .x = 0, .y = 1 } ).road = true;
    w.square( { .x = 1, .y = 2 } ).road = true;
    w.square( { .x = 2, .y = 1 } ).road = true;
    w.square( { .x = 1, .y = 1 } ).road = true;
    w.map_updater().make_squares_visible(
        w.default_player_type(), { { .x = 1, .y = 0 } } );
    w.map_updater().make_squares_visible(
        w.default_player_type(), { { .x = 0, .y = 1 } } );
    w.map_updater().make_squares_fogged(
        w.default_player_type(), { { .x = 0, .y = 1 } } );
    w.gui().EXPECT__check_box_selector( _, _ ).returns(
        wait( unordered_map<int, bool>{
          { static_cast<int>( e_tribe::tupi ), true } } ) );
    mock_land_view.EXPECT__animate( _ );
    w.gui().EXPECT__message_box(
        "The [Tupi] tribe has been wiped out." );
    REQUIRE( w.player_square( { .x = 1, .y = 0 } )
                 .inner_if<explored>()
                 .get_if<clear>() );
    REQUIRE( w.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .get_if<fogged>() );
    REQUIRE( w.player_square( { .x = 1, .y = 2 } ) ==
             unexplored{} );
    REQUIRE( w.player_square( { .x = 2, .y = 1 } ) ==
             unexplored{} );
    REQUIRE( w.player_square( { .x = 1, .y = 1 } ) ==
             unexplored{} );
    // Only the fogged one got its fog square updated, because it
    // was flipped from visible to fogged.
    REQUIRE( w.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .inner_if<fogged>()
                 .value()
                 .dwelling.has_value() );
    REQUIRE( w.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .inner_if<fogged>()
                 .value()
                 .square.road );
    f();
    REQUIRE( !w.natives().tribe_exists( e_tribe::tupi ) );
    REQUIRE( w.natives().tribe_exists( e_tribe::iroquois ) );
    REQUIRE( !w.natives().dwelling_exists( dwelling_id_1 ) );
    REQUIRE( !w.natives().dwelling_exists( dwelling_id_2 ) );
    REQUIRE( !w.natives().dwelling_exists( dwelling_id_3 ) );
    REQUIRE( !w.natives().dwelling_exists( dwelling_id_4 ) );
    REQUIRE( w.natives().dwelling_exists( dwelling_id_5 ) );
    VisibilityForPlayer const viz( w.ss(),
                                   w.default_player_type() );
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
    REQUIRE( w.player_square( { .x = 1, .y = 0 } )
                 .inner_if<explored>()
                 .get_if<clear>() );
    REQUIRE( w.player_square( { .x = 0, .y = 1 } )
                 .inner_if<explored>()
                 .get_if<fogged>() );
    REQUIRE( w.player_square( { .x = 1, .y = 2 } ) ==
             unexplored{} );
    REQUIRE( w.player_square( { .x = 2, .y = 1 } ) ==
             unexplored{} );
    REQUIRE( w.player_square( { .x = 1, .y = 1 } ) ==
             unexplored{} );
    // Only the fogged one got its fog square updated.
    REQUIRE( !w.player_square( { .x = 0, .y = 1 } )
                  .inner_if<explored>()
                  .inner_if<fogged>()
                  .value()
                  .dwelling.has_value() );
    REQUIRE( !w.player_square( { .x = 0, .y = 1 } )
                  .inner_if<explored>()
                  .inner_if<fogged>()
                  .value()
                  .square.road );
  }
}

TEST_CASE( "[cheat] cheat_toggle_reveal_full_map" ) {
  world w;
  w.create_default_map();
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  auto f = [&] {
    cheat_toggle_reveal_full_map( w.ss(), w.ts() );
  };

  auto& show_indian_moves =
      w.settings().in_game_options.game_menu_options
          [e_game_menu_option::show_indian_moves];
  auto& show_foreign_moves =
      w.settings().in_game_options.game_menu_options
          [e_game_menu_option::show_foreign_moves];
  auto const& map_revealed = w.land_view().map_revealed;

  w.turn().cycle =
      TurnCycle::nation{ .european_nation = european_nation_for(
                             w.default_player_type() ) };

  // Starting state sanity check.
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::no_special_view>() );

  mock_land_view.EXPECT__set_visibility( maybe<e_player>{} );
  f();
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::entire>() );

  show_indian_moves  = true;
  show_foreign_moves = true;
  mock_land_view.EXPECT__set_visibility( e_player::dutch );
  f();
  REQUIRE( show_indian_moves );
  REQUIRE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::no_special_view>() );

  show_indian_moves  = true;
  show_foreign_moves = true;
  mock_land_view.EXPECT__set_visibility( maybe<e_player>{} );
  f();
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::entire>() );

  mock_land_view.EXPECT__set_visibility( e_player::dutch );
  f();
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::no_special_view>() );
}

TEST_CASE( "[cheat] cheat_advance_colony_one_turn" ) {
  world w;
  MockIColonyEvolver mock_colony_evolver;

  Colony& colony = w.add_colony( { .x = 1, .y = 1 } );

  auto f = [&] {
    cheat_advance_colony_one_turn( mock_colony_evolver, colony );
  };

  // The Eq-ref trick is to prevent the matcher from storing a
  // copy of the object, which it would still do with only ref.
  mock_colony_evolver.EXPECT__evolve_colony_one_turn(
      Eq( ref( colony ) ) );
  ++colony.id; // make sure the mock is not holding a copy.
  f();
}

TEST_CASE( "[cheat] cheat_target_square" ) {
  world w;
  maybe<point> expected;

  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  auto f = [&] {
    return cheat_target_square( w.ss().as_const, w.ts() );
  };

  // Neither a unit nor a white box.
  mock_land_view.EXPECT__unit_blinking().returns( nothing );
  mock_land_view.EXPECT__white_box().returns( nothing );
  expected = nothing;
  REQUIRE( f() == expected );

  // White box.
  mock_land_view.EXPECT__unit_blinking().returns( nothing );
  mock_land_view.EXPECT__white_box().returns(
      point{ .x = 1, .y = 2 } );
  expected = { .x = 1, .y = 2 };
  REQUIRE( f() == expected );

  // Unit.
  UnitId const unit_id =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 0 } )
          .id();
  mock_land_view.EXPECT__unit_blinking().returns( unit_id );
  expected = { .x = 1, .y = 0 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[cheat] cheat_mode_enabled" ) {
  world w;

  auto f = [&] { return cheat_mode_enabled( w.ss().as_const ); };

  REQUIRE_FALSE( f() );
  w.settings().cheat_options.enabled = true;
  REQUIRE( f() );
  w.settings().cheat_options.enabled = false;
  REQUIRE_FALSE( f() );
}

TEST_CASE( "[cheat] enable_cheat_mode" ) {
  world w;
  MockIMenuServer mock_menu_server;
  w.planes().get().menu = mock_menu_server;

  auto f = [&] { enable_cheat_mode( w.ss(), w.ts() ); };

  REQUIRE_FALSE( w.settings().cheat_options.enabled );

  mock_menu_server.EXPECT__enable_cheat_menu( true );
  f();
  REQUIRE( w.settings().cheat_options.enabled );

  mock_menu_server.EXPECT__enable_cheat_menu( true );
  f();
  REQUIRE( w.settings().cheat_options.enabled );
}

TEST_CASE( "[cheat] monitor_magic_key_sequence" ) {
  co::stream<char> stream;

  auto f = [&] { return monitor_magic_key_sequence( stream ); };

  wait<> const w = f();
  run_all_coroutines();
  REQUIRE( !w.ready() );

  stream.send( 'w' );
  run_all_coroutines();
  REQUIRE( !w.ready() );

  stream.send( 'i' );
  run_all_coroutines();
  REQUIRE( !w.ready() );

  stream.send( 'i' );
  run_all_coroutines();
  REQUIRE( !w.ready() );

  stream.send( 'n' );
  run_all_coroutines();
  REQUIRE( !w.ready() );

  stream.send( 'w' );
  run_all_coroutines();
  REQUIRE( !w.ready() );

  stream.send( 'i' );
  run_all_coroutines();
  REQUIRE( !w.ready() );

  stream.send( 'n' );
  run_all_coroutines();
  REQUIRE( w.ready() );
}

TEST_CASE( "[cheat] cheat_advance_revolution_status" ) {
  world w;
}

} // namespace
} // namespace rn
