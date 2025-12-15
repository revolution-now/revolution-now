/****************************************************************
**charter-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-14.
*
* Description: Unit tests for the charter module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/charter.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/turn.rds.hpp"

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
    using enum e_player;
    add_player( english );
    set_default_player_type( english );
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const S = make_ocean();
    static MapSquare const _ = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{/*
          0 1 2 3 4 5 6 7
      0*/ S,_,_,_,_,_,_,S, /*0
      1*/ S,_,_,_,_,_,_,S, /*1
      2*/ S,_,_,_,_,_,_,S, /*2
      3*/ S,_,_,_,_,_,_,S, /*3
      4*/ S,_,_,_,_,_,_,S, /*4
      5*/ S,_,_,_,_,_,_,S, /*5
      6*/ S,_,_,_,_,_,_,S, /*6
      7*/ S,_,_,_,_,_,_,S, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[charter] charter_end_year" ) {
  REQUIRE( charter_end_year() == 1600 );
}

TEST_CASE( "[charter] should_warn_about_charter_end" ) {
  world w;

  auto const f = [&] [[clang::noinline]] (
                     e_player const player_type =
                         e_player::english ) {
    return should_warn_about_charter_end( w.ss(), player_type );
  };

  w.turn().time_point.year = 0;
  REQUIRE_FALSE( f() );

  w.turn().time_point.year = 1574;
  REQUIRE_FALSE( f() );

  w.turn().time_point.year = 1575;
  REQUIRE( f() );

  w.turn().time_point.year = 1599;
  REQUIRE( f() );

  w.turn().time_point.year = 1600;
  REQUIRE( f() );

  w.turn().time_point.year = 1700;
  REQUIRE( f() );

  w.turn().time_point.year = 9999;
  REQUIRE( f() );

  w.add_player( e_player::french );
  w.french().control = e_player_control::human;

  w.turn().time_point.year = 9999;
  REQUIRE_FALSE( f() );

  w.french().control = e_player_control::inactive;
  REQUIRE( f() );

  REQUIRE_FALSE( f( e_player::french ) );

  w.english().revolution.status = e_revolution_status::declared;
  REQUIRE_FALSE( f() );

  w.english().revolution.status =
      e_revolution_status::not_declared;
  REQUIRE( f() );

  w.add_player( e_player::ref_english );
  w.ref_english().control = e_player_control::human;
  w.english().control     = e_player_control::inactive;
  REQUIRE_FALSE( f( e_player::ref_english ) );

  w.ref_english().control = e_player_control::inactive;
  w.english().control     = e_player_control::human;
  REQUIRE( f() );
}

TEST_CASE( "[charter] should_end_charter" ) {
  world w;

  auto const f =
      [&] [[clang::noinline]] ( e_player const player_type =
                                    e_player::english ) {
        return should_end_charter( w.ss(), player_type );
      };

  w.add_player( e_player::french );

  SECTION( "no colonies" ) {
    w.turn().time_point.year = 0;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1599;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1600;
    REQUIRE( f() );

    w.turn().time_point.year = 1700;
    REQUIRE( f() );

    w.turn().time_point.year = 1800;
    REQUIRE( f() );
  }

  SECTION( "no colonies + two humans" ) {
    w.french().control = e_player_control::human;

    w.turn().time_point.year = 0;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1599;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1600;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1700;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1800;
    REQUIRE_FALSE( f() );
  }

  SECTION( "with colonies" ) {
    w.add_colony( { .x = 1, .y = 1 }, e_player::french );

    w.turn().time_point.year = 0;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1599;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1600;
    REQUIRE( f() );

    w.turn().time_point.year = 1700;
    REQUIRE( f() );

    w.turn().time_point.year = 1800;
    REQUIRE( f() );
  }

  SECTION( "with colonies" ) {
    w.add_colony( { .x = 1, .y = 1 } );

    w.turn().time_point.year = 0;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1599;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1600;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1700;
    REQUIRE_FALSE( f() );

    w.turn().time_point.year = 1800;
    REQUIRE_FALSE( f() );
  }
}

TEST_CASE( "[charter] end_charter_ui_seq" ) {
  world w;

  auto const f = [&] [[clang::noinline]] {
    co_await_test( end_charter_ui_seq(
        w.ss(), w.gui(), w.default_player_type() ) );
  };

  w.settings().game_setup_options.difficulty =
      e_difficulty::explorer;
  w.default_player().name = "Some Name";

  w.gui().EXPECT__message_box(
      "Explorer Some Name. Our time in the New World has "
      "achieved little of consequence. We will be promptly "
      "ending your tenure as Viceroy in favour of someone more "
      "capable. It is, though, still your privilege to kiss our "
      "royal pinky ring." );
  f();
}

} // namespace
} // namespace rn
