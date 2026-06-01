/****************************************************************
**range-helpers-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-06-01.
*
* Description: Unit tests for the config/range-helpers module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/config/range-helpers.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn::config {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[config/range-helpers] multiply( Probability )" ) {
  Probability const l{ .probability = .7 },
      r{ .probability = .5 };
  auto const res = l * r;
  REQUIRE( res.probability == .35 );
}

} // namespace
} // namespace rn::config
