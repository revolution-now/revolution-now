/****************************************************************
**co-combinator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-24.
*
* Description: Unit tests for the src/co-combinator.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/co-combinator.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[co-combinator] when_any" ) {
  waitable_promise<> p1, p2;
  waitable<>         w1 = p1.get_waitable();
  waitable<>         w2 = p2.get_waitable();
  waitable<>         w  = when_any( w1, w2 );
  REQUIRE( !w.ready() );
  SECTION( "first" ) {
    p1.set_value_emplace();
    REQUIRE( w1.ready() );
    REQUIRE( !w2.ready() );
  }
  SECTION( "second" ) {
    p2.set_value_emplace();
    REQUIRE( !w1.ready() );
    REQUIRE( w2.ready() );
  }
  SECTION( "both" ) {
    p1.set_value_emplace();
    p2.set_value_emplace();
    REQUIRE( w1.ready() );
    REQUIRE( w2.ready() );
  }
  REQUIRE( w.ready() );
}

} // namespace
} // namespace rn
