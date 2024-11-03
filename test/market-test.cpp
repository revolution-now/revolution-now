/****************************************************************
**market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Unit tests for the src/market.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/market.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"

// config
#include "config/market.rds.hpp"

// ss
#include "ss/player.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

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
    set_default_player( e_nation::french );
    add_player( e_nation::french );
    add_player( e_nation::english );
    add_player( e_nation::dutch );
    add_player( e_nation::spanish );
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[market] market_price" ) {
  World   W;
  Player& french = W.player( e_nation::french );

  french.old_world.market.commodities[e_commodity::ore]
      .bid_price = 5;
  REQUIRE( market_price( french, e_commodity::ore ) ==
           CommodityPrice{ .bid = 5, .ask = 8 } );

  french.old_world.market.commodities[e_commodity::food]
      .bid_price = 1;
  REQUIRE( market_price( french, e_commodity::food ) ==
           CommodityPrice{ .bid = 1, .ask = 9 } );

  french.old_world.market.commodities[e_commodity::muskets]
      .bid_price = 3;
  REQUIRE( market_price( french, e_commodity::muskets ) ==
           CommodityPrice{ .bid = 3, .ask = 4 } );
}

TEST_CASE( "[market] create_price_change" ) {
  World W;
  W.set_current_bid_price( e_commodity::ore, 10 );
  PriceChange const change =
      create_price_change( W.player(), e_commodity::ore,
                           /*price_change=*/3 );
  PriceChange const expected{ .type  = e_commodity::ore,
                              .from  = { .bid = 10, .ask = 13 },
                              .to    = { .bid = 13, .ask = 16 },
                              .delta = 3 };
  REQUIRE( change == expected );
}

TEST_CASE( "[market] display_price_change_notification" ) {
  World         W;
  Player const& player = W.default_player();
  PriceChange   change;
  wait<>        w = make_wait<>();

  W.set_current_bid_price( e_commodity::ore, 10 );

  auto f = [&] {
    return display_price_change_notification( W.ts(), player,
                                              change );
  };

  change = {};
  w      = f();
  REQUIRE_FALSE( w.exception() );
  REQUIRE( w.ready() );

  change = create_price_change( W.default_player(),
                                e_commodity::ore, 3 );
  W.gui()
      .EXPECT__message_box(
          "The price of [ore] in La "
          "Rochelle has risen to 13." )
      .returns( make_wait<>() );
  w = f();
  REQUIRE_FALSE( w.exception() );
  REQUIRE( w.ready() );

  change = create_price_change( W.default_player(),
                                e_commodity::ore, -1 );
  W.gui()
      .EXPECT__message_box(
          "The price of [ore] in La "
          "Rochelle has fallen to 9." )
      .returns( make_wait<>() );
  w = f();
  REQUIRE_FALSE( w.exception() );
  REQUIRE( w.ready() );
}

TEST_CASE( "[market] ask_from_bid" ) {
  REQUIRE( ask_from_bid( e_commodity::food, 2 ) == 10 );
  REQUIRE( ask_from_bid( e_commodity::silver, 2 ) == 3 );
}

TEST_CASE( "[market] apply_invoice" ) {
  World   W;
  Invoice invoice;

  W.set_current_bid_price( e_commodity::silver, 10 );

  invoice = {
    .what = Commodity{ .type     = e_commodity::silver,
                       .quantity = 100 },
    .money_delta_before_taxes = 9999, // shouldn't be used.
    .tax_rate                 = 9999, // shouldn't be used.
    .tax_amount               = 9999, // shouldn't be used.
    .money_delta_final        = 123,
    .player_volume_delta      = 345,
    .intrinsic_volume_delta =
        {
          { e_nation::english, 9 },
          { e_nation::french, 11 },
        },
    .global_intrinsic_volume_deltas =
        {
          { e_commodity::food, 13 },
          { e_commodity::muskets, 15 },
        },
    .price_change = create_price_change(
        W.player(), e_commodity::silver, 3 ),
  };

  Player const& p = W.default_player();
  REQUIRE( p.money == 0 );
  REQUIRE( p.old_world.market.commodities[e_commodity::silver]
               .player_traded_volume == 0 );
  REQUIRE( W.player( e_nation::english )
               .old_world.market.commodities[e_commodity::silver]
               .intrinsic_volume == 0 );
  REQUIRE( W.player( e_nation::french )
               .old_world.market.commodities[e_commodity::silver]
               .intrinsic_volume == 0 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::food]
               .intrinsic_volume == 0 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::muskets]
               .intrinsic_volume == 0 );
  REQUIRE( p.old_world.market.commodities[e_commodity::silver]
               .bid_price == 10 );

  apply_invoice( W.ss(), W.default_player(), invoice );

  REQUIRE( p.money == 123 );
  REQUIRE( p.old_world.market.commodities[e_commodity::silver]
               .player_traded_volume == 345 );
  REQUIRE( W.player( e_nation::english )
               .old_world.market.commodities[e_commodity::silver]
               .intrinsic_volume == 9 );
  REQUIRE( W.player( e_nation::french )
               .old_world.market.commodities[e_commodity::silver]
               .intrinsic_volume == 11 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::food]
               .intrinsic_volume == 13 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::muskets]
               .intrinsic_volume == 15 );
  REQUIRE( p.old_world.market.commodities[e_commodity::silver]
               .bid_price == 13 );

  apply_invoice( W.ss(), W.default_player(), invoice );

  REQUIRE( p.money == 123 * 2 );
  REQUIRE( p.old_world.market.commodities[e_commodity::silver]
               .player_traded_volume == 345 * 2 );
  REQUIRE( W.player( e_nation::english )
               .old_world.market.commodities[e_commodity::silver]
               .intrinsic_volume == 9 * 2 );
  REQUIRE( W.player( e_nation::french )
               .old_world.market.commodities[e_commodity::silver]
               .intrinsic_volume == 11 * 2 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::food]
               .intrinsic_volume == 13 * 2 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::muskets]
               .intrinsic_volume == 15 * 2 );
  REQUIRE( p.old_world.market.commodities[e_commodity::silver]
               .bid_price == 16 );
}

