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

TEST_CASE( "[market] transaction_invoice" ) {
  World W;
  // TODO
}

TEST_CASE( "[market] evolve_player_prices" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
