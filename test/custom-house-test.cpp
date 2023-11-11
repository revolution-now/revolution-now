/****************************************************************
**custom-house.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-22.
*
* Description: Unit tests for the src/custom-house.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/custom-house.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"

// Revolution Now
#include "src/market.hpp"

// config
#include "config/market.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"

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
    add_player( e_nation::dutch );
    add_player( e_nation::french );
    set_default_player( e_nation::dutch );
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
TEST_CASE( "[custom-house] set_default_custom_house_state" ) {
  World W;
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{} );

  // Sanity check.
  for( auto& [comm, on] : colony.custom_house ) {
    INFO( fmt::format( "commodity: {}", comm ) );
    REQUIRE( !on );
  }

  set_default_custom_house_state( colony );

  REQUIRE_FALSE( colony.custom_house[e_commodity::food] );
  REQUIRE( colony.custom_house[e_commodity::sugar] );
  REQUIRE( colony.custom_house[e_commodity::tobacco] );
  REQUIRE( colony.custom_house[e_commodity::cotton] );
  REQUIRE( colony.custom_house[e_commodity::furs] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::lumber] );
  REQUIRE( colony.custom_house[e_commodity::ore] );
  REQUIRE( colony.custom_house[e_commodity::silver] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::horses] );
  REQUIRE( colony.custom_house[e_commodity::rum] );
  REQUIRE( colony.custom_house[e_commodity::cigars] );
  REQUIRE( colony.custom_house[e_commodity::cloth] );
  REQUIRE( colony.custom_house[e_commodity::coats] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::trade_goods] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::tools] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::muskets] );
}

TEST_CASE( "[custom-house] apply_custom_house_sales" ) {
  World   W;
  Player& player = W.default_player();
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{} );
  CustomHouseSales sales;

  auto f = [&] {
    apply_custom_house_sales( W.ss(), player, colony, sales );
  };

  player.money = 1000;

  for( e_commodity comm : refl::enum_values<e_commodity> ) {
    colony.commodities[comm]                            = 150;
    player.old_world.market.commodities[comm].bid_price = 10;
  }

  // Make sure that it supports selling boycotted goods just in
  // case the relevant config flag (`respect_boycotts`) is off.
  W.player( e_nation::dutch )
      .old_world.market.commodities[e_commodity::sugar]
      .boycott = true;
  W.player( e_nation::french )
      .old_world.market.commodities[e_commodity::sugar]
      .boycott = true;

  sales.invoices = {
      Invoice{
          .what = Commodity{ .type     = e_commodity::sugar,
                             .quantity = 100 },
          .money_delta_before_taxes = 100,
          .tax_rate                 = 7,
          .tax_amount               = 7,
          .money_delta_final        = 93,
          .player_volume_delta      = 100,
          .intrinsic_volume_delta   = { { e_nation::dutch, 1 },
                                        { e_nation::french, 1 } },
          .global_intrinsic_volume_deltas =
              { { e_commodity::sugar, 2 } },
          .price_change = create_price_change(
              W.default_player(), e_commodity::sugar,
              /*price_change=*/0 ) },
      Invoice{
          .what = Commodity{ .type     = e_commodity::rum,
                             .quantity = 200 },
          .money_delta_before_taxes = 200,
          .tax_rate                 = 14,
          .tax_amount               = 14,
          .money_delta_final        = 186,
          .player_volume_delta      = 200,
          .intrinsic_volume_delta   = { { e_nation::dutch, 2 },
                                        { e_nation::french, 2 } },
          .global_intrinsic_volume_deltas = { { e_commodity::rum,
                                                4 } },
          .price_change                   = create_price_change(
              W.default_player(), e_commodity::sugar,
              /*price_change=*/0 ) } };
  f();
  REQUIRE( player.money == 1000 + 186 + 93 );
  for( e_commodity comm : refl::enum_values<e_commodity> ) {
    INFO( fmt::format( "comm: {}", comm ) );
    if( comm == e_commodity::sugar ) {
      REQUIRE( colony.commodities[comm] == 50 );
      REQUIRE( W.player( e_nation::french )
                   .old_world.market.commodities[comm]
                   .intrinsic_volume == 1 );
      REQUIRE( W.player( e_nation::dutch )
                   .old_world.market.commodities[comm]
                   .intrinsic_volume == 1 );
      REQUIRE( W.players()
                   .global_market_state.commodities[comm]
                   .intrinsic_volume == 2 );
      REQUIRE(
          player.old_world.market.commodities[comm].bid_price ==
          10 );
    } else if( comm == e_commodity::rum ) {
      REQUIRE( colony.commodities[comm] == 50 );
      REQUIRE( W.player( e_nation::french )
                   .old_world.market.commodities[comm]
                   .intrinsic_volume == 2 );
      REQUIRE( W.player( e_nation::dutch )
                   .old_world.market.commodities[comm]
                   .intrinsic_volume == 2 );
      REQUIRE( W.players()
                   .global_market_state.commodities[comm]
                   .intrinsic_volume == 4 );
      REQUIRE(
          player.old_world.market.commodities[comm].bid_price ==
          10 );
    } else {
      REQUIRE( colony.commodities[comm] == 150 );
      REQUIRE( W.player( e_nation::french )
                   .old_world.market.commodities[comm]
                   .intrinsic_volume == 0 );
      REQUIRE( W.player( e_nation::dutch )
                   .old_world.market.commodities[comm]
                   .intrinsic_volume == 0 );
      REQUIRE( W.players()
                   .global_market_state.commodities[comm]
                   .intrinsic_volume == 0 );
      REQUIRE(
          player.old_world.market.commodities[comm].bid_price ==
          10 );
    }
  }
}

