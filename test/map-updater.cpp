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
    expected                   = { { .tile        = coord1,
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
