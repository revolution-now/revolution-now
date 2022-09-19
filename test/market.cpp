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

// refl
#include "refl/to-str.hpp"

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
  EXPECT_CALL( W.gui(),
               message_box( "The price of @[H]ore@[] in La "
                            "Rochelle has risen to 13." )
                   .returns( make_wait<>() ) );
  w = f();
  REQUIRE_FALSE( w.exception() );
  REQUIRE( w.ready() );

  change = create_price_change( W.default_player(),
                                e_commodity::ore, -1 );
  EXPECT_CALL( W.gui(),
               message_box( "The price of @[H]ore@[] in La "
                            "Rochelle has fallen to 9." )
                   .returns( make_wait<>() ) );
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
  REQUIRE( intrinsic_vol( e_commodity::sugar ) == -8 );
  REQUIRE( traded_vol( e_commodity::sugar ) == 0 );

  REQUIRE( curr_price( e_commodity::tobacco ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::tobacco ) == -10 );
  REQUIRE( traded_vol( e_commodity::tobacco ) == 0 );

  REQUIRE( curr_price( e_commodity::cotton ) == 2 );
  REQUIRE( intrinsic_vol( e_commodity::cotton ) == -11 );
  REQUIRE( traded_vol( e_commodity::cotton ) == 0 );

  REQUIRE( curr_price( e_commodity::fur ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::fur ) == -13 );
  REQUIRE( traded_vol( e_commodity::fur ) == 0 );

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
  vol( e_commodity::fur ) += 0;       // no change.
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

  REQUIRE( curr_price( e_commodity::fur ) == 3 );
  REQUIRE( intrinsic_vol( e_commodity::fur ) == 0 );
  REQUIRE( traded_vol( e_commodity::fur ) == 0 );
  REQUIRE( changes[e_commodity::fur].delta == 3 - 3 );

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

TEST_CASE( "[market] transaction_invoice" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
