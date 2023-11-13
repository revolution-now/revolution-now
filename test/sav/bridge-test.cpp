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

using ::base::nothing;
using ::base::valid;

/****************************************************************
** Scenario 1 (must support bi-directional conversion).
*****************************************************************/
void og_bidirectional_scenario_1( sav::ColonySAV& out ) {
  // Header.
  auto& H      = out.header;
  H.colonize   = { 'C', 'O', 'L', 'O', 'N', 'I', 'Z', 'E', 0 };
  H.map_size_x = 8;
  H.map_size_y = 10;

  out.tile.resize( H.map_size_x * H.map_size_y );
  // Initialize everything to ocean. We need to do this because
  // the default values of TILE don't produce that, unlike in the
  // RN version. Also, the OG seems to put ocean around the
  // outter border as well, so this works there.
  for( sav::TILE& tile : out.tile )
    tile = sav::TILE{ .tile = sav::terrain_5bit_type::ttt };
  using T   = sav::TILE;
  using Ter = sav::terrain_5bit_type;
  using HR  = sav::hills_river_3bit_type;

  out.tile[8 + 1]  = T{ .tile = Ter::tu };
  out.tile[8 + 2]  = T{ .tile = Ter::de };
  out.tile[8 + 3]  = T{ .tile = Ter::pl };
  out.tile[8 + 4]  = T{ .tile = Ter::pr };
  out.tile[8 + 5]  = T{ .tile = Ter::gr };
  out.tile[8 + 6]  = T{ .tile = Ter::sa };
  out.tile[8 + 9]  = T{ .tile = Ter::sw };
  out.tile[8 + 10] = T{ .tile = Ter::mr };
  out.tile[8 + 11] = T{ .tile = Ter::tuf };
  out.tile[8 + 12] = T{ .tile = Ter::def };
  out.tile[8 + 13] = T{ .tile = Ter::plf };
  out.tile[8 + 14] = T{ .tile = Ter::prf };
  out.tile[8 + 17] = T{ .tile = Ter::grf };
  out.tile[8 + 18] = T{ .tile = Ter::saf };
  out.tile[8 + 19] = T{ .tile = Ter::swf };
  out.tile[8 + 20] = T{ .tile = Ter::mrf };

  out.tile[8 + 21] = T{ .tile = Ter::arc };
  out.tile[8 + 22] = T{ .tile = Ter::ttt };
  out.tile[8 + 25] = T{ .tile = Ter::tnt };
  out.tile[8 + 26] = T{ .tile = Ter::arc };

  out.tile[8 + 2].hill_river  = HR::c;
  out.tile[8 + 3].hill_river  = HR::t;
  out.tile[8 + 4].hill_river  = HR::tc;
  out.tile[8 + 5].hill_river  = HR::cc;
  out.tile[8 + 6].hill_river  = HR::tt;
  out.tile[8 + 21].hill_river = HR::empty;
  out.tile[8 + 26].hill_river = HR::cc;

  // TODO: Add more here.
}

void rn_bidirectional_scenario_1( rn::RootState& out ) {
  rn::wrapped::TerrainState terrain_o;

  vector<rn::MapSquare> squares;
  squares.resize( 6 * 8 );
  using M     = rn::MapSquare;
  squares[0]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::tundra };
  squares[1]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::desert };
  squares[2]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::plains };
  squares[3]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::prairie };
  squares[4]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::grassland };
  squares[5]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::savannah };
  squares[6]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::swamp };
  squares[7]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::marsh };
  squares[8]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::tundra,
                   .overlay = rn::e_land_overlay::forest };
  squares[9]  = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::desert,
                   .overlay = rn::e_land_overlay::forest };
  squares[10] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::plains,
                   .overlay = rn::e_land_overlay::forest };
  squares[11] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::prairie,
                   .overlay = rn::e_land_overlay::forest };
  squares[12] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::grassland,
                   .overlay = rn::e_land_overlay::forest };
  squares[13] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::savannah,
                   .overlay = rn::e_land_overlay::forest };
  squares[14] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::swamp,
                   .overlay = rn::e_land_overlay::forest };
  squares[15] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::marsh,
                   .overlay = rn::e_land_overlay::forest };

  squares[16] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::arctic };
  squares[17] = M{ .surface = rn::e_surface::water,
                   .ground  = rn::e_ground_terrain::arctic };
  squares[18] = M{ .sea_lane = true };
  squares[19] = M{ .surface = rn::e_surface::land,
                   .ground  = rn::e_ground_terrain::arctic,
                   .overlay = rn::e_land_overlay::mountains };

  squares[1].overlay  = rn::e_land_overlay::hills;
  squares[2].river    = rn::e_river::minor;
  squares[3].overlay  = rn::e_land_overlay::hills;
  squares[3].river    = rn::e_river::minor;
  squares[4].overlay  = rn::e_land_overlay::mountains;
  squares[5].river    = rn::e_river::major;
  squares[16].overlay = nothing;
  squares[19].overlay = rn::e_land_overlay::mountains;

  terrain_o.real_terrain.map =
      gfx::Matrix<rn::MapSquare>( std::move( squares ), 6 );
  out.zzz_terrain = rn::TerrainState( std::move( terrain_o ) );

  // TODO: add more here.
}

// NOTE: it shouldn't really be necessary to modify this test
// case itself; instead, just add new stuff into the scenario
// preparation each time a new element is translated.
TEST_CASE( "[sav/bridge] OG to RN [scenario 1]" ) {
  sav::ColonySAV classic;
  og_bidirectional_scenario_1( classic );

  rn::RootState expected;
  rn_bidirectional_scenario_1( expected );

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
  REQUIRE( converted.zzz_terrain == expected.zzz_terrain );
}

// NOTE: it shouldn't really be necessary to modify this test
// case itself; instead, just add new stuff into the scenario
// preparation each time a new element is translated.
TEST_CASE( "[sav/bridge] RN to OG [scenario 1]" ) {
  rn::RootState modern;
  rn_bidirectional_scenario_1( modern );

  sav::ColonySAV expected;
  og_bidirectional_scenario_1( expected );

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
  REQUIRE( converted.tile == expected.tile );
  REQUIRE( converted.mask == expected.mask );
  REQUIRE( converted.path == expected.path );
  REQUIRE( converted.seen == expected.seen );
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
