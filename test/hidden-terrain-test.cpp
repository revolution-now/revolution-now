/****************************************************************
**hidden-terrain-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-10.
*
* Description: Unit tests for the hidden-terrain module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/hidden-terrain.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/visibility.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/unit-composition.hpp"

// rcl
#include "src/rcl/golden.hpp"

// refl
#include "src/refl/cdr.hpp"
#include "src/refl/to-str.hpp"

// cdr
#include "src/cdr/ext-base.hpp"
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// base
#include "src/base/to-str-ext-chrono.hpp"
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::rcl::Golden;
using ::testing::data_dir;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_default_player(); }

  void create_map( Delta size ) {
    vector<MapSquare> tiles( size.area(), make_grassland() );
    build_map( std::move( tiles ), size.w );
  }
};

/****************************************************************
** Static Checks.
*****************************************************************/
// We need this to be true so that we can use the golden file
// framework below as a testing mechanism, otherwise this data
// structure is too complex to write out explicitly.
static_assert( cdr::Canonical<HiddenTerrainAnimationSequence> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[hidden-terrain] anim_seq_for_hidden_terrain, empty" ) {
  World W;
  W.create_map( { .w = 0, .h = 0 } );
  HiddenTerrainAnimationSequence expected;

  VisibilityEntire const viz( W.ss() );

  auto const seq =
      anim_seq_for_hidden_terrain( W.ss(), viz, W.rand() );

  expected = { .hide = { .sequence = { {} } },
               .hold = { .sequence = { {} } },
               .show = { .sequence = { {} } } };

  REQUIRE( seq == expected );
}

TEST_CASE(
    "[hidden-terrain] anim_seq_for_hidden_terrain, 1x1 no "
    "changes" ) {
  World W;
  W.create_map( { .w = 1, .h = 1 } );
  HiddenTerrainAnimationSequence expected;

  VisibilityEntire const viz( W.ss() );

  auto const seq =
      anim_seq_for_hidden_terrain( W.ss(), viz, W.rand() );

  expected = { .hide = { .sequence = { {} } },
               .hold = { .sequence = { {} } },
               .show = { .sequence = { {} } } };

  REQUIRE( seq == expected );
}

TEST_CASE(
    "[hidden-terrain] anim_seq_for_hidden_terrain, 1x1 with "
    "changes" ) {
  World W;
  W.create_map( { .w = 1, .h = 1 } );
  HiddenTerrainAnimationSequence expected;

  VisibilityEntire const viz( W.ss() );

  MapSquare& square = W.square( { .x = 0, .y = 0 } );

  square.road            = true;
  square.river           = e_river::minor;
  square.overlay         = e_land_overlay::forest;
  square.irrigation      = true;
  square.forest_resource = e_natural_resource::deer;
  square.ground_resource = e_natural_resource::tobacco;
  square.lost_city_rumor = true;

  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 } );
  W.add_colony( { .x = 0, .y = 0 } );
  W.add_dwelling( { .x = 0, .y = 0 }, e_tribe::apache );

  auto const seq =
      anim_seq_for_hidden_terrain( W.ss(), viz, W.rand() );

  Golden const gold( seq, "1x1-all" );

  REQUIRE( gold.is_golden() == base::valid );
}

TEST_CASE(
    "[hidden-terrain] anim_seq_for_hidden_terrain, all "
    "invisible" ) {
  World W;
  HiddenTerrainAnimationSequence expected;
  W.create_map( { .w = 4, .h = 4 } );
  e_player const player = W.default_player_type();
  VisibilityForNation const viz( W.ss(), player );

  auto const seq =
      anim_seq_for_hidden_terrain( W.ss(), viz, W.rand() );

  expected = { .hide = { .sequence = { {} } },
               .hold = { .sequence = { {} } },
               .show = { .sequence = { {} } } };

  REQUIRE( seq == expected );
}

