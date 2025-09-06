/****************************************************************
**tiles-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-06.
*
* Description: Unit tests for the tiles module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/tiles.hpp"

// Revolution Now
#include "src/config/tile-enum.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::size;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[tiles] burrowed_sprite_plan_for" ) {
  rr::TexturedDepixelatePlan expected;
  e_tile rendered_tile  = {};
  e_tile reference_tile = {};
  size tile_offset      = {};

  auto const f = [&] [[clang::noinline]] {
    return burrowed_sprite_plan_for(
        rendered_tile, reference_tile, tile_offset );
  };

  rendered_tile  = e_tile::terrain_forest_all;
  reference_tile = e_tile::village;

  testing_set_burrow_cache( e_tile::village, 5 );

  tile_offset = { .w = 0, .h = 0 };
  expected    = { .reference_atlas_id    = 5,
                  .offset_into_reference = { .w = 6, .h = 6 } };
  REQUIRE( f() == expected );

  tile_offset = { .w = -1, .h = 1 };
  expected    = {
       .reference_atlas_id    = 5,
       .offset_into_reference = { .w = 6 - 32, .h = 6 + 32 } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