TEST_CASE( "[market] evolve_group_model_volumes" ) {
  World W;
  // We'll set dutch=true just to make sure that nothing special
  // happens during the evolution; the dutch should only get an
  // advantage when buying/selling in the price group model.
  Player& player = W.player( e_nation::dutch );

  W.ss()
      .players.global_market_state.commodities[e_commodity::rum]
      .intrinsic_volume = 5000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::cigars]
      .intrinsic_volume = 4000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::cloth]
      .intrinsic_volume = 3000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::coats]
      .intrinsic_volume = 2000;

  // These should influence the evolution but they should not
  // change.
  player.old_world.market.commodities[e_commodity::rum]
      .player_traded_volume = 1000;
  player.old_world.market.commodities[e_commodity::cigars]
      .player_traded_volume = 2000;
  player.old_world.market.commodities[e_commodity::cloth]
      .player_traded_volume = 3000;
  player.old_world.market.commodities[e_commodity::coats]
      .player_traded_volume = 4000;

  // Do the evolution.
  evolve_group_model_volumes( W.ss() );

  // Tests.
  REQUIRE( player.old_world.market.commodities[e_commodity::rum]
               .player_traded_volume == 1000 );
  REQUIRE(
      player.old_world.market.commodities[e_commodity::cigars]
          .player_traded_volume == 2000 );
  REQUIRE(
      player.old_world.market.commodities[e_commodity::cloth]
          .player_traded_volume == 3000 );
  REQUIRE(
      player.old_world.market.commodities[e_commodity::coats]
          .player_traded_volume == 4000 );

  int expected = 0;

  expected =
      lround( ( 5000.0 + 1000.0 + .5 ) * .9921875 - 1000.0 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::rum]
               .intrinsic_volume == expected );

  expected =
      lround( ( 4000.0 + 2000.0 + .5 ) * .9921875 - 2000.0 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::cigars]
               .intrinsic_volume == expected );

  expected =
      lround( ( 3000.0 + 3000.0 + .5 ) * .9921875 - 3000.0 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::cloth]
               .intrinsic_volume == expected );

  expected =
      lround( ( 2000.0 + 4000.0 + .5 ) * .9921875 - 4000.0 );
  REQUIRE( W.ss()
               .players.global_market_state
               .commodities[e_commodity::coats]
               .intrinsic_volume == expected );
}