TEST_CASE( "[custom-house] compute_custom_house_sales" ) {
  World W;
  W.settings().difficulty = e_difficulty::conquistador;
  Player& dutch           = W.dutch();
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{} );
  CustomHouseSales expected;
  colony.buildings[e_colony_building::custom_house] = true;
  for( e_commodity comm : refl::enum_values<e_commodity> ) {
    colony.commodities[comm] = 200;
    dutch.old_world.market.commodities[comm].bid_price =
        config_market.price_behavior[comm]
            .price_limits.bid_price_max;
  }
  colony.commodities[e_commodity::cloth]   = 100;
  colony.commodities[e_commodity::muskets] = 99;
  dutch.old_world.taxes.tax_rate           = 10;

  W.players()
      .global_market_state.commodities[e_commodity::cloth]
      .intrinsic_volume = 1000;

  SECTION( "independence not declared" ) {
    colony.custom_house[e_commodity::furs]    = true;
    colony.custom_house[e_commodity::ore]     = true;
    colony.custom_house[e_commodity::cloth]   = true;
    colony.custom_house[e_commodity::muskets] = true;
    dutch.revolution_status = e_revolution_status::not_declared;
    W.set_human_player( e_nation::dutch );
    CustomHouseSales const res =
        compute_custom_house_sales( W.ss(), dutch, colony );
    expected =
        {
            .invoices =
                {
                    Invoice{
                        .what =
                            Commodity{ .type = e_commodity::furs,
                                       .quantity = 150 },
                        .money_delta_before_taxes = 150 * 5,
                        .tax_rate                 = 10,
                        .tax_amount               = 75,
                        .money_delta_final        = 675,
                        .player_volume_delta      = 150,
                        .intrinsic_volume_delta =
                            { { e_nation::dutch, 199 },
                              { e_nation::french, 300 } },
                        .global_intrinsic_volume_deltas = {
                            /*only processed goods*/ },
                        .price_change = create_price_change(
                            dutch, e_commodity::furs,
                            /*price_change=*/0 ) },
                    Invoice{
                        .what =
                            Commodity{ .type = e_commodity::ore,
                                       .quantity = 150 },
                        .money_delta_before_taxes = 150 * 5,
                        .tax_rate                 = 10,
                        .tax_amount               = 75,
                        .money_delta_final        = 675,
                        .player_volume_delta      = 150,
                        .intrinsic_volume_delta =
                            { { e_nation::dutch, 99 },
                              { e_nation::french, 150 } },
                        .global_intrinsic_volume_deltas = {
                            /*only processed goods*/ },
                        .price_change = create_price_change(
                            dutch, e_commodity::ore,
                            /*price_change=*/0 ) },
                    Invoice{ .what =
                                 Commodity{
                                     .type = e_commodity::cloth,
                                     .quantity = 50 },
                             .money_delta_before_taxes = 50 * 19,
                             .tax_rate                 = 10,
                             .tax_amount               = 95,
                             .money_delta_final   = 50 * 19 - 95,
                             .player_volume_delta = 50,
                             .intrinsic_volume_delta = {
                                 /*non-processed goods only*/ },
                             .global_intrinsic_volume_deltas = { /*these don't evolve when the dutch make the trade.*/ },
                             .price_change = create_price_change(
                                 dutch, e_commodity::cloth,
                                 /*price_change=*/0 ) },
                },
            .boycotted = {},
        };
    REQUIRE( res == expected );
  }

  SECTION( "declared independence" ) {
    // Charge (in place of tax rate) is 50%.
    dutch.revolution_status = e_revolution_status::declared;
    // One should be enough here.
    colony.custom_house[e_commodity::furs] = true;
    W.set_human_player( e_nation::dutch );
    CustomHouseSales const res =
        compute_custom_house_sales( W.ss(), dutch, colony );
    expected = {
        .invoices =
            {
                Invoice{
                    .what = Commodity{ .type = e_commodity::furs,
                                       .quantity = 150 },
                    .money_delta_before_taxes = 150 * 5,
                    .tax_rate                 = 50,
                    .tax_amount               = 375,
                    .money_delta_final        = 150 * 5 - 375,
                    .player_volume_delta      = 150,
                    .intrinsic_volume_delta =
                        { { e_nation::dutch, 199 },
                          { e_nation::french, 300 } },
                    .global_intrinsic_volume_deltas = {
                        /*only processed goods*/ },
                    .price_change = create_price_change(
                        dutch, e_commodity::furs,
                        /*price_change=*/0 ) },
            },
        .boycotted = {},
    };
    REQUIRE( res == expected );
  }

  SECTION( "respects boycotts" ) {
    W.default_player()
        .old_world.market.commodities[e_commodity::ore]
        .boycott                              = true;
    colony.custom_house[e_commodity::furs]    = true;
    colony.custom_house[e_commodity::ore]     = true;
    colony.custom_house[e_commodity::cloth]   = true;
    colony.custom_house[e_commodity::muskets] = true;
    dutch.revolution_status = e_revolution_status::not_declared;
    W.set_human_player( e_nation::dutch );
    CustomHouseSales const res =
        compute_custom_house_sales( W.ss(), dutch, colony );
    expected =
        {
            .invoices =
                {
                    Invoice{
                        .what =
                            Commodity{ .type = e_commodity::furs,
                                       .quantity = 150 },
                        .money_delta_before_taxes = 150 * 5,
                        .tax_rate                 = 10,
                        .tax_amount               = 75,
                        .money_delta_final        = 675,
                        .player_volume_delta      = 150,
                        .intrinsic_volume_delta =
                            { { e_nation::dutch, 199 },
                              { e_nation::french, 300 } },
                        .global_intrinsic_volume_deltas = {
                            /*only processed goods*/ },
                        .price_change = create_price_change(
                            dutch, e_commodity::furs,
                            /*price_change=*/0 ) },
                    /* !! ore omitted due to boycott */
                    Invoice{ .what =
                                 Commodity{
                                     .type = e_commodity::cloth,
                                     .quantity = 50 },
                             .money_delta_before_taxes = 50 * 19,
                             .tax_rate                 = 10,
                             .tax_amount               = 95,
                             .money_delta_final   = 50 * 19 - 95,
                             .player_volume_delta = 50,
                             .intrinsic_volume_delta = {
                                 /*non-processed goods only*/ },
                             .global_intrinsic_volume_deltas = { /*these don't evolve when the dutch make the trade.*/ },
                             .price_change = create_price_change(
                                 dutch, e_commodity::cloth,
                                 /*price_change=*/0 ) },
                },
            .boycotted = { e_commodity::ore },
        };
    REQUIRE( res == expected );
  }
}

