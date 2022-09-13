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

// ss
#include "ss/player.hpp"

// Testing
#include "test/fake/world.hpp"

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
    add_player( e_nation::french );
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

TEST_CASE( "[market] purchase_invoice" ) {
  World           W;
  Player&         player = W.default_player();
  Commodity       input;
  PurchaseInvoice expected;

  auto f = [&] { return purchase_invoice( player, input ); };

  W.set_current_bid_price( e_commodity::horses, 4 );
  W.set_current_bid_price( e_commodity::food, 2 );

  input    = { .type = e_commodity::horses, .quantity = 50 };
  expected = { .purchased = input, .cost = ( 4 + 1 ) * 100 / 2 };
  REQUIRE( f() == expected );

  input    = { .type = e_commodity::food, .quantity = 100 };
  expected = { .purchased = input, .cost = ( 2 + 8 ) * 100 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[market] sale_invoice" ) {
  World       W;
  Player&     player = W.default_player();
  Commodity   input;
  SaleInvoice expected = {};

  auto f = [&] { return sale_invoice( player, input ); };

  W.set_current_bid_price( e_commodity::horses, 4 );
  W.set_current_bid_price( e_commodity::food, 2 );

  // Zero tax rate.
  W.set_tax_rate( 0 );

  input    = { .type = e_commodity::horses, .quantity = 50 };
  expected = SaleInvoice{ .sold                  = input,
                          .received_before_taxes = 4 * 100 / 2,
                          .tax_rate              = 0,
                          .tax_amount            = 0,
                          .received_final        = 4 * 100 / 2 };
  REQUIRE( f() == expected );

  input    = { .type = e_commodity::food, .quantity = 100 };
  expected = SaleInvoice{ .sold                  = input,
                          .received_before_taxes = 2 * 100,
                          .tax_rate              = 0,
                          .tax_amount            = 0,
                          .received_final        = 2 * 100 };
  REQUIRE( f() == expected );

  // 7% tax rate.
  W.set_tax_rate( 7 );

  input = { .type = e_commodity::horses, .quantity = 50 };
  expected =
      SaleInvoice{ .sold                  = input,
                   .received_before_taxes = 4 * 100 / 2,
                   .tax_rate              = 7,
                   .tax_amount            = 7 * 4 / 2,
                   .received_final = 4 * 100 / 2 - 7 * 4 / 2 };
  REQUIRE( f() == expected );

  input    = { .type = e_commodity::food, .quantity = 100 };
  expected = SaleInvoice{ .sold                  = input,
                          .received_before_taxes = 2 * 100,
                          .tax_rate              = 7,
                          .tax_amount            = 7 * 2,
                          .received_final = 2 * 100 - 7 * 2 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[market] compute_equilibrium_prices" ) {
  // TODO
}

TEST_CASE( "[market] compute_price_change" ) {
  // TODO
}

TEST_CASE( "[market] buy_comomdity_from_harbor" ) {
  // TODO
}

TEST_CASE( "[market] sell_comomdity_from_harbor" ) {
  // TODO
}

TEST_CASE( "[market] evolve_volume" ) {
  // TODO
}

} // namespace
} // namespace rn
