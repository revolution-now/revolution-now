/****************************************************************
**immigration.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-28.
*
* Description: Unit tests for the src/immigration.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/immigration.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/harbor-units.hpp"
#include "src/igui.hpp"
#include "src/map-square.hpp"
#include "src/ustate.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/settings.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

// Rds
#include "old-world-state.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::AllOf;
using ::mock::matchers::Approx;
using ::mock::matchers::Field;
using ::mock::matchers::HasSize;
using ::mock::matchers::StartsWith;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_default_player(); }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[immigration] ask_player_to_choose_immigrant" ) {
  ImmigrationState immigration{
      .immigrants_pool = { e_unit_type::expert_farmer,
                           e_unit_type::veteran_soldier,
                           e_unit_type::seasoned_scout } };
  MockIGui gui;

  EXPECT_CALL(
      gui,
      choice(
          ChoiceConfig{
              .msg = "please select one",
              .options =
                  vector<ChoiceConfigOption>{
                      { .key          = "0",
                        .display_name = "Expert Farmer" },
                      { .key          = "1",
                        .display_name = "Veteran Soldier" },
                      { .key          = "2",
                        .display_name = "Seasoned Scout" } } },
          e_input_required::no ) )
      .returns( make_wait<maybe<string>>( "1" ) );

  wait<maybe<int>> w = ask_player_to_choose_immigrant(
      gui, immigration, "please select one" );
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == 1 );
}

TEST_CASE(
    "[immigration] ask_player_to_choose_immigrant cancels" ) {
  ImmigrationState immigration{
      .immigrants_pool = { e_unit_type::expert_farmer,
                           e_unit_type::veteran_soldier,
                           e_unit_type::seasoned_scout } };
  MockIGui gui;

  EXPECT_CALL(
      gui,
      choice(
          ChoiceConfig{
              .msg = "please select one",
              .options =
                  vector<ChoiceConfigOption>{
                      { .key          = "0",
                        .display_name = "Expert Farmer" },
                      { .key          = "1",
                        .display_name = "Veteran Soldier" },
                      { .key          = "2",
                        .display_name = "Seasoned Scout" } } },
          e_input_required::no ) )
      .returns( make_wait<maybe<string>>( nothing ) );

  wait<maybe<int>> w = ask_player_to_choose_immigrant(
      gui, immigration, "please select one" );
  REQUIRE( w.ready() );
  REQUIRE( *w == nothing );
}

TEST_CASE( "[immigration] compute_crosses (dutch)" ) {
  World world;
  world.add_player( e_nation::dutch );
  world.set_default_player( e_nation::dutch );
  world.create_default_map();

  UnitsState const& units_state = world.units();
  Player&           player      = world.default_player();
  REQUIRE( player.crosses == 0 );

  CrossesCalculation crosses, expected;

  SECTION( "default" ) {
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed     = 8 + 2 * ( 0 + 0 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "some units in new world" ) {
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed     = 8 + 2 * ( 2 + 0 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "one unit on dock, no production" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -2,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 1 + 1 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/0,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 0 );
  }

  SECTION( "one unit on dock" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -2,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 1 + 1 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 1 );
  }

  SECTION( "two units in cargo of harbor ship" ) {
    // In this section we're testing that units that are in the
    // cargo of ships in harbor get treated as being on the dock.
    // This is done to uphold the spirit of the original game,
    // which did not have a concept of units being inside the
    // cargo of a ship in either the harbor view or colony view.
    UnitId ship = world.add_unit_in_port( e_unit_type::caravel );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -4,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 2 + 3 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 0 );
  }

  SECTION( "two units in cargo of outbound ship" ) {
    // In this section we're testing that units that are in the
    // cargo of ships in harbor get treated as being on the dock.
    // This is done to uphold the spirit of the original game,
    // which did not have a concept of units being inside the
    // cargo of a ship in either the harbor view or colony view.
    UnitId ship = world.add_unit_in_port( e_unit_type::caravel );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    world.ship_to_outbound( ship );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 0 + 3 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "units on dock and in new world" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    world.add_unit_in_port( e_unit_type::free_colonist );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -4,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 4 + 2 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/8,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 4 );
  }
}

TEST_CASE( "[immigration] compute_crosses (english)" ) {
  World world;
  world.add_player( e_nation::english );
  world.set_default_player( e_nation::english );
  world.create_default_map();

  UnitsState const& units_state = world.units();
  Player&           player      = world.default_player();
  REQUIRE( player.crosses == 0 );

  CrossesCalculation crosses, expected;

  SECTION( "default" ) {
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed =
            int( std::lround( .6666 * ( 8 + 2 * ( 0 + 0 ) ) ) ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "some units in new world" ) {
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed =
            int( std::lround( .6666 * ( 8 + 2 * ( 2 + 0 ) ) ) ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "one unit on dock, no production" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -2,
        // Dock units are counted twice.
        .crosses_needed =
            int( std::lround( .6666 * ( 8 + 2 * ( 1 + 1 ) ) ) ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/0,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 0 );
  }
}

TEST_CASE( "[immigration] take_immigrant_from_pool" ) {
  ImmigrationState state;
  state.immigrants_pool[0] = e_unit_type::expert_tobacco_planter;
  state.immigrants_pool[1] = e_unit_type::free_colonist;
  state.immigrants_pool[2] = e_unit_type::veteran_dragoon;

  take_immigrant_from_pool( state, 1,
                            e_unit_type::petty_criminal );

  REQUIRE( state.immigrants_pool[0] ==
           e_unit_type::expert_tobacco_planter );
  REQUIRE( state.immigrants_pool[1] ==
           e_unit_type::petty_criminal );
  REQUIRE( state.immigrants_pool[2] ==
           e_unit_type::veteran_dragoon );
}

TEST_CASE( "[immigration] pick_next_unit_for_pool" ) {
  World   W;
  Player& player = W.default_player();

  bool found_petty_criminal     = false;
  bool found_indentured_servant = false;
  bool found_free_colonist      = false;
  bool found_expert_colonist    = false;

  W.settings().difficulty = e_difficulty::conquistador;
  // Calculated theoretically by computing all of the weights for
  // all unit types on the conquistador difficulty level and sum-
  // ming them, then truncating, since this needs to be slightly
  // smaller than the result if it is not equal so that d does
  // not exceed it.
  double const kUpperLimit = 6808.69;
  for( double d = 0.0; d < kUpperLimit; d += 100.0 ) {
    EXPECT_CALL( W.rand(), between_doubles(
                               0, Approx( kUpperLimit, .1 ) ) )
        .returns( d );
    e_unit_type type = pick_next_unit_for_pool( W.rand(), player,
                                                W.settings() );
    if( type == e_unit_type::petty_criminal )
      found_petty_criminal = true;
    if( type == e_unit_type::indentured_servant )
      found_indentured_servant = true;
    if( type == e_unit_type::free_colonist )
      found_free_colonist = true;
    if( unit_attr( type ).expertise.has_value() )
      found_expert_colonist = true;
  }

  REQUIRE( found_petty_criminal == true );
  REQUIRE( found_indentured_servant == true );
  REQUIRE( found_free_colonist == true );
  REQUIRE( found_expert_colonist == true );
}

TEST_CASE(
    "[immigration] pick_next_unit_for_pool with brewster" ) {
  World   W;
  Player& player = W.default_player();
  player.fathers.has[e_founding_father::william_brewster] = true;

  bool found_free_colonist   = false;
  bool found_expert_colonist = false;

  W.settings().difficulty = e_difficulty::conquistador;
  // Calculated theoretically by computing all of the weights for
  // all unit types on the conquistador difficulty level and sum-
  // ming them, except assuming that the weights for petty crim-
  // inal and indentured servant are zero. Then the sum is trun-
  // cated, since this needs to be slightly smaller than the re-
  // sult if it is not equal so that d does not exceed it.
  double const kUpperLimit = 4580.4;
  for( double d = 0.0; d < kUpperLimit; d += 100.0 ) {
    EXPECT_CALL( W.rand(), between_doubles(
                               0, Approx( kUpperLimit, .1 ) ) )
        .returns( d );
    e_unit_type type = pick_next_unit_for_pool( W.rand(), player,
                                                W.settings() );
    REQUIRE( type != e_unit_type::petty_criminal );
    REQUIRE( type != e_unit_type::indentured_servant );
    if( type == e_unit_type::free_colonist )
      found_free_colonist = true;
    if( unit_attr( type ).expertise.has_value() )
      found_expert_colonist = true;
  }

  REQUIRE( found_free_colonist == true );
  REQUIRE( found_expert_colonist == true );
}

TEST_CASE( "[immigration] check_for_new_immigrant" ) {
  World W;

  // Set up the immigrants pool.
  ImmigrationState const initial_state = {
      .immigrants_pool = { e_unit_type::expert_farmer,
                           e_unit_type::veteran_soldier,
                           e_unit_type::seasoned_scout } };
  Player& player = W.default_player();
  player.old_world.immigration.immigrants_pool =
      initial_state.immigrants_pool;

  SECTION( "not enough crosses" ) {
    player.crosses                     = 10;
    int const           crosses_needed = 11;
    wait<maybe<UnitId>> w              = check_for_new_immigrant(
        W.ss(), W.ts(), player, crosses_needed );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( player.crosses == 10 );
    REQUIRE( W.units().all().size() == 0 );
    REQUIRE( player.old_world.immigration.immigrants_pool ==
             initial_state.immigrants_pool );
  }

  SECTION( "just enough crosses, no brewster" ) {
    player.crosses           = 11;
    int const crosses_needed = 11;

    EXPECT_CALL( W.gui(), message_box( StartsWith(
                              "Word of religious freedom has "
                              "spread! A new immigrant" ) ) )
        .returns( make_wait<>() );
    // This one is to choose which immigrant we get, which is
    // done randomly because we don't have brewster.
    EXPECT_CALL(
        W.rand(),
        between_ints( 0, 2, IRand::e_interval::closed ) )
        .returns( 1 );
    // This one is to choose that unit's replacement in the pool,
    // which is always done randomly. 9960.0 was found by summing
    // all of the unit type weights for all units on the discov-
    // erer level.
    EXPECT_CALL( W.rand(),
                 between_doubles( 0, Approx( 9960.0, .00001 ) ) )
        .returns( 5000.0 ); // chosen to give scout.
    wait<maybe<UnitId>> w = check_for_new_immigrant(
        W.ss(), W.ts(), player, crosses_needed );
    REQUIRE( w.ready() );
    REQUIRE( *w == UnitId{ 1 } );
    REQUIRE( W.units().unit_for( **w ).type() ==
             e_unit_type::veteran_soldier );
    REQUIRE( player.old_world.immigration.immigrants_pool[1] ==
             e_unit_type::scout );

    REQUIRE( player.crosses == 0 );
    REQUIRE( W.units().all().size() == 1 );
    REQUIRE( W.units().all().begin()->first == UnitId{ 1 } );
    UnitOwnership_t const expected_ownership{
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{} } } };
    REQUIRE( W.units().all().begin()->second.ownership ==
             expected_ownership );
  }

  SECTION( "enough crosses, with brewster" ) {
    player.crosses = 13;
    player.fathers.has[e_founding_father::william_brewster] =
        true;
    int const crosses_needed = 11;

    EXPECT_CALL(
        W.gui(),
        choice(
            AllOf(
                Field(
                    &ChoiceConfig::msg,
                    StartsWith(
                        "Word of religious freedom has spread! "
                        "New immigrants are ready to join us in "
                        "the New World.  Which of the following "
                        "shall we choose?" ) ),
                Field( &ChoiceConfig::options, HasSize( 3 ) ) ),
            e_input_required::no ) )
        .returns( make_wait<maybe<string>>( "1" ) );
    // This one is to choose that unit's replacement in the pool,
    // which is always done randomly. 8960.0 was found by summing
    // all of the weights for the unit types on the discoverer
    // level except assuming that the petty criminal and inden-
    // tured servant weights are zero.
    EXPECT_CALL( W.rand(),
                 between_doubles( 0, Approx( 8960.0, .00001 ) ) )
        .returns( 5000.0 ); // chosen to give expert lumberjack.
    wait<maybe<UnitId>> w = check_for_new_immigrant(
        W.ss(), W.ts(), player, crosses_needed );
    REQUIRE( w.ready() );
    REQUIRE( w->has_value() );
    REQUIRE( **w == UnitId{ 1 } );
    REQUIRE( W.units().unit_for( **w ).type() ==
             e_unit_type::veteran_soldier );
    REQUIRE( player.old_world.immigration.immigrants_pool[1] ==
             e_unit_type::expert_lumberjack );

    REQUIRE( player.crosses == 2 );
    REQUIRE( W.units().all().size() == 1 );
    REQUIRE( W.units().all().begin()->first == UnitId{ 1 } );
    UnitOwnership_t const expected_ownership{
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{} } } };
    REQUIRE( W.units().all().begin()->second.ownership ==
             expected_ownership );
  }
}

TEST_CASE( "[immigration] cost_of_recruit" ) {
  World        W;
  Player&      player         = W.default_player();
  e_difficulty difficulty     = {};
  int          crosses_needed = 0;

  auto f = [&] {
    return cost_of_recruit( player, crosses_needed, difficulty );
  };

  difficulty     = e_difficulty::discoverer;
  crosses_needed = 10;
  player.crosses = 0;
  player.old_world.immigration.num_recruits_rushed = 0;
  REQUIRE( f() == 140 );

  difficulty     = e_difficulty::discoverer;
  crosses_needed = 10;
  player.crosses = 5;
  player.old_world.immigration.num_recruits_rushed = 0;
  REQUIRE( f() == 120 );

  difficulty     = e_difficulty::discoverer;
  crosses_needed = 10;
  player.crosses = 10;
  player.old_world.immigration.num_recruits_rushed = 0;
  REQUIRE( f() == 100 );

  difficulty     = e_difficulty::discoverer;
  crosses_needed = 100;
  player.crosses = 50;
  player.old_world.immigration.num_recruits_rushed = 0;
  REQUIRE( f() == 120 );

  difficulty     = e_difficulty::explorer;
  crosses_needed = 100;
  player.crosses = 77;
  player.old_world.immigration.num_recruits_rushed = 4;
  REQUIRE( f() == 132 );

  difficulty     = e_difficulty::conquistador;
  crosses_needed = 85;
  player.crosses = 33;
  player.old_world.immigration.num_recruits_rushed = 1;
  REQUIRE( f() == 161 );

  difficulty     = e_difficulty::viceroy;
  crosses_needed = 1;
  player.crosses = 1;
  player.old_world.immigration.num_recruits_rushed = 0;
  REQUIRE( f() == 100 );

  difficulty     = e_difficulty::viceroy;
  crosses_needed = 1;
  player.crosses = 0;
  player.old_world.immigration.num_recruits_rushed = 0;
  REQUIRE( f() == 220 );

  difficulty     = e_difficulty::viceroy;
  crosses_needed = 14;
  player.crosses = 10;
  player.old_world.immigration.num_recruits_rushed = 10;
  REQUIRE( f() == 191 );

  difficulty     = e_difficulty::governor;
  crosses_needed = 20;
  player.crosses = 30;
  player.old_world.immigration.num_recruits_rushed = 1;
  REQUIRE( f() == 40 );

  difficulty     = e_difficulty::discoverer;
  crosses_needed = 200;
  player.crosses = 613;
  player.old_world.immigration.num_recruits_rushed = 1;
  // Would be -24, but it bottoms out at the minimum=10.
  REQUIRE( f() == 10 );

  difficulty     = e_difficulty::discoverer;
  crosses_needed = 200;
  player.crosses = 510;
  player.old_world.immigration.num_recruits_rushed = 1;
  // Would be 7, but it bottoms out at the minimum=10.
  REQUIRE( f() == 10 );
}

TEST_CASE( "[immigration] rush_recruit_next_immigrant" ) {
  World   W;
  Player& player = W.default_player();

  W.settings().difficulty = e_difficulty::conquistador;
  // Calculated theoretically by computing all of the weights for
  // all unit types on the conquistador difficulty level and sum-
  // ming them, then truncating, since this needs to be slightly
  // smaller than the result if it is not equal so that d does
  // not exceed it.
  double const kUpperLimit = 6808.69;
  // The 2229.0 should just barely put us in the range of the
  // free colonist.
  EXPECT_CALL( W.rand(),
               between_doubles( 0, Approx( kUpperLimit, .1 ) ) )
      .returns( 2229.0 );

  auto& pool   = player.old_world.immigration.immigrants_pool;
  pool[0]      = e_unit_type::veteran_soldier;
  pool[1]      = e_unit_type::pioneer;
  pool[2]      = e_unit_type::petty_criminal;
  player.money = 1000;
  REQUIRE( compute_crosses( W.units(), player.nation )
               .crosses_needed == 8 ); // sanity check.
  player.crosses                                   = 5;
  player.old_world.immigration.num_recruits_rushed = 3;

  REQUIRE( W.ss().units.all().size() == 0 );

  rush_recruit_next_immigrant( W.ss(), W.ts(), player,
                               /*slot_selected=*/1 );

  REQUIRE( player.crosses == 0 );
  REQUIRE( player.old_world.immigration.num_recruits_rushed ==
           4 );
  REQUIRE( player.money == 1000 - 153 );

  REQUIRE( pool[0] == e_unit_type::veteran_soldier );
  REQUIRE( pool[1] == e_unit_type::free_colonist ); // replaced
  REQUIRE( pool[2] == e_unit_type::petty_criminal );

  // Make sure that the unit was created on the dock.
  REQUIRE( W.ss().units.all().size() == 1 );
  REQUIRE( W.units().all().begin()->first == UnitId{ 1 } );
  REQUIRE( W.units().all().begin()->second.unit.type() ==
           e_unit_type::pioneer );
  UnitOwnership_t const expected_ownership{
      UnitOwnership::harbor{
          .st = UnitHarborViewState{
              .port_status = PortStatus::in_port{} } } };
  REQUIRE( W.units().all().begin()->second.ownership ==
           expected_ownership );
}

} // namespace
} // namespace rn