TEST_CASE(
    "[hidden-terrain] anim_seq_for_hidden_terrain, all "
    "visible" ) {
  World W;
  W.create_map( { .w = 4, .h = 4 } );
  VisibilityEntire const viz( W.ss() );

  vector<int> const indices{ 0, 1, 2,  3,  4,  5,  6,  7,
                             8, 9, 10, 11, 12, 13, 14, 15 };
  expect_shuffle( W.rand(), indices );

  W.square( { .x = 0, .y = 0 } ).surface = e_surface::water;
  // This is important to test because fish are not supposed to
  // be removed when prime resources are removed.
  W.square( { .x = 0, .y = 0 } ).ground_resource =
      e_natural_resource::fish;

  W.square( { .x = 1, .y = 1 } ).road = true;
  W.square( { .x = 1, .y = 1 } ).overlay =
      e_land_overlay::forest;

  vector<Coord> tiles;

  // Ocean.
  tiles = { { .x = 0, .y = 0 } };
  for( Coord const tile : tiles ) {
    W.square( tile ).surface         = e_surface::water;
    W.square( tile ).ground_resource = e_natural_resource::fish;
  }

  // NOTE: the coordinate lists below were randomly generated and
  // so may contain repeats and/or may lead to invalid land-
  // scapes, but it should matter much for the purpose of this
  // test.

  // Roads.
  tiles = {
    { .x = 1, .y = 3 }, { .x = 1, .y = 0 }, { .x = 2, .y = 2 },
    { .x = 2, .y = 2 }, { .x = 0, .y = 3 }, { .x = 3, .y = 0 },
  };
  for( Coord const tile : tiles ) W.square( tile ).road = true;

  // Irrigation.
  tiles = {
    { .x = 1, .y = 0 }, { .x = 3, .y = 3 }, { .x = 1, .y = 0 },
    { .x = 3, .y = 2 }, { .x = 0, .y = 1 }, { .x = 3, .y = 2 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).irrigation = true;

  // Rivers.
  tiles = {
    { .x = 2, .y = 1 }, { .x = 0, .y = 2 }, { .x = 1, .y = 1 },
    { .x = 1, .y = 0 }, { .x = 1, .y = 3 }, { .x = 2, .y = 1 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).river = e_river::minor;

  // Mountains.
  tiles = {
    { .x = 1, .y = 2 }, { .x = 0, .y = 1 }, { .x = 3, .y = 1 },
    { .x = 1, .y = 3 }, { .x = 1, .y = 1 }, { .x = 0, .y = 2 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).overlay = e_land_overlay::mountains;

  // Hills.
  tiles = {
    { .x = 2, .y = 2 }, { .x = 2, .y = 1 }, { .x = 3, .y = 1 },
    { .x = 1, .y = 0 }, { .x = 3, .y = 0 }, { .x = 3, .y = 1 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).overlay = e_land_overlay::hills;

  // Forests.
  tiles = {
    { .x = 2, .y = 1 }, { .x = 1, .y = 1 }, { .x = 2, .y = 0 },
    { .x = 2, .y = 0 }, { .x = 1, .y = 2 }, { .x = 2, .y = 2 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).overlay = e_land_overlay::forest;

  // Ground resources.
  tiles = {
    { .x = 3, .y = 0 }, { .x = 2, .y = 0 }, { .x = 2, .y = 0 },
    { .x = 2, .y = 1 }, { .x = 1, .y = 0 }, { .x = 0, .y = 2 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).ground_resource =
        e_natural_resource::tobacco;

  // Forest resources.
  tiles = {
    { .x = 0, .y = 1 }, { .x = 3, .y = 3 }, { .x = 1, .y = 2 },
    { .x = 2, .y = 1 }, { .x = 2, .y = 0 }, { .x = 2, .y = 3 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).forest_resource = e_natural_resource::deer;

  // LCRs.
  tiles = {
    { .x = 0, .y = 3 }, { .x = 0, .y = 2 }, { .x = 0, .y = 3 },
    { .x = 0, .y = 2 }, { .x = 3, .y = 2 }, { .x = 2, .y = 3 },
  };
  for( Coord const tile : tiles )
    W.square( tile ).lost_city_rumor = true;

  // Colonies.
  tiles = {
    { .x = 3, .y = 3 }, { .x = 0, .y = 3 }, { .x = 2, .y = 1 },
    { .x = 2, .y = 2 }, { .x = 3, .y = 1 }, { .x = 1, .y = 0 },
  };
  for( Coord const tile : tiles ) W.add_colony( tile );

  // Dwellings.
  tiles = {
    { .x = 2, .y = 0 }, { .x = 1, .y = 3 }, { .x = 3, .y = 2 },
    { .x = 1, .y = 2 }, { .x = 2, .y = 1 }, { .x = 2, .y = 2 },
  };
  for( Coord const tile : tiles )
    W.add_dwelling( tile, e_tribe::apache );

  // Units.
  tiles = {
    { .x = 1, .y = 3 }, { .x = 1, .y = 3 }, { .x = 2, .y = 3 },
    { .x = 1, .y = 3 }, { .x = 1, .y = 1 }, { .x = 3, .y = 3 },
  };
  W.add_unit_on_map( e_unit_type::caravel, { .x = 0, .y = 0 } );
  for( Coord const tile : tiles )
    W.add_unit_on_map( e_unit_type::free_colonist, tile );

  tiles = {
    { .x = 3, .y = 1 }, { .x = 3, .y = 3 }, { .x = 3, .y = 0 },
    { .x = 1, .y = 3 }, { .x = 1, .y = 2 }, { .x = 1, .y = 2 },
  };
  for( Coord const tile : tiles )
    W.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              DwellingId{ 1 } );

  auto const seq =
      anim_seq_for_hidden_terrain( W.ss(), viz, W.rand() );

  Golden const gold( seq, "all-visible" );

  REQUIRE( gold.is_golden() == base::valid );
}

} // namespace
} // namespace rn