TEST_CASE( "[market] evolve_player_prices (non-dutch)" ) {
  World W;
  W.set_default_player( e_nation::french );
  Player& player = W.default_player();
  // Set each price to its middle value.
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    if( is_in_processed_goods_price_group( c ) ) continue;
    int const min = config_market.price_behavior[c]
                        .price_limits.bid_price_min;
    int const max = config_market.price_behavior[c]
                        .price_limits.bid_price_max;
    int const middle = ( min + max ) / 2;
    W.set_stable_bid_price( c, middle );
  }
  GlobalMarketItem& global_rum =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::rum];
  GlobalMarketItem& global_cigars =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::cigars];
  GlobalMarketItem& global_cloth =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::cloth];
  GlobalMarketItem& global_coats =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::coats];

  W.player( e_nation::dutch )
      .old_world.market.commodities[e_commodity::rum]
      .player_traded_volume = 100;
  W.player( e_nation::french )
      .old_world.market.commodities[e_commodity::rum]
      .player_traded_volume   = 100;
  global_rum.intrinsic_volume = 200;
  // Rum total: 400.

  W.player( e_nation::french )
      .old_world.market.commodities[e_commodity::cigars]
      .player_traded_volume      = 100;
  global_cigars.intrinsic_volume = 200;
  // Cigars total: 300.

  global_cloth.intrinsic_volume = 200;
  // Cloth total: 200.

  W.player( e_nation::english )
      .old_world.market.commodities[e_commodity::coats]
      .player_traded_volume = 100;
  W.player( e_nation::french )
      .old_world.market.commodities[e_commodity::coats]
      .player_traded_volume = 100;
  W.player( e_nation::spanish )
      .old_world.market.commodities[e_commodity::coats]
      .player_traded_volume     = 100;
  global_coats.intrinsic_volume = 100;
  // Coats total: 400.

  // These should be the equilibrium bid prices.
  int const starting_rum_bid    = 8;
  int const starting_cigars_bid = 12;
  int const starting_cloth_bid  = 18;
  int const starting_coats_bid  = 8;
  W.set_current_bid_price( e_commodity::rum, starting_rum_bid );
  W.set_current_bid_price( e_commodity::cigars,
                           starting_cigars_bid );
  W.set_current_bid_price( e_commodity::cloth,
                           starting_cloth_bid );
  W.set_current_bid_price( e_commodity::coats,
                           starting_coats_bid );

  auto curr_price = [&]( e_commodity c ) {
    return player.old_world.market.commodities[c].bid_price;
  };
  auto intrinsic_vol = [&]( e_commodity c ) {
    return player.old_world.market.commodities[c]
        .intrinsic_volume;
  };
  auto traded_vol = [&]( e_commodity c ) {
    return player.old_world.market.commodities[c]
        .player_traded_volume;
  };
  auto total_traded_vol = [&]( e_commodity c ) {
    int sum = 0;
    for( auto const& [nation, player] : W.ss().players.players )
      if( player.has_value() )
        sum += player->old_world.market.commodities[c]
                   .player_traded_volume;
    return sum;
  };

  refl::enum_map<e_commodity, PriceChange> changes;

  // Do the first evolution, which should not move the prices be-
  // cause we've set all of the prices to their equilibrium
  // values and the attrition isn't enough to move them.
  changes = evolve_player_prices( W.ss(), player );

  // None of the prices should have evolved.
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    REQUIRE( changes[c].type == c );
    REQUIRE( changes[c].delta == 0 );
    REQUIRE( changes[c].from == changes[c].to );
  }

  REQUIRE( curr_price( e_commodity::food ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::food ) == -1 );
  REQUIRE( traded_vol( e_commodity::food ) == 0 );

  REQUIRE( curr_price( e_commodity::sugar ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::sugar ) == -13 );
  REQUIRE( traded_vol( e_commodity::sugar ) == 0 );

  REQUIRE( curr_price( e_commodity::tobacco ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::tobacco ) == -16 );
  REQUIRE( traded_vol( e_commodity::tobacco ) == 0 );

  REQUIRE( curr_price( e_commodity::cotton ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::cotton ) == -16 );
  REQUIRE( traded_vol( e_commodity::cotton ) == 0 );

  REQUIRE( curr_price( e_commodity::furs ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::furs ) == -25 );
  REQUIRE( traded_vol( e_commodity::furs ) == 0 );

  REQUIRE( curr_price( e_commodity::lumber ) == 1 );
  REQUIRE( intrinsic_vol( e_commodity::lumber ) == 0 );
  REQUIRE( traded_vol( e_commodity::lumber ) == 0 );

  REQUIRE( curr_price( e_commodity::ore ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::ore ) == -7 );
  REQUIRE( traded_vol( e_commodity::ore ) == 0 );

  REQUIRE( curr_price( e_commodity::silver ) == 10 );
  REQUIRE( intrinsic_vol( e_commodity::silver ) == -8 );
  REQUIRE( traded_vol( e_commodity::silver ) == 0 );

  REQUIRE( curr_price( e_commodity::horses ) == 5 );
  REQUIRE( intrinsic_vol( e_commodity::horses ) == -3 );
  REQUIRE( traded_vol( e_commodity::horses ) == 0 );

  REQUIRE( curr_price( e_commodity::rum ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::rum ) == 0 );
  REQUIRE( traded_vol( e_commodity::rum ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::rum ) == 200 );

  REQUIRE( curr_price( e_commodity::cigars ) == 12 );
  REQUIRE( intrinsic_vol( e_commodity::cigars ) == 0 );
  REQUIRE( traded_vol( e_commodity::cigars ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::cigars ) == 100 );

  REQUIRE( curr_price( e_commodity::cloth ) == 18 );
  REQUIRE( intrinsic_vol( e_commodity::cloth ) == 0 );
  REQUIRE( traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cloth ) == 0 );

  REQUIRE( curr_price( e_commodity::coats ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::coats ) == 0 );
  REQUIRE( traded_vol( e_commodity::coats ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::coats ) == 300 );

  REQUIRE( curr_price( e_commodity::trade_goods ) == 6 );
  REQUIRE( intrinsic_vol( e_commodity::trade_goods ) == 4 );
  REQUIRE( traded_vol( e_commodity::trade_goods ) == 0 );

  REQUIRE( curr_price( e_commodity::tools ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::tools ) == 5 );
  REQUIRE( traded_vol( e_commodity::tools ) == 0 );

  REQUIRE( curr_price( e_commodity::muskets ) == 10 );
  REQUIRE( intrinsic_vol( e_commodity::muskets ) == 6 );
  REQUIRE( traded_vol( e_commodity::muskets ) == 0 );

  // Now let's bump some of the volumes.
  auto vol = [&]( e_commodity c ) -> int& {
    return player.old_world.market.commodities[c]
        .intrinsic_volume;
  };

  // For the below we need to add the opposite of the attrition
  // in order to put the target where we want it. As a convenient
  // means for doing that, we will just negate the current val-
  // ues, since they hold the attritions from the evolution that
  // we just did.
  for( e_commodity c : refl::enum_values<e_commodity> )
    vol( c ) = -vol( c );

  vol( e_commodity::food ) += -300 + 1; // not quite a rise.
  vol( e_commodity::sugar ) += -400;    // just at a rise.
  vol( e_commodity::tobacco ) += -401;  // just over the rise.
  vol( e_commodity::cotton ) += -850; // more than twice a rise.
  vol( e_commodity::furs ) += 0;      // no change.
  vol( e_commodity::lumber ) += -500; // rise, but won't move.
  vol( e_commodity::ore ) += 399;     // just below fall.
  vol( e_commodity::silver ) += 100;  // just at fall.
  vol( e_commodity::horses ) += 201;  // just above fall.
  vol( e_commodity::rum )    = 9999; // shouldn't have an effect.
  vol( e_commodity::cigars ) = 9999; // shouldn't have an effect.
  vol( e_commodity::cloth ) = -9999; // shouldn't have an effect.
  vol( e_commodity::coats ) = -9999; // shouldn't have an effect.
  vol( e_commodity::trade_goods ) += 900; // three falls.
  vol( e_commodity::tools ) += 100;       // below a fall.
  vol( e_commodity::muskets ) += 200;     // just at a fall.

  global_rum.intrinsic_volume    = 0;
  global_cigars.intrinsic_volume = 200;
  global_cloth.intrinsic_volume  = 400;
  global_coats.intrinsic_volume  = 100;
  // Equilibrium prices should be:
  //   rum    = 18
  //   cigars = 12
  //   cloth  = 8
  //   coats  = 8
  // This basically swaps rum with cloth, which should cause
  // movement in only those two.

  // Second evolution.
  changes = evolve_player_prices( W.ss(), player );

  REQUIRE( curr_price( e_commodity::food ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::food ) == -299 );
  REQUIRE( traded_vol( e_commodity::food ) == 0 );
  REQUIRE( changes[e_commodity::food].delta == 2 - 2 );

  REQUIRE( curr_price( e_commodity::sugar ) == 5 );
  REQUIRE( intrinsic_vol( e_commodity::sugar ) == 0 );
  REQUIRE( traded_vol( e_commodity::sugar ) == 0 );
  REQUIRE( changes[e_commodity::sugar].delta == 5 - 4 );

  REQUIRE( curr_price( e_commodity::tobacco ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::tobacco ) == -1 );
  REQUIRE( traded_vol( e_commodity::tobacco ) == 0 );
  REQUIRE( changes[e_commodity::tobacco].delta == 3 - 2 );

  REQUIRE( curr_price( e_commodity::cotton ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::cotton ) == -450 );
  REQUIRE( traded_vol( e_commodity::cotton ) == 0 );
  REQUIRE( changes[e_commodity::cotton].delta == 3 - 2 );

  REQUIRE( curr_price( e_commodity::furs ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::furs ) == 0 );
  REQUIRE( traded_vol( e_commodity::furs ) == 0 );
  REQUIRE( changes[e_commodity::furs].delta == 3 - 3 );

  REQUIRE( curr_price( e_commodity::lumber ) == 1 );
  REQUIRE( intrinsic_vol( e_commodity::lumber ) == -200 );
  REQUIRE( traded_vol( e_commodity::lumber ) == 0 );
  REQUIRE( changes[e_commodity::lumber].delta == 1 - 1 );

  REQUIRE( curr_price( e_commodity::ore ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::ore ) == 399 );
  REQUIRE( traded_vol( e_commodity::ore ) == 0 );
  REQUIRE( changes[e_commodity::ore].delta == 3 - 3 );

  REQUIRE( curr_price( e_commodity::silver ) == 9 );
  REQUIRE( intrinsic_vol( e_commodity::silver ) == 0 );
  REQUIRE( traded_vol( e_commodity::silver ) == 0 );
  REQUIRE( changes[e_commodity::silver].delta == 9 - 10 );

  REQUIRE( curr_price( e_commodity::horses ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::horses ) == 1 );
  REQUIRE( traded_vol( e_commodity::horses ) == 0 );
  REQUIRE( changes[e_commodity::horses].delta == 4 - 5 );

  REQUIRE( curr_price( e_commodity::rum ) == 9 );
  REQUIRE( intrinsic_vol( e_commodity::rum ) == 9999 );
  REQUIRE( traded_vol( e_commodity::rum ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::rum ) == 200 );
  REQUIRE( changes[e_commodity::rum].delta == 1 );

  REQUIRE( curr_price( e_commodity::cigars ) == 12 );
  REQUIRE( intrinsic_vol( e_commodity::cigars ) == 9999 );
  REQUIRE( traded_vol( e_commodity::cigars ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::cigars ) == 100 );
  REQUIRE( changes[e_commodity::cigars].delta == 0 );

  REQUIRE( curr_price( e_commodity::cloth ) == 17 );
  REQUIRE( intrinsic_vol( e_commodity::cloth ) == -9999 );
  REQUIRE( traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( changes[e_commodity::cloth].delta == -1 );

  REQUIRE( curr_price( e_commodity::coats ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::coats ) == -9999 );
  REQUIRE( traded_vol( e_commodity::coats ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::coats ) == 300 );
  REQUIRE( changes[e_commodity::coats].delta == 0 );

  REQUIRE( curr_price( e_commodity::trade_goods ) == 5 );
  REQUIRE( intrinsic_vol( e_commodity::trade_goods ) == 600 );
  REQUIRE( traded_vol( e_commodity::trade_goods ) == 0 );
  REQUIRE( changes[e_commodity::trade_goods].delta == 5 - 6 );

  REQUIRE( curr_price( e_commodity::tools ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::tools ) == 100 );
  REQUIRE( traded_vol( e_commodity::tools ) == 0 );
  REQUIRE( changes[e_commodity::tools].delta == 4 - 4 );

  REQUIRE( curr_price( e_commodity::muskets ) == 9 );
  REQUIRE( intrinsic_vol( e_commodity::muskets ) == 0 );
  REQUIRE( traded_vol( e_commodity::muskets ) == 0 );
  REQUIRE( changes[e_commodity::muskets].delta == 9 - 10 );

  // We'll do this again just for the processed goods.
  changes = evolve_player_prices( W.ss(), player );

  REQUIRE( curr_price( e_commodity::rum ) == 10 );
  REQUIRE( intrinsic_vol( e_commodity::rum ) == 9999 );
  REQUIRE( traded_vol( e_commodity::rum ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::rum ) == 200 );
  REQUIRE( changes[e_commodity::rum].delta == 1 );

  REQUIRE( curr_price( e_commodity::cigars ) == 12 );
  REQUIRE( intrinsic_vol( e_commodity::cigars ) == 9999 );
  REQUIRE( traded_vol( e_commodity::cigars ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::cigars ) == 100 );
  REQUIRE( changes[e_commodity::cigars].delta == 0 );

  REQUIRE( curr_price( e_commodity::cloth ) == 16 );
  REQUIRE( intrinsic_vol( e_commodity::cloth ) == -9999 );
  REQUIRE( traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( changes[e_commodity::cloth].delta == -1 );

  REQUIRE( curr_price( e_commodity::coats ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::coats ) == -9999 );
  REQUIRE( traded_vol( e_commodity::coats ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::coats ) == 300 );
  REQUIRE( changes[e_commodity::coats].delta == 0 );
}

TEST_CASE( "[market] evolve_player_prices (dutch)" ) {
  World W;
  W.set_default_player( e_nation::dutch );
  Player& player = W.default_player();
  // Set each price to its middle value.
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    if( is_in_processed_goods_price_group( c ) ) continue;
    int const min = config_market.price_behavior[c]
                        .price_limits.bid_price_min;
    int const max = config_market.price_behavior[c]
                        .price_limits.bid_price_max;
    int const middle = ( min + max ) / 2;
    W.set_stable_bid_price( c, middle );
  }
  GlobalMarketItem& global_rum =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::rum];
  GlobalMarketItem& global_cigars =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::cigars];
  GlobalMarketItem& global_cloth =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::cloth];
  GlobalMarketItem& global_coats =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::coats];

  W.player( e_nation::dutch )
      .old_world.market.commodities[e_commodity::rum]
      .player_traded_volume = 100;
  W.player( e_nation::french )
      .old_world.market.commodities[e_commodity::rum]
      .player_traded_volume   = 100;
  global_rum.intrinsic_volume = 200;
  // Rum total: 400.

  W.player( e_nation::french )
      .old_world.market.commodities[e_commodity::cigars]
      .player_traded_volume      = 100;
  global_cigars.intrinsic_volume = 200;
  // Cigars total: 300.

  global_cloth.intrinsic_volume = 200;
  // Cloth total: 200.

  W.player( e_nation::english )
      .old_world.market.commodities[e_commodity::coats]
      .player_traded_volume = 100;
  W.player( e_nation::french )
      .old_world.market.commodities[e_commodity::coats]
      .player_traded_volume = 100;
  W.player( e_nation::spanish )
      .old_world.market.commodities[e_commodity::coats]
      .player_traded_volume     = 100;
  global_coats.intrinsic_volume = 100;
  // Coats total: 400.

  // These should be the equilibrium bid prices.
  int const starting_rum_bid    = 8;
  int const starting_cigars_bid = 12;
  int const starting_cloth_bid  = 18;
  int const starting_coats_bid  = 8;
  W.set_current_bid_price( e_commodity::rum, starting_rum_bid );
  W.set_current_bid_price( e_commodity::cigars,
                           starting_cigars_bid );
  W.set_current_bid_price( e_commodity::cloth,
                           starting_cloth_bid );
  W.set_current_bid_price( e_commodity::coats,
                           starting_coats_bid );

  auto curr_price = [&]( e_commodity c ) {
    return player.old_world.market.commodities[c].bid_price;
  };
  auto intrinsic_vol = [&]( e_commodity c ) {
    return player.old_world.market.commodities[c]
        .intrinsic_volume;
  };
  auto traded_vol = [&]( e_commodity c ) {
    return player.old_world.market.commodities[c]
        .player_traded_volume;
  };
  auto total_traded_vol = [&]( e_commodity c ) {
    int sum = 0;
    for( auto const& [nation, player] : W.ss().players.players )
      if( player.has_value() )
        sum += player->old_world.market.commodities[c]
                   .player_traded_volume;
    return sum;
  };

  refl::enum_map<e_commodity, PriceChange> changes;

  // Do the first evolution, which should not move the prices be-
  // cause we've set all of the prices to their equilibrium
  // values and the attrition isn't enough to move them.
  changes = evolve_player_prices( W.ss(), player );

  // None of the prices should have evolved.
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    REQUIRE( changes[c].type == c );
    REQUIRE( changes[c].delta == 0 );
    REQUIRE( changes[c].from == changes[c].to );
  }

  REQUIRE( curr_price( e_commodity::food ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::food ) == -2 );
  REQUIRE( traded_vol( e_commodity::food ) == 0 );

  REQUIRE( curr_price( e_commodity::sugar ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::sugar ) == -20 );
  REQUIRE( traded_vol( e_commodity::sugar ) == 0 );

  REQUIRE( curr_price( e_commodity::tobacco ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::tobacco ) == -24 );
  REQUIRE( traded_vol( e_commodity::tobacco ) == 0 );

  REQUIRE( curr_price( e_commodity::cotton ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::cotton ) == -24 );
  REQUIRE( traded_vol( e_commodity::cotton ) == 0 );

  REQUIRE( curr_price( e_commodity::furs ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::furs ) == -38 );
  REQUIRE( traded_vol( e_commodity::furs ) == 0 );

  REQUIRE( curr_price( e_commodity::lumber ) == 1 );
  REQUIRE( intrinsic_vol( e_commodity::lumber ) == 0 );
  REQUIRE( traded_vol( e_commodity::lumber ) == 0 );

  REQUIRE( curr_price( e_commodity::ore ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::ore ) == -11 );
  REQUIRE( traded_vol( e_commodity::ore ) == 0 );

  REQUIRE( curr_price( e_commodity::silver ) == 10 );
  REQUIRE( intrinsic_vol( e_commodity::silver ) == -12 );
  REQUIRE( traded_vol( e_commodity::silver ) == 0 );

  REQUIRE( curr_price( e_commodity::horses ) == 5 );
  REQUIRE( intrinsic_vol( e_commodity::horses ) == -5 );
  REQUIRE( traded_vol( e_commodity::horses ) == 0 );

  REQUIRE( curr_price( e_commodity::rum ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::rum ) == 0 );
  REQUIRE( traded_vol( e_commodity::rum ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::rum ) == 200 );

  REQUIRE( curr_price( e_commodity::cigars ) == 12 );
  REQUIRE( intrinsic_vol( e_commodity::cigars ) == 0 );
  REQUIRE( traded_vol( e_commodity::cigars ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cigars ) == 100 );

  REQUIRE( curr_price( e_commodity::cloth ) == 18 );
  REQUIRE( intrinsic_vol( e_commodity::cloth ) == 0 );
  REQUIRE( traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cloth ) == 0 );

  REQUIRE( curr_price( e_commodity::coats ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::coats ) == 0 );
  REQUIRE( traded_vol( e_commodity::coats ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::coats ) == 300 );

  REQUIRE( curr_price( e_commodity::trade_goods ) == 6 );
  REQUIRE( intrinsic_vol( e_commodity::trade_goods ) == 6 );
  REQUIRE( traded_vol( e_commodity::trade_goods ) == 0 );

  REQUIRE( curr_price( e_commodity::tools ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::tools ) == 8 );
  REQUIRE( traded_vol( e_commodity::tools ) == 0 );

  REQUIRE( curr_price( e_commodity::muskets ) == 10 );
  REQUIRE( intrinsic_vol( e_commodity::muskets ) == 9 );
  REQUIRE( traded_vol( e_commodity::muskets ) == 0 );

  // Now let's bump some of the volumes.
  auto vol = [&]( e_commodity c ) -> int& {
    return player.old_world.market.commodities[c]
        .intrinsic_volume;
  };

  // For the below we need to add the opposite of the attrition
  // in order to put the target where we want it. As a convenient
  // means for doing that, we will just negate the current val-
  // ues, since they hold the attritions from the evolution that
  // we just did.
  for( e_commodity c : refl::enum_values<e_commodity> )
    vol( c ) = -vol( c );

  vol( e_commodity::food ) += -300 + 1; // not quite a rise.
  vol( e_commodity::sugar ) += -400;    // just at a rise.
  vol( e_commodity::tobacco ) += -401;  // just over the rise.
  vol( e_commodity::cotton ) += -850; // more than twice a rise.
  vol( e_commodity::furs ) += 0;      // no change.
  vol( e_commodity::lumber ) += -500; // rise, but won't move.
  vol( e_commodity::ore ) += 399;     // just below fall.
  vol( e_commodity::silver ) += 100;  // just at fall.
  vol( e_commodity::horses ) += 201;  // just above fall.
  vol( e_commodity::rum )    = 9999; // shouldn't have an effect.
  vol( e_commodity::cigars ) = 9999; // shouldn't have an effect.
  vol( e_commodity::cloth ) = -9999; // shouldn't have an effect.
  vol( e_commodity::coats ) = -9999; // shouldn't have an effect.
  vol( e_commodity::trade_goods ) += 900; // three falls.
  vol( e_commodity::tools ) += 100;       // below a fall.
  vol( e_commodity::muskets ) += 200;     // just at a fall.

  global_rum.intrinsic_volume    = 0;
  global_cigars.intrinsic_volume = 200;
  global_cloth.intrinsic_volume  = 400;
  global_coats.intrinsic_volume  = 100;
  // Equilibrium prices should be:
  //   rum    = 18
  //   cigars = 12
  //   cloth  = 8
  //   coats  = 8
  // This basically swaps rum with cloth, which should cause
  // movement in only those two.

  // Second evolution.
  changes = evolve_player_prices( W.ss(), player );

  REQUIRE( curr_price( e_commodity::food ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::food ) == -299 );
  REQUIRE( traded_vol( e_commodity::food ) == 0 );
  REQUIRE( changes[e_commodity::food].delta == 2 - 2 );

  REQUIRE( curr_price( e_commodity::sugar ) == 5 );
  REQUIRE( intrinsic_vol( e_commodity::sugar ) == 0 );
  REQUIRE( traded_vol( e_commodity::sugar ) == 0 );
  REQUIRE( changes[e_commodity::sugar].delta == 5 - 4 );

  REQUIRE( curr_price( e_commodity::tobacco ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::tobacco ) == -1 );
  REQUIRE( traded_vol( e_commodity::tobacco ) == 0 );
  REQUIRE( changes[e_commodity::tobacco].delta == 3 - 2 );

  REQUIRE( curr_price( e_commodity::cotton ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::cotton ) == -450 );
  REQUIRE( traded_vol( e_commodity::cotton ) == 0 );
  REQUIRE( changes[e_commodity::cotton].delta == 3 - 2 );

  REQUIRE( curr_price( e_commodity::furs ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::furs ) == 0 );
  REQUIRE( traded_vol( e_commodity::furs ) == 0 );
  REQUIRE( changes[e_commodity::furs].delta == 3 - 3 );

  REQUIRE( curr_price( e_commodity::lumber ) == 1 );
  REQUIRE( intrinsic_vol( e_commodity::lumber ) == -200 );
  REQUIRE( traded_vol( e_commodity::lumber ) == 0 );
  REQUIRE( changes[e_commodity::lumber].delta == 1 - 1 );

  REQUIRE( curr_price( e_commodity::ore ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::ore ) == 399 );
  REQUIRE( traded_vol( e_commodity::ore ) == 0 );
  REQUIRE( changes[e_commodity::ore].delta == 3 - 3 );

  REQUIRE( curr_price( e_commodity::silver ) == 9 );
  REQUIRE( intrinsic_vol( e_commodity::silver ) == 0 );
  REQUIRE( traded_vol( e_commodity::silver ) == 0 );
  REQUIRE( changes[e_commodity::silver].delta == 9 - 10 );

  REQUIRE( curr_price( e_commodity::horses ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::horses ) == 1 );
  REQUIRE( traded_vol( e_commodity::horses ) == 0 );
  REQUIRE( changes[e_commodity::horses].delta == 4 - 5 );

  REQUIRE( curr_price( e_commodity::rum ) == 9 );
  REQUIRE( intrinsic_vol( e_commodity::rum ) == 9999 );
  REQUIRE( traded_vol( e_commodity::rum ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::rum ) == 200 );
  REQUIRE( changes[e_commodity::rum].delta == 1 );

  REQUIRE( curr_price( e_commodity::cigars ) == 12 );
  REQUIRE( intrinsic_vol( e_commodity::cigars ) == 9999 );
  REQUIRE( traded_vol( e_commodity::cigars ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cigars ) == 100 );
  REQUIRE( changes[e_commodity::cigars].delta == 0 );

  REQUIRE( curr_price( e_commodity::cloth ) == 17 );
  REQUIRE( intrinsic_vol( e_commodity::cloth ) == -9999 );
  REQUIRE( traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( changes[e_commodity::cloth].delta == -1 );

  REQUIRE( curr_price( e_commodity::coats ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::coats ) == -9999 );
  REQUIRE( traded_vol( e_commodity::coats ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::coats ) == 300 );
  REQUIRE( changes[e_commodity::coats].delta == 0 );

  REQUIRE( curr_price( e_commodity::trade_goods ) == 5 );
  REQUIRE( intrinsic_vol( e_commodity::trade_goods ) == 600 );
  REQUIRE( traded_vol( e_commodity::trade_goods ) == 0 );
  REQUIRE( changes[e_commodity::trade_goods].delta == 5 - 6 );

  REQUIRE( curr_price( e_commodity::tools ) == 4 );
  REQUIRE( intrinsic_vol( e_commodity::tools ) == 100 );
  REQUIRE( traded_vol( e_commodity::tools ) == 0 );
  REQUIRE( changes[e_commodity::tools].delta == 4 - 4 );

  REQUIRE( curr_price( e_commodity::muskets ) == 9 );
  REQUIRE( intrinsic_vol( e_commodity::muskets ) == 0 );
  REQUIRE( traded_vol( e_commodity::muskets ) == 0 );
  REQUIRE( changes[e_commodity::muskets].delta == 9 - 10 );

  // We'll do this again just for the processed goods.
  changes = evolve_player_prices( W.ss(), player );

  REQUIRE( curr_price( e_commodity::rum ) == 10 );
  REQUIRE( intrinsic_vol( e_commodity::rum ) == 9999 );
  REQUIRE( traded_vol( e_commodity::rum ) == 100 );
  REQUIRE( total_traded_vol( e_commodity::rum ) == 200 );
  REQUIRE( changes[e_commodity::rum].delta == 1 );

  REQUIRE( curr_price( e_commodity::cigars ) == 12 );
  REQUIRE( intrinsic_vol( e_commodity::cigars ) == 9999 );
  REQUIRE( traded_vol( e_commodity::cigars ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cigars ) == 100 );
  REQUIRE( changes[e_commodity::cigars].delta == 0 );

  REQUIRE( curr_price( e_commodity::cloth ) == 16 );
  REQUIRE( intrinsic_vol( e_commodity::cloth ) == -9999 );
  REQUIRE( traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::cloth ) == 0 );
  REQUIRE( changes[e_commodity::cloth].delta == -1 );

  REQUIRE( curr_price( e_commodity::coats ) == 8 );
  REQUIRE( intrinsic_vol( e_commodity::coats ) == -9999 );
  REQUIRE( traded_vol( e_commodity::coats ) == 0 );
  REQUIRE( total_traded_vol( e_commodity::coats ) == 300 );
  REQUIRE( changes[e_commodity::coats].delta == 0 );
}

