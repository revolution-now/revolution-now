/****************************************************************
**lambda.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-21.
*
* Description: Unit tests for the src/base/lambda.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/lambda.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[lambda] Multi arg lambda" ) {
  auto f = [] λ( _1 + _2 );
  REQUIRE( f( 1, 2 ) == 3 );
  REQUIRE( f( 1, 2, 3 ) == 3 );

  auto g = [] λ( _ * _ );
  REQUIRE( g( 1, 2 ) == 1 );
  REQUIRE( g( 3 ) == 9 );
}

} // namespace
} // namespace base
