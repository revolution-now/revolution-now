/****************************************************************
**trade-route-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-11-08.
*
* Description: Unit tests for the ss/trade-route module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/trade-route.hpp"

// Testing
#include "test/mocking.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Matchers::Contains;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[ss/trade-route] TradeRouteTarget::harbor::validate" ) {
  REQUIRE( TradeRouteTarget::harbor{}.validate() == valid );
}

TEST_CASE(
    "[ss/trade-route] TradeRouteTarget::colony::validate" ) {
  auto v = TradeRouteTarget::colony{}.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT(
      v.error(),
      Contains( "invalid colony ID in trade route target: 0" ) );

  v = TradeRouteTarget::colony{ .colony_id = 1 }.validate();
  REQUIRE( v.valid() );
}

TEST_CASE( "[ss/trade-route] operator<( TradeRouteTarget )" ) {
  using T = TradeRouteTarget;
  REQUIRE_FALSE( T{ T::harbor{} } < T{ T::harbor{} } );
  REQUIRE( T{ T::harbor{} } < T{ T::colony{} } );
  REQUIRE( T{ T::harbor{} } < T{ T::colony{ .colony_id = 2 } } );
  REQUIRE_FALSE( T{ T::colony{} } < T{ T::harbor{} } );
  REQUIRE_FALSE( T{ T::colony{ .colony_id = 2 } } <
                 T{ T::harbor{} } );
  REQUIRE_FALSE( T{ T::colony{} } < T{ T::colony{} } );
  REQUIRE_FALSE( T{ T::colony{ .colony_id = 2 } } <
                 T{ T::colony{ .colony_id = 2 } } );
  REQUIRE( T{ T::colony{ .colony_id = 2 } } <
           T{ T::colony{ .colony_id = 3 } } );
  REQUIRE_FALSE( T{ T::colony{ .colony_id = 3 } } <
                 T{ T::colony{ .colony_id = 2 } } );
}

TEST_CASE( "[ss/trade-route] TradeRoute::validate" ) {
  TradeRoute route;

  route  = {};
  auto v = route.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT(
      v.error(),
      Contains( "invalid trade route ID in trade route: 0" ) );
  route.id = 1;

  v = route.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT( v.error(),
                Contains( "trade route 1 has no stops" ) );
  route.stops.resize( 5 );

  v = route.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT(
      v.error(),
      Contains(
          "has 5 stops which exceeds the maximum of four" ) );
  route.stops.resize( 4 );

  route.stops[0].target = TradeRouteTarget::harbor{};
  route.type            = e_trade_route_type::land;
  v                     = route.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT( v.error(),
                Contains( "A land trade route cannot have the "
                          "European Harbor" ) );
  route.type = e_trade_route_type::sea;

  v = route.validate();
  REQUIRE( v.valid() );
}

TEST_CASE( "[ss/trade-route] TradeRouteState::validate" ) {
  TradeRouteState state;

  state  = {};
  auto v = state.validate();
  REQUIRE( v.valid() );

  state.routes[0] = {};
  v               = state.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT( v.error(), Contains( "zero which is invalid" ) );
  state.routes.erase( 0 );

  state.routes[7]           = { .id = 7 };
  state.last_trade_route_id = 5;
  v                         = state.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT(
      v.error(),
      Contains( "there is an active trade route with id=7" ) );
  state.routes.erase( 7 );
  state.last_trade_route_id = 0;

  state.routes[1] = {};
  v               = state.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT( v.error(),
                Contains( "should be at most 0 active trade "
                          "routes, but there are 1" ) );
  ++state.last_trade_route_id;

  state.routes[1].id = 2;
  v                  = state.validate();
  REQUIRE( !v.valid() );
  REQUIRE_THAT(
      v.error(),
      Contains( "inconsistent trade route IDs found: 2 != 1" ) );
  state.routes[1].id = 1;

  v = state.validate();
  REQUIRE( v.valid() );
}

} // namespace
} // namespace rn
