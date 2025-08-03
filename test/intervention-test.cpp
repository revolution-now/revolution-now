/****************************************************************
**intervention-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-11.
*
* Description: Unit tests for the intervention module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/intervention.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/plane-stack.hpp"

// ss
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::english );
    add_player( e_player::spanish );
    add_player( e_player::dutch );
    create_default_map();

    // Make sure to leave the french non-existent so that
    // throughout this module we can verify that the nation
    // chosen for intervention does not actually need to exist.
    // This is important because in a game where not all players
    // exist, we that might be the case.
    BASE_CHECK(
        !players().players[e_player::french].has_value() );
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
      0  1  2  3  4  5  6  7  8  9 */
      L, L, L, L, L, L, L, L, L, _, // 0
      L, _, L, L, L, L, L, L, L, _, // 1
      L, L, L, L, L, L, L, L, L, _, // 2
      L, L, L, L, L, L, L, L, L, _, // 3
      _, L, L, L, L, L, L, L, L, _, // 4
      _, L, L, L, L, L, L, L, L, _, // 5
      _, L, L, _, L, L, L, L, L, _, // 6
      _, L, L, _, L, L, _, _, _, _, // 7
      _, L, L, _, L, L, L, L, L, _, // 8
      _, L, L, L, L, L, L, L, L, _, // 9
      _, L, L, L, L, L, L, L, L, _, // A
    };
    // clang-format on
    build_map( std::move( tiles ), 10 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[intervention] bells_required_for_intervention" ) {
  world w;

  auto const f = [&] {
    return bells_required_for_intervention( w.settings() );
  };

  auto& difficulty = w.settings().game_setup_options.difficulty;

  using enum e_difficulty;

  difficulty = discoverer;
  REQUIRE( f() == 2000 );

  difficulty = explorer;
  REQUIRE( f() == 3500 );

  difficulty = conquistador;
  REQUIRE( f() == 5000 );

  difficulty = governor;
  REQUIRE( f() == 6500 );

  difficulty = viceroy;
  REQUIRE( f() == 8000 );
}

TEST_CASE( "[intervention] select_nation_for_intervention" ) {
  world w;

  auto const f = [&]( e_nation const for_nation ) {
    return select_nation_for_intervention( for_nation );
  };

  using enum e_nation;

  REQUIRE( f( english ) == french );
  REQUIRE( f( french ) == spanish );
  REQUIRE( f( spanish ) == french );
  REQUIRE( f( dutch ) == french );
}

TEST_CASE( "[intervention] should_trigger_intervention" ) {
  world w;

  Player& player = w.english();

  auto const f = [&] {
    return should_trigger_intervention( w.ss(), player );
  };

  using enum e_difficulty;
  using enum e_revolution_status;

  auto& difficulty = w.settings().game_setup_options.difficulty;

  // Default.
  REQUIRE_FALSE( f() );

  // Trampoline.
  difficulty                                    = conquistador;
  player.revolution.status                      = declared;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 5000;
  REQUIRE( f() );

  difficulty                                    = explorer;
  player.revolution.status                      = declared;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 5000;
  REQUIRE( f() );

  difficulty                                    = governor;
  player.revolution.status                      = declared;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 5000;
  REQUIRE_FALSE( f() );

  difficulty                                    = conquistador;
  player.revolution.status                      = not_declared;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 5000;
  REQUIRE_FALSE( f() );

  difficulty                                    = conquistador;
  player.revolution.status                      = won;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 5000;
  REQUIRE_FALSE( f() );

  difficulty                                    = conquistador;
  player.revolution.status                      = lost;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 5000;
  REQUIRE_FALSE( f() );

  difficulty                                    = conquistador;
  player.revolution.status                      = declared;
  player.revolution.intervention_force_deployed = true;
  player.bells                                  = 5000;
  REQUIRE_FALSE( f() );

  difficulty                                    = conquistador;
  player.revolution.status                      = declared;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 4999;
  REQUIRE_FALSE( f() );

  difficulty                                    = conquistador;
  player.revolution.status                      = declared;
  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 5000;
  REQUIRE( f() );
}

