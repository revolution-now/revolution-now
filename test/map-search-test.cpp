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

using ::gfx::point;

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
TEST_CASE(
    "[map-search] outward_spiral_pythdist_search_existing" ) {
  World  W;
  double max_distance = 0;

  SECTION( "zero distance" ) {
    max_distance      = 0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{ { .x = 2, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=.5" ) {
    max_distance      = .5;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{ { .x = 2, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=1" ) {
    max_distance      = 1.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{ { .x = 2, .y = 3 },
                                  { .x = 2, .y = 2 },
                                  { .x = 3, .y = 3 },
                                  { .x = 2, .y = 4 },
                                  { .x = 1, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=2" ) {
    max_distance      = 2.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
        { .x = 2, .y = 3 }, { .x = 1, .y = 2 },
        { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
        { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
        { .x = 2, .y = 4 }, { .x = 1, .y = 4 },
        { .x = 1, .y = 3 }, { .x = 2, .y = 1 },
        { .x = 4, .y = 3 }, { .x = 0, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=3" ) {
    max_distance      = 3.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
        { .x = 2, .y = 3 }, { .x = 1, .y = 2 },
        { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
        { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
        { .x = 2, .y = 4 }, { .x = 1, .y = 4 },
        { .x = 1, .y = 3 }, { .x = 0, .y = 1 },
        { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
        { .x = 3, .y = 1 }, { .x = 4, .y = 1 },
        { .x = 4, .y = 2 }, { .x = 4, .y = 3 },
        { .x = 4, .y = 4 }, { .x = 0, .y = 4 },
        { .x = 0, .y = 3 }, { .x = 0, .y = 2 },
        { .x = 2, .y = 0 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=4" ) {
    max_distance      = 4.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
        { .x = 2, .y = 3 }, { .x = 1, .y = 2 },
        { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
        { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
        { .x = 2, .y = 4 }, { .x = 1, .y = 4 },
        { .x = 1, .y = 3 }, { .x = 0, .y = 1 },
        { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
        { .x = 3, .y = 1 }, { .x = 4, .y = 1 },
        { .x = 4, .y = 2 }, { .x = 4, .y = 3 },
        { .x = 4, .y = 4 }, { .x = 0, .y = 4 },
        { .x = 0, .y = 3 }, { .x = 0, .y = 2 },
        { .x = 0, .y = 0 }, { .x = 1, .y = 0 },
        { .x = 2, .y = 0 }, { .x = 3, .y = 0 },
        { .x = 4, .y = 0 } };
    REQUIRE( vec == expected );
  }

  SECTION( "max distance" ) {
    max_distance      = 1000;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
        { .x = 2, .y = 3 }, { .x = 1, .y = 2 },
        { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
        { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
        { .x = 2, .y = 4 }, { .x = 1, .y = 4 },
        { .x = 1, .y = 3 }, { .x = 0, .y = 1 },
        { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
        { .x = 3, .y = 1 }, { .x = 4, .y = 1 },
        { .x = 4, .y = 2 }, { .x = 4, .y = 3 },
        { .x = 4, .y = 4 }, { .x = 0, .y = 4 },
        { .x = 0, .y = 3 }, { .x = 0, .y = 2 },
        { .x = 0, .y = 0 }, { .x = 1, .y = 0 },
        { .x = 2, .y = 0 }, { .x = 3, .y = 0 },
        { .x = 4, .y = 0 } };
    REQUIRE( vec == expected );
  }
}

TEST_CASE( "[map-search] find_close_explored_colony" ) {
  World W;
}

TEST_CASE( "[map-search] close_friendly_colonies" ) {
  World W;
}

} // namespace
} // namespace rn
