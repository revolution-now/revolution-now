/****************************************************************
**white-box-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-07.
*
* Description: Unit tests for the white-box module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/white-box.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/ref.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::gfx::rect;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[white-box] white_box_tile" ) {
  world w;
  point expected;

  auto f = [&] { return white_box_tile( w.ss() ); };

  expected = {};
  REQUIRE( f() == expected );

  w.land_view().white_box = { .x = 1, .y = 2 };
  expected                = { .x = 1, .y = 2 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[white-box] set_white_box_tile" ) {
  world w;
  point expected;

  expected = {};
  REQUIRE( w.land_view().white_box == expected );

  set_white_box_tile( w.ss(), { .x = 1, .y = 2 } );
  expected = { .x = 1, .y = 2 };
  REQUIRE( w.land_view().white_box == expected );
}

TEST_CASE( "[white-box] find_a_good_white_box_location" ) {
  world w;
  rect covered_tiles;
  point expected;

  auto f = [&] {
    return find_a_good_white_box_location( w.ss(),
                                           covered_tiles );
  };

  set_white_box_tile( w.ss(), point{ .x = 0, .y = 0 } );

  // default.
  expected = {};
  REQUIRE( f() == expected );

  // No visible white box.
  covered_tiles = { .origin = { .x = 3, .y = 4 },
                    .size   = { .w = 6, .h = 8 } };
  expected      = { .x = 6, .y = 8 };
  REQUIRE( f() == expected );

  // On the edge, not visible.
  set_white_box_tile( w.ss(), point{ .x = 2, .y = 3 } );
  covered_tiles = { .origin = { .x = 3, .y = 4 },
                    .size   = { .w = 6, .h = 8 } };
  expected      = { .x = 6, .y = 8 };
  REQUIRE( f() == expected );

  // Getting closer.
  set_white_box_tile( w.ss(), point{ .x = 3, .y = 4 } );
  covered_tiles = { .origin = { .x = 3, .y = 4 },
                    .size   = { .w = 6, .h = 8 } };
  expected      = { .x = 6, .y = 8 };
  REQUIRE( f() == expected );

  // Inside.
  set_white_box_tile( w.ss(), point{ .x = 4, .y = 5 } );
  covered_tiles = { .origin = { .x = 3, .y = 4 },
                    .size   = { .w = 6, .h = 8 } };
  expected      = { .x = 4, .y = 5 };
  REQUIRE( f() == expected );

  // Inside.
  set_white_box_tile( w.ss(), point{ .x = 5, .y = 6 } );
  covered_tiles = { .origin = { .x = 3, .y = 4 },
                    .size   = { .w = 6, .h = 8 } };
  expected      = { .x = 5, .y = 6 };
  REQUIRE( f() == expected );

  // Inside.
  set_white_box_tile( w.ss(), point{ .x = 6, .y = 7 } );
  covered_tiles = { .origin = { .x = 3, .y = 4 },
                    .size   = { .w = 6, .h = 8 } };
  expected      = { .x = 6, .y = 7 };
  REQUIRE( f() == expected );

  // Not inside.
  set_white_box_tile( w.ss(), point{ .x = 7, .y = 8 } );
  covered_tiles = { .origin = { .x = 3, .y = 4 },
                    .size   = { .w = 6, .h = 8 } };
  expected      = { .x = 6, .y = 8 };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
