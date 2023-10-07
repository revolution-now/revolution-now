/****************************************************************
**map-updater.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-04.
*
* Description: Unit tests for the src/map-updater.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/map-updater.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[map-updater] "
    "NonRenderingMapUpdater::make_squares_fogged" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  vector<BuffersUpdated> expected_buffers;
  e_nation const         nation = W.default_nation();
  PlayerTerrain&         player_terrain =
      W.ss()
          .mutable_terrain_use_with_care.mutable_player_terrain(
              nation );
  Coord const tile = { .x = 1, .y = 1 };

  MapSquare&       real_square = W.square( tile );
  maybe<FogSquare> expected_fog_square;

  auto f = [&] {
    return map_updater.make_squares_fogged( nation, { tile } );
  };

  auto fog_square = [&]() -> maybe<FogSquare>& {
    return player_terrain.map[tile];
  };

  // Initially totally not visible.
  REQUIRE( !fog_square().has_value() );

  // On hidden.
  expected_buffers    = { { .tile        = tile,
                            .landscape   = false,
                            .obfuscation = false } };
  expected_fog_square = nothing;
  REQUIRE( f() == expected_buffers );
  REQUIRE( fog_square() == expected_fog_square );

  // Now make it visible.
  fog_square()        = FogSquare{ .square             = real_square,
                                   .fog_of_war_removed = true };
  expected_buffers    = { { .tile        = tile,
                            .landscape   = false,
                            .obfuscation = true } };
  expected_fog_square = FogSquare{ .square = real_square,
                                   .fog_of_war_removed = false };
  REQUIRE( f() == expected_buffers );
  REQUIRE( fog_square() == expected_fog_square );

  // Now change the real square and verify no update.
  BASE_CHECK( !real_square.road );
  real_square.road    = true;
  expected_buffers    = { { .tile        = tile,
                            .landscape   = false,
                            .obfuscation = false } };
  expected_fog_square = FogSquare{ .square = real_square,
                                   .fog_of_war_removed = false };
  expected_fog_square->square.road = false;
  REQUIRE( f() == expected_buffers );
  REQUIRE( fog_square() == expected_fog_square );

  // Remove fog and try again.
  fog_square()->fog_of_war_removed = true;
  // landscape is false here because the make_squares_fogged
  // method assumes that, if the fog was removed, then the tile
  // has already been rendered in its actual state.
  expected_buffers = { { .tile        = tile,
                         .landscape   = false,
                         .obfuscation = true } };
  BASE_CHECK( real_square.road );
  expected_fog_square = FogSquare{ .square = real_square,
                                   .fog_of_war_removed = false };
  REQUIRE( f() == expected_buffers );
  REQUIRE( fog_square() == expected_fog_square );

  // Try again but with fog rendering disabled.
  {
    auto _ = map_updater.push_options_and_redraw(
        []( auto& options ) {
          options.render_fog_of_war = false;
        } );
    fog_square()->fog_of_war_removed = true;

    expected_buffers = { { .tile      = tile,
                           .landscape = false,
                           // The difference is here.
                           .obfuscation = false } };

    expected_fog_square = FogSquare{
        .square = real_square, .fog_of_war_removed = false };
    BASE_CHECK( real_square.road );
    REQUIRE( f() == expected_buffers );
    REQUIRE( fog_square() == expected_fog_square );
  }
}

// This test case will do some additional (possibly redundant)
// testing on the transitioning of tiles to and from various
// states of visible, hidden, and fog.
TEST_CASE( "[map-updater] fog of war" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  vector<BuffersUpdated> expected;
  e_nation const         nation = W.default_nation();
  PlayerTerrain&         player_terrain =
      W.ss()
          .mutable_terrain_use_with_care.mutable_player_terrain(
              nation );
  Coord const coord1 = { .x = 0, .y = 0 };
  Coord const coord2 = { .x = 1, .y = 0 };

  REQUIRE( !player_terrain.map[coord1].has_value() );
  REQUIRE( !player_terrain.map[coord2].has_value() );

  {
    expected = { { .tile        = coord1,
                   .landscape   = true,
                   .obfuscation = true },
                 { .tile        = coord2,
                   .landscape   = true,
                   .obfuscation = true } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( player_terrain.map[coord1].has_value() );
    REQUIRE( player_terrain.map[coord2].has_value() );
    FogSquare const& fog_square1 = *player_terrain.map[coord1];
    REQUIRE( fog_square1.fog_of_war_removed );
    FogSquare const& fog_square2 = *player_terrain.map[coord2];
    REQUIRE( fog_square2.fog_of_war_removed );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = true } };
    REQUIRE( map_updater.make_squares_fogged(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( player_terrain.map[coord1].has_value() );
    REQUIRE( player_terrain.map[coord2].has_value() );
    FogSquare const& fog_square1 = *player_terrain.map[coord1];
    REQUIRE( !fog_square1.fog_of_war_removed );
    FogSquare const& fog_square2 = *player_terrain.map[coord2];
    REQUIRE( !fog_square2.fog_of_war_removed );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1 } ) == expected );
    REQUIRE( player_terrain.map[coord1].has_value() );
    REQUIRE( player_terrain.map[coord2].has_value() );
    FogSquare const& fog_square1 = *player_terrain.map[coord1];
    REQUIRE( fog_square1.fog_of_war_removed );
    FogSquare const& fog_square2 = *player_terrain.map[coord2];
    REQUIRE( !fog_square2.fog_of_war_removed );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = true,
                   .obfuscation = true },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = true } };

    player_terrain.map[coord1] = nothing;
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( player_terrain.map[coord1].has_value() );
    REQUIRE( player_terrain.map[coord2].has_value() );
    FogSquare const& fog_square1 = *player_terrain.map[coord1];
    REQUIRE( fog_square1.fog_of_war_removed );
    FogSquare const& fog_square2 = *player_terrain.map[coord2];
    REQUIRE( fog_square2.fog_of_war_removed );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = false },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = false } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( player_terrain.map[coord1].has_value() );
    REQUIRE( player_terrain.map[coord2].has_value() );
    FogSquare const& fog_square1 = *player_terrain.map[coord1];
    REQUIRE( fog_square1.fog_of_war_removed );
    FogSquare const& fog_square2 = *player_terrain.map[coord2];
    REQUIRE( fog_square2.fog_of_war_removed );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = false },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = false } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( player_terrain.map[coord1].has_value() );
    REQUIRE( player_terrain.map[coord2].has_value() );
    FogSquare const& fog_square1 = *player_terrain.map[coord1];
    REQUIRE( fog_square1.fog_of_war_removed );
    FogSquare const& fog_square2 = *player_terrain.map[coord2];
    REQUIRE( fog_square2.fog_of_war_removed );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true } };
    REQUIRE( map_updater.make_squares_fogged(
                 nation, { coord1 } ) == expected );
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = false } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( player_terrain.map[coord1].has_value() );
    REQUIRE( player_terrain.map[coord2].has_value() );
    FogSquare const& fog_square1 = *player_terrain.map[coord1];
    REQUIRE( fog_square1.fog_of_war_removed );
    FogSquare const& fog_square2 = *player_terrain.map[coord2];
    REQUIRE( fog_square2.fog_of_war_removed );
  }
}

} // namespace
} // namespace rn
