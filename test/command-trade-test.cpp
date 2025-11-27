/****************************************************************
**command-trade-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Unit tests for the command-trade module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/command-trade.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// ss
#include "ss/trade-route.rds.hpp"
#include "ss/unit-composition.hpp"
#include "ss/unit.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::Field;
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
    static MapSquare const _ = make_ocean();
    static MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,X,_, /*2
      3*/ _,X,X,X,X,X,X,_, /*3
      4*/ _,X,X,X,X,X,X,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,X,X,X,X,X,X,_, /*6
      7*/ _,X,X,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[command-trade] some test" ) {
  world w;
  MockIAgent& agent = w.agent();

  Colony const colony1 = w.add_colony( { .x = 1, .y = 0 } );
  Colony const colony2 = w.add_colony( { .x = 3, .y = 0 } );

  TradeRoute& route1 = w.trade_routes().routes[1];
  route1.id          = 1;
  route1.name        = "some route";
  route1.player      = w.default_player_type();
  route1.type        = e_trade_route_type::land;
  route1.stops.resize( 2 );
  TradeRouteStop& stop1 = route1.stops[0];
  TradeRouteStop& stop2 = route1.stops[1];
  stop1.target =
      TradeRouteTarget::colony{ .colony_id = colony1.id };
  stop2.target =
      TradeRouteTarget::colony{ .colony_id = colony2.id };

  Unit& unit = w.add_unit_on_map( e_unit_type::wagon_train,
                                  { .x = 2, .y = 2 } );

  command::trade_route const trade_route;

  auto const handler = handle_command(
      w.engine(), w.ss(), w.ts(), agent, w.default_player(),
      unit.id(), trade_route );

  auto const confirm = [&] [[clang::noinline]] {
    return co_await_test( handler->confirm() );
  };

  auto const perform = [&] [[clang::noinline]] {
    co_await_test( handler->perform() );
  };

  REQUIRE_FALSE( unit.mv_pts_exhausted() );
  REQUIRE( unit.orders() == unit_orders::none{} );

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains(
                     "Select Trade Route for [Wagon Train]" ) ) )
      .returns( "1" );
  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "Select initial destination:" ) ) )
      .returns( "1" );

  REQUIRE( confirm() == true );
  perform();

  REQUIRE_FALSE( unit.mv_pts_exhausted() );
  REQUIRE( unit.orders() == unit_orders::trade_route{
                              .id = 1, .en_route_to_stop = 1 } );

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains(
                     "Select Trade Route for [Wagon Train]" ) ) )
      .returns( "1" );
  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "Select initial destination:" ) ) )
      .returns( nothing );

  REQUIRE( confirm() == false );
}

} // namespace
} // namespace rn
