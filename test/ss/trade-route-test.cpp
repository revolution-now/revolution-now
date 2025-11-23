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

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[ss/trade-route] TradeRouteTarget::harbor::validate" ) {
}

TEST_CASE(
    "[ss/trade-route] TradeRouteTarget::colony::validate" ) {
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
}

TEST_CASE( "[ss/trade-route] TradeRouteState::validate" ) {
}

} // namespace
} // namespace rn
