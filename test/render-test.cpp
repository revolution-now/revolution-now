/****************************************************************
**render-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-13.
*
* Description: Unit tests for the render module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/tiles.hpp"

// config
#include "src/config/tile-enum.rds.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/ref.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

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
TEST_CASE( "[render] tile_for_unit_type" ) {
  e_unit_type type = {};
  e_tile expected  = {};

  auto f = [&] { return tile_for_unit_type( type ); };

  type     = e_unit_type::petty_criminal;
  expected = e_tile::petty_criminal;
  REQUIRE( f() == expected );

  type     = e_unit_type::expert_farmer;
  expected = e_tile::expert_farmer;
  REQUIRE( f() == expected );

  type     = e_unit_type::regular;
  expected = e_tile::regular;
  REQUIRE( f() == expected );

  type     = e_unit_type::veteran_dragoon;
  expected = e_tile::veteran_dragoon;
  REQUIRE( f() == expected );
}

TEST_CASE( "[render] trimmed_area_for_unit_type" ) {
  e_unit_type type = {};
  rect expected    = {};

  auto f = [&] { return trimmed_area_for_unit_type( type ); };

  testing_set_trimmed_cache(
      e_tile::regular, rect{ .origin = { .x = 1, .y = 2 },
                             .size   = { .w = 3, .h = 4 } } );
  testing_set_trimmed_cache(
      e_tile::veteran_dragoon,
      rect{ .origin = { .x = 5, .y = 6 },
            .size   = { .w = 7, .h = 8 } } );

  type     = e_unit_type::regular;
  expected = { .origin = { .x = 1, .y = 2 },
               .size   = { .w = 3, .h = 4 } };
  REQUIRE( f() == expected );

  type     = e_unit_type::veteran_dragoon;
  expected = { .origin = { .x = 5, .y = 6 },
               .size   = { .w = 7, .h = 8 } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[render] houses_tile_for_colony" ) {
  Colony colony;
  e_tile expected = {};

  auto f = [&] { return houses_tile_for_colony( colony ); };

  expected = e_tile::colony_basic_houses;
  REQUIRE( f() == expected );

  colony.buildings[e_colony_building::stockade] = true;
  expected = e_tile::colony_stockade_houses;
  REQUIRE( f() == expected );

  colony.buildings[e_colony_building::fort] = true;
  expected = e_tile::colony_fort_houses;
  REQUIRE( f() == expected );

  colony.buildings[e_colony_building::fortress] = true;
  expected = e_tile::colony_fortress_houses;
  REQUIRE( f() == expected );
}

TEST_CASE( "[render] dwelling_tile_for_tribe" ) {
  e_tribe tribe_type = {};
  e_tile expected    = {};

  auto f = [&] { return dwelling_tile_for_tribe( tribe_type ); };

  tribe_type = e_tribe::inca;
  expected   = e_tile::grey_pyramid;
  REQUIRE( f() == expected );

  tribe_type = e_tribe::aztec;
  expected   = e_tile::tan_pyramid;
  REQUIRE( f() == expected );

  tribe_type = e_tribe::sioux;
  expected   = e_tile::camp;
  REQUIRE( f() == expected );

  tribe_type = e_tribe::cherokee;
  expected   = e_tile::village;
  REQUIRE( f() == expected );
}

TEST_CASE( "[render] tile_for_dwelling" ) {
  world w;
  Dwelling dwelling;
  e_tile expected = {};

  auto f = [&] { return tile_for_dwelling( w.ss(), dwelling ); };

  // Test with a frozen dwelling.
  dwelling = {
    .id     = 0,
    .frozen = FrozenDwelling{ .tribe = e_tribe::cherokee } };
  expected = e_tile::village;
  REQUIRE( f() == expected );

  dwelling = w.add_dwelling( { .x = 1, .y = 2 }, e_tribe::tupi );
  expected = e_tile::camp;
  REQUIRE( f() == expected );

  dwelling =
      w.add_dwelling( { .x = 1, .y = 0 }, e_tribe::aztec );
  expected = e_tile::tan_pyramid;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
