/****************************************************************
**declare-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-13.
*
* Description: Unit tests for the declare module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/declare.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
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
TEST_CASE( "[declare] can_declare_independence" ) {
  world w;

  Player& player = w.default_player();

  auto const f = [&] {
    return can_declare_independence( w.ss().as_const,
                                     as_const( player ) );
  };

  w.settings().game_setup_options.difficulty =
      e_difficulty::discoverer;

  using enum e_declare_rejection;

  // Default.
  REQUIRE( f() == rebel_sentiment_too_low );

  SECTION( "rebel sentiment" ) {
    player.revolution.rebel_sentiment = 0;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 10;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 30;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 49;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 50;
    REQUIRE( f() == valid );

    player.revolution.rebel_sentiment = 60;
    REQUIRE( f() == valid );

    player.revolution.rebel_sentiment = 99;
    REQUIRE( f() == valid );

    player.revolution.rebel_sentiment = 100;
    REQUIRE( f() == valid );

    w.settings().game_setup_options.difficulty =
        e_difficulty::viceroy;

    player.revolution.rebel_sentiment = 0;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 10;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 30;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 49;
    REQUIRE( f() == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 50;
    REQUIRE( f() == valid );

    player.revolution.rebel_sentiment = 60;
    REQUIRE( f() == valid );

    player.revolution.rebel_sentiment = 99;
    REQUIRE( f() == valid );

    player.revolution.rebel_sentiment = 100;
    REQUIRE( f() == valid );
  }

  SECTION( "Foreign Nation" ) {
  }

  SECTION( "Already Declared" ) {
  }

  SECTION( "Already Won" ) {
  }
}

TEST_CASE( "[declare] show_declare_rejection_msg" ) {
  world w;
}

TEST_CASE( "[declare] ask_declare" ) {
  world w;
}

TEST_CASE( "[declare] declare_independence_ui_sequence_pre" ) {
  world w;
}

TEST_CASE( "[declare] declare_independence" ) {
  world w;
}

TEST_CASE( "[declare] declare_independence_ui_sequence_post" ) {
  world w;
}

} // namespace
} // namespace rn
