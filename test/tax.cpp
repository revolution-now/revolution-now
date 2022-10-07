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
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    create_default_map();
    add_default_player();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L, L, L };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[tax] try_trade_boycotted_commodity" ) {
  World       W;
  Player&     player = W.default_player();
  e_commodity type   = {};
  wait<>      w      = make_wait<>();

  player.old_world.taxes.tax_rate = 7;

  auto f = [&] {
    player.old_world.market.commodities[type].boycott = true;
    wait<> w =
        try_trade_boycotted_commodity( W.ts(), player, type,
                                       /*back_taxes=*/33 );
    BASE_CHECK( !w.exception() );
    BASE_CHECK( w.ready() );
  };

  SECTION( "food, not enough money" ) {
    type         = e_commodity::food;
    player.money = 32;

    string const expected_msg =
        "@[H]Food@[] is currently under Parliamentary boycott. "
        "We cannot trade food until Parliament lifts the "
        "boycott, which it will not do unless we agree to pay "
        "@[H]33@[] in back taxes. Treasury: 32.";
    EXPECT_CALL( W.gui(), message_box( expected_msg ) )
        .returns( make_wait<>() );

    f();
    REQUIRE( player.money == 32 );
    REQUIRE( player.old_world.market.commodities[type].boycott );
    REQUIRE( player.old_world.taxes.tax_rate == 7 );
  }

  SECTION( "muskets, enough money, selects no" ) {
    type         = e_commodity::muskets;
    player.money = 33;

    string const expected_msg =
        "@[H]Muskets@[] are currently under Parliamentary "
        "boycott. We cannot trade muskets until Parliament "
        "lifts the boycott, which it will not do unless we "
        "agree to pay @[H]33@[] in back taxes.";
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
                .display_name = "Pay @[H]33@[].",
            } } };
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
        .returns( make_wait<maybe<string>>( "no" ) );

    f();
    REQUIRE( player.money == 33 );
    REQUIRE( player.old_world.market.commodities[type].boycott );
    REQUIRE( player.old_world.taxes.tax_rate == 7 );
  }

  SECTION( "tools, enough money, selects yes" ) {
    type         = e_commodity::tools;
    player.money = 34;

    string const expected_msg =
        "@[H]Tools@[] are currently under Parliamentary "
        "boycott. We cannot trade tools until Parliament "
        "lifts the boycott, which it will not do unless we "
        "agree to pay @[H]33@[] in back taxes.";
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
                .display_name = "Pay @[H]33@[].",
            } } };
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
        .returns( make_wait<maybe<string>>( "yes" ) );

    f();
    REQUIRE( player.money == 1 );
    REQUIRE(
        !player.old_world.market.commodities[type].boycott );
    REQUIRE( player.old_world.taxes.tax_rate == 7 );
  }
}

TEST_CASE( "[tax] back_tax_for_boycotted_commodity" ) {
  World       W;
  Player&     player   = W.default_player();
  e_commodity type     = {};
  int         expected = 0;

  player.old_world.taxes.tax_rate = 7;

  auto f = [&] {
    return back_tax_for_boycotted_commodity( as_const( player ),
                                             type );
  };

  type = e_commodity::ore;
  player.old_world.market.commodities[type].bid_price = 5;
  expected                                            = 4000;
  REQUIRE( f() == expected );
  REQUIRE( player.old_world.taxes.tax_rate == 7 );

  type = e_commodity::silver;
  player.old_world.market.commodities[type].bid_price = 19;
  expected                                            = 10000;
  REQUIRE( f() == expected );
  REQUIRE( player.old_world.taxes.tax_rate == 7 );

  type = e_commodity::rum;
  player.old_world.market.commodities[type].bid_price = 2;
  expected                                            = 1500;
  REQUIRE( f() == expected );
  REQUIRE( player.old_world.taxes.tax_rate == 7 );
}

TEST_CASE( "[tax] apply_tax_result" ) {
  World             W;
  Player&           player              = W.default_player();
  int               next_tax_event_turn = 0;
  TaxChangeResult_t change;

  W.turn().time_point.turns       = 5;
  player.old_world.taxes.tax_rate = 50;

  Colony& colony = W.add_colony_with_new_unit( Coord{} );

  int bid = 1;
  for( e_commodity type : refl::enum_values<e_commodity> ) {
    player.old_world.market.commodities[type].bid_price = bid++;
    colony.commodities[type] = bid * 10;
  }
  colony.sons_of_liberty.num_rebels_from_bells_only = 1.0;

  Colony            colony_saved = colony;
  PlayerMarketState market_saved = player.old_world.market;

  auto f = [&] {
    apply_tax_result( W.ss(), player, next_tax_event_turn,
                      change );
  };

  next_tax_event_turn = 6;

  SECTION( "none" ) {
    change = TaxChangeResult::none{};
    f();
    REQUIRE( player.old_world.taxes.tax_rate == 50 );
    REQUIRE( colony == colony_saved );
    REQUIRE( player.old_world.market == market_saved );
  }

  SECTION( "tax increase" ) {
    change = TaxChangeResult::tax_change{ .amount = 5 };
    f();
    REQUIRE( player.old_world.taxes.tax_rate == 55 );
    REQUIRE( colony == colony_saved );
    REQUIRE( player.old_world.market == market_saved );
  }

  SECTION( "tax decrease" ) {
    change = TaxChangeResult::tax_change{ .amount = -5 };
    f();
    REQUIRE( player.old_world.taxes.tax_rate == 45 );
    REQUIRE( colony == colony_saved );
    REQUIRE( player.old_world.market == market_saved );
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
    REQUIRE( player.old_world.taxes.tax_rate == 50 );
    colony_saved.commodities[e_commodity::ore] = 1;
    colony_saved.sons_of_liberty.num_rebels_from_bells_only =
        1.5;
    REQUIRE( colony == colony_saved );
    market_saved.commodities[e_commodity::ore].boycott = true;
    REQUIRE( player.old_world.market == market_saved );
  }
}

