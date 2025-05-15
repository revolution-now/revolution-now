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

// ss
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_nation::english );
    add_player( e_nation::spanish );
    add_player( e_nation::dutch );
    create_default_map();

    // Make sure to leave the french non-existent so that
    // throughout this module we can verify that the nation
    // chosen for intervention does not actually need to exist.
    // This is important because in a game where not all players
    // exist, we that might be the case.
    BASE_CHECK(
        !players().players[e_nation::french].has_value() );
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
}

TEST_CASE( "[intervention] find_intervention_deploy_tile" ) {
  world w;
}

TEST_CASE( "[intervention] deploy_intervention_forces" ) {
  world w;
}

TEST_CASE( "[intervention] intervention_forces_intro_ui_seq" ) {
  world w;
}

TEST_CASE(
    "[intervention] intervention_forces_triggered_ui_seq" ) {
  world w;
}

TEST_CASE(
    "[intervention] intervention_forces_deployed_ui_seq" ) {
  world w;
}

TEST_CASE(
    "[intervention] "
    "animate_move_intervention_units_into_colony" ) {
  world w;
}

TEST_CASE(
    "[intervention] move_intervention_units_into_colony" ) {
  world w;
}

} // namespace
} // namespace rn
