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
#include "test/mocks/irand.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

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
  int count = {};
  vector<e_unit_type> expected;

  auto const f = [&] [[clang::noinline]] {
    return generate_uprising_units( w.rand(), count );
  };

  count = 4;
  w.rand().EXPECT__between_ints( 0, 99 ).returns( 78 );
  w.rand().EXPECT__between_ints( 0, 99 ).returns( 50 );
  w.rand().EXPECT__between_ints( 0, 99 ).returns( 95 );
  w.rand().EXPECT__between_ints( 0, 99 ).returns( 0 );
  expected = {
    e_unit_type::dragoon,
    e_unit_type::veteran_soldier,
    e_unit_type::veteran_dragoon,
    e_unit_type::soldier,
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[uprising] distribute_uprising_units" ) {
  world w;
  UprisingColony up_colony;
  vector<e_unit_type> unit_types;
  vector<pair<e_unit_type, point>> expected;

  auto const f = [&] [[clang::noinline]] {
    return distribute_uprising_units( w.rand(), up_colony,
                                      unit_types );
  };

  using enum e_unit_type;

  up_colony = {
    .available_tiles_adjacent =
        {
          { .x = 2, .y = 2 },
        },
  };
  unit_types = {};
  expected   = {};
  REQUIRE( f() == expected );

  up_colony = {
    .available_tiles_adjacent =
        {
          { .x = 2, .y = 2 },
          { .x = 3, .y = 2 },
          { .x = 4, .y = 2 },
          { .x = 4, .y = 4 },
        },
    .available_tiles_beyond =
        {
          { .x = 3, .y = 1 },
          { .x = 4, .y = 1 },
        },
  };
  unit_types = { soldier, dragoon, soldier, veteran_dragoon,
                 dragoon, soldier, soldier, soldier,
                 dragoon, dragoon };
  expected   = {
    { soldier, { .x = 2, .y = 2 } },
    { dragoon, { .x = 4, .y = 4 } },
    { soldier, { .x = 3, .y = 2 } },
    { veteran_dragoon, { .x = 4, .y = 2 } },
    { dragoon, { .x = 2, .y = 2 } },
    { soldier, { .x = 4, .y = 4 } },
    { soldier, { .x = 3, .y = 2 } },
    { soldier, { .x = 4, .y = 2 } },
    { dragoon, { .x = 3, .y = 1 } },
    { dragoon, { .x = 4, .y = 1 } },
  };
  vector<int> const adjacent_idxs{ 0, 3, 1, 2 };
  vector<int> const beyond_idxs{ 0, 1 };
  expect_shuffle( w.rand(), adjacent_idxs );
  expect_shuffle( w.rand(), beyond_idxs );
  REQUIRE( f() == expected );
}

TEST_CASE( "[uprising] deploy_uprising_units" ) {
  world w;
  UprisingColony up_colony;
  vector<pair<e_unit_type, point>> units;
  Player const& ref_player = w.ref_english();

  auto const f = [&] [[clang::noinline]] {
    deploy_uprising_units( w.ss(), ref_player, w.map_updater(),
                           up_colony, units );
  };

  Colony& colony      = w.add_colony( { .x = 3, .y = 3 } );
  up_colony.colony_id = colony.id;

  units = {
    { e_unit_type::soldier, { .x = 2, .y = 2 } },
    { e_unit_type::dragoon, { .x = 4, .y = 4 } },
  };

  REQUIRE( colony.had_tory_uprising == false );
  REQUIRE( w.units().all().size() == 0 );

  f();

  REQUIRE( colony.had_tory_uprising == true );
  REQUIRE( w.units().all().size() == 2 );
  Unit const& unit1 = w.units().unit_for( UnitId{ 1 } );
  Unit const& unit2 = w.units().unit_for( UnitId{ 2 } );
  REQUIRE( unit1.has_full_mv_points() );
  REQUIRE( unit2.has_full_mv_points() );
  REQUIRE( unit1.type() == e_unit_type::soldier );
  REQUIRE( unit2.type() == e_unit_type::dragoon );
  REQUIRE( as_const( w.units() ).ownership_of( unit1.id() ) ==
           UnitOwnership::world{ .coord = { .x = 2, .y = 2 } } );
  REQUIRE( as_const( w.units() ).ownership_of( unit2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 4, .y = 4 } } );
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
