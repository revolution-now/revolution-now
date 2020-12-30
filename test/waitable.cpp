/****************************************************************
**waitable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-28.
*
* Description: Unit tests for the src/waitable.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/waitable.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[waitable] correct result" ) {
  REQUIRE( test_waitable( /*logging=*/false ) ==
           "3-6-8.800000" );
}

} // namespace
} // namespace rn
