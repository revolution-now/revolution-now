/****************************************************************
**tax.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Unit tests for the src/tax.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/tax.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/connectivity.hpp"
#include "src/imap-updater.hpp"

// config
#include "src/config/old-world.rds.hpp"

// ss
#include "src/ss/old-world-state.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/turn.rds.hpp"

// base
#include "src/base/no-discard.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      _, _, _, //
      _, L, L, //
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
TEST_CASE( "[tax] try_trade_boycotted_commodity" ) {
  world W;
  Player& player   = W.default_player();
  e_commodity type = {};
  wait<> w         = make_wait<>();

  W.old_world( player ).taxes.tax_rate = 7;

  auto const f = [&] [[clang::noinline]] {
    W.old_world( player ).market.commodities[type].boycott =
        true;
    auto const w = try_trade_boycotted_commodity(
        W.ss(), W.gui(), player, type,
        /*back_taxes=*/33 );
    BASE_CHECK( !w.exception() );
    BASE_CHECK( w.ready() );
    return *w;
  };

  SECTION( "food, not enough money" ) {
    type         = e_commodity::food;
    player.money = 32;

    string const expected_msg =
        "[Food] is currently under Parliamentary boycott. "
        "We cannot trade food until Parliament lifts the "
        "boycott, which it will not do unless we agree to pay "
        "[33] in back taxes. Treasury: 32.";
    W.gui()
        .EXPECT__message_box( expected_msg )
        .returns( make_wait<>() );

    REQUIRE( f() );
    REQUIRE( player.money == 32 );
    REQUIRE(
        W.old_world( player ).market.commodities[type].boycott );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 7 );
  }

  SECTION( "muskets, enough money, cancels" ) {
    type         = e_commodity::muskets;
    player.money = 33;

    string const expected_msg =
        "[Muskets] are currently under Parliamentary "
        "boycott. We cannot trade muskets until Parliament "
        "lifts the boycott, which it will not do unless we "
        "agree to pay [33] in back taxes.";
    ChoiceConfig const config{
      .msg     = expected_msg,
      .options = {
        ChoiceConfigOption{
          .key = "no",
          .display_name =
              "This is taxation without representation!",
        },
        ChoiceConfigOption{
          .key          = "yes",
          .display_name = "Pay [33].",
        } } };
    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( nothing ) );

    REQUIRE( f() );
    REQUIRE( player.money == 33 );
    REQUIRE(
        W.old_world( player ).market.commodities[type].boycott );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 7 );
  }

  SECTION( "muskets, enough money, selects no" ) {
    type         = e_commodity::muskets;
    player.money = 33;

    string const expected_msg =
        "[Muskets] are currently under Parliamentary "
        "boycott. We cannot trade muskets until Parliament "
        "lifts the boycott, which it will not do unless we "
        "agree to pay [33] in back taxes.";
    ChoiceConfig const config{
      .msg     = expected_msg,
      .options = {
        ChoiceConfigOption{
          .key = "no",
          .display_name =
              "This is taxation without representation!",
        },
        ChoiceConfigOption{
          .key          = "yes",
          .display_name = "Pay [33].",
        } } };
    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "no" ) );

    REQUIRE( f() );
    REQUIRE( player.money == 33 );
    REQUIRE(
        W.old_world( player ).market.commodities[type].boycott );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 7 );
  }

  SECTION( "tools, enough money, selects yes" ) {
    type         = e_commodity::tools;
    player.money = 34;

    string const expected_msg =
        "[Tools] are currently under Parliamentary "
        "boycott. We cannot trade tools until Parliament "
        "lifts the boycott, which it will not do unless we "
        "agree to pay [33] in back taxes.";
    ChoiceConfig const config{
      .msg     = expected_msg,
      .options = {
        ChoiceConfigOption{
          .key = "no",
          .display_name =
              "This is taxation without representation!",
        },
        ChoiceConfigOption{
          .key          = "yes",
          .display_name = "Pay [33].",
        } } };
    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "yes" ) );

    REQUIRE_FALSE( f() );
    REQUIRE( player.money == 1 );
    REQUIRE( !W.old_world( player )
                  .market.commodities[type]
                  .boycott );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 7 );
  }
}

