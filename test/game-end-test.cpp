/****************************************************************
**game-end-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-17.
*
* Description: Unit tests for the game-end module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/game-end.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/plane-stack.hpp"

// ss
#include "src/ss/events.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/turn.rds.hpp"

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
    add_all_non_ref_players( /*human=*/e_player::french );
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, _, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[game-end] check_time_up" ) {
  world w;

  auto const f = [&] [[clang::noinline]] {
    return check_time_up( w.ss() );
  };

  // Default.
  REQUIRE( f() == false );

  // Make true.
  w.turn().time_point.year   = 1850;
  w.turn().time_point.season = e_season::autumn;
  REQUIRE( f() == true );

  SECTION( "already won" ) {
    w.french().revolution.status = e_revolution_status::won;
    REQUIRE( f() == false );
  }

  SECTION( "wrong season" ) {
    w.turn().time_point.season = e_season::spring;
    REQUIRE( f() == false );
  }

  SECTION( "wrong year (before)" ) {
    w.turn().time_point.year = 1849;
    REQUIRE( f() == false );
  }

  SECTION( "wrong year (after)" ) {
    w.turn().time_point.year = 1851;
    REQUIRE( f() == false );
  }

  SECTION( "no human player" ) {
    w.french().control = e_player_control::ai;
    REQUIRE( f() == false );
  }

  SECTION( "no time limit" ) {
    w.settings()
        .game_setup_options.customized_rules
        .deadline_for_winning = nothing;
    REQUIRE( f() == false );
  }

  SECTION( "no changes (sanity check)" ) {
    REQUIRE( f() == true );
  }
}

TEST_CASE( "[game-end] do_time_up" ) {
  world w;

  w.french().name   = "My Name";
  w.dutch().control = e_player_control::human;

  auto const f = [&] [[clang::noinline]] {
    return co_await_test( do_time_up( w.ss(), w.gui() ) );
  };

  using enum e_game_end;
  using enum e_revolution_status;
  using enum e_difficulty;

  w.turn().time_point.year = 1850;

  w.english().revolution.status = not_declared;
  w.french().revolution.status  = not_declared;
  w.spanish().revolution.status = not_declared;
  w.dutch().revolution.status   = not_declared;
  w.gui().EXPECT__message_box(
      "Discoverer My Name resigns after 358 years of dedicated "
      "service to the Crown." );
  w.gui().EXPECT__choice( _ ).returns( "yes" );
  REQUIRE( f() == ended_and_player_continues );
  REQUIRE( w.english().revolution.status == not_declared );
  REQUIRE( w.french().revolution.status == lost );
  REQUIRE( w.spanish().revolution.status == not_declared );
  REQUIRE( w.dutch().revolution.status == lost );

  w.english().revolution.status              = not_declared;
  w.french().revolution.status               = not_declared;
  w.spanish().revolution.status              = not_declared;
  w.dutch().revolution.status                = not_declared;
  w.settings().game_setup_options.difficulty = explorer;
  w.gui().EXPECT__message_box(
      "Explorer My Name resigns after 358 years of dedicated "
      "service to the Crown." );
  w.gui().EXPECT__choice( _ ).returns( "no" );
  REQUIRE( f() == ended_and_back_to_main_menu );
  REQUIRE( w.english().revolution.status == not_declared );
  REQUIRE( w.french().revolution.status == lost );
  REQUIRE( w.spanish().revolution.status == not_declared );
  REQUIRE( w.dutch().revolution.status == lost );

  w.english().revolution.status              = not_declared;
  w.french().revolution.status               = declared;
  w.spanish().revolution.status              = not_declared;
  w.dutch().revolution.status                = not_declared;
  w.settings().game_setup_options.difficulty = conquistador;
  w.gui().EXPECT__message_box(
      "Conquistador My Name surrenders to the Crown." );
  w.gui().EXPECT__choice( _ ).returns( "yes" );
  REQUIRE( f() == ended_and_player_continues );
  REQUIRE( w.english().revolution.status == not_declared );
  REQUIRE( w.french().revolution.status == lost );
  REQUIRE( w.spanish().revolution.status == not_declared );
  REQUIRE( w.dutch().revolution.status == lost );

  w.english().revolution.status = not_declared;
  w.french().revolution.status  = not_declared;
  w.spanish().revolution.status = not_declared;
  w.dutch().revolution.status   = not_declared;
  w.french().control            = e_player_control::ai;
  w.dutch().control             = e_player_control::ai;
  REQUIRE( f() == not_ended );
  REQUIRE( w.english().revolution.status == not_declared );
  REQUIRE( w.french().revolution.status == not_declared );
  REQUIRE( w.spanish().revolution.status == not_declared );
  REQUIRE( w.dutch().revolution.status == not_declared );
}