// This tests that a custom house selling silver will cause the
// price to drop at the proper rate for a non-dutch nation.
TEST_CASE(
    "[custom-house] custom house affects french market "
    "properly" ) {
  World W;
  // Expected values.
  CustomHouseSales expected;
  PriceChange      expected_silver_change;
  // Init settings.
  W.settings().difficulty = e_difficulty::conquistador;
  // Init player.
  Player& french = W.french();
  W.set_human_player( e_nation::french );
  french.revolution_status = e_revolution_status::not_declared;
  // Init colony.
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{} );
  colony.buildings[e_colony_building::custom_house] = true;
  colony.commodities[e_commodity::silver]           = 150;
  colony.custom_house[e_commodity::silver]          = true;
  // Init market prices.
  for( e_commodity comm : refl::enum_values<e_commodity> )
    french.old_world.market.commodities[comm].bid_price =
        config_market.price_behavior[comm]
            .price_limits.bid_price_max;
  int& silver_bid =
      french.old_world.market.commodities[e_commodity::silver]
          .bid_price;

  CustomHouseSales const res =
      compute_custom_house_sales( W.ss(), french, colony );
  expected = {
      .invoices = {
          Invoice{
              .what = Commodity{ .type     = e_commodity::silver,
                                 .quantity = 100 },
              .money_delta_before_taxes = 100 * 19,
              .tax_rate                 = 0,
              .tax_amount               = 0,
              .money_delta_final        = 1900,
              .player_volume_delta      = 100,
              .intrinsic_volume_delta =
                  { { e_nation::dutch, 266 },
                    { e_nation::french, 400 } },
              .global_intrinsic_volume_deltas = {
                  /*only processed goods*/ },
              .price_change = create_price_change(
                  french, e_commodity::silver,
                  /*price_change=*/0 ) },
      } };
  REQUIRE( res == expected );

  // Now apply the changes so that the price will start falling.
  apply_custom_house_sales( W.ss(), french, colony, res );

  auto evolve = [&] {
    auto changes = evolve_player_prices( W.ss(), french );
    return changes[e_commodity::silver];
  };

  REQUIRE( silver_bid == 19 );

  expected_silver_change =
      create_price_change( french, e_commodity::silver, -1 );
  REQUIRE( evolve() == expected_silver_change );
  REQUIRE( silver_bid == 18 );

  expected_silver_change =
      create_price_change( french, e_commodity::silver, -1 );
  REQUIRE( evolve() == expected_silver_change );
  REQUIRE( silver_bid == 17 );

  expected_silver_change =
      create_price_change( french, e_commodity::silver, -1 );
  REQUIRE( evolve() == expected_silver_change );
  REQUIRE( silver_bid == 16 );

  // Stops evolving.
  expected_silver_change =
      create_price_change( french, e_commodity::silver, 0 );
  REQUIRE( evolve() == expected_silver_change );
  REQUIRE( silver_bid == 16 );
}