TEST_CASE( "[tax] back_tax_for_boycotted_commodity" ) {
  world W;
  Player& player   = W.default_player();
  e_commodity type = {};
  int expected     = 0;

  W.old_world( player ).taxes.tax_rate = 7;

  auto const f = [&] [[clang::noinline]] {
    return back_tax_for_boycotted_commodity(
        W.ss().as_const, as_const( player ), type );
  };

  type = e_commodity::ore;
  W.old_world( player ).market.commodities[type].bid_price = 5;
  expected = 4000;
  REQUIRE( f() == expected );
  REQUIRE( W.old_world( player ).taxes.tax_rate == 7 );

  type = e_commodity::silver;
  W.old_world( player ).market.commodities[type].bid_price = 19;
  expected = 10000;
  REQUIRE( f() == expected );
  REQUIRE( W.old_world( player ).taxes.tax_rate == 7 );

  type = e_commodity::rum;
  W.old_world( player ).market.commodities[type].bid_price = 2;
  expected = 1500;
  REQUIRE( f() == expected );
  REQUIRE( W.old_world( player ).taxes.tax_rate == 7 );
}

TEST_CASE( "[tax] apply_tax_result" ) {
  world W;
  Player& player          = W.default_player();
  int next_tax_event_turn = 0;
  TaxChangeResult change;

  W.turn().time_point.turns            = 5;
  W.old_world( player ).taxes.tax_rate = 50;

  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{} );

  int bid = 1;
  for( e_commodity type : refl::enum_values<e_commodity> ) {
    W.old_world( player ).market.commodities[type].bid_price =
        bid++;
    colony.commodities[type] = bid * 10;
  }
  colony.sons_of_liberty.num_rebels_from_bells_only = 1.0;

  Colony colony_saved            = colony;
  PlayerMarketState market_saved = W.old_world( player ).market;

  auto const f = [&] [[clang::noinline]] {
    apply_tax_result( W.ss(), player, next_tax_event_turn,
                      change );
  };

  next_tax_event_turn = 6;

  SECTION( "none" ) {
    change = TaxChangeResult::none{};
    f();
    REQUIRE( W.old_world( player ).taxes.tax_rate == 50 );
    REQUIRE( colony == colony_saved );
    REQUIRE( W.old_world( player ).market == market_saved );
  }

  SECTION( "tax increase" ) {
    change = TaxChangeResult::tax_change{ .amount = 5 };
    f();
    REQUIRE( W.old_world( player ).taxes.tax_rate == 55 );
    REQUIRE( colony == colony_saved );
    REQUIRE( W.old_world( player ).market == market_saved );
  }

  SECTION( "tax decrease" ) {
    change = TaxChangeResult::tax_change{ .amount = -5 };
    f();
    REQUIRE( W.old_world( player ).taxes.tax_rate == 45 );
    REQUIRE( colony == colony_saved );
    REQUIRE( W.old_world( player ).market == market_saved );
  }

  SECTION( "party" ) {
    change = TaxChangeResult::party{
      .how = TeaParty{
        .commodity =
            CommodityInColony{
              .colony_id = 1,
              .type_and_quantity =
                  Commodity{ .type     = e_commodity::ore,
                             .quantity = 79 } },
        .rebels_bump = .5 } };
    f();
    REQUIRE( W.old_world( player ).taxes.tax_rate == 50 );
    colony_saved.commodities[e_commodity::ore] = 1;
    colony_saved.sons_of_liberty.num_rebels_from_bells_only =
        1.5;
    REQUIRE( colony == colony_saved );
    market_saved.commodities[e_commodity::ore].boycott = true;
    REQUIRE( W.old_world( player ).market == market_saved );
  }
}