// Tests that when we buy a processed good that is at is maximum
// price that the price doesn't go over the limit.
TEST_CASE(
    "[market] processed good buy remains within limits" ) {
  World W;
  W.set_default_player( e_nation::french );
  e_immediate_price_change_allowed const
      immediate_price_change_allowed =
          e_immediate_price_change_allowed::allowed;
  GlobalMarketItem& global_rum =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::rum];
  GlobalMarketItem& global_cigars =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::cigars];
  GlobalMarketItem& global_cloth =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::cloth];
  GlobalMarketItem& global_coats =
      W.ss()
          .players.global_market_state
          .commodities[e_commodity::coats];

  global_rum.intrinsic_volume    = 100;
  global_cigars.intrinsic_volume = 100;
  global_cloth.intrinsic_volume  = 10;
  global_coats.intrinsic_volume  = 100;

  W.set_current_bid_price( e_commodity::cloth, 19 );

  Invoice const invoice = transaction_invoice(
      W.ss(), W.player(),
      Commodity{ .type = e_commodity::cloth, .quantity = 100 },
      e_transaction::buy, immediate_price_change_allowed );

  REQUIRE( invoice.price_change.delta == 0 );
}

TEST_CASE(
    "[market] default model good buy remains within limits" ) {
  World W;
  W.set_default_player( e_nation::french );
  e_immediate_price_change_allowed const
      immediate_price_change_allowed =
          e_immediate_price_change_allowed::allowed;

  W.set_current_bid_price( e_commodity::muskets, 19 );

  // Do the first evolution, which should not move the prices be-
  // cause we've set all of the prices to their equilibrium
  // values and the attrition isn't enough to move them.
  Invoice const invoice = transaction_invoice(
      W.ss(), W.player(),
      Commodity{ .type     = e_commodity::muskets,
                 .quantity = 1000 },
      e_transaction::buy, immediate_price_change_allowed );

  REQUIRE( invoice.price_change.delta == 0 );
}

