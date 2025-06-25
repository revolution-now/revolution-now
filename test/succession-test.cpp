/****************************************************************
**succession-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-11.
*
* Description: Unit tests for the succession module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/succession.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/harbor-units.hpp"
#include "src/imap-updater.hpp"
#include "src/unit-ownership.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/events.rds.hpp"
#include "src/ss/nation.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::mock::matchers::StrContains;
using ::refl::enum_values;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::english );
    add_player( e_player::french );
    add_player( e_player::spanish );
    add_player( e_player::dutch );
    set_default_player_type( e_player::french );
    english().control = e_player_control::ai;
    french().control  = e_player_control::human;
    spanish().control = e_player_control::ai;
    dutch().control   = e_player_control::ai;
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rebel-sentiment] should_do_war_of_succession" ) {
  world w;

  Player& human_player = w.french();
  Player& other_player = w.english();

  auto const f = [&] {
    return should_do_war_of_succession(
        w.ss().as_const, as_const( human_player ) );
  };

  BASE_CHECK( human_player.control == e_player_control::human );

  // Default.
  REQUIRE_FALSE( f() );

  // Set up the conditions for a war of succession by default,
  // then test for each negative case.
  w.settings().game_setup_options.disable_war_of_succession =
      false;
  human_player.revolution.rebel_sentiment = 50;

  SECTION( "lacking rebel sentiment" ) {
    REQUIRE( f() );
    human_player.revolution.rebel_sentiment = 49;
    REQUIRE_FALSE( f() );
  }

  SECTION( "one player withdrawn" ) {
    REQUIRE( f() );
    w.spanish().control = e_player_control::withdrawn;
    REQUIRE_FALSE( f() );
  }

  SECTION( "multiple humans" ) {
    REQUIRE( f() );
    w.spanish().control = e_player_control::human;
    REQUIRE_FALSE( f() );
  }

  SECTION( "no humans" ) {
    REQUIRE( f() );
    human_player.control = e_player_control::ai;
    REQUIRE_FALSE( f() );
  }

  SECTION( "AI granted independence" ) {
    human_player.control = e_player_control::human;
    other_player.control = e_player_control::ai;
    REQUIRE( f() );
    other_player.revolution.status = e_revolution_status::won;
    REQUIRE_FALSE( f() );
  }

  SECTION( "one human but not the caller's player" ) {
    REQUIRE( f() );
    human_player.control = e_player_control::ai;
    w.spanish().control  = e_player_control::human;
    REQUIRE_FALSE( f() );
  }

  SECTION( "fewer than four players" ) {
    REQUIRE( f() );
    w.players().players[e_player::spanish].reset();
    REQUIRE_FALSE( f() );
  }

  SECTION( "war of succession already done" ) {
    REQUIRE( f() );
    w.events().war_of_succession_done = true;
    REQUIRE_FALSE( f() );
  }

  SECTION( "war of succession not enabled" ) {
    REQUIRE( f() );
    w.settings().game_setup_options.disable_war_of_succession =
        true;
    REQUIRE_FALSE( f() );
  }
}

TEST_CASE(
    "[rebel-sentiment] select_players_for_war_of_succession" ) {
  world w;
  WarOfSuccessionNations expected;

  auto const f = [&] {
    return select_players_for_war_of_succession( w.ss() );
  };

  // Start off with no human players.
  for( e_player const player : enum_values<e_player> )
    if( !is_ref( player ) ) //
      w.player( player ).control = e_player_control::ai;

  // No humans.
  expected = { .withdraws = e_nation::english,
               .receives  = e_nation::french };
  REQUIRE( f() == expected );

  w.english().control = e_player_control::human;
  expected            = { .withdraws = e_nation::french,
                          .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.english().control = e_player_control::ai;
  w.french().control  = e_player_control::human;
  expected            = { .withdraws = e_nation::english,
                          .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.english().control = e_player_control::withdrawn;
  w.french().control  = e_player_control::human;
  expected            = { .withdraws = e_nation::spanish,
                          .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.english().control = e_player_control::ai;
  w.french().control  = e_player_control::human;
  w.spanish().control = e_player_control::human;
  expected            = { .withdraws = e_nation::english,
                          .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.spanish().control = e_player_control::ai;
  expected            = { .withdraws = e_nation::english,
                          .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::artillery, { .x = 0, .y = 0 },
                     e_player::english );
  expected = { .withdraws = e_nation::english,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::dragoon, { .x = 0, .y = 0 },
                     e_player::english );
  expected = { .withdraws = e_nation::spanish,
               .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::soldier, { .x = 1, .y = 0 },
                     e_player::spanish );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::english };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::native_convert,
                     { .x = 1, .y = 0 }, e_player::english );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.french().control = e_player_control::ai;
  expected           = { .withdraws = e_nation::french,
                         .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.english().control = e_player_control::human;
  expected            = { .withdraws = e_nation::french,
                          .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 0, .y = 0 }, e_player::french );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::french };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 2, .y = 0 }, e_player::french );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.players().players[e_player::french].reset();
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );
}

TEST_CASE( "[rebel-sentiment] war_of_succession_plan" ) {
  world w;
  WarOfSuccessionNations nations;
  WarOfSuccessionPlan expected;

  auto const f = [&] {
    return war_of_succession_plan( w.ss(), nations );
  };

  // Default.
  nations  = {};
  expected = {};
  REQUIRE( f() == expected );

  nations.receives  = e_nation::french;
  nations.withdraws = e_nation::spanish;

  expected = { .nations = nations };
  REQUIRE( f() == expected );

  UnitId const removed_1 =
      w.add_unit_in_port( e_unit_type::free_colonist,
                          e_player::spanish )
          .id();
  w.add_unit_in_port( e_unit_type::free_colonist,
                      e_player::french );
  UnitId const reassign_unit_1 =
      w.add_unit_in_port( e_unit_type::caravel,
                          e_player::spanish )
          .id();
  unit_sail_to_new_world( w.ss(), reassign_unit_1 );
  w.add_unit_in_cargo( e_unit_type::free_colonist,
                       reassign_unit_1 );
  UnitId const reassign_unit_2 =
      w.add_unit_in_port( e_unit_type::privateer,
                          e_player::spanish )
          .id();
  unit_sail_to_new_world( w.ss(), reassign_unit_2 );
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.spanish(),
                                      reassign_unit_2 ) ==
           e_high_seas_result::still_traveling );
  unit_sail_to_harbor( w.ss(), reassign_unit_2 );

  ColonyId const reassign_colony_1 =
      w.add_colony( { .x = 0, .y = 0 }, e_player::spanish ).id;
  w.add_unit_indoors( reassign_colony_1, e_indoor_job::bells );
  w.add_colony( { .x = 2, .y = 0 }, e_player::french );

  ColonyId const reassign_colony_2 =
      w.add_colony( { .x = 2, .y = 2 }, e_player::spanish ).id;
  UnitId const reassign_unit_3 =
      w.add_unit_on_map( e_unit_type::dragoon,
                         { .x = 2, .y = 1 }, e_player::spanish )
          .id();
  w.add_unit_on_map( e_unit_type::dragoon, { .x = 0, .y = 1 },
                     e_player::french );
  ColonyId const reassign_colony_3 =
      w.add_colony( { .x = 0, .y = 2 }, e_player::spanish ).id;

  w.add_dwelling( { .x = 1, .y = 2 }, e_tribe::iroquois );
  DwellingId const dwelling_id_2 =
      w.add_dwelling( { .x = 1, .y = 0 }, e_tribe::iroquois ).id;
  UnitId const reassign_unit_4 =
      w.add_missionary_in_dwelling(
           e_unit_type::jesuit_missionary, dwelling_id_2,
           e_player::spanish )
          .id();

  expected = {
    .nations            = nations,
    .remove_units       = { removed_1 },
    .reassign_units     = { reassign_unit_1, reassign_unit_2,
                            reassign_unit_3, reassign_unit_4 },
    .reassign_colonies  = { reassign_colony_1, reassign_colony_2,
                            reassign_colony_3 },
    .update_fog_squares = { { .x = 0, .y = 0 },
                            { .x = 1, .y = 0 },
                            { .x = 0, .y = 2 },
                            { .x = 2, .y = 2 } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[rebel-sentiment] do_war_of_succession" ) {
  world w;
  WarOfSuccessionPlan plan;

  Player const& observer = w.english();

  auto const f = [&] {
    return do_war_of_succession( w.ss(), w.ts(), observer,
                                 plan );
  };

  UnitId const removed_1 =
      w.add_unit_in_port( e_unit_type::free_colonist,
                          e_player::spanish )
          .id();
  UnitId const dutch_unit_1 =
      w.add_unit_in_port( e_unit_type::free_colonist,
                          e_player::dutch )
          .id();
  UnitId const reassign_unit_1 =
      w.add_unit_in_port( e_unit_type::caravel,
                          e_player::spanish )
          .id();
  unit_sail_to_new_world( w.ss(), reassign_unit_1 );
  w.add_unit_in_cargo( e_unit_type::free_colonist,
                       reassign_unit_1 );
  UnitId const reassign_unit_2 =
      w.add_unit_in_port( e_unit_type::privateer,
                          e_player::spanish )
          .id();
  unit_sail_to_new_world( w.ss(), reassign_unit_2 );
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.spanish(),
                                      reassign_unit_2 ) ==
           e_high_seas_result::still_traveling );
  unit_sail_to_harbor( w.ss(), reassign_unit_2 );

  Colony& reassign_colony_1_obj =
      w.add_colony( { .x = 0, .y = 0 }, e_player::spanish );
  reassign_colony_1_obj.sons_of_liberty
      .num_rebels_from_bells_only = 2;
  reassign_colony_1_obj.sons_of_liberty
      .last_sons_of_liberty_integral_percent = 3;
  ColonyId const reassign_colony_1 = reassign_colony_1_obj.id;
  w.add_unit_indoors( reassign_colony_1, e_indoor_job::bells );
  ColonyId const french_colony_1 =
      w.add_colony( { .x = 2, .y = 0 }, e_player::french ).id;

  ColonyId const reassign_colony_2 =
      w.add_colony( { .x = 2, .y = 2 }, e_player::spanish ).id;
  UnitId const reassign_unit_3 =
      w.add_unit_on_map( e_unit_type::dragoon,
                         { .x = 2, .y = 1 }, e_player::spanish )
          .id();
  UnitId const french_unit_1 =
      w.add_unit_on_map( e_unit_type::dragoon,
                         { .x = 0, .y = 1 }, e_player::french )
          .id();
  ColonyId const reassign_colony_3 =
      w.add_colony( { .x = 0, .y = 2 }, e_player::spanish ).id;

  w.add_dwelling( { .x = 1, .y = 2 }, e_tribe::iroquois );
  DwellingId const dwelling_id_2 =
      w.add_dwelling( { .x = 1, .y = 0 }, e_tribe::iroquois ).id;
  UnitId const reassign_unit_4 =
      w.add_missionary_in_dwelling(
           e_unit_type::jesuit_missionary, dwelling_id_2,
           e_player::spanish )
          .id();
  UnitId const reassign_unit_5 =
      w.add_unit_on_map( e_unit_type::soldier,
                         { .x = 2, .y = 1 }, e_player::spanish )
          .id();
  UnitOwnershipChanger( w.ss(), reassign_unit_5 ).destroy();

  w.map_updater().make_squares_visible(
      observer.type, { point{ .x = 0, .y = 0 } } );
  w.map_updater().make_squares_visible(
      observer.type, { point{ .x = 1, .y = 0 } } );
  w.map_updater().make_squares_visible(
      observer.type, { point{ .x = 0, .y = 2 } } );
  w.map_updater().make_squares_visible(
      observer.type, { point{ .x = 2, .y = 2 } } );

  w.map_updater().make_squares_fogged(
      observer.type, { point{ .x = 1, .y = 0 } } );
  VisibilityForPlayer const viz( w.ss(), observer.type );
  BASE_CHECK( viz.visible( { .x = 1, .y = 0 } ) ==
              e_tile_visibility::fogged );

  REQUIRE( w.units().all().size() == 9 );
  REQUIRE( w.units().exists( removed_1 ) );
  REQUIRE( w.units().exists( reassign_unit_1 ) );
  REQUIRE( w.units().exists( reassign_unit_2 ) );
  REQUIRE( w.units().exists( reassign_unit_3 ) );
  REQUIRE( w.units().exists( reassign_unit_4 ) );
  REQUIRE( w.units().exists( dutch_unit_1 ) );
  REQUIRE( w.units().exists( french_unit_1 ) );
  REQUIRE_FALSE( w.units().exists( reassign_unit_5 ) );
  REQUIRE( w.units().unit_for( reassign_unit_1 ).player_type() ==
           e_player::spanish );
  REQUIRE( w.units().unit_for( reassign_unit_2 ).player_type() ==
           e_player::spanish );
  REQUIRE( w.units().unit_for( reassign_unit_3 ).player_type() ==
           e_player::spanish );
  REQUIRE( w.units().unit_for( reassign_unit_4 ).player_type() ==
           e_player::spanish );
  REQUIRE( w.units().unit_for( dutch_unit_1 ).player_type() ==
           e_player::dutch );
  REQUIRE( w.units().unit_for( french_unit_1 ).player_type() ==
           e_player::french );
  REQUIRE( w.colonies().all().size() == 4 );
  REQUIRE( w.colonies().exists( reassign_colony_1 ) );
  REQUIRE( w.colonies().exists( reassign_colony_2 ) );
  REQUIRE( w.colonies().exists( reassign_colony_3 ) );
  REQUIRE( w.colonies().exists( french_colony_1 ) );
  REQUIRE( w.colonies().colony_for( reassign_colony_1 ).player ==
           e_player::spanish );
  REQUIRE( w.colonies().colony_for( reassign_colony_2 ).player ==
           e_player::spanish );
  REQUIRE( w.colonies().colony_for( reassign_colony_3 ).player ==
           e_player::spanish );
  REQUIRE( w.colonies().colony_for( french_colony_1 ).player ==
           e_player::french );
  REQUIRE( reassign_colony_1_obj.sons_of_liberty
               .num_rebels_from_bells_only == 2 );
  REQUIRE( reassign_colony_1_obj.sons_of_liberty
               .last_sons_of_liberty_integral_percent == 3 );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
           e_tile_visibility::fogged );
  REQUIRE( viz.visible( { .x = 0, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.dwelling_at( { .x = 1, .y = 0 } ).has_value() );
  REQUIRE( viz.dwelling_at( { .x = 1, .y = 0 } )
               ->frozen.has_value() );
  REQUIRE( viz.dwelling_at( { .x = 1, .y = 0 } )
               ->frozen->mission.has_value() );
  REQUIRE(
      *viz.dwelling_at( { .x = 1, .y = 0 } )->frozen->mission ==
      FrozenMission{ .player = e_player::spanish,
                     .level  = e_missionary_type::jesuit } );
  REQUIRE( w.players().players[e_player::spanish].has_value() );
  REQUIRE( w.players().players[e_player::spanish]->control ==
           e_player_control::ai );
  REQUIRE( w.players().players[e_player::french].has_value() );
  REQUIRE( w.players().players[e_player::french]->control ==
           e_player_control::human );
  REQUIRE_FALSE( w.events().war_of_succession_done );

  plan = {
    .nations            = { .withdraws = e_nation::spanish,
                            .receives  = e_nation::french },
    .remove_units       = { removed_1 },
    .reassign_units     = { reassign_unit_1, reassign_unit_2,
                            reassign_unit_3, reassign_unit_4,
                            reassign_unit_5 },
    .reassign_colonies  = { reassign_colony_1, reassign_colony_2,
                            reassign_colony_3 },
    .update_fog_squares = { { .x = 0, .y = 0 },
                            { .x = 1, .y = 0 },
                            { .x = 0, .y = 2 },
                            { .x = 2, .y = 2 } } };

  f();

  REQUIRE( w.units().all().size() == 8 );
  REQUIRE_FALSE( w.units().exists( removed_1 ) );
  REQUIRE( w.units().exists( reassign_unit_1 ) );
  REQUIRE( w.units().exists( reassign_unit_2 ) );
  REQUIRE( w.units().exists( reassign_unit_3 ) );
  REQUIRE( w.units().exists( reassign_unit_4 ) );
  REQUIRE( w.units().exists( dutch_unit_1 ) );
  REQUIRE( w.units().exists( french_unit_1 ) );
  REQUIRE_FALSE( w.units().exists( reassign_unit_5 ) );
  REQUIRE( w.units().unit_for( reassign_unit_1 ).player_type() ==
           e_player::french );
  REQUIRE( w.units().unit_for( reassign_unit_2 ).player_type() ==
           e_player::french );
  REQUIRE( w.units().unit_for( reassign_unit_3 ).player_type() ==
           e_player::french );
  REQUIRE( w.units().unit_for( reassign_unit_4 ).player_type() ==
           e_player::french );
  REQUIRE( w.units().unit_for( dutch_unit_1 ).player_type() ==
           e_player::dutch );
  REQUIRE( w.units().unit_for( french_unit_1 ).player_type() ==
           e_player::french );
  REQUIRE( w.colonies().all().size() == 4 );
  REQUIRE( w.colonies().exists( reassign_colony_1 ) );
  REQUIRE( w.colonies().exists( reassign_colony_2 ) );
  REQUIRE( w.colonies().exists( reassign_colony_3 ) );
  REQUIRE( w.colonies().exists( french_colony_1 ) );
  REQUIRE( w.colonies().colony_for( reassign_colony_1 ).player ==
           e_player::french );
  REQUIRE( w.colonies().colony_for( reassign_colony_2 ).player ==
           e_player::french );
  REQUIRE( w.colonies().colony_for( reassign_colony_3 ).player ==
           e_player::french );
  REQUIRE( w.colonies().colony_for( french_colony_1 ).player ==
           e_player::french );
  REQUIRE( reassign_colony_1_obj.sons_of_liberty
               .num_rebels_from_bells_only == 0 );
  REQUIRE( reassign_colony_1_obj.sons_of_liberty
               .last_sons_of_liberty_integral_percent == 0 );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
           e_tile_visibility::fogged );
  REQUIRE( viz.visible( { .x = 0, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.dwelling_at( { .x = 1, .y = 0 } ).has_value() );
  REQUIRE( viz.dwelling_at( { .x = 1, .y = 0 } )
               ->frozen.has_value() );
  REQUIRE( viz.dwelling_at( { .x = 1, .y = 0 } )
               ->frozen->mission.has_value() );
  REQUIRE(
      *viz.dwelling_at( { .x = 1, .y = 0 } )->frozen->mission ==
      FrozenMission{ .player = e_player::french,
                     .level  = e_missionary_type::jesuit } );
  REQUIRE( w.players().players[e_player::spanish].has_value() );
  REQUIRE( w.players().players[e_player::spanish]->control ==
           e_player_control::withdrawn );
  REQUIRE( w.players().players[e_player::french].has_value() );
  REQUIRE( w.players().players[e_player::french]->control ==
           e_player_control::human );
  REQUIRE( w.events().war_of_succession_done );
}

TEST_CASE( "[rebel-sentiment] do_war_of_succession_ui_seq" ) {
  world w;
  WarOfSuccessionPlan plan;

  auto const f = [&] {
    co_await_test( do_war_of_succession_ui_seq( w.ts(), plan ) );
  };

  plan = { .nations = { .withdraws = e_nation::spanish,
                        .receives  = e_nation::french } };

  w.gui().EXPECT__message_box( StrContains(
      "All property and territory owned by the [Spanish] has "
      "been ceded to the [French].  As a result, the [Spanish] "
      "have withdrawn from the New World." ) );
  f();
}

} // namespace
} // namespace rn
