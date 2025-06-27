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
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

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
using ::mock::matchers::_;
using ::mock::matchers::StrContains;

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
TEST_CASE( "[declare] human_player_that_declared" ) {
  world w;
}

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

  SECTION( "Other Human Already Declared" ) {
  }

  SECTION( "REF cannot declare" ) {
  }

  SECTION( "Already Won" ) {
  }
}

TEST_CASE( "[declare] show_declare_rejection_msg" ) {
  world w;
}

TEST_CASE( "[declare] ask_declare" ) {
  world w;

  w.add_player( e_player::french );

  auto const f = [&] {
    return co_await_test( ask_declare( w.gui(), w.french() ) );
  };

  w.gui().EXPECT__choice( _ ).returns( nothing );
  REQUIRE( f() == ui::e_confirm::no );

  w.gui().EXPECT__choice( _ ).returns( "no" );
  REQUIRE( f() == ui::e_confirm::no );

  w.gui().EXPECT__choice( _ ).returns( "yes" );
  REQUIRE( f() == ui::e_confirm::yes );
}

TEST_CASE( "[declare] declare_independence_ui_sequence_pre" ) {
  world w;
  Player const& player = w.default_player();

  auto const f = [&] {
    co_await_test( declare_independence_ui_sequence_pre(
        w.ss(), w.ts(), player ) );
  };

  auto const expect_msg = [&]( string const& msg ) {
    w.gui().EXPECT__message_box( StrContains( msg ) );
  };

  expect_msg( "(signing of signature on declaration)" );

  f();
}

TEST_CASE( "[declare] declare_independence" ) {
  world w;
}

TEST_CASE( "[declare] declare_independence_ui_sequence_post" ) {
  world w;
  DeclarationResult decl_res;

  Player const& player = w.default_player();

  auto const f = [&] {
    co_await_test( declare_independence_ui_sequence_post(
        w.ss(), w.ts(), player, decl_res ) );
  };

  auto const expect_msg = [&]( string const& msg ) {
    w.gui().EXPECT__message_box( StrContains( msg ) );
  };

  using enum e_unit_type;

  decl_res = { .seized_ships     = { { privateer, 2 },
                                     { merchantman, 3 },
                                     { galleon, 1 } },
               .offboarded_units = true };

  expect_msg( "signs [Declaration of Independence]" );
  expect_msg( "seized [three Merchantmen]" );
  expect_msg( "seized [one Galleon]" );
  expect_msg( "seized [two Privateers]" );
  expect_msg( "ships in our colonies have offboarded" );

  f();
}

TEST_CASE( "[declare] post_declaration_turn" ) {
  world w;

  Player& player = w.default_player();

  auto const f = [&] { return post_declaration_turn( player ); };

  using enum e_turn_after_declaration;
  using enum e_revolution_status;

  REQUIRE( f() == zero );
  REQUIRE( f() == zero );

  player.revolution.status = declared;
  REQUIRE( f() == one );
  REQUIRE( f() == one );

  player.revolution.continental_army_mobilized = true;
  REQUIRE( f() == two );
  REQUIRE( f() == two );

  player.revolution.gave_independence_war_hints = true;
  REQUIRE( f() == done );
  REQUIRE( f() == done );

  player.revolution.continental_army_mobilized = false;
  REQUIRE( f() == one );
  REQUIRE( f() == one );

  player.revolution.status = not_declared;
  REQUIRE( f() == zero );
  REQUIRE( f() == zero );
}

} // namespace
} // namespace rn
