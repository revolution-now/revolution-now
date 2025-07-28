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
#include "test/mocks/iagent.hpp"
#include "test/util/coro.hpp"

// ss
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
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::french );
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
      w.add_colony( { .x = 0, .y = 0 }, e_player::english );
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
      w.add_colony( { .x = 1, .y = 0 }, e_player::french );
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
      w.add_colony( { .x = 0, .y = 1 }, e_player::french );
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
                     e_player::english );
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
  int next = 0;

  auto const f = [&] {
    return should_show_rebel_sentiment_report(
        w.ss(), w.default_player(), next );
  };

  Player& player = w.default_player();
  int& last = player.revolution.last_reported_rebel_sentiment;

  // Default
  next = 0;
  last = 0;
  REQUIRE_FALSE( f() );

  // Add a delta.
  next = 10;
  last = 0;
  REQUIRE_FALSE( f() );

  // Add colony.
  Colony const& colony = w.add_colony( { .x = 0, .y = 0 } );
  next                 = 10;
  last                 = 0;
  REQUIRE_FALSE( f() );

  // Add some units, just below required number.
  w.add_unit_indoors( colony.id, e_indoor_job::bells );
  tie( last, next ) = pair{ 0, 10 };
  REQUIRE_FALSE( f() );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  tie( last, next ) = pair{ 0, 10 };
  REQUIRE_FALSE( f() );
  w.add_unit_in_port( e_unit_type::free_colonist );
  tie( last, next ) = pair{ 0, 10 };
  REQUIRE_FALSE( f() );
  // Now we have 4, the required number.
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  tie( last, next ) = pair{ 0, 10 };
  REQUIRE( f() );

  tie( last, next ) = pair{ 0, 9 };
  REQUIRE_FALSE( f() );
  tie( last, next ) = pair{ 0, 0 };
  REQUIRE_FALSE( f() );
  tie( last, next ) = pair{ 0, 10 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 0, 20 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 10, 19 };
  REQUIRE_FALSE( f() );
  tie( last, next ) = pair{ 10, 20 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 11, 20 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 10, 21 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 90, 99 };
  REQUIRE_FALSE( f() );
  tie( last, next ) = pair{ 90, 100 };
  REQUIRE( f() );

  // Decreasing.
  tie( last, next ) = pair{ 100, 96 };
  REQUIRE_FALSE( f() );
  tie( last, next ) = pair{ 100, 95 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 60, 59 };
  REQUIRE_FALSE( f() );
  tie( last, next ) = pair{ 60, 56 };
  REQUIRE_FALSE( f() );
  tie( last, next ) = pair{ 60, 55 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 61, 55 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 61, 56 };
  REQUIRE( f() );
  tie( last, next ) = pair{ 61, 57 };
  REQUIRE_FALSE( f() );

  tie( last, next ) = pair{ 0, 10 };

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

  auto& agent          = w.agent( e_player::french );
  Player const& player = w.french();

  auto const f = [&] [[clang::noinline]] {
    co_await_test( show_rebel_sentiment_change_report(
        w.french(), w.agent( e_player::french ), report ) );
  };

  REQUIRE( player.revolution.last_reported_rebel_sentiment ==
           0 );

  // Default.
  f();
  REQUIRE( player.revolution.last_reported_rebel_sentiment ==
           0 );

  // Default.
  report.nova = 1;
  agent.EXPECT__message_box(
      "[Rebel] sentiment is on the rise, Your Excellency!  [1%] "
      "of the population now supports the idea of independence "
      "from France." );
  agent.EXPECT__handle(
      signal::RebelSentimentChanged{ .old = 0, .nu = 1 } );
  f();
  REQUIRE( player.revolution.last_reported_rebel_sentiment ==
           1 );

  report.prev = 1;
  report.nova = 2;
  agent.EXPECT__message_box(
      "[Rebel] sentiment is on the rise, Your Excellency!  [2%] "
      "of the population now supports the idea of independence "
      "from France." );
  agent.EXPECT__handle(
      signal::RebelSentimentChanged{ .old = 1, .nu = 2 } );
  f();
  REQUIRE( player.revolution.last_reported_rebel_sentiment ==
           2 );

  report.prev = 0;
  report.nova = 100;
  agent.EXPECT__message_box(
      "[Rebel] sentiment is on the rise, Your Excellency!  "
      "[100%] of the population now supports the idea of "
      "independence from France." );
  agent.EXPECT__handle(
      signal::RebelSentimentChanged{ .old = 0, .nu = 100 } );
  f();
  REQUIRE( player.revolution.last_reported_rebel_sentiment ==
           100 );

  report.prev = 100;
  report.nova = 99;
  agent.EXPECT__message_box(
      "[Tory] sentiment is on the rise, Your Excellency.  Only "
      "[99%] of the population now supports the idea of "
      "independence from France." );
  agent.EXPECT__handle(
      signal::RebelSentimentChanged{ .old = 100, .nu = 99 } );
  f();
  REQUIRE( player.revolution.last_reported_rebel_sentiment ==
           99 );

  report.prev = 20;
  report.nova = 0;
  agent.EXPECT__message_box(
      "[Tory] sentiment is on the rise, Your Excellency.  None "
      "of the population supports the idea of independence from "
      "France." );
  agent.EXPECT__handle(
      signal::RebelSentimentChanged{ .old = 20, .nu = 0 } );
  f();
  REQUIRE( player.revolution.last_reported_rebel_sentiment ==
           0 );
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

  auto const f = [&] {
    return required_rebel_sentiment_for_declaration( w.ss() );
  };

  w.settings().game_setup_options.difficulty =
      e_difficulty::discoverer;
  REQUIRE( f() == 50 );

  w.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;
  REQUIRE( f() == 50 );

  w.settings().game_setup_options.difficulty =
      e_difficulty::viceroy;
  REQUIRE( f() == 50 );
}

TEST_CASE( "[rebel-sentiment] unit_count_for_rebel_sentiment" ) {
  world w;

  auto const f = [&]( e_player const player ) {
    return unit_count_for_rebel_sentiment( w.ss(), player );
  };

  using enum e_player;

  // Default.
  REQUIRE( f( english ) == 0 );
  REQUIRE( f( french ) == 0 );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 }, e_player::french );
  REQUIRE( f( english ) == 0 );
  REQUIRE( f( french ) == 1 );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 0 }, e_player::english );
  REQUIRE( f( english ) == 1 );
  REQUIRE( f( french ) == 1 );

  w.add_unit_on_map( e_unit_type::artillery, { .x = 0, .y = 0 },
                     e_player::french );
  REQUIRE( f( english ) == 1 );
  REQUIRE( f( french ) == 1 );
}

} // namespace
} // namespace rn
