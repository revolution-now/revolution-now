/****************************************************************
**uprising-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-07.
*
* Description: Unit tests for the uprising module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/uprising.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"

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
    add_player( e_player::english );
    add_player( e_player::ref_english );
    set_default_player_type( e_player::english );
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,X,_, /*2
      3*/ _,X,X,X,X,X,X,_, /*3
      4*/ _,X,X,X,X,X,X,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,X,X,X,X,X,X,_, /*6
      7*/ _,X,X,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[uprising] should_attempt_uprising" ) {
  world w;
  Player& colonial_player       = w.english();
  Player const& ref_player      = w.ref_english();
  bool did_deploy_ref_this_turn = {};

  auto const f = [&] [[clang::noinline]] {
    return should_attempt_uprising( w.ss(), colonial_player,
                                    ref_player,
                                    did_deploy_ref_this_turn );
  };

  // Default.
  REQUIRE_FALSE( f() );

  // Make true.
  colonial_player.revolution.status =
      e_revolution_status::declared;
  did_deploy_ref_this_turn                       = false;
  colonial_player.revolution.expeditionary_force = {};
  REQUIRE( f() );

  colonial_player.revolution.status = e_revolution_status::won;
  did_deploy_ref_this_turn          = false;
  colonial_player.revolution.expeditionary_force = {};
  REQUIRE_FALSE( f() );

  colonial_player.revolution.status =
      e_revolution_status::not_declared;
  did_deploy_ref_this_turn                       = false;
  colonial_player.revolution.expeditionary_force = {};
  REQUIRE_FALSE( f() );

  colonial_player.revolution.status =
      e_revolution_status::declared;
  did_deploy_ref_this_turn                       = true;
  colonial_player.revolution.expeditionary_force = {};
  REQUIRE_FALSE( f() );

  colonial_player.revolution.status =
      e_revolution_status::declared;
  did_deploy_ref_this_turn                               = false;
  colonial_player.revolution.expeditionary_force.regular = 1;
  REQUIRE_FALSE( f() );

  w.settings()
      .game_setup_options.customized_rules.ref_can_spawn_ships =
      false;
  REQUIRE( f() );

  colonial_player.revolution.status =
      e_revolution_status::declared;
  did_deploy_ref_this_turn                               = false;
  colonial_player.revolution.expeditionary_force.regular = 1;
  colonial_player.revolution.expeditionary_force.man_o_war = 1;
  REQUIRE_FALSE( f() );

  colonial_player.revolution.expeditionary_force.man_o_war = 0;
  REQUIRE( f() );

  w.add_unit_on_map( e_unit_type::man_o_war, { .x = 0, .y = 0 },
                     e_player::english );
  REQUIRE( f() );

  w.add_unit_on_map( e_unit_type::man_o_war, { .x = 7, .y = 0 },
                     e_player::ref_english );
  REQUIRE_FALSE( f() );
}

TEST_CASE( "[uprising] find_uprising_colonies" ) {
  world w;
}

TEST_CASE( "[uprising] select_uprising_colony" ) {
  world w;
}

TEST_CASE( "[uprising] generate_uprising_units" ) {
  world w;
}

TEST_CASE( "[uprising] distribute_uprising_units" ) {
  world w;
}

TEST_CASE( "[uprising] deploy_uprising_units" ) {
  world w;
}

TEST_CASE( "[uprising] show_uprising_msg" ) {
  world w;
  UprisingColony up_colony;

  auto const f = [&] [[clang::noinline]] {
    co_await_test( show_uprising_msg( w.ss().as_const, w.gui(),
                                      up_colony ) );
  };

  Colony& colony = w.add_colony( { .x = 1, .y = 0 } );
  colony.name    = "my colony";

  up_colony = { .colony_id = colony.id };
  w.gui().EXPECT__message_box( "Tory uprising in [my colony]!" );
  f();
}

} // namespace
} // namespace rn