TEST_CASE( "[tax] prompt_for_tax_change_result" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  world W;
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();
  TaxChangeProposal proposal;
  TaxChangeResult expected;

  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{} );
  colony.name = "my colony";

  auto const f = [&] [[clang::noinline]] {
    wait<TaxChangeResult> w = prompt_for_tax_change_result(
        W.ss(), W.rand(), player, W.agent(), proposal );
    CHECK( !w.exception() );
    CHECK( w.ready() );
    return *w;
  };

  W.old_world( player ).taxes.tax_rate = 50;

  SECTION( "decrease" ) {
    proposal = TaxChangeProposal::decrease{ .amount = 13 };
    expected = TaxChangeResult::tax_change{ .amount = -13 };
    string const expected_msg =
        "The crown has graciously decided to LOWER your tax "
        "rate by [13%].  The tax rate is now [37%].";
    agent.EXPECT__message_box( expected_msg )
        .returns( make_wait<>() );
    agent.EXPECT__handle(
        signal::TaxRateWillChange{ .delta = -13 } );
    REQUIRE( f() == expected );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 50 );
  }

  SECTION( "increase" ) {
    proposal = TaxChangeProposal::increase{ .amount = 13 };
    expected = TaxChangeResult::tax_change{ .amount = 13 };
    string const expected_msg =
        "In honor of our marriage to our 8th wife, we have "
        "decided to raise your tax rate by [13%].  The tax "
        "rate is now [63%]. We will graciously allow you "
        "to kiss our royal pinky ring.";
    W.rand().EXPECT__between_ints( 1, 3 ).returns( 2 );
    agent.EXPECT__handle(
        signal::TaxRateWillChange{ .delta = 13 } );
    agent.EXPECT__message_box( expected_msg );
    REQUIRE( f() == expected );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 50 );
  }

  SECTION( "increase_or_party, chooses yes" ) {
    TeaParty const party = TeaParty{
      .commodity =
          CommodityInColony{
            .colony_id = 1,
            .type_and_quantity =
                Commodity{ .type     = e_commodity::cigars,
                           .quantity = 23 } },
      .rebels_bump = 0.5 };
    proposal = TaxChangeProposal::increase_or_party{
      .amount = 13, .party = party };
    expected = TaxChangeResult::tax_change{ .amount = 13 };
    string const expected_msg =
        "In honor of our marriage to our 8th wife, we have "
        "decided to raise your tax rate by [13%].  The tax "
        "rate is now [63%]. We will graciously allow you "
        "to kiss our royal pinky ring.";
    // Re-marriage number.
    W.rand().EXPECT__between_ints( 1, 3 ).returns( 2 );
    ChoiceConfig const config{
      .msg     = expected_msg,
      .options = {
        ChoiceConfigOption{
          .key          = "yes",
          .display_name = "Kiss pinky ring.",
        },
        ChoiceConfigOption{
          .key          = "no",
          .display_name = "Hold '[my colony Cigars party]'!",
        } } };
    agent
        .EXPECT__kiss_pinky_ring( expected_msg, 1,
                                  e_commodity::cigars, 13 )
        .returns( ui::e_confirm::yes );
    agent.EXPECT__handle(
        signal::TaxRateWillChange{ .delta = 13 } );
    REQUIRE( f() == expected );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 50 );
  }

  SECTION( "increase_or_party, chooses no" ) {
    TeaParty const party = TeaParty{
      .commodity =
          CommodityInColony{
            .colony_id = 1,
            .type_and_quantity =
                Commodity{ .type     = e_commodity::cigars,
                           .quantity = 23 } },
      .rebels_bump = 0.5 };
    proposal = TaxChangeProposal::increase_or_party{
      .amount = 13, .party = party };
    expected = TaxChangeResult::party{ .how = party };
    string const expected_msg =
        "In honor of our marriage to our 8th wife, we have "
        "decided to raise your tax rate by [13%].  The tax "
        "rate is now [63%]. We will graciously allow you "
        "to kiss our royal pinky ring.";
    // Re-marriage number.
    W.rand().EXPECT__between_ints( 1, 3 ).returns( 2 );
    ChoiceConfig const config{
      .msg     = expected_msg,
      .options = {
        ChoiceConfigOption{
          .key          = "yes",
          .display_name = "Kiss pinky ring.",
        },
        ChoiceConfigOption{
          .key          = "no",
          .display_name = "Hold '[my colony Cigars party]'!",
        } } };
    agent
        .EXPECT__kiss_pinky_ring( expected_msg, 1,
                                  e_commodity::cigars, 13 )
        .returns( ui::e_confirm::no );
    REQUIRE( f() == expected );
    REQUIRE( W.old_world( player ).taxes.tax_rate == 50 );
  }
}