TEST_CASE( "[intervention] trigger_intervention" ) {
  world w;

  Player& player = w.english();

  player.revolution.intervention_force_deployed = false;
  player.bells                                  = 123;

  auto old_player = player;
  trigger_intervention( player );
  REQUIRE_FALSE( ( old_player == player ) );

  old_player.revolution.intervention_force_deployed = true;
  old_player.bells                                  = 0;
  REQUIRE( ( old_player == player ) );
}

TEST_CASE( "[intervention] pick_forces_to_deploy" ) {
  world w;
  InterventionLandUnits expected;

  Player& player = w.spanish();
  auto& force    = player.revolution.intervention_force;

  auto const f = [&] { return pick_forces_to_deploy( player ); };

  // Default.
  REQUIRE( f() == nothing );

  force.continental_army    = 1;
  force.continental_cavalry = 2;
  force.artillery           = 3;
  REQUIRE( f() == nothing );

  force = {};
  REQUIRE( f() == nothing );

  force.man_o_war = 1;
  expected        = {};
  REQUIRE( f() == expected );

  force.man_o_war = 10;
  expected        = {};
  REQUIRE( f() == expected );

  force                  = {};
  force.continental_army = 1;
  force.man_o_war        = 1;
  expected               = { .continental_army = 1 };
  REQUIRE( f() == expected );

  force                     = {};
  force.continental_army    = 1;
  force.continental_cavalry = 1;
  force.man_o_war           = 1;
  expected = { .continental_army = 1, .continental_cavalry = 1 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 1,
               .continental_cavalry = 1,
               .artillery           = 1,
               .man_o_war           = 1 };
  expected = { .continental_army    = 1,
               .continental_cavalry = 1,
               .artillery           = 1 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 3,
               .continental_cavalry = 2,
               .artillery           = 2,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 6,
               .continental_cavalry = 2,
               .artillery           = 2,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 6,
               .continental_cavalry = 6,
               .artillery           = 6,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 6,
               .continental_cavalry = 6,
               .artillery           = 60,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 6,
               .continental_cavalry = 2,
               .artillery           = 60,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 6,
               .continental_cavalry = 1,
               .artillery           = 60,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 1,
               .artillery           = 3 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 6,
               .continental_cavalry = 1,
               .artillery           = 6,
               .man_o_war           = 2 };
  expected = { .continental_army    = 3,
               .continental_cavalry = 1,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 6,
               .continental_cavalry = 1,
               .artillery           = 5,
               .man_o_war           = 2 };
  expected = { .continental_army    = 3,
               .continental_cavalry = 1,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 5,
               .continental_cavalry = 1,
               .artillery           = 5,
               .man_o_war           = 2 };
  expected = { .continental_army    = 3,
               .continental_cavalry = 1,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 5,
               .continental_cavalry = 0,
               .artillery           = 5,
               .man_o_war           = 2 };
  expected = { .continental_army    = 3,
               .continental_cavalry = 0,
               .artillery           = 3 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 5,
               .continental_cavalry = 0,
               .artillery           = 3,
               .man_o_war           = 2 };
  expected = { .continental_army    = 4,
               .continental_cavalry = 0,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 0,
               .continental_cavalry = 5,
               .artillery           = 3,
               .man_o_war           = 2 };
  expected = { .continental_army    = 0,
               .continental_cavalry = 4,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 0,
               .continental_cavalry = 3,
               .artillery           = 3,
               .man_o_war           = 2 };
  expected = { .continental_army    = 0,
               .continental_cavalry = 3,
               .artillery           = 3 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 0,
               .continental_cavalry = 3,
               .artillery           = 2,
               .man_o_war           = 2 };
  expected = { .continental_army    = 0,
               .continental_cavalry = 3,
               .artillery           = 2 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 0,
               .continental_cavalry = 3,
               .artillery           = 1,
               .man_o_war           = 2 };
  expected = { .continental_army    = 0,
               .continental_cavalry = 3,
               .artillery           = 1 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 0,
               .continental_cavalry = 2,
               .artillery           = 0,
               .man_o_war           = 2 };
  expected = { .continental_army    = 0,
               .continental_cavalry = 2,
               .artillery           = 0 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 0,
               .continental_cavalry = 1,
               .artillery           = 0,
               .man_o_war           = 2 };
  expected = { .continental_army    = 0,
               .continental_cavalry = 1,
               .artillery           = 0 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 0,
               .continental_cavalry = 0,
               .artillery           = 0,
               .man_o_war           = 2 };
  expected = { .continental_army    = 0,
               .continental_cavalry = 0,
               .artillery           = 0 };
  REQUIRE( f() == expected );

  force    = { .continental_army    = 300,
               .continental_cavalry = 200,
               .artillery           = 100,
               .man_o_war           = 2 };
  expected = { .continental_army    = 2,
               .continental_cavalry = 2,
               .artillery           = 2 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[intervention] find_intervention_deploy_tile" ) {
  world w;
  InterventionDeployTile expected;

  w.update_terrain_connectivity();

  Player const& player = w.english();

  auto const f = [&] {
    return find_intervention_deploy_tile(
        w.ss().as_const, w.rand(), w.connectivity(), player );
  };

  static vector<int> const direction_idxs{ 0, 1, 2, 3,
                                           4, 5, 6, 7 };
  expect_shuffle( w.rand(), direction_idxs );

  SECTION( "default" ) { REQUIRE( f() == nothing ); }

  SECTION( "selects col_id=3, (6,7) [sw]" ) {
    // There are four friendly colonies.
    static vector<int> const colonies_idxs{ 0, 1, 2, 3 };
    expect_shuffle( w.rand(), colonies_idxs );
    // has sea lane, foreign.
    w.add_colony( { .x = 5, .y = 7 }, e_player::spanish );
    // Land Locked.
    w.add_colony( { .x = 6, .y = 4 }, player.type );
    // no sea lane.
    w.add_colony( { .x = 2, .y = 7 }, player.type );
    // has sea lane.
    w.add_colony( { .x = 7, .y = 6 }, player.type );
    // has sea lane, map edge.
    w.add_colony( { .x = 8, .y = 10 }, player.type );

    expected = {
      .tile      = { .x = 6, .y = 7 },
      .colony_id = 4 // one based.
    };
    REQUIRE( f() == expected );
  }

  SECTION( "colonies sorted differently" ) {
    // There are four friendly colonies.
    static vector<int> const colonies_idxs{ 0, 1, 3, 2 };
    expect_shuffle( w.rand(), colonies_idxs );
    // has sea lane, foreign.
    w.add_colony( { .x = 5, .y = 7 }, e_player::spanish );
    // Land Locked.
    w.add_colony( { .x = 6, .y = 4 }, player.type );
    // no sea lane.
    w.add_colony( { .x = 2, .y = 7 }, player.type );
    // has sea lane.
    w.add_colony( { .x = 7, .y = 6 }, player.type );
    // has sea lane, map edge.
    w.add_colony( { .x = 8, .y = 10 }, player.type );

    expected = {
      .tile      = { .x = 9, .y = 9 },
      .colony_id = 5 // one based.
    };
    REQUIRE( f() == expected );
  }

  SECTION( "friendly unit does not block" ) {
    // There are four friendly colonies.
    static vector<int> const colonies_idxs{ 0, 1, 3, 2 };
    expect_shuffle( w.rand(), colonies_idxs );
    // has sea lane, foreign.
    w.add_colony( { .x = 5, .y = 7 }, e_player::spanish );
    // Land Locked.
    w.add_colony( { .x = 6, .y = 4 }, player.type );
    // no sea lane.
    w.add_colony( { .x = 2, .y = 7 }, player.type );
    // has sea lane.
    w.add_colony( { .x = 7, .y = 6 }, player.type );
    // has sea lane, map edge.
    w.add_colony( { .x = 8, .y = 10 }, player.type );

    // Friendly unit should not interfere.
    w.add_unit_on_map( e_unit_type::caravel, { .x = 9, .y = 9 },
                       e_player::english );

    expected = {
      .tile      = { .x = 9, .y = 9 },
      .colony_id = 5 // one based.
    };
    REQUIRE( f() == expected );
  }

  SECTION( "first choice tile blocked by foreign unit" ) {
    // There are four friendly colonies.
    static vector<int> const colonies_idxs{ 0, 1, 3, 2 };
    expect_shuffle( w.rand(), colonies_idxs );
    // has sea lane, foreign.
    w.add_colony( { .x = 5, .y = 7 }, e_player::spanish );
    // Land Locked.
    w.add_colony( { .x = 6, .y = 4 }, player.type );
    // no sea lane.
    w.add_colony( { .x = 2, .y = 7 }, player.type );
    // has sea lane.
    w.add_colony( { .x = 7, .y = 6 }, player.type );
    // has sea lane, map edge.
    w.add_colony( { .x = 8, .y = 10 }, player.type );

    // Normally it would have chosen this square.
    w.add_unit_on_map( e_unit_type::caravel, { .x = 9, .y = 9 },
                       e_player::spanish );

    expected = {
      .tile      = { .x = 9, .y = 10 },
      .colony_id = 5 // one based.
    };
    REQUIRE( f() == expected );
  }

  SECTION( "probes off-map tiles." ) {
    // There are four friendly colonies.
    static vector<int> const colonies_idxs{ 0, 1, 3, 2 };
    expect_shuffle( w.rand(), colonies_idxs );
    // has sea lane, foreign.
    w.add_colony( { .x = 5, .y = 7 }, e_player::spanish );
    // Land Locked.
    w.add_colony( { .x = 6, .y = 4 }, player.type );
    // no sea lane.
    w.add_colony( { .x = 2, .y = 7 }, player.type );
    // has sea lane.
    w.add_colony( { .x = 7, .y = 6 }, player.type );
    // has sea lane, map edge.
    w.add_colony( { .x = 8, .y = 10 }, player.type );

    // Normally it would have chosen one of these squares.
    w.add_unit_on_map( e_unit_type::caravel, { .x = 9, .y = 9 },
                       e_player::spanish );
    w.add_unit_on_map( e_unit_type::caravel, { .x = 9, .y = 10 },
                       e_player::spanish );

    expected = {
      .tile      = { .x = 6, .y = 7 },
      .colony_id = 4 // one based.
    };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[intervention] deploy_intervention_forces" ) {
  world w;
  InterventionLandUnits forces;

  Player& player = w.english();

  w.add_colony( { .x = 1, .y = 5 }, player.type );
  InterventionDeployTile const location{
    .tile = { .x = 0, .y = 5 }, .colony_id = 1 };

  auto const f = [&] {
    return deploy_intervention_forces( w.ss(), w.ts(), location,
                                       forces );
  };

  using enum e_unit_type;

  REQUIRE( w.units().all().size() == 0 );

  SECTION( "ship only" ) {
    player.revolution.intervention_force.man_o_war = 1;
    REQUIRE( f() == UnitId{ 1 } );
    REQUIRE( w.units().all().size() == 1 );
    REQUIRE( w.units().unit_for( UnitId{ 1 } ).type() ==
             man_o_war );
    REQUIRE(
        as_const( w.units() ).ownership_of( UnitId{ 1 } ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 5 } } );
  }

  SECTION( "partial" ) {
    player.revolution.intervention_force.man_o_war = 1;
    player.revolution.intervention_force.continental_cavalry =
        10;
    player.revolution.intervention_force.continental_army = 10;
    forces.artillery                                      = 0;
    forces.continental_cavalry                            = 1;
    forces.continental_army                               = 2;
    REQUIRE( f() == UnitId{ 1 } );
    REQUIRE( w.units().all().size() == 4 );
    REQUIRE( w.units().unit_for( UnitId{ 1 } ).type() ==
             man_o_war );
    REQUIRE( w.units().unit_for( UnitId{ 2 } ).type() ==
             continental_army );
    REQUIRE( w.units().unit_for( UnitId{ 3 } ).type() ==
             continental_army );
    REQUIRE( w.units().unit_for( UnitId{ 4 } ).type() ==
             continental_cavalry );
    REQUIRE(
        as_const( w.units() ).ownership_of( UnitId{ 1 } ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 5 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 2 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 3 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 4 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  }

  SECTION( "full" ) {
    player.revolution.intervention_force.man_o_war           = 1;
    player.revolution.intervention_force.continental_cavalry = 4;
    player.revolution.intervention_force.continental_army    = 3;
    player.revolution.intervention_force.artillery           = 2;
    forces.artillery                                         = 2;
    forces.continental_cavalry                               = 2;
    forces.continental_army                                  = 2;
    REQUIRE( f() == UnitId{ 1 } );
    REQUIRE( w.units().all().size() == 7 );
    REQUIRE( w.units().unit_for( UnitId{ 1 } ).type() ==
             man_o_war );
    REQUIRE( w.units().unit_for( UnitId{ 2 } ).type() ==
             continental_army );
    REQUIRE( w.units().unit_for( UnitId{ 3 } ).type() ==
             continental_army );
    REQUIRE( w.units().unit_for( UnitId{ 4 } ).type() ==
             continental_cavalry );
    REQUIRE( w.units().unit_for( UnitId{ 5 } ).type() ==
             continental_cavalry );
    REQUIRE( w.units().unit_for( UnitId{ 6 } ).type() ==
             artillery );
    REQUIRE( w.units().unit_for( UnitId{ 7 } ).type() ==
             artillery );
    REQUIRE(
        as_const( w.units() ).ownership_of( UnitId{ 1 } ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 5 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 2 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 3 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 4 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 5 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 6 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 7 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  }
}

TEST_CASE( "[intervention] intervention_forces_intro_ui_seq" ) {
  world w;

  e_nation receiving   = {};
  e_nation intervening = {};

  auto const f = [&] {
    co_await_test( intervention_forces_intro_ui_seq(
        w.ss(), w.gui(), receiving, intervening ) );
  };

  using enum e_nation;
  using enum e_difficulty;

  receiving                                  = spanish;
  intervening                                = french;
  w.settings().game_setup_options.difficulty = conquistador;
  w.gui().EXPECT__message_box(
      "The [French] are considering intervening in the war "
      "against the Spanish Crown, your Excellency.  They will "
      "do so if we can generate [5000] liberty bells during the "
      "war." );
  f();

  receiving                                  = english;
  intervening                                = spanish;
  w.settings().game_setup_options.difficulty = viceroy;
  w.gui().EXPECT__message_box(
      "The [Spanish] are considering intervening in the war "
      "against the English Crown, your Excellency.  They will "
      "do so if we can generate [8000] liberty bells during the "
      "war." );
  f();
}

TEST_CASE(
    "[intervention] intervention_forces_triggered_ui_seq" ) {
  world w;

  e_nation receiving   = {};
  e_nation intervening = {};

  auto const f = [&] {
    co_await_test( intervention_forces_triggered_ui_seq(
        w.ss(), w.gui(), receiving, intervening ) );
  };

  using enum e_nation;
  using enum e_difficulty;

  // No colonies.
  f();

  // One empty colony.
  Colony& colony_1 =
      w.add_colony( { .x = 0, .y = 0 }, e_player::spanish );
  colony_1.name                              = "abc";
  receiving                                  = spanish;
  intervening                                = french;
  w.settings().game_setup_options.difficulty = conquistador;
  w.gui().EXPECT__message_box(
      "[France] declares war on Spain and joins the War of "
      "Independence on the side of the Rebels! French Generals "
      "meet in abc for strategic discussions. French Navy plans "
      "to bombard Tory strongholds in the New World." );
  f();

  // Two colonies.
  Colony& colony_2 =
      w.add_colony( { .x = 2, .y = 0 }, e_player::spanish );
  colony_2.name = "def";
  w.add_unit_indoors( colony_2.id, e_indoor_job::bells );
  w.add_unit_indoors( colony_2.id, e_indoor_job::bells );
  receiving                                  = spanish;
  intervening                                = french;
  w.settings().game_setup_options.difficulty = conquistador;
  w.gui().EXPECT__message_box(
      "[France] declares war on Spain and joins the War of "
      "Independence on the side of the Rebels! French Generals "
      "meet in def for strategic discussions. French Navy plans "
      "to bombard Tory strongholds in the New World." );
  f();

  // Three colonies.
  Colony& colony_3 =
      w.add_colony( { .x = 0, .y = 2 }, e_player::spanish );
  colony_3.name = "ghi";
  w.add_unit_indoors( colony_3.id, e_indoor_job::bells );
  receiving                                  = spanish;
  intervening                                = french;
  w.settings().game_setup_options.difficulty = conquistador;
  w.gui().EXPECT__message_box(
      "[France] declares war on Spain and joins the War of "
      "Independence on the side of the Rebels! French Generals "
      "meet in def for strategic discussions. French Navy plans "
      "to bombard Tory strongholds in the New World." );
  f();
}

TEST_CASE(
    "[intervention] intervention_forces_deployed_ui_seq" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  using enum e_nation;

  e_nation intervening = {};

  Colony& colony =
      w.add_colony( { .x = 1, .y = 5 }, e_player::english );
  colony.name = "abc";

  auto const f = [&] {
    co_await_test( intervention_forces_deployed_ui_seq(
        w.ts(), colony, intervening ) );
  };

  intervening = french;
  mock_land_view.EXPECT__ensure_visible(
      Coord{ .x = 1, .y = 5 } );
  w.gui().EXPECT__message_box(
      "French intervention forces arrive in [abc].  French "
      "General joins forces with the Rebels." );
  f();
}

TEST_CASE(
    "[intervention] "
    "animate_move_intervention_units_into_colony" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  using enum e_player;

  Colony& colony = w.add_colony( { .x = 1, .y = 5 }, english );
  UnitId const ship_id =
      w.add_unit_on_map( e_unit_type::man_o_war,
                         { .x = 0, .y = 5 } )
          .id();

  auto const f = [&] {
    co_await_test( animate_move_intervention_units_into_colony(
        w.ss(), w.ts(), ship_id, colony ) );
  };

  mock_land_view.EXPECT__animate_if_visible( _ );
  f();
}

TEST_CASE(
    "[intervention] move_intervention_units_into_colony" ) {
  world w;

  using enum e_player;
  using enum e_unit_type;

  Colony& colony = w.add_colony( { .x = 1, .y = 5 }, english );
  UnitId const ship_id =
      w.add_unit_on_map( e_unit_type::man_o_war,
                         { .x = 0, .y = 5 } )
          .id();

  auto const f = [&] {
    move_intervention_units_into_colony( w.ss(), w.ts(), ship_id,
                                         colony );
  };

  w.add_unit_in_cargo( continental_army, ship_id );
  w.add_unit_in_cargo( continental_army, ship_id );
  w.add_unit_in_cargo( continental_cavalry, ship_id );
  w.add_unit_in_cargo( continental_cavalry, ship_id );
  w.add_unit_in_cargo( artillery, ship_id );
  w.add_unit_in_cargo( artillery, ship_id );

  REQUIRE( w.units().all().size() == 7 );
  REQUIRE( w.units().unit_for( UnitId{ 1 } ).type() ==
           man_o_war );
  REQUIRE( w.units().unit_for( UnitId{ 2 } ).type() ==
           continental_army );
  REQUIRE( w.units().unit_for( UnitId{ 3 } ).type() ==
           continental_army );
  REQUIRE( w.units().unit_for( UnitId{ 4 } ).type() ==
           continental_cavalry );
  REQUIRE( w.units().unit_for( UnitId{ 5 } ).type() ==
           continental_cavalry );
  REQUIRE( w.units().unit_for( UnitId{ 6 } ).type() ==
           artillery );
  REQUIRE( w.units().unit_for( UnitId{ 7 } ).type() ==
           artillery );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 1 } ) ==
           UnitOwnership::world{ .coord = { .x = 0, .y = 5 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 2 } ) ==
           UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 3 } ) ==
           UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 4 } ) ==
           UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 5 } ) ==
           UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 6 } ) ==
           UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 7 } ) ==
           UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  REQUIRE( w.units().unit_for( UnitId{ 1 } ).orders() ==
           unit_orders::none{} );
  REQUIRE( w.units().unit_for( UnitId{ 2 } ).orders() ==
           unit_orders::sentry{} );
  REQUIRE( w.units().unit_for( UnitId{ 3 } ).orders() ==
           unit_orders::sentry{} );
  REQUIRE( w.units().unit_for( UnitId{ 4 } ).orders() ==
           unit_orders::sentry{} );
  REQUIRE( w.units().unit_for( UnitId{ 5 } ).orders() ==
           unit_orders::sentry{} );
  REQUIRE( w.units().unit_for( UnitId{ 6 } ).orders() ==
           unit_orders::sentry{} );
  REQUIRE( w.units().unit_for( UnitId{ 7 } ).orders() ==
           unit_orders::sentry{} );

  f();

  REQUIRE( w.units().all().size() == 7 );
  REQUIRE( w.units().unit_for( UnitId{ 1 } ).type() ==
           man_o_war );
  REQUIRE( w.units().unit_for( UnitId{ 2 } ).type() ==
           continental_army );
  REQUIRE( w.units().unit_for( UnitId{ 3 } ).type() ==
           continental_army );
  REQUIRE( w.units().unit_for( UnitId{ 4 } ).type() ==
           continental_cavalry );
  REQUIRE( w.units().unit_for( UnitId{ 5 } ).type() ==
           continental_cavalry );
  REQUIRE( w.units().unit_for( UnitId{ 6 } ).type() ==
           artillery );
  REQUIRE( w.units().unit_for( UnitId{ 7 } ).type() ==
           artillery );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 1 } ) ==
           UnitOwnership::world{ .coord = { .x = 0, .y = 5 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 2 } ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 5 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 3 } ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 5 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 4 } ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 5 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 5 } ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 5 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 6 } ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 5 } } );
  REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 7 } ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 5 } } );
  REQUIRE( w.units().unit_for( UnitId{ 1 } ).orders() ==
           unit_orders::none{} );
  REQUIRE( w.units().unit_for( UnitId{ 2 } ).orders() ==
           unit_orders::none{} );
  REQUIRE( w.units().unit_for( UnitId{ 3 } ).orders() ==
           unit_orders::none{} );
  REQUIRE( w.units().unit_for( UnitId{ 4 } ).orders() ==
           unit_orders::none{} );
  REQUIRE( w.units().unit_for( UnitId{ 5 } ).orders() ==
           unit_orders::none{} );
  REQUIRE( w.units().unit_for( UnitId{ 6 } ).orders() ==
           unit_orders::none{} );
  REQUIRE( w.units().unit_for( UnitId{ 7 } ).orders() ==
           unit_orders::none{} );
}

} // namespace
} // namespace rn
