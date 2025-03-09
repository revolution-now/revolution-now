/****************************************************************
**spread-builder-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Unit tests for the spread-builder module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/spread-builder.hpp"

// Testing
#include "test/mocking.hpp"
#include "test/mocks/render/itextometer.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread-builder] build_tile_spread" ) {
  rr::MockTextometer textometer;
}

TEST_CASE( "[spread-builder] build_progress_tile_spread" ) {
  rr::MockTextometer textometer;
}

TEST_CASE( "[spread-builder] build_tile_spread_multi" ) {
  rr::MockTextometer textometer;
}

} // namespace
} // namespace rn
