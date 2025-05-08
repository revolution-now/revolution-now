/****************************************************************
**rebel-sentiment-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-06.
*
* Description: Unit tests for the rebel-sentiment module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rebel-sentiment.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/ieuro-mind.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/events.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::refl::enum_values;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_nation::english );
    add_player( e_nation::french );
    add_player( e_nation::spanish );
    add_player( e_nation::dutch );
    set_default_player( e_nation::french );
    set_default_player_as_human();
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
TEST_CASE( "[rebel-sentiment] updated_rebel_sentiment" ) {
  world w;

  auto const f = [&] {
    return updated_rebel_sentiment( w.ss().as_const,
                                    as_const( w.french() ) );
  };

  // Default case.
  REQUIRE( f() == 0 );

  // One foreign colony, empty.
  Colony& foreign_colony_1 =
      w.add_colony( { .x = 0, .y = 0 }, e_nation::english );
  REQUIRE( f() == 0 );

  // With some non-rebels.
  w.add_unit_indoors( foreign_colony_1.id, e_indoor_job::bells );
  w.add_unit_indoors( foreign_colony_1.id, e_indoor_job::bells );
  REQUIRE( f() == 0 );

  // With a rebel.
  foreign_colony_1.sons_of_liberty.num_rebels_from_bells_only =
      1;
  REQUIRE( f() == 0 );

  // With two rebels.
  foreign_colony_1.sons_of_liberty.num_rebels_from_bells_only =
      2;
  REQUIRE( f() == 0 );

  // One friendly colony, empty.
  Colony& colony_1 =
      w.add_colony( { .x = 1, .y = 0 }, e_nation::french );
  REQUIRE( f() == 0 );

  // With some non-rebels.
  w.add_unit_indoors( colony_1.id, e_indoor_job::bells );
  w.add_unit_indoors( colony_1.id, e_indoor_job::bells );
  REQUIRE( f() == 0 );

  // With a fractional rebel.
  colony_1.sons_of_liberty.num_rebels_from_bells_only = .5;
  REQUIRE( f() == 25 );

  // With a rebel.
  colony_1.sons_of_liberty.num_rebels_from_bells_only = 1;
  REQUIRE( f() == 50 );

  // With two rebels.
  colony_1.sons_of_liberty.num_rebels_from_bells_only = 2;
  REQUIRE( f() == 100 );

  // A second friendly colony, empty.
  Colony& colony_2 =
      w.add_colony( { .x = 0, .y = 1 }, e_nation::french );
  REQUIRE( f() == 100 );

  // With some non-rebels.
  w.add_unit_indoors( colony_2.id, e_indoor_job::bells );
  w.add_unit_indoors( colony_2.id, e_indoor_job::bells );
  REQUIRE( f() == 50 );

  // With a fractional rebel.
  colony_2.sons_of_liberty.num_rebels_from_bells_only = .5;
  REQUIRE( f() == 62 );

  // With a rebel.
  colony_2.sons_of_liberty.num_rebels_from_bells_only = 1;
  REQUIRE( f() == 75 );

  // With two rebels.
  colony_2.sons_of_liberty.num_rebels_from_bells_only = 2;
  REQUIRE( f() == 100 );

  // Back down.
  colony_2.sons_of_liberty.num_rebels_from_bells_only = 1;
  REQUIRE( f() == 75 );

  // Add some other units to make sure they don't affect the
  // rebel sentiment. NOTE: These units would affect the number
  // of rebels/tories in the Rebel Sentiment display on the Con-
  // tinental Congress report, but that is just projecting the
  // rebel sentiment onto the entire population; the rebel senti-
  // ment itself is calculated in a way that ignores them.
  w.add_unit_on_map( e_unit_type::free_colonist,
                     colony_1.location );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     colony_2.location );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     foreign_colony_1.location,
                     e_nation::english );
  w.add_unit_in_port( e_unit_type::free_colonist );
  REQUIRE( f() == 75 );
}

TEST_CASE( "[rebel-sentiment] update_rebel_sentiment" ) {
  world w;
  int updated = {};
  RebelSentimentChangeReport expected;

  auto const f = [&] {
    return update_rebel_sentiment( w.default_player(), updated );
  };

  Player& player = w.default_player();

  int const& rebel_sentiment = player.revolution.rebel_sentiment;

  // Default.
  updated  = 0;
  expected = {};
  REQUIRE( f() == expected );
  REQUIRE( rebel_sentiment == 0 );

  updated  = 10;
  expected = { .prev = 0, .nova = 10 };
  REQUIRE( f() == expected );
  REQUIRE( rebel_sentiment == 10 );

  updated  = 35;
  expected = { .prev = 10, .nova = 35 };
  REQUIRE( f() == expected );
  REQUIRE( rebel_sentiment == 35 );

  updated  = 100;
  expected = { .prev = 35, .nova = 100 };
  REQUIRE( f() == expected );
  REQUIRE( rebel_sentiment == 100 );

  updated  = 0;
  expected = { .prev = 100, .nova = 0 };
  REQUIRE( f() == expected );
  REQUIRE( rebel_sentiment == 0 );
}

TEST_CASE(
    "[rebel-sentiment] should_show_rebel_sentiment_report" ) {
  world w;
  RebelSentimentChangeReport report;

  auto const f = [&] {
    return should_show_rebel_sentiment_report(
        w.ss(), w.default_player(), report );
  };

  Player& player = w.default_player();

  // Default
  report = {};
  REQUIRE_FALSE( f() );

  // Add a delta.
  report = { .nova = 10 };
  REQUIRE_FALSE( f() );

  // Add colony.
  Colony const& colony = w.add_colony( { .x = 0, .y = 0 } );
  report               = { .nova = 10 };
  REQUIRE_FALSE( f() );

  // Add some units, just below required number.
  w.add_unit_indoors( colony.id, e_indoor_job::bells );
  report = { .nova = 10 };
  REQUIRE_FALSE( f() );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  report = { .nova = 10 };
  REQUIRE_FALSE( f() );
  w.add_unit_in_port( e_unit_type::free_colonist );
  report = { .nova = 10 };
  REQUIRE_FALSE( f() );
  // Now we have 4, the required number.
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  report = { .nova = 10 };
  REQUIRE( f() );

  report = { .nova = 9 };
  REQUIRE_FALSE( f() );
  report = { .nova = 0 };
  REQUIRE_FALSE( f() );
  report = { .nova = 10 };
  REQUIRE( f() );
  report = { .nova = 20 };
  REQUIRE( f() );

  player.revolution.status = e_revolution_status::not_declared;
  REQUIRE( f() );
  player.revolution.status = e_revolution_status::declared;
  REQUIRE_FALSE( f() );
  player.revolution.status = e_revolution_status::won;
  REQUIRE_FALSE( f() );
}

TEST_CASE(
    "[rebel-sentiment] show_rebel_sentiment_change_report" ) {
  world w;
  RebelSentimentChangeReport report;

  auto& mind = w.euro_mind( e_nation::french );

  auto const f = [&] {
    co_await_test( show_rebel_sentiment_change_report(
        w.euro_mind( e_nation::french ), report ) );
  };

  // Default.
  f();

  // Default.
  report.nova = 1;
  mind.EXPECT__message_box(
      "[Rebel] sentiment is on the rise, Your Excellency!  [1%] "
      "of the population now supports the idea of independence "
      "from France." );
  f();

  report.prev = 1;
  report.nova = 2;
  mind.EXPECT__message_box(
      "[Rebel] sentiment is on the rise, Your Excellency!  [2%] "
      "of the population now supports the idea of independence "
      "from France." );
  f();

  report.prev = 0;
  report.nova = 100;
  mind.EXPECT__message_box(
      "[Rebel] sentiment is on the rise, Your Excellency!  "
      "[100%] of the population now supports the idea of "
      "independence from France." );
  f();

  report.prev = 100;
  report.nova = 99;
  mind.EXPECT__message_box(
      "[Tory] sentiment is on the rise, Your Excellency.  Only "
      "[99%] of the population now supports the idea of "
      "independence from France." );
  f();

  report.prev = 20;
  report.nova = 0;
  mind.EXPECT__message_box(
      "[Tory] sentiment is on the rise, Your Excellency.  None "
      "of the population supports the idea of independence from "
      "France." );
  f();
}

TEST_CASE(
    "[rebel-sentiment] rebel_sentiment_report_for_cc_report" ) {
  world w;
  RebelSentimentReport expected;

  Player& player = w.default_player();

  auto const f = [&] {
    return rebel_sentiment_report_for_cc_report(
        w.ss().as_const, as_const( player ) );
  };

  // Default.
  expected = {};
  REQUIRE( f() == expected );

  player.revolution.rebel_sentiment = 30;
  expected                          = { .rebel_sentiment = 30 };
  REQUIRE( f() == expected );

  Colony const& colony = w.add_colony( { .x = 0, .y = 0 } );
  w.add_unit_indoors( colony.id, e_indoor_job::bells );
  expected = { .rebel_sentiment = 30, .tories = 1 };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  expected = { .rebel_sentiment = 30, .tories = 2 };
  REQUIRE( f() == expected );

  w.add_unit_in_port( e_unit_type::free_colonist );
  expected = { .rebel_sentiment = 30, .tories = 3 };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  expected = { .rebel_sentiment = 30, .rebels = 1, .tories = 3 };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  expected = { .rebel_sentiment = 30, .rebels = 2, .tories = 6 };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  expected = { .rebel_sentiment = 30, .rebels = 3, .tories = 9 };
  REQUIRE( f() == expected );

  player.revolution.rebel_sentiment = 0;

  expected = { .rebel_sentiment = 0, .rebels = 0, .tories = 12 };
  REQUIRE( f() == expected );

  player.revolution.rebel_sentiment = 100;

  expected = {
    .rebel_sentiment = 100, .rebels = 12, .tories = 0 };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[rebel-sentiment] "
    "required_rebel_sentiment_for_declaration" ) {
  world w;
}

TEST_CASE( "[rebel-sentiment] should_do_war_of_succession" ) {
  world w;

  Player& human_player = w.french();
  Player& other_player = w.english();

  auto const f = [&] {
    return should_do_war_of_succession(
        w.ss().as_const, as_const( human_player ) );
  };

  BASE_CHECK( human_player.human );

  // Default.
  REQUIRE_FALSE( f() );

  // Set up the conditions for a war of succession by default,
  // then test for each negative case.
  w.settings().game_setup_options.enable_war_of_succession =
      true;
  human_player.revolution.rebel_sentiment = 50;

  SECTION( "lacking rebel sentiment" ) {
    REQUIRE( f() );
    human_player.revolution.rebel_sentiment = 49;
    REQUIRE_FALSE( f() );
  }

  SECTION( "multiple humans" ) {
    REQUIRE( f() );
    w.spanish().human = true;
    REQUIRE_FALSE( f() );
  }

  SECTION( "no humans" ) {
    REQUIRE( f() );
    human_player.human = false;
    REQUIRE_FALSE( f() );
  }

  SECTION( "AI granted independence" ) {
    human_player.human = true;
    other_player.human = false;
    REQUIRE( f() );
    other_player.revolution.status = e_revolution_status::won;
    REQUIRE_FALSE( f() );
  }

  SECTION( "one human but not the caller's player" ) {
    REQUIRE( f() );
    human_player.human = false;
    w.spanish().human  = true;
    REQUIRE_FALSE( f() );
  }

  SECTION( "fewer than four players" ) {
    REQUIRE( f() );
    w.players().players[e_nation::spanish].reset();
    REQUIRE_FALSE( f() );
  }

  SECTION( "war of succession already done" ) {
    REQUIRE( f() );
    w.events().war_of_succession_done = true;
    REQUIRE_FALSE( f() );
  }

  SECTION( "war of succession not enabled" ) {
    REQUIRE( f() );
    w.settings().game_setup_options.enable_war_of_succession =
        false;
    REQUIRE_FALSE( f() );
  }
}

TEST_CASE(
    "[rebel-sentiment] select_nations_for_war_of_succession" ) {
  world w;
  WarOfSuccessionNations expected;

  auto const f = [&] {
    return select_nations_for_war_of_succession( w.ss() );
  };

  // Start off with no human players.
  for( e_nation const nation : enum_values<e_nation> )
    w.player( nation ).human = false;

  // No humans.
  expected = { .withdraws = e_nation::english,
               .receives  = e_nation::french };
  REQUIRE( f() == expected );

  w.english().human = true;
  expected          = { .withdraws = e_nation::french,
                        .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.english().human = false;
  w.french().human  = true;
  expected          = { .withdraws = e_nation::english,
                        .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.spanish().human = true;
  expected          = { .withdraws = e_nation::english,
                        .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.spanish().human = false;
  expected          = { .withdraws = e_nation::english,
                        .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::artillery, { .x = 0, .y = 0 },
                     e_nation::english );
  expected = { .withdraws = e_nation::english,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::dragoon, { .x = 0, .y = 0 },
                     e_nation::english );
  expected = { .withdraws = e_nation::spanish,
               .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::soldier, { .x = 1, .y = 0 },
                     e_nation::spanish );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::english };
  REQUIRE( f() == expected );

  w.add_unit_on_map( e_unit_type::native_convert,
                     { .x = 1, .y = 0 }, e_nation::english );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.french().human = false;
  expected         = { .withdraws = e_nation::french,
                       .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.english().human = true;
  expected          = { .withdraws = e_nation::french,
                        .receives  = e_nation::dutch };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 0, .y = 0 }, e_nation::french );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::french };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 2, .y = 0 }, e_nation::french );
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );

  w.players().players[e_nation::french].reset();
  expected = { .withdraws = e_nation::dutch,
               .receives  = e_nation::spanish };
  REQUIRE( f() == expected );
}

TEST_CASE( "[rebel-sentiment] war_of_succession_plan" ) {
  world w;
}

TEST_CASE( "[rebel-sentiment] do_war_of_succession" ) {
  world w;
}

TEST_CASE( "[rebel-sentiment] do_war_of_succession_ui_seq" ) {
  world w;
}

} // namespace
} // namespace rn
