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

// Revolution Now
#include "src/ss/ref.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"

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
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
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
  world         w;
  rect          covered_tiles;
  maybe<UnitId> last_unit_input;
  point         expected;

  auto f = [&] {
    return find_a_good_white_box_location(
        w.ss(), last_unit_input, covered_tiles );
  };

  // default.
  expected = {};
  REQUIRE( f() == expected );

  // No visible white box and no unit.
  covered_tiles = rect{ .origin = { .x = 3, .y = 4 },
                        .size   = { .w = 4, .h = 6 } };
  expected      = { .x = 5, .y = 7 };
  REQUIRE( f() == expected );

  // With unit, not visible.
  UnitId const unit_id =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         point{ .x = 1, .y = 2 } )
          .id();
  last_unit_input = unit_id;
  expected        = { .x = 5, .y = 7 };
  REQUIRE( f() == expected );

  // With unit, visible.
  covered_tiles = rect{ .origin = { .x = 1, .y = 1 },
                        .size   = { .w = 4, .h = 6 } };
  expected      = { .x = 1, .y = 2 };
  REQUIRE( f() == expected );

  // With unit, visible, and visible white box.
  set_white_box_tile( w.ss(), { .x = 2, .y = 4 } );
  covered_tiles = rect{ .origin = { .x = 1, .y = 1 },
                        .size   = { .w = 4, .h = 6 } };
  expected      = { .x = 2, .y = 4 };
  REQUIRE( f() == expected );

  // Nuke unit.
  last_unit_input = nothing;
  expected        = { .x = 2, .y = 4 };
  REQUIRE( f() == expected );

  // Move white box out of visibility.
  set_white_box_tile( w.ss(), point{} );
  expected = { .x = 3, .y = 4 };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
