/****************************************************************
**map-search.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Unit tests for the src/map-search.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/map-search.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/ref.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/range-lite.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

namespace rl = ::base::rl;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    MapSquare const L = make_grassland();
    build_map( vector<MapSquare>( 5 * 5, L ), 5 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[map-search] outward_spiral_search" ) {
  base::generator<gfx::point> gen =
      outward_spiral_search( { .x = 2, .y = 3 } );
  vector<gfx::point> const vec =
      rl::all( gen ).take( 40 ).to_vector();
  vector<gfx::point> const expected{
      { .x = 2, .y = 3 }, { .x = 1, .y = 2 },
      { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
      { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
      { .x = 2, .y = 4 }, { .x = 1, .y = 4 },
      { .x = 1, .y = 3 }, { .x = 0, .y = 1 },
      { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
      { .x = 3, .y = 1 }, { .x = 4, .y = 1 },
      { .x = 4, .y = 2 }, { .x = 4, .y = 3 },
      { .x = 4, .y = 4 }, { .x = 4, .y = 5 },
      { .x = 3, .y = 5 }, { .x = 2, .y = 5 },
      { .x = 1, .y = 5 }, { .x = 0, .y = 5 },
      { .x = 0, .y = 4 }, { .x = 0, .y = 3 },
      { .x = 0, .y = 2 }, { .x = -1, .y = 0 },
      { .x = 0, .y = 0 }, { .x = 1, .y = 0 },
      { .x = 2, .y = 0 }, { .x = 3, .y = 0 },
      { .x = 4, .y = 0 }, { .x = 5, .y = 0 },
      { .x = 5, .y = 1 }, { .x = 5, .y = 2 },
      { .x = 5, .y = 3 }, { .x = 5, .y = 4 },
      { .x = 5, .y = 5 }, { .x = 5, .y = 6 },
      { .x = 4, .y = 6 }, { .x = 3, .y = 6 } };
  REQUIRE( vec == expected );
}

TEST_CASE( "[map-search] outward_spiral_search_existing" ) {
  World                       W;
  base::generator<gfx::point> gen =
      outward_spiral_search_existing( W.ss(),
                                      { .x = 2, .y = 3 } );
  // Don't cut off the stream just to make sure that it termi-
  // nates on its own.
  vector<gfx::point> const vec = rl::all( gen ).to_vector();
  vector<gfx::point> const expected{
      { .x = 2, .y = 3 }, { .x = 1, .y = 2 }, { .x = 2, .y = 2 },
      { .x = 3, .y = 2 }, { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
      { .x = 2, .y = 4 }, { .x = 1, .y = 4 }, { .x = 1, .y = 3 },
      { .x = 0, .y = 1 }, { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
      { .x = 3, .y = 1 }, { .x = 4, .y = 1 }, { .x = 4, .y = 2 },
      { .x = 4, .y = 3 }, { .x = 4, .y = 4 }, { .x = 0, .y = 4 },
      { .x = 0, .y = 3 }, { .x = 0, .y = 2 }, { .x = 0, .y = 0 },
      { .x = 1, .y = 0 }, { .x = 2, .y = 0 }, { .x = 3, .y = 0 },
      { .x = 4, .y = 0 } };
  REQUIRE( vec == expected );
}

} // namespace
} // namespace rn
