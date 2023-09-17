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
    add_player( e_nation::dutch );
    add_player( e_nation::english );
    set_default_player( e_nation::dutch );
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        L, L, L, L, L, //
        L, L, L, L, L, //
        L, L, L, L, L, //
        L, L, L, L, L, //
        L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 5 );
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

TEST_CASE( "[map-search] find_any_close_colony" ) {
  World           W;
  Coord           start        = {};
  double          max_distance = {};
  maybe<ColonyId> expected     = {};

  base::function_ref<bool( Colony const& )> const pred_default =
      +[]( Colony const& ) { return true; };
  base::function_ref<bool( Colony const& )> pred = pred_default;

  auto f = [&] {
    return find_any_close_colony( W.ss(), start, max_distance,
                                  pred )
        .fmap(
            []( Colony const& colony ) { return colony.id; } );
  };

  // No colonies, max distance 0.
  start        = { .x = 2, .y = 2 };
  max_distance = 0;
  expected     = nothing;
  REQUIRE( f() == expected );

  // No colonies, max distance 10.
  max_distance = 10;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 0.
  Colony& colony_1 =
      W.add_colony( { .x = 0, .y = 0 }, e_nation::dutch );
  max_distance = 0;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 1.
  max_distance = 1;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 2.
  max_distance = 2;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 3.
  max_distance = 3;
  expected     = colony_1.id;
  REQUIRE( f() == expected );

  // One colony, max distance 3, with pred, wrong nation.
  max_distance = 3;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::english;
  };
  expected = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 4, with pred, right nation.
  max_distance = 4;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::dutch;
  };
  expected = colony_1.id;
  REQUIRE( f() == expected );

  // Two colonies, max distance 5.
  Colony& colony_2 =
      W.add_colony( { .x = 2, .y = 4 }, e_nation::english );
  max_distance = 5;
  pred         = pred_default;
  expected     = colony_2.id;
  REQUIRE( f() == expected );

  // Two colonies, max distance 5, with pred dutch.
  max_distance = 5;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::dutch;
  };
  expected = colony_1.id;
  REQUIRE( f() == expected );

  // Three colonies, max distance 0.
  Colony& colony_3 =
      W.add_colony( { .x = 2, .y = 2 }, e_nation::dutch );
  max_distance = 0;
  pred         = pred_default;
  expected     = colony_3.id;
  REQUIRE( f() == expected );

  // Three colonies, max distance 1.
  max_distance = 1;
  pred         = pred_default;
  expected     = colony_3.id;
  REQUIRE( f() == expected );

  // Three colonies, max distance 1, with pred english.
  max_distance = 1;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::english;
  };
  expected = nothing;
  REQUIRE( f() == expected );

  // Three colonies, max distance 2, with pred english.
  max_distance = 2;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::english;
  };
  expected = colony_2.id;
  REQUIRE( f() == expected );
}

TEST_CASE( "[map-search] find_close_explored_colony" ) {
  World W;
}

TEST_CASE( "[map-search] close_friendly_colonies" ) {
  World W;
}

} // namespace
} // namespace rn