TEST_CASE( "[market] transaction_invoice buy" ) {
  World     W;
  Commodity to_buy;
  Invoice   expected;
  e_immediate_price_change_allowed
      immediate_price_change_allowed =
          e_immediate_price_change_allowed::allowed;

  auto f = [&] {
    return transaction_invoice( W.ss(), W.default_player(),
                                to_buy, e_transaction::buy,
                                immediate_price_change_allowed );
  };

  W.ss()
      .players.global_market_state.commodities[e_commodity::rum]
      .intrinsic_volume = 1000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::cigars]
      .intrinsic_volume = 1000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::cloth]
      .intrinsic_volume = 1000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::coats]
      .intrinsic_volume = 1000;

  SECTION( "human, conquistador, non-dutch" ) {
    W.set_default_player( e_nation::french );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_buy   = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -200 }, // volatility...
            { e_nation::french, -200 },
            { e_nation::spanish, -200 },
            { e_nation::dutch, -200 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, 1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "human, conquistador, dutch" ) {
    W.set_default_player( e_nation::dutch );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_buy   = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -200 }, // volatility...
            { e_nation::french, -200 },
            { e_nation::spanish, -200 },
            { e_nation::dutch, -200 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, 1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "human, discoverer, non-dutch" ) {
    W.set_default_player( e_nation::french );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::discoverer;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_buy   = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -133 }, // volatility...
            { e_nation::french, -133 },
            { e_nation::spanish, -133 },
            { e_nation::dutch, -133 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, 1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "human, discoverer, dutch" ) {
    W.set_default_player( e_nation::dutch );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::discoverer;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_buy   = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -133 }, // volatility...
            { e_nation::french, -133 },
            { e_nation::spanish, -133 },
            { e_nation::dutch, -133 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, 1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, conquistador, non-dutch" ) {
    W.set_default_player( e_nation::french );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::ore]
        .bid_price = 3;
    player.old_world.market.commodities[e_commodity::cigars]
        .bid_price = 10;

    to_buy   = { e_commodity::ore, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -6 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -6 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -33 },
            { e_nation::french, -33 },
            { e_nation::spanish, -33 },
            { e_nation::dutch, -33 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::ore, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cigars, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cigars, 1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, conquistador, dutch" ) {
    W.set_default_player( e_nation::dutch );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::muskets]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cigars]
        .bid_price = 10;

    to_buy   = { e_commodity::muskets, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -33 },
            { e_nation::french, -33 },
            { e_nation::spanish, -33 },
            { e_nation::dutch, -33 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::muskets, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cigars, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cigars, 1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, discoverer, non-dutch" ) {
    W.set_default_player( e_nation::french );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::muskets]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cigars]
        .bid_price = 10;

    to_buy   = { e_commodity::muskets, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -33 },
            { e_nation::french, -33 },
            { e_nation::spanish, -33 },
            { e_nation::dutch, -33 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::muskets, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cigars, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cigars, 1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, discoverer, dutch" ) {
    W.set_default_player( e_nation::dutch );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::muskets]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cigars]
        .bid_price = 10;

    to_buy   = { e_commodity::muskets, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -33 },
            { e_nation::french, -33 },
            { e_nation::spanish, -33 },
            { e_nation::dutch, -33 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::muskets, 0 ),
    };
    REQUIRE( f() == expected );

    to_buy   = { e_commodity::cigars, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cigars, 1 ),
    };
    REQUIRE( f() == expected );
  }

  SECTION( "price change suppression" ) {
    W.set_default_player( e_nation::french );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_buy   = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 50,
      .player_volume_delta      = -50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, -200 }, // volatility...
            { e_nation::french, -200 },
            { e_nation::spanish, -200 },
            { e_nation::dutch, -200 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::suppressed;
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::allowed;

    to_buy   = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_buy,
      .money_delta_before_taxes = -11 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 0,
      .money_delta_final        = -11 * 100,
      .player_volume_delta      = -100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, 1 ),
    };
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::suppressed;
    expected.price_change =
        create_price_change( player, e_commodity::cloth, 0 );
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::allowed;
  }
}