TEST_CASE( "[tax] compute_tax_change" ) {
  world W;
  Player& player = W.default_player();
  TaxUpdateComputation expected;

  auto const f = [&] [[clang::noinline]] {
    return compute_tax_change( W.ss(),
                               W.map_updater().connectivity(),
                               W.rand(), player );
  };

  W.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  W.turn().time_point.year = 1550;

  SECTION( "not yet" ) {
    W.turn().time_point.turns                       = 10;
    W.old_world( player ).taxes.next_tax_event_turn = 15;
    expected = { .next_tax_event_turn = 15,
                 .proposed_tax_change = {} };
    REQUIRE( f() == expected );

    W.old_world( player ).taxes.next_tax_event_turn = 11;
    expected = { .next_tax_event_turn = 11,
                 .proposed_tax_change = {} };
    REQUIRE( f() == expected );
  }

  SECTION( "it is here" ) {
    SECTION( "next_tax_event_turn=0" ) {
      W.turn().time_point.turns                       = 10;
      W.old_world( player ).taxes.next_tax_event_turn = 0;
      expected = { .next_tax_event_turn = 36,
                   .proposed_tax_change = {} };
      REQUIRE( f() == expected );
    }

    SECTION( "after 0" ) {
      W.turn().time_point.turns                       = 36;
      W.old_world( player ).taxes.next_tax_event_turn = 37;

      SECTION( "too early" ) {
        expected = { .next_tax_event_turn = 37,
                     .proposed_tax_change = {} };
        REQUIRE( f() == expected );
      }

      SECTION( "late enough" ) {
        W.rand().EXPECT__between_ints( 17, 19 ).returns( 13 );

        W.turn().time_point.turns                       = 38;
        W.old_world( player ).taxes.next_tax_event_turn = 37;

        SECTION( "no colonies" ) {
          expected = { .next_tax_event_turn = 51,
                       .proposed_tax_change = {} };
          REQUIRE( f() == expected );
        }

        SECTION( "with colonies" ) {
          auto [colony1, founder1] =
              W.found_colony_with_new_unit( Coord{} );
          auto [colony2, founder2] =
              W.found_colony_with_new_unit( Coord{ .x = 2 } );

          // Tax change amount.
          W.rand().EXPECT__between_ints( 1, 8 ).returns( 4 );

          SECTION( "decrease" ) {
            // Tax increase probability.
            W.rand().EXPECT__bernoulli( .98 ).returns( false );
            SECTION( "already zero" ) {
              W.old_world( player ).taxes.tax_rate = 0;
              expected = { .next_tax_event_turn = 51,
                           .proposed_tax_change =
                               TaxChangeProposal::none{} };
              REQUIRE( f() == expected );
            }

            SECTION( "larger than zero" ) {
              W.old_world( player ).taxes.tax_rate = 5;
              expected                             = {
                                            .next_tax_event_turn = 51,
                                            .proposed_tax_change =
                    TaxChangeProposal::decrease{ .amount = 4 } };
              REQUIRE( f() == expected );
            }

            SECTION( "capped" ) {
              W.old_world( player ).taxes.tax_rate = 3;
              expected                             = {
                                            .next_tax_event_turn = 51,
                                            .proposed_tax_change =
                    TaxChangeProposal::decrease{ .amount = 3 } };
              REQUIRE( f() == expected );
            }
          }

          SECTION( "increase" ) {
            // Tax increase probability.
            W.rand().EXPECT__bernoulli( .98 ).returns( true );

            SECTION( "already max" ) {
              W.old_world( player ).taxes.tax_rate = 75;
              expected = { .next_tax_event_turn = 51,
                           .proposed_tax_change =
                               TaxChangeProposal::none{} };
              REQUIRE( f() == expected );
            }

            SECTION( "not at max" ) {
              W.old_world( player ).taxes.tax_rate = 72;

              SECTION( "no boycottable goods" ) {
                expected = { .next_tax_event_turn = 51,
                             .proposed_tax_change =
                                 TaxChangeProposal::increase{
                                   .amount = 3 } };
                REQUIRE( f() == expected );
              }

              SECTION( "Have boycottable goods" ) {
                int bid = 1;
                for( e_commodity type :
                     refl::enum_values<e_commodity> )
                  W.old_world( player )
                      .market.commodities[type]
                      .bid_price = bid++;
                // Worth: 100*8 = 800.
                colony1.commodities[e_commodity::silver] = 100;
                // Worth: 80*14 = 1120.
                colony2.commodities[e_commodity::trade_goods] =
                    80;
                // Worth: 60*1 = 60.
                colony1.commodities[e_commodity::food] = 60;
                // Worth: 40*2 = 80.
                colony2.commodities[e_commodity::sugar] = 40;
                // Worth: 20*12 = 240.
                colony1.commodities[e_commodity::cloth] = 20;
                // Worth: 1*16 = 16.
                colony2.commodities[e_commodity::muskets] = 1;

                // Rebels bump.
                W.rand()
                    .EXPECT__between_doubles( 0.0, 1.0 )
                    .returns( .7 );

                TeaParty const party{
                  .commodity =
                      CommodityInColony{
                        .colony_id = 2,
                        .type_and_quantity =
                            Commodity{
                              .type = e_commodity::trade_goods,
                              .quantity = 80 } },
                  .rebels_bump = .7 };
                expected = {
                  .next_tax_event_turn = 51,
                  .proposed_tax_change =
                      TaxChangeProposal::increase_or_party{
                        .amount = 3, .party = party } };
                REQUIRE( f() == expected );
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE(
    "[tax] compute_tax_change skips colonies without ocean "
    "access" ) {
  world W;
  Player& player = W.default_player();
  TaxUpdateComputation expected;

  auto const f = [&] [[clang::noinline]] {
    return compute_tax_change( W.ss(),
                               W.map_updater().connectivity(),
                               W.rand(), player );
  };

  W.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  W.rand().EXPECT__between_ints( 17, 19 ).returns( 13 );

  W.turn().time_point.turns                       = 38;
  W.old_world( player ).taxes.next_tax_event_turn = 37;

  auto [colony1, founder1] =
      W.found_colony_with_new_unit( { .x = 2, .y = 3 } );
  auto [colony2, founder2] =
      W.found_colony_with_new_unit( { .x = 0, .y = 3 } );

  // Sanity check that we're testing what we think we're testing.
  REQUIRE_FALSE( tile_has_surrounding_ocean_access(
      W.ss(), W.map_updater().connectivity(),
      colony1.location ) );
  REQUIRE( tile_has_surrounding_ocean_access(
      W.ss(), W.map_updater().connectivity(),
      colony2.location ) );

  // Tax change amount.
  W.rand().EXPECT__between_ints( 1, 8 ).returns( 4 );

  // Tax increase probability.
  W.rand().EXPECT__bernoulli( .98 ).returns( true );

  W.old_world( player ).taxes.tax_rate = 72;

  int bid = 1;
  for( e_commodity type : refl::enum_values<e_commodity> )
    W.old_world( player ).market.commodities[type].bid_price =
        bid++;

  // Would pick colony 1 (silver, 800) but since it has no
  // ocean access it should pick colony 2.

  // Worth: 100*8 = 800.
  colony1.commodities[e_commodity::silver] = 100;
  // Worth: 60*1 = 60.
  colony1.commodities[e_commodity::food] = 60;
  // Worth: 20*12 = 240.
  colony1.commodities[e_commodity::cloth] = 20;
  // Worth: 1*16 = 16.
  colony2.commodities[e_commodity::muskets] = 1;

  // Rebels bump.
  W.rand().EXPECT__between_doubles( 0.0, 1.0 ).returns( .7 );

  TeaParty const party{
    .commodity =
        CommodityInColony{
          .colony_id = 2,
          .type_and_quantity =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 1 } },
    .rebels_bump = .7 };
  expected = {
    .next_tax_event_turn = 51,
    .proposed_tax_change = TaxChangeProposal::increase_or_party{
      .amount = 3, .party = party } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[tax] compute_tax_change has year dependence" ) {
  world w;
  Player& player = w.default_player();
  TaxUpdateComputation expected;

  auto const f = [&] [[clang::noinline]] {
    return compute_tax_change( w.ss(),
                               w.map_updater().connectivity(),
                               w.rand(), player );
  };

  w.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  w.turn().time_point.turns                       = 38;
  w.old_world( player ).taxes.next_tax_event_turn = 37;
  w.old_world( player ).taxes.tax_rate            = 10;

  w.add_colony( { .x = 0, .y = 0 } );

  SECTION( "0-1599" ) {
    w.turn().time_point.year = 1599;
    w.rand().EXPECT__between_ints( 17, 19 ).returns( 13 );
    // Tax change amount.
    w.rand().EXPECT__between_ints( 1, 8 ).returns( 3 );
    // Tax increase probability.
    w.rand().EXPECT__bernoulli( .98 ).returns( false );

    expected = {
      .next_tax_event_turn = 51,
      .proposed_tax_change =
          TaxChangeProposal::decrease{ .amount = 3 } };
    REQUIRE( f() == expected );
  }

  SECTION( "1600-1699" ) {
    w.turn().time_point.year = 1699;
    w.rand().EXPECT__between_ints( 14, 16 ).returns( 13 );
    // Tax change amount.
    w.rand().EXPECT__between_ints( 1, 8 ).returns( 3 );
    // Tax increase probability.
    w.rand().EXPECT__bernoulli( .98 ).returns( false );

    expected = {
      .next_tax_event_turn = 51,
      .proposed_tax_change =
          TaxChangeProposal::decrease{ .amount = 3 } };
    REQUIRE( f() == expected );
  }

  SECTION( "1700-1799" ) {
    w.turn().time_point.year = 1799;
    w.rand().EXPECT__between_ints( 11, 13 ).returns( 13 );
    // Tax change amount.
    w.rand().EXPECT__between_ints( 1, 8 ).returns( 3 );
    // Tax increase probability.
    w.rand().EXPECT__bernoulli( .98 ).returns( false );

    expected = {
      .next_tax_event_turn = 51,
      .proposed_tax_change =
          TaxChangeProposal::decrease{ .amount = 3 } };
    REQUIRE( f() == expected );
  }

  SECTION( "1800-1899" ) {
    w.turn().time_point.year = 1899;
    w.rand().EXPECT__between_ints( 8, 10 ).returns( 13 );
    // Tax change amount.
    w.rand().EXPECT__between_ints( 1, 8 ).returns( 3 );
    // Tax increase probability.
    w.rand().EXPECT__bernoulli( .98 ).returns( false );

    expected = {
      .next_tax_event_turn = 51,
      .proposed_tax_change =
          TaxChangeProposal::decrease{ .amount = 3 } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[tax] start_of_turn_tax_check" ) {
  world W;
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();

  auto const f = [&] [[clang::noinline]] {
    return start_of_turn_tax_check(
        W.ss(), W.rand(), W.map_updater().connectivity(), player,
        W.agent() );
  };

  W.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  W.rand().EXPECT__between_ints( 17, 19 ).returns( 13 );

  W.turn().time_point.turns                       = 38;
  W.old_world( player ).taxes.next_tax_event_turn = 37;
  W.old_world( player ).taxes.tax_rate            = 72;

  auto [colony1, founder1] =
      W.found_colony_with_new_unit( Coord{} );
  colony1.name = "my colony 1";
  colony1.sons_of_liberty.num_rebels_from_bells_only = .3;
  auto [colony2, founder2] =
      W.found_colony_with_new_unit( Coord{ .x = 2 } );
  colony2.name = "my colony 2";
  colony2.sons_of_liberty.num_rebels_from_bells_only = .4;

  // Tax change amount.
  W.rand().EXPECT__between_ints( 1, 8 ).returns( 4 );

  // Tax increase probability.
  W.rand().EXPECT__bernoulli( .98 ).returns( true );

  int bid = 1;
  for( e_commodity type : refl::enum_values<e_commodity> )
    W.old_world( player ).market.commodities[type].bid_price =
        bid++;
  // Worth: 100*8 = 800.
  colony1.commodities[e_commodity::silver] = 100;
  // Worth: 80*14 = 1120.
  colony2.commodities[e_commodity::trade_goods] = 80;
  // Worth: 60*1 = 60.
  colony1.commodities[e_commodity::food] = 60;
  // Worth: 40*2 = 80.
  colony2.commodities[e_commodity::sugar] = 40;
  // Worth: 20*12 = 240.
  colony1.commodities[e_commodity::cloth] = 20;
  // Worth: 1*16 = 16.
  colony2.commodities[e_commodity::muskets] = 1;

  Colony colony1_saved           = colony1;
  Colony colony2_saved           = colony2;
  PlayerMarketState market_saved = W.old_world( player ).market;

  // Rebels bump.
  W.rand().EXPECT__between_doubles( 0.0, 1.0 ).returns( .7 );

  string const expected_msg =
      "In honor of our marriage to our 8th wife, we have "
      "decided to raise your tax rate by [3%].  The tax "
      "rate is now [75%]. We will graciously allow you "
      "to kiss our royal pinky ring.";
  // Re-marriage number.
  W.rand().EXPECT__between_ints( 1, 3 ).returns( 2 );
  ChoiceConfig const config{
    .msg     = expected_msg,
    .options = { ChoiceConfigOption{
                   .key          = "yes",
                   .display_name = "Kiss pinky ring.",
                 },
                 ChoiceConfigOption{
                   .key = "no",
                   .display_name =
                       "Hold '[my colony 2 Trade Goods party]'!",
                 } } };
  agent
      .EXPECT__kiss_pinky_ring( expected_msg, 2,
                                e_commodity::trade_goods, 3 )
      .returns( ui::e_confirm::no );
  REQUIRE( W.old_world( player ).taxes.tax_rate == 72 );

  string const boycott_msg =
      "Colonists in my colony 2 hold [Trade Goods Party]!  "
      "Amid colonists' refusal to pay new tax, Sons of Liberty "
      "throw [80] tons of trade goods into the sea!  The "
      "Dutch Parliament announces boycott of trade goods.  "
      "Trade Goods cannot be traded in Amsterdam until boycott "
      "is lifted.";
  agent.EXPECT__message_box( boycott_msg );
  agent.EXPECT__handle( signal::TeaParty{
    .what      = Commodity{ .type     = e_commodity::trade_goods,
                            .quantity = 80 },
    .colony_id = colony2.id } );

  wait<> w = f();
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );

  REQUIRE( W.old_world( player ).taxes.tax_rate == 72 );
  REQUIRE( colony1 == colony1_saved );
  colony2_saved.commodities[e_commodity::trade_goods]      = 0;
  colony2_saved.sons_of_liberty.num_rebels_from_bells_only = 1.1;
  REQUIRE( colony2 == colony2_saved );
  market_saved.commodities[e_commodity::trade_goods].boycott =
      true;
  REQUIRE( W.old_world( player ).market == market_saved );
}

// This tests that we don't crash if the start of turn tax check
// runs and the player has manually adjusted the tax rate to be
// out of the normally allowed bounds, then the "tax decrease"
// option gets selected.
TEST_CASE( "[tax] start_of_turn_tax_check (out of range)" ) {
  world w;
  Player& player = w.default_player();

  auto const f = [&] [[clang::noinline]] {
    co_await_test( start_of_turn_tax_check(
        w.ss(), w.rand(), w.map_updater().connectivity(), player,
        w.agent() ) );
  };

  w.settings().game_setup_options.difficulty =
      e_difficulty::discoverer;

  w.rand().EXPECT__between_ints( 21, 23 ).returns( 18 );

  w.turn().time_point.turns                       = 38;
  w.old_world( player ).taxes.next_tax_event_turn = 37;
  w.old_world( player ).taxes.tax_rate            = 80;

  // Make sure we're testing what we think we're testing.
  BASE_CHECK(
      w.old_world( player ).taxes.tax_rate >
      config_old_world
          .taxes[w.ss().settings.game_setup_options.difficulty]
          .maximum_tax_rate );

  auto [colony1, founder1] =
      w.found_colony_with_new_unit( Coord{} );
  colony1.name = "my colony 1";
  colony1.sons_of_liberty.num_rebels_from_bells_only = .3;

  REQUIRE( w.old_world( player ).taxes.tax_rate == 80 );
  f();
  REQUIRE( w.old_world( player ).taxes.tax_rate == 80 );
}

TEST_CASE( "[tax] compute_tax_change when over max" ) {
  world W;
  Player& player = W.default_player();
  TaxUpdateComputation expected;

  auto const f = [&] [[clang::noinline]] {
    return compute_tax_change( W.ss(),
                               W.map_updater().connectivity(),
                               W.rand(), player );
  };

  W.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  W.rand().EXPECT__between_ints( 17, 19 ).returns( 13 );

  W.turn().time_point.turns                       = 38;
  W.old_world( player ).taxes.next_tax_event_turn = 37;

  W.found_colony_with_new_unit( Coord{} );
  W.found_colony_with_new_unit( Coord{ .x = 2 } );

  W.old_world( player ).taxes.tax_rate = 76;
  BASE_CHECK(
      W.old_world( player ).taxes.tax_rate >
      config_old_world
          .taxes[W.settings().game_setup_options.difficulty]
          .maximum_tax_rate );

  expected = {
    .next_tax_event_turn = 51,
    .proposed_tax_change = TaxChangeProposal::none{} };

  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
