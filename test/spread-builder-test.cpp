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

// Revolution Now
#include "src/tiles.hpp"

// config
#include "src/config/tile-enum.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread-builder] build_fixed_tile_spread" ) {
  FixedTileSpreadConfig config;
  TileSpreadRenderPlan ex;

  auto f = [&] { return build_fixed_tile_spread( config ); };

  testing_set_trimmed_cache(
      e_tile::commodity_tobacco_20,
      rect{ .origin = { .x = 4, .y = 1 },
            .size   = { .w = 13, .h = 19 } } );

  config = { .tile = e_tile::commodity_tobacco_20 };
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 0,
             .spacing        = 0 };
  ex     = {};
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 0,
             .spacing        = 4 };
  ex     = {};
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 1,
             .spacing        = 4 };
  ex     = {};
  ex.tiles.resize( 1 );
  ex.tiles[0].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[0].where = { .x = -4, .y = 0 };
  ex.bounds.origin  = { .x = 0, .y = 0 };
  ex.bounds.size    = { .w = 13, .h = 20 };
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 4,
             .spacing        = 4 };
  ex     = {};
  ex.tiles.resize( 4 );
  ex.tiles[0].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[0].where = { .x = -4, .y = 0 };
  ex.tiles[1].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[1].where = { .x = 0, .y = 0 };
  ex.tiles[2].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[2].where = { .x = 4, .y = 0 };
  ex.tiles[3].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[3].where = { .x = 8, .y = 0 };
  ex.bounds.origin  = { .x = 0, .y = 0 };
  ex.bounds.size    = { .w = 4 * 3 + 13, .h = 20 };
  REQUIRE( f() == ex );
}

TEST_CASE( "[spread-builder] build_tile_spread" ) {
  rr::MockTextometer textometer;
}

TEST_CASE( "[spread-builder] build_progress_tile_spread" ) {
  rr::MockTextometer textometer;
}

TEST_CASE( "[spread-builder] build_inhomogeneous_tile_spread" ) {
  rr::MockTextometer textometer;
}

TEST_CASE( "[spread-builder] build_tile_spread_multi" ) {
  rr::MockTextometer textometer;
}

} // namespace
} // namespace rn