TEST_CASE( "[market] transaction_invoice sell" ) {
  World     W;
  Commodity to_sell;
  Invoice   expected;
  e_immediate_price_change_allowed
      immediate_price_change_allowed =
          e_immediate_price_change_allowed::allowed;

  auto f = [&] {
    return transaction_invoice( W.ss(), W.default_player(),
                                to_sell, e_transaction::sell,
                                immediate_price_change_allowed );
  };

  W.ss()
      .players.global_market_state.commodities[e_commodity::rum]
      .intrinsic_volume = 1000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::cigars]
      .intrinsic_volume = 1000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::cloth]
      .intrinsic_volume = 1000;
  W.ss()
      .players.global_market_state
      .commodities[e_commodity::coats]
      .intrinsic_volume = 1000;

  SECTION( "human, conquistador, non-dutch, tax=50" ) {
    W.set_default_player( e_nation::french );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 200 }, // volatility...
            { e_nation::french, 100 },  // price fell...
            { e_nation::spanish, 200 },
            { e_nation::dutch, 133 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, -1 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, -7 },
            { e_commodity::cigars, -7 },
            { e_commodity::cloth, -8 },
            { e_commodity::coats, -7 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "human, conquistador, dutch, tax=50" ) {
    W.set_default_player( e_nation::dutch );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 200 }, // volatility...
            { e_nation::french, 200 },  // price fell...
            { e_nation::spanish, 200 },
            { e_nation::dutch, 33 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, -1 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "human, discoverer, non-dutch, tax=50" ) {
    W.set_default_player( e_nation::french );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::discoverer;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 133 }, // volatility...
            { e_nation::french, 33 },   // price fell...
            { e_nation::spanish, 133 },
            { e_nation::dutch, 88 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, -1 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, -7 },
            { e_commodity::cigars, -7 },
            { e_commodity::cloth, -8 },
            { e_commodity::coats, -7 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "human, discoverer, dutch, tax=50" ) {
    W.set_default_player( e_nation::dutch );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::discoverer;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 133 }, // volatility...
            { e_nation::french, 133 },  // price fell...
            { e_nation::spanish, 133 },
            { e_nation::dutch, 88 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, conquistador, non-dutch, tax=50" ) {
    W.set_default_player( e_nation::french );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::ore]
        .bid_price = 3;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::ore, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 3 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 75,
      .money_delta_final        = 75,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 33 }, // volatility...
            { e_nation::french, 33 },  // price fell...
            { e_nation::spanish, 33 },
            { e_nation::dutch, 22 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::ore, 0 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, -7 },
            { e_commodity::cigars, -7 },
            { e_commodity::cloth, -8 },
            { e_commodity::coats, -7 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, conquistador, dutch, tax=50" ) {
    W.set_default_player( e_nation::dutch );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 133 }, // volatility...
            { e_nation::french, 133 },  // price fell...
            { e_nation::spanish, 133 },
            { e_nation::dutch, 88 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, discoverer, non-dutch, tax=50" ) {
    W.set_default_player( e_nation::french );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 133 }, // volatility...
            { e_nation::french, 33 },   // price fell...
            { e_nation::spanish, 133 },
            { e_nation::dutch, 88 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, -1 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, -7 },
            { e_commodity::cigars, -7 },
            { e_commodity::cloth, -8 },
            { e_commodity::coats, -7 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }
  SECTION( "AI, discoverer, dutch, tax=50" ) {
    W.set_default_player( e_nation::dutch );
    Player& player          = W.default_player();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 133 }, // volatility...
            { e_nation::french, 133 },  // price fell...
            { e_nation::spanish, 133 },
            { e_nation::dutch, 88 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, 0 ),
    };
    REQUIRE( f() == expected );

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, 0 },
            { e_commodity::cigars, 0 },
            { e_commodity::cloth, 0 },
            { e_commodity::coats, 0 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
  }

  SECTION( "price change suppression" ) {
    W.set_default_player( e_nation::french );
    Player& player = W.default_player();
    W.set_default_player_as_human();
    W.settings().difficulty = e_difficulty::conquistador;
    W.set_tax_rate( 50 );

    player.old_world.market.commodities[e_commodity::silver]
        .bid_price = 10;
    player.old_world.market.commodities[e_commodity::cloth]
        .bid_price = 10;

    to_sell  = { e_commodity::silver, 50 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 50,
      .tax_rate                 = 50,
      .tax_amount               = 250,
      .money_delta_final        = 250,
      .player_volume_delta      = 50,
      .intrinsic_volume_delta =
          {
            { e_nation::english, 200 }, // volatility...
            { e_nation::french, 100 },  // price fell...
            { e_nation::spanish, 200 },
            { e_nation::dutch, 133 },
          },
      .global_intrinsic_volume_deltas = {},
      .price_change =
          create_price_change( player, e_commodity::silver, -1 ),
    };
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::suppressed;
    expected.price_change =
        create_price_change( player, e_commodity::silver, 0 );
    // Since the price was not adjusted, the intrinsic volume of
    // the french won't have been adjusted either.
    expected.intrinsic_volume_delta[e_nation::french] = 200;
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::allowed;

    to_sell  = { e_commodity::cloth, 100 };
    expected = {
      .what                     = to_sell,
      .money_delta_before_taxes = 10 * 100,
      .tax_rate                 = 50,
      .tax_amount               = 500,
      .money_delta_final        = 500,
      .player_volume_delta      = 100,
      .intrinsic_volume_delta   = {},
      .global_intrinsic_volume_deltas =
          {
            { e_commodity::rum, -7 },
            { e_commodity::cigars, -7 },
            { e_commodity::cloth, -8 },
            { e_commodity::coats, -7 },
          },
      .price_change =
          create_price_change( player, e_commodity::cloth, -1 ),
    };
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::suppressed;
    expected.price_change =
        create_price_change( player, e_commodity::cloth, 0 );
    REQUIRE( f() == expected );
    immediate_price_change_allowed =
        e_immediate_price_change_allowed::allowed;
  }
}

