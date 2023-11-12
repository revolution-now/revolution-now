/****************************************************************
**bridge-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-12.
*
* Description: Unit tests for the sav/bridge module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/bridge.hpp"

// sav
#include "src/sav/sav-struct.hpp"

// ss
#include "src/ss/root.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"
#include "src/base/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace bridge {
namespace {

using namespace std;

using ::base::valid;

// The reason that we have two scenarios below is that we won't
// always be able to have a single pair of of (ColonySAV, Root-
// State) that can be converted to each other in both directions.
// So we have separate scenarios for each conversion direction.
// That said, it is preferable to try to keep them in sync as
// much as possible, which should be doable for the most part be-
// cause most of the elements of a save file can be round-tripped
// between the new/old game without any information loss.

/****************************************************************
** Scenario 1 (used for OG -> RN ).
*****************************************************************/
void source_og_scenario_1( sav::ColonySAV& out ) {
  // Header.
  auto& H      = out.header;
  H.colonize   = { 'C', 'O', 'L', 'O', 'N', 'I', 'Z', 'E', 0 };
  H.map_size_x = 8;
  H.map_size_y = 10;

  // TODO: add more here.
}

void expected_rn_scenario_1( rn::RootState& out ) {
  rn::wrapped::TerrainState terrain_o;

  auto& map = terrain_o.real_terrain.map;
  map =
      gfx::Matrix<rn::MapSquare>( rn::Delta{ .w = 6, .h = 8 } );
  // ...
  out.zzz_terrain = rn::TerrainState( std::move( terrain_o ) );

  // TODO: add more here.
}

/****************************************************************
** Scenario 2 (used for RN -> OG ).
*****************************************************************/
void source_og_scenario_2( sav::ColonySAV& out ) {
  // Header.
  auto& H      = out.header;
  H.colonize   = { 'C', 'O', 'L', 'O', 'N', 'I', 'Z', 'E', 0 };
  H.map_size_x = 8;
  H.map_size_y = 10;

  // TODO: add more here.
}

void expected_rn_scenario_2( rn::RootState& out ) {
  rn::wrapped::TerrainState terrain_o;

  auto& map = terrain_o.real_terrain.map;
  map =
      gfx::Matrix<rn::MapSquare>( rn::Delta{ .w = 6, .h = 8 } );
  // ...
  out.zzz_terrain = rn::TerrainState( std::move( terrain_o ) );

  // TODO: add more here.
}

/****************************************************************
** Test Cases
*****************************************************************/
// NOTE: it shouldn't really be necessary to modify this test
// case itself; instead, just add new stuff into the scenario
// preparation each time a new element is translated.
TEST_CASE( "[sav/bridge] OG to RN [scenario 1]" ) {
  sav::ColonySAV classic;
  source_og_scenario_1( classic );

  rn::RootState expected;
  expected_rn_scenario_1( expected );

  rn::RootState converted;
  REQUIRE( convert_to_rn( classic, converted ) == valid );

  REQUIRE( converted.version == expected.version );
  REQUIRE( converted.settings == expected.settings );
  REQUIRE( converted.events == expected.events );
  REQUIRE( converted.units == expected.units );
  REQUIRE( converted.players == expected.players );
  REQUIRE( converted.turn == expected.turn );
  REQUIRE( converted.colonies == expected.colonies );
  REQUIRE( converted.natives == expected.natives );
  REQUIRE( converted.land_view == expected.land_view );
  REQUIRE( ( converted.zzz_terrain == expected.zzz_terrain ) );
}

// NOTE: it shouldn't really be necessary to modify this test
// case itself; instead, just add new stuff into the scenario
// preparation each time a new element is translated.
TEST_CASE( "[sav/bridge] RN to OG [scenario 2]" ) {
  rn::RootState modern;
  expected_rn_scenario_2( modern );

  sav::ColonySAV expected;
  source_og_scenario_2( expected );

  sav::ColonySAV converted;
  REQUIRE( convert_to_og( modern, converted ) == valid );

  REQUIRE( converted.header == expected.header );
  REQUIRE( converted.player == expected.player );
  REQUIRE( converted.other == expected.other );
  REQUIRE( converted.colony == expected.colony );
  REQUIRE( converted.unit == expected.unit );
  REQUIRE( converted.nation == expected.nation );
  REQUIRE( converted.dwelling == expected.dwelling );
  REQUIRE( converted.tribe == expected.tribe );
  REQUIRE( converted.stuff == expected.stuff );
  REQUIRE( ( converted.tile == expected.tile ) );
  REQUIRE( ( converted.mask == expected.mask ) );
  REQUIRE( ( converted.path == expected.path ) );
  REQUIRE( ( converted.seen == expected.seen ) );
  REQUIRE( converted.unknown_map38a == expected.unknown_map38a );
  REQUIRE( converted.unknown_map38b == expected.unknown_map38b );
  REQUIRE( converted.unknown39a == expected.unknown39a );
  REQUIRE( converted.unknown39b == expected.unknown39b );
  REQUIRE( converted.prime_resource_seed ==
           expected.prime_resource_seed );
  REQUIRE( converted.unknown39d == expected.unknown39d );
  REQUIRE( converted.trade_route == expected.trade_route );
}

} // namespace
} // namespace bridge