TEST_CASE( "[tax] prompt_for_tax_change_result" ) {
  World               W;
  Player&             player = W.default_player();
  TaxChangeProposal_t proposal;
  TaxChangeResult_t   expected;

  Colony& colony = W.add_colony_with_new_unit( Coord{} );
  colony.name    = "my colony";

  auto f = [&] {
    wait<TaxChangeResult_t> w = prompt_for_tax_change_result(
        W.ss(), W.ts(), player, proposal );
    CHECK( !w.exception() );
    CHECK( w.ready() );
    return *w;
  };

  player.old_world.taxes.tax_rate = 50;

  SECTION( "decrease" ) {
    proposal = TaxChangeProposal::decrease{ .amount = 13 };
    expected = TaxChangeResult::tax_change{ .amount = -13 };
    string const expected_msg =
        "The crown has graciously decided to LOWER your tax "
        "rate by @[H]13%@[].  The tax rate is now @[H]37%@[].";
    EXPECT_CALL( W.gui(), message_box( expected_msg ) )
        .returns( make_wait<>() );
    REQUIRE( f() == expected );
    REQUIRE( player.old_world.taxes.tax_rate == 50 );
  }

  SECTION( "increase" ) {
    proposal = TaxChangeProposal::increase{ .amount = 13 };
    expected = TaxChangeResult::tax_change{ .amount = 13 };
    string const expected_msg =
        "In honor of our marriage to our 8th wife, we have "
        "decided to raise your tax rate by @[H]13%@[].  The tax "
        "rate is now @[H]63%@[]. We will graciously allow you "
        "to kiss our royal pinky ring.";
    EXPECT_CALL(
        W.rand(),
        between_ints( 1, 3, IRand::e_interval::closed ) )
        .returns( 2 );
    EXPECT_CALL( W.gui(), message_box( expected_msg ) )
        .returns( make_wait<>() );
    REQUIRE( f() == expected );
    REQUIRE( player.old_world.taxes.tax_rate == 50 );
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
        "decided to raise your tax rate by @[H]13%@[].  The tax "
        "rate is now @[H]63%@[]. We will graciously allow you "
        "to kiss our royal pinky ring.";
    EXPECT_CALL(
        W.rand(),
        between_ints( 1, 3, IRand::e_interval::closed ) )
        .returns( 2 );
    ChoiceConfig const config{
        .msg     = expected_msg,
        .options = {
            ChoiceConfigOption{
                .key          = "yes",
                .display_name = "Kiss pinky ring.",
            },
            ChoiceConfigOption{
                .key = "no",
                .display_name =
                    "Hold '@[H]my colony Cigars party@[]'!",
            } } };
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
        .returns( make_wait<maybe<string>>( "yes" ) );
    REQUIRE( f() == expected );
    REQUIRE( player.old_world.taxes.tax_rate == 50 );
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
    expected = TaxChangeResult::party{ .how = party };
    string const expected_msg =
        "In honor of our marriage to our 8th wife, we have "
        "decided to raise your tax rate by @[H]13%@[].  The tax "
        "rate is now @[H]63%@[]. We will graciously allow you "
        "to kiss our royal pinky ring.";
    EXPECT_CALL(
        W.rand(),
        between_ints( 1, 3, IRand::e_interval::closed ) )
        .returns( 2 );
    ChoiceConfig const config{
        .msg     = expected_msg,
        .options = {
            ChoiceConfigOption{
                .key          = "yes",
                .display_name = "Kiss pinky ring.",
            },
            ChoiceConfigOption{
                .key = "no",
                .display_name =
                    "Hold '@[H]my colony Cigars party@[]'!",
            } } };
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
        .returns( make_wait<maybe<string>>( "no" ) );
    REQUIRE( f() == expected );
    REQUIRE( player.old_world.taxes.tax_rate == 50 );
  }
}

TEST_CASE( "[tax] back_tax_for_boycotted_commodity" ) {
  World W;
  // TODO
}

TEST_CASE( "[tax] try_trade_boycotted_commodity" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