TEST_CASE( "[market] price_limits_for_commodity" ) {
  e_commodity in       = {};
  PriceLimits expected = {};

  auto f = [&] { return price_limits_for_commodity( in ); };

  in       = e_commodity::silver;
  expected = { .low  = { .bid = 1, .ask = 2 },
               .high = { .bid = 19, .ask = 20 } };
  REQUIRE( f() == expected );

  in       = e_commodity::ore;
  expected = { .low  = { .bid = 1, .ask = 4 },
               .high = { .bid = 5, .ask = 8 } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[market] attrition bonus" ) {
  World W;
  W.set_default_player( e_nation::french );
  Player& player = W.default_player();
  W.init_prices_to_average();
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    if( is_in_processed_goods_price_group( c ) ) continue;
    player.old_world.market.commodities[c].intrinsic_volume =
        1000;
  }

  (void)evolve_player_prices( W.ss(), player );

  auto iv = [&]( e_commodity comm ) {
    return player.old_world.market.commodities[comm]
        .intrinsic_volume;
  };

  // These are the ones that have an attrition bonus.
  REQUIRE( iv( e_commodity::sugar ) == 387 );
  REQUIRE( iv( e_commodity::tobacco ) == 184 );
  REQUIRE( iv( e_commodity::cotton ) == 384 );
  REQUIRE( iv( e_commodity::furs ) == 975 );

  REQUIRE( iv( e_commodity::food ) == 799 );
  REQUIRE( iv( e_commodity::lumber ) == 800 );
  REQUIRE( iv( e_commodity::ore ) == 593 );
  REQUIRE( iv( e_commodity::silver ) == 892 );
  REQUIRE( iv( e_commodity::horses ) == 797 );
  REQUIRE( iv( e_commodity::trade_goods ) == 704 );
  REQUIRE( iv( e_commodity::tools ) == 805 );
  REQUIRE( iv( e_commodity::muskets ) == 806 );
}

} // namespace
} // namespace rn
