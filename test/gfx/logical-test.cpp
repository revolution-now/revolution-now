/****************************************************************
**logical-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-25.
*
* Description: Unit tests for the gfx/logical module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/logical.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace gfx {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
#if 0
TEST_CASE( "[gfx/logical] supported_logical_resolutions" ) {
  vector<LogicalResolution> expected;
  size                      resolution;

  auto f = [&] {
    return supported_logical_resolutions( resolution );
  };

  resolution = { .w = 1, .h = 1 };
  expected   = {
    { .dimensions = { .w = 1, .h = 1 }, .scale = 1 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 1, .h = 2 };
  expected   = {
    { .dimensions = { .w = 1, .h = 2 }, .scale = 1 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 2, .h = 2 };
  expected   = {
    { .dimensions = { .w = 2, .h = 2 }, .scale = 1 },
    { .dimensions = { .w = 1, .h = 1 }, .scale = 2 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 3, .h = 2 };
  expected   = {
    { .dimensions = { .w = 3, .h = 2 }, .scale = 1 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 4, .h = 2 };
  expected   = {
    { .dimensions = { .w = 4, .h = 2 }, .scale = 1 },
    { .dimensions = { .w = 2, .h = 1 }, .scale = 2 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 3'840, .h = 2'160 };
  expected   = {
    { .dimensions = { .w = 3'840, .h = 2'160 }, .scale = 1 },
    { .dimensions = { .w = 1'920, .h = 1'080 }, .scale = 2 },
    { .dimensions = { .w = 1'280, .h = 720 }, .scale = 3 },
    { .dimensions = { .w = 960, .h = 540 }, .scale = 4 },
    { .dimensions = { .w = 768, .h = 432 }, .scale = 5 },
    { .dimensions = { .w = 640, .h = 360 }, .scale = 6 },
    { .dimensions = { .w = 480, .h = 270 }, .scale = 8 },
    { .dimensions = { .w = 384, .h = 216 }, .scale = 10 },
    { .dimensions = { .w = 320, .h = 180 }, .scale = 12 },
    { .dimensions = { .w = 256, .h = 144 }, .scale = 15 },
    { .dimensions = { .w = 240, .h = 135 }, .scale = 16 },
    { .dimensions = { .w = 192, .h = 108 }, .scale = 20 },
    { .dimensions = { .w = 160, .h = 90 }, .scale = 24 },
    { .dimensions = { .w = 128, .h = 72 }, .scale = 30 },
    { .dimensions = { .w = 96, .h = 54 }, .scale = 40 },
    { .dimensions = { .w = 80, .h = 45 }, .scale = 48 },
    { .dimensions = { .w = 64, .h = 36 }, .scale = 60 },
    { .dimensions = { .w = 48, .h = 27 }, .scale = 80 },
    { .dimensions = { .w = 32, .h = 18 }, .scale = 120 },
    { .dimensions = { .w = 16, .h = 9 }, .scale = 240 },
  };
  REQUIRE( f() == expected );
}
#endif

TEST_CASE( "[gfx/logical] resolution_analysis" ) {
  // TODO
}

TEST_CASE( "[gfx/logical] recommended_resolution" ) {
  // TODO
}

} // namespace
} // namespace gfx
