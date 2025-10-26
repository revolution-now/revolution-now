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
#include "test/mocks/iagent.hpp"
#include "test/mocks/iengine.hpp"
#include "test/util/coro.hpp"

// ss
#include "ss/unit-composition.hpp"
#include "ss/unit.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

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

  Unit& unit = w.add_unit_on_map( e_unit_type::free_colonist,
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

  REQUIRE( confirm() == true );
  perform();

  REQUIRE_FALSE( unit.mv_pts_exhausted() );
  REQUIRE( unit.orders() == unit_orders::trade_route{} );
}

} // namespace
} // namespace rn