// This method is composed of other methods that themselves are
// tested already, so we're going to go light here.
TEST_CASE( "[game-end] check_for_ref_win" ) {
  world w;
  w.update_terrain_connectivity();

  Player const& ref_player =
      w.add_player( e_player::ref_french );
  Player& colonial_player = w.french();

  auto const f = [&] [[clang::noinline]] {
    return co_await_test( check_for_ref_win(
        w.ss(), w.ts(), as_const( ref_player ) ) );
  };

  using enum e_game_end;
  using enum e_revolution_status;

  colonial_player.revolution.status = declared;

  w.gui().EXPECT__message_box( _ ).returns();
  w.gui().EXPECT__message_box( _ ).returns();
  REQUIRE( f() == ended_and_back_to_main_menu );
  REQUIRE( colonial_player.revolution.status == lost );

  colonial_player.revolution.status = declared;
  Colony const& colony =
      w.add_colony( { .x = 2, .y = 2 }, e_player::french );
  w.add_unit_indoors( colony.id, e_indoor_job::bells );
  REQUIRE( f() == not_ended );
  REQUIRE( colonial_player.revolution.status == declared );

  colonial_player.revolution.status = won;
  REQUIRE( f() == not_ended );
  REQUIRE( colonial_player.revolution.status == won );
}

// This method is composed of other methods that themselves are
// tested already, so we're going to go light here.
TEST_CASE( "[game-end] check_for_ref_forfeight" ) {
  world w;
  MockLandViewPlane land_view_plane;
  w.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& ref_player      = w.add_player( e_player::ref_french );
  Player& colonial_player = w.french();
  Player& other_player    = w.english();

  auto const f = [&] [[clang::noinline]] {
    return co_await_test(
        check_for_ref_forfeight( w.ss(), w.ts(), ref_player ) );
  };

  using enum e_game_end;
  using enum e_revolution_status;
  using enum e_player_control;

  other_player.control              = inactive;
  colonial_player.revolution.status = declared;
  colonial_player.revolution.expeditionary_force.regular   = 0;
  colonial_player.revolution.expeditionary_force.man_o_war = 0;
  w.dutch().control = e_player_control::inactive;
  w.gui().EXPECT__message_box( _ );
  w.gui().EXPECT__message_box( _ );
  w.gui().EXPECT__choice( _ ).returns( "no" );
  REQUIRE( f() == ended_and_back_to_main_menu );
  REQUIRE( colonial_player.revolution.status == won );
  REQUIRE( ref_player.control == inactive );
  REQUIRE( colonial_player.control == human );
  REQUIRE( other_player.control == inactive );

  other_player.control              = inactive;
  colonial_player.revolution.status = declared;
  colonial_player.revolution.expeditionary_force.regular   = 1;
  colonial_player.revolution.expeditionary_force.man_o_war = 1;
  w.dutch().control = e_player_control::inactive;
  REQUIRE( f() == not_ended );
  REQUIRE( colonial_player.revolution.status == declared );
  REQUIRE( ref_player.control == inactive );
  REQUIRE( colonial_player.control == human );
  REQUIRE( other_player.control == inactive );

  other_player.control              = inactive;
  colonial_player.revolution.status = declared;
  colonial_player.revolution.expeditionary_force.regular   = 0;
  colonial_player.revolution.expeditionary_force.man_o_war = 0;
  w.gui().EXPECT__message_box( _ );
  w.gui().EXPECT__message_box( _ );
  w.gui().EXPECT__choice( _ ).returns( "yes" );
  land_view_plane.EXPECT__set_visibility( nothing );
  REQUIRE( f() == ended_and_player_continues );
  REQUIRE( colonial_player.revolution.status == won );
  REQUIRE( ref_player.control == inactive );
  REQUIRE( colonial_player.control == human );
  REQUIRE( other_player.control == ai );

  other_player.control              = inactive;
  colonial_player.revolution.status = declared;
  colonial_player.revolution.expeditionary_force.regular   = 0;
  colonial_player.revolution.expeditionary_force.man_o_war = 0;
  w.events().war_of_succession_done = other_player.nation;
  w.gui().EXPECT__message_box( _ );
  w.gui().EXPECT__message_box( _ );
  w.gui().EXPECT__choice( _ ).returns( "yes" );
  land_view_plane.EXPECT__set_visibility( nothing );
  REQUIRE( f() == ended_and_player_continues );
  REQUIRE( colonial_player.revolution.status == won );
  REQUIRE( ref_player.control == inactive );
  REQUIRE( colonial_player.control == human );
  REQUIRE( other_player.control == inactive );

  other_player.control              = inactive;
  colonial_player.revolution.status = won;
  colonial_player.revolution.expeditionary_force.regular   = 0;
  colonial_player.revolution.expeditionary_force.man_o_war = 0;
  REQUIRE( f() == not_ended );
  REQUIRE( colonial_player.revolution.status == won );
  REQUIRE( ref_player.control == inactive );
  REQUIRE( colonial_player.control == human );
  REQUIRE( other_player.control == inactive );
}

} // namespace
} // namespace rn
