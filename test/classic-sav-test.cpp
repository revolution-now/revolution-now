/****************************************************************
**classic-sav-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-13.
*
* Description: Unit tests for the classic-sav module.
*
*****************************************************************/
#include "terrain-enums.rds.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/classic-sav.hpp"

// ss
#include "ss/terrain.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;
using ::testing::data_dir;

/****************************************************************
** Helpers.
*****************************************************************/
fs::path classic_sav_dir() {
  return data_dir() / "saves" / "classic";
}

fs::path classic_map_dir() { return classic_sav_dir() / "map"; }

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[classic-sav] load_classic_map_file" ) {
  Coord     coord;
  MapSquare expected;
  fs::path  input;

  SECTION( "old" ) {
    // This one tests the original America map file that ships
    // with the game that uses the "old" (likely deprecated) en-
    // coding for forest tiles.
    input = classic_map_dir() / "AMER2.MP";
  }
  SECTION( "new" ) {
    // This one tests the original America map file that ships
    // with the game but that is loaded into the map editor and
    // then saved again so that it uses the "new" encoding for
    // forest tiles.
    input = classic_map_dir() / "AMER-NEW.MP";
  }

  RealTerrain real_terrain;
  REQUIRE( load_classic_map_file( input, real_terrain ) ==
           valid );

  coord    = { .x = 0, .y = 0 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::tundra,
                        .overlay = e_land_overlay::hills };
  REQUIRE( real_terrain.map[coord] == expected );

  coord    = { .x = 23, .y = 19 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::grassland,
                        .overlay = e_land_overlay::hills };
  REQUIRE( real_terrain.map[coord] == expected );

  coord    = { .x = 21, .y = 21 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::prairie,
                        .overlay = e_land_overlay::forest,
                        .river   = e_river::major };
  REQUIRE( real_terrain.map[coord] == expected );

  coord    = { .x = 21, .y = 31 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::swamp,
                        .overlay = e_land_overlay::forest };
  REQUIRE( real_terrain.map[coord] == expected );

  coord    = { .x = 27, .y = 20 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::marsh,
                        .overlay = e_land_overlay::forest };
  REQUIRE( real_terrain.map[coord] == expected );

  coord    = { .x = 44, .y = 47 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::marsh,
                        .river   = e_river::minor };
  REQUIRE( real_terrain.map[coord] == expected );

  coord    = { .x = 7, .y = 14 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::desert,
                        .overlay = e_land_overlay::mountains };
  REQUIRE( real_terrain.map[coord] == expected );

  coord    = { .x = 10, .y = 10 };
  expected = MapSquare{ .surface = e_surface::land,
                        .ground  = e_ground_terrain::plains };
  REQUIRE( real_terrain.map[coord] == expected );
}

} // namespace
} // namespace rn
