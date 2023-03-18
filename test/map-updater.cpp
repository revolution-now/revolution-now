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
  BuffersUpdated         expected;
  e_nation const         nation = W.default_nation();
  UNWRAP_CHECK( player_terrain,
                W.terrain().player_terrain( nation ) );
  Coord const coord = { .x = 0, .y = 0 };

  auto f = [&] {
    return map_updater.make_square_visible( coord, nation );
  };

  REQUIRE( !player_terrain.map[coord].has_value() );

  {
    expected = { .landscape = true, .obfuscation = true };
    REQUIRE( f() == expected );
    REQUIRE( player_terrain.map[coord].has_value() );
    FogSquare const& fog_square = *player_terrain.map[coord];
    REQUIRE( fog_square.fog_of_war_removed );
  }

  {
    expected = { .landscape = false, .obfuscation = true };
    REQUIRE( map_updater.make_square_fogged( coord, nation ) ==
             expected );
    REQUIRE( player_terrain.map[coord].has_value() );
    FogSquare const& fog_square = *player_terrain.map[coord];
    REQUIRE( !fog_square.fog_of_war_removed );
  }

  {
    expected = { .landscape = false, .obfuscation = true };
    REQUIRE( f() == expected );
    REQUIRE( player_terrain.map[coord].has_value() );
    FogSquare const& fog_square = *player_terrain.map[coord];
    REQUIRE( fog_square.fog_of_war_removed );
  }

  {
    expected = { .landscape = false, .obfuscation = false };
    REQUIRE( f() == expected );
    REQUIRE( player_terrain.map[coord].has_value() );
    FogSquare const& fog_square = *player_terrain.map[coord];
    REQUIRE( fog_square.fog_of_war_removed );
  }
}

} // namespace
} // namespace rn