// This tests that a custom house selling silver will cause the
// price to drop at the proper rate for the dutch.
TEST_CASE(
    "[custom-house] custom house affects dutch market "
    "properly" ) {
  World W;
  // Expected values.
  CustomHouseSales expected;
  PriceChange      expected_silver_change;
  // Init settings.
  W.settings().difficulty = e_difficulty::conquistador;
  // Init player.
  Player& dutch = W.dutch();
  W.set_human_player( e_nation::dutch );
  dutch.revolution_status = e_revolution_status::not_declared;
  // Init colony.
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{} );
  colony.buildings[e_colony_building::custom_house] = true;
  colony.commodities[e_commodity::silver]           = 150;
  colony.custom_house[e_commodity::silver]          = true;
  // Init market prices.
  for( e_commodity comm : refl::enum_values<e_commodity> )
    dutch.old_world.market.commodities[comm].bid_price =
        config_market.price_behavior[comm]
            .price_limits.bid_price_max;
  int& silver_bid =
      dutch.old_world.market.commodities[e_commodity::silver]
          .bid_price;

  CustomHouseSales const res =
      compute_custom_house_sales( W.ss(), dutch, colony );
  expected = {
      .invoices = {
          Invoice{
              .what = Commodity{ .type     = e_commodity::silver,
                                 .quantity = 100 },
              .money_delta_before_taxes = 100 * 19,
              .tax_rate                 = 0,
              .tax_amount               = 0,
              .money_delta_final        = 1900,
              .player_volume_delta      = 100,
              .intrinsic_volume_delta =
                  { { e_nation::dutch, 266 },
                    { e_nation::french, 400 } },
              .global_intrinsic_volume_deltas = {
                  /*only processed goods*/ },
              .price_change = create_price_change(
                  dutch, e_commodity::silver,
                  /*price_change=*/0 ) },
      } };
  REQUIRE( res == expected );

  // Now apply the changes so that the price will start falling.
  apply_custom_house_sales( W.ss(), dutch, colony, res );

  auto evolve = [&] {
    auto changes = evolve_player_prices( W.ss(), dutch );
    return changes[e_commodity::silver];
  };

  REQUIRE( silver_bid == 19 );

  expected_silver_change =
      create_price_change( dutch, e_commodity::silver, -1 );
  REQUIRE( evolve() == expected_silver_change );
  REQUIRE( silver_bid == 18 );

  expected_silver_change =
      create_price_change( dutch, e_commodity::silver, -1 );
  REQUIRE( evolve() == expected_silver_change );
  REQUIRE( silver_bid == 17 );

  // Stops evolving.
  expected_silver_change =
      create_price_change( dutch, e_commodity::silver, 0 );
  REQUIRE( evolve() == expected_silver_change );
  REQUIRE( silver_bid == 17 );
}

} // namespace
} // namespace rn
