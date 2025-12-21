/****************************************************************
**harbor-view-market-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-16.
*
* Description: Unit tests for the harbor-view-market module.
*
*****************************************************************/
#include "frame.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/harbor-view-market.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/co-scheduler.hpp"
#include "src/frame.hpp"
#include "src/harbor-view-status.hpp"

// ss
#include "src/ss/old-world-state.rds.hpp"
#include "src/ss/player.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
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
TEST_CASE( "[harbor-view-market] unload_one" ) {
  world w;
  rect const canvas{ .size = { .w = 640, .h = 360 } };
  vector<pair<Commodity, int>> expected;

  auto const p_status_bar = HarborStatusBar::create(
      w.engine(), w.ss(), w.ts(), w.default_player(), canvas );
  auto const p_market = HarborMarketCommodities::create(
      w.engine(), w.ss(), w.ts(), w.default_player(), canvas,
      *p_status_bar.actual );
  auto& market_ref = *p_market.actual;

  auto& money = w.default_player().money;

  auto const f = [&] {
    co_await_test( market_ref.unload_one() );
  };

  auto& selected_unit_id =
      w.old_world().harbor_state.selected_unit;

  selected_unit_id = nothing;
  f();
  REQUIRE( money == 0 );

  Unit& galleon = w.add_unit_in_port( e_unit_type::galleon );

  w.set_current_bid_price( e_commodity::ore, 5 );
  w.set_current_bid_price( e_commodity::silver, 19 );
  w.set_current_bid_price( e_commodity::food, 0 );
  w.set_current_bid_price( e_commodity::muskets, 6 );
  w.set_current_bid_price( e_commodity::trade_goods, 3 );

  // pre-tax value = 5*50 = 250.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::ore, .quantity = 50 },
      galleon.cargo(), /*slot=*/0,
      /*try_other_slots=*/false );

  // pre-tax value = 19*1 = 19.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::silver, .quantity = 1 },
      galleon.cargo(), /*slot=*/1,
      /*try_other_slots=*/false );

  // pre-tax value = 0*100 = 0.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::food, .quantity = 100 },
      galleon.cargo(), /*slot=*/2,
      /*try_other_slots=*/false );

  // pre-tax value = 6*100 = 600.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::muskets, .quantity = 100 },
      galleon.cargo(), /*slot=*/4,
      /*try_other_slots=*/false );

  // pre-tax value = 3*100 = 300.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type     = e_commodity::trade_goods,
                 .quantity = 100 },
      galleon.cargo(), /*slot=*/5,
      /*try_other_slots=*/false );

  money = 0;
  w.old_world()
      .market.commodities[e_commodity::muskets]
      .boycott = true;

  expected = {
    { { .type = e_commodity::ore, .quantity = 50 }, 0 },
    { { .type = e_commodity::silver, .quantity = 1 }, 1 },
    { { .type = e_commodity::food, .quantity = 100 }, 2 },
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
    { { .type = e_commodity::trade_goods, .quantity = 100 }, 5 },
  };
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 0 );

  expected = {
    { { .type = e_commodity::ore, .quantity = 50 }, 0 },
    { { .type = e_commodity::silver, .quantity = 1 }, 1 },
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
    { { .type = e_commodity::trade_goods, .quantity = 100 }, 5 },
  };
  f();
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 0 );

  expected = {
    { { .type = e_commodity::ore, .quantity = 50 }, 0 },
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
    { { .type = e_commodity::trade_goods, .quantity = 100 }, 5 },
  };
  f();
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 19 );

  expected = {
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
    { { .type = e_commodity::trade_goods, .quantity = 100 }, 5 },
  };
  f();
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 269 );

  expected = {
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
  };
  f();
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 569 );

  expected = {
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
  };
  w.gui().EXPECT__message_box( StrContains( "boycott" ) );
  f();
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 569 );

  money = 10000;

  expected = {
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
  };
  w.gui().EXPECT__choice( _ ).returns<maybe<string>>( "no" );
  f();
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 10000 );

  expected = {};
  w.gui().EXPECT__choice( _ ).returns<maybe<string>>( "yes" );
  f();
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 7100 );
}

TEST_CASE( "[harbor-view-market] unload_all" ) {
  world w;
  rect const canvas{ .size = { .w = 640, .h = 360 } };
  vector<pair<Commodity, int>> expected;

  auto const p_status_bar = HarborStatusBar::create(
      w.engine(), w.ss(), w.ts(), w.default_player(), canvas );
  auto const p_market = HarborMarketCommodities::create(
      w.engine(), w.ss(), w.ts(), w.default_player(), canvas,
      *p_status_bar.actual );
  auto& market_ref = *p_market.actual;

  auto& money = w.default_player().money;

  auto& selected_unit_id =
      w.old_world().harbor_state.selected_unit;

  selected_unit_id = nothing;

  Unit& galleon = w.add_unit_in_port( e_unit_type::galleon );

  w.set_current_bid_price( e_commodity::ore, 5 );
  w.set_current_bid_price( e_commodity::silver, 19 );
  w.set_current_bid_price( e_commodity::food, 0 );
  w.set_current_bid_price( e_commodity::muskets, 6 );
  w.set_current_bid_price( e_commodity::trade_goods, 3 );

  // pre-tax value = 5*50 = 250.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::ore, .quantity = 50 },
      galleon.cargo(), /*slot=*/0,
      /*try_other_slots=*/false );

  // pre-tax value = 19*1 = 19.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::silver, .quantity = 1 },
      galleon.cargo(), /*slot=*/1,
      /*try_other_slots=*/false );

  // pre-tax value = 0*100 = 0.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::food, .quantity = 100 },
      galleon.cargo(), /*slot=*/2,
      /*try_other_slots=*/false );

  // pre-tax value = 6*100 = 600.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::muskets, .quantity = 100 },
      galleon.cargo(), /*slot=*/4,
      /*try_other_slots=*/false );

  // pre-tax value = 3*100 = 300.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type     = e_commodity::trade_goods,
                 .quantity = 100 },
      galleon.cargo(), /*slot=*/5,
      /*try_other_slots=*/false );

  w.old_world()
      .market.commodities[e_commodity::muskets]
      .boycott = true;

  expected = {
    { { .type = e_commodity::muskets, .quantity = 100 }, 4 },
  };
  w.gui().EXPECT__message_box( StrContains( "boycott" ) );
  wait<> wt = market_ref.unload_all();
  REQUIRE( !wt.ready() );
  testing_notify_all_subscribers();
  run_all_cpp_coroutines();
  REQUIRE( !wt.ready() );
  testing_notify_all_subscribers();
  run_all_cpp_coroutines();
  REQUIRE( !wt.ready() );
  testing_notify_all_subscribers();
  run_all_cpp_coroutines();
  co_await_test( std::move( wt ) );
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 569 );

  w.old_world()
      .market.commodities[e_commodity::muskets]
      .boycott = false;
  expected     = {};
  co_await_test( market_ref.unload_all() );
  REQUIRE( galleon.cargo().commodities() == expected );
  REQUIRE( money == 1169 );
}

} // namespace
} // namespace rn
