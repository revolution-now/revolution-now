/****************************************************************
**spread-algo-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: Unit tests for the spread-algo module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/spread-algo.hpp"

// config
#include "src/config/tile-enum.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::interval;
using ::gfx::point;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread] compute_icon_spread" ) {
}

TEST_CASE( "[spread] requires_label" ) {
}

} // namespace
} // namespace rn
