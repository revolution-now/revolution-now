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
#include "src/sav/map-file.hpp"
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
** Helpers.
*****************************************************************/
[[maybe_unused]] uint8_t bit_field_to_uint( auto o ) {
  static_assert( sizeof( uint8_t ) == sizeof( o ) );
  return bit_cast<uint8_t>( o );
}

[[maybe_unused]] void print_connectivity_board(
    string_view name, auto const& board, int width,
    int height ) {
  fmt::println( "  {}:", name );
  int const height_quads = ( height + 3 ) / 4;
  int const width_quads  = ( width + 3 ) / 4;
  for( int qy = 0; qy < height_quads; ++qy ) {
    fmt::print( "    " );
    for( int qx = 0; qx < width_quads; ++qx ) {
      int const     offset = qx * height_quads + qy;
      auto const    val    = board[offset];
      uint8_t const casted = bit_field_to_uint( val );
      fmt::print( "{:02x} ", casted );
    }
    fmt::println( "" );
  }
}

[[maybe_unused]] void print_connectivity_old_new(
    auto const& old, auto const& new_, int width, int height ) {
  print_connectivity_board( "OLD", old, width, height );
  print_connectivity_board( "NEW", new_, width, height );
}

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

  // Keep this updated.
  //
  //   x: land
  //   o: water
  //
  //   . . . .  . . . .
  //   . x x x  x x x .
  //   . x x x  x x x .
  //   . x x x  x x o .
  //
  //   . o x .  . . . .
  //   . . . .  . . . .
  //   . . . .  . . . .
  //   . . . .  . . . .
  //
  //   . . . .  . . . .
  //   . . . .  . . . .
  //
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

  out.path = vector<sav::PATH>( 8 * 10 ); // TODO: temporary.

  // Index goes down columns first.
  //
  //   04 40
  //   00 00
  //   00 00
  //
  out.connectivity.land_connectivity[0].east = true;
  out.connectivity.land_connectivity[3].west = true;

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
TEST_CASE( "[sav/bridge] OG to NG [scenario 1]" ) {
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
TEST_CASE( "[sav/bridge] NG to OG [scenario 1]" ) {
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
#if 0
  print_connectivity_old_new(
      expected.connectivity.sea_lane_connectivity,
      converted.connectivity.sea_lane_connectivity, 8, 10 );
  print_connectivity_old_new(
      expected.connectivity.land_connectivity,
      converted.connectivity.land_connectivity, 8, 10 );
#endif
  REQUIRE( ( converted.connectivity == expected.connectivity ) );
  REQUIRE( converted.unknown_map38c2 ==
           expected.unknown_map38c2 );
  REQUIRE( converted.unknown_map38c3 ==
           expected.unknown_map38c3 );
  REQUIRE( converted.strategy == expected.strategy );
  REQUIRE( converted.unknown_map38d == expected.unknown_map38d );
  REQUIRE( converted.prime_resource_seed ==
           expected.prime_resource_seed );
  REQUIRE( converted.unknown39d == expected.unknown39d );
  REQUIRE( converted.trade_route == expected.trade_route );
}

/****************************************************************
** Scenario 2 (only for OG -> NG).
*****************************************************************/
// You can put things in this scenario that only support con-
// verting from the OG to the NG.
void og_unidirectional_scenario_1( sav::ColonySAV& out ) {
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
  out.tile[8 + 11] = T{ .tile = Ter::tuw };
  out.tile[8 + 12] = T{ .tile = Ter::dew };
  out.tile[8 + 13] = T{ .tile = Ter::plw };
  out.tile[8 + 14] = T{ .tile = Ter::prw };
  out.tile[8 + 17] = T{ .tile = Ter::grw };
  out.tile[8 + 18] = T{ .tile = Ter::saw };
  out.tile[8 + 19] = T{ .tile = Ter::sww };
  out.tile[8 + 20] = T{ .tile = Ter::mrw };

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

void rn_unidirectional_scenario_1( rn::RootState& out ) {
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
TEST_CASE( "[sav/bridge] OG to NG [scenario 2]" ) {
  sav::ColonySAV classic;
  og_unidirectional_scenario_1( classic );

  rn::RootState expected;
  rn_unidirectional_scenario_1( expected );

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

// This test should not ever need to be changed.
TEST_CASE( "[sav/bridge] OG to NG [MapFile]" ) {
  using T   = sav::TILE;
  using MS  = rn::MapSquare;
  using t5t = sav::terrain_5bit_type;
  using hr3 = sav::hills_river_3bit_type;

  sav::MapFile classic{ .map_size_x = 5 + 2,
                        .map_size_y = 6 + 2 };
  classic.tile.resize( ( 5 + 2 ) * ( 6 + 2 ) );

  rn::MapSquare   expected;
  rn::RealTerrain modern;
  modern.map.reset( { .w = 5, .h = 6 } );

  auto classic_at = [&]( rn::Coord coord ) -> decltype( auto ) {
    coord.x += 1;
    coord.y += 1;
    int const width = 5 + 2;
    int const idx   = coord.x + ( coord.y * width );
    BASE_CHECK( idx < int( classic.tile.size() ) );
    return classic.tile[idx];
  };

  classic_at( { .x = 0, .y = 0 } ) = T{ .tile = t5t::ttt };
  classic_at( { .x = 1, .y = 0 } ) = T{ .tile = t5t::tnt };
  classic_at( { .x = 2, .y = 0 } ) = T{ .tile = t5t::tu };
  classic_at( { .x = 3, .y = 0 } ) = T{ .tile = t5t::mr };
  classic_at( { .x = 4, .y = 0 } ) = T{ .tile = t5t::sw };
  classic_at( { .x = 0, .y = 1 } ) = T{ .tile = t5t::arc };
  classic_at( { .x = 1, .y = 1 } ) =
      T{ .tile = t5t::de, .hill_river = hr3::c };
  classic_at( { .x = 2, .y = 1 } ) =
      T{ .tile = t5t::pl, .hill_river = hr3::cc };
  classic_at( { .x = 3, .y = 1 } ) =
      T{ .tile = t5t::pr, .hill_river = hr3::t };
  classic_at( { .x = 4, .y = 1 } ) =
      T{ .tile = t5t::sa, .hill_river = hr3::tt };
  classic_at( { .x = 0, .y = 2 } ) =
      T{ .tile = t5t::gr, .hill_river = hr3::tc };
  classic_at( { .x = 1, .y = 2 } ) = T{ .tile = t5t::tuf };
  classic_at( { .x = 2, .y = 2 } ) = T{ .tile = t5t::grw };
  classic_at( { .x = 3, .y = 2 } ) = T{ .tile = t5t::saf };
  classic_at( { .x = 4, .y = 2 } ) = T{ .tile = t5t::prw };
  classic_at( { .x = 0, .y = 3 } ) = T{ .tile = t5t::plf };
  classic_at( { .x = 1, .y = 3 } ) = T{ .tile = t5t::dew };
  classic_at( { .x = 2, .y = 3 } ) =
      T{ .tile = t5t::arc, .hill_river = hr3::cc };
  classic_at( { .x = 3, .y = 3 } ) = T{ .tile = t5t::swf };
  classic_at( { .x = 4, .y = 3 } ) = T{ .tile = t5t::mrw };

  REQUIRE( convert_to_rn( classic, modern ) == valid );

  auto const& map = modern.map;
  REQUIRE( map.size() == gfx::size{ .w = 5, .h = 6 } );

  expected = MS{ .surface = rn::e_surface::water };
  REQUIRE( map[{ .x = 0, .y = 0 }] == expected );
  expected =
      MS{ .surface = rn::e_surface::water, .sea_lane = true };
  REQUIRE( map[{ .x = 1, .y = 0 }] == expected );

  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 2, .y = 0 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::marsh };
  REQUIRE( map[{ .x = 3, .y = 0 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::swamp };
  REQUIRE( map[{ .x = 4, .y = 0 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::arctic };
  REQUIRE( map[{ .x = 0, .y = 1 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::desert,
                 .overlay = rn::e_land_overlay::hills };
  REQUIRE( map[{ .x = 1, .y = 1 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::plains,
                 .overlay = rn::e_land_overlay::mountains };
  REQUIRE( map[{ .x = 2, .y = 1 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::prairie,
                 .river   = rn::e_river::minor };
  REQUIRE( map[{ .x = 3, .y = 1 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::savannah,
                 .river   = rn::e_river::major };
  REQUIRE( map[{ .x = 4, .y = 1 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::grassland,
                 .overlay = rn::e_land_overlay::hills,
                 .river   = rn::e_river::minor };
  REQUIRE( map[{ .x = 0, .y = 2 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 1, .y = 2 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::grassland,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 2, .y = 2 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::savannah,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 3, .y = 2 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::prairie,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 4, .y = 2 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::plains,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 0, .y = 3 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::desert,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 1, .y = 3 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::arctic,
                 .overlay = rn::e_land_overlay::mountains };
  REQUIRE( map[{ .x = 2, .y = 3 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::swamp,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 3, .y = 3 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::marsh,
                 .overlay = rn::e_land_overlay::forest };
  REQUIRE( map[{ .x = 4, .y = 3 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 0, .y = 4 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 1, .y = 4 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 2, .y = 4 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 3, .y = 4 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 4, .y = 4 }] == expected );
  REQUIRE( map[{ .x = 0, .y = 5 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 1, .y = 5 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 2, .y = 5 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 3, .y = 5 }] == expected );
  expected = MS{ .surface = rn::e_surface::land,
                 .ground  = rn::e_ground_terrain::tundra };
  REQUIRE( map[{ .x = 4, .y = 5 }] == expected );
}

// This test should not ever need to be changed. NOTE: in this
// test we don't involve the "w" forest variants, e.g.
// terrain_5bit_type::tuw, because this conversion happens in the
// direction of NG -> OG, and so in that case we get to choose
// which variant we want, and we always go for the more modern
// "f" variants, e.g. terrain_5bit_type::tuf.
TEST_CASE( "[sav/bridge] NG to OG [MapFile]" ) {
  using t5t = sav::terrain_5bit_type;
  using hr3 = sav::hills_river_3bit_type;
  using MS  = rn::MapSquare;

  sav::TILE    expected;
  sav::MapFile classic;

  rn::RealTerrain modern;
  modern.map.reset( { .w = 5, .h = 6 } );

  auto modern_at = [&]( rn::Coord coord ) -> decltype( auto ) {
    return modern.map[coord];
  };

  modern_at( { .x = 0, .y = 0 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::tundra };
  modern_at( { .x = 1, .y = 0 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::desert };
  modern_at( { .x = 2, .y = 0 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::plains };
  modern_at( { .x = 3, .y = 0 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::prairie };
  modern_at( { .x = 4, .y = 0 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::grassland };
  modern_at( { .x = 0, .y = 1 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::savannah };
  modern_at( { .x = 1, .y = 1 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::marsh };
  modern_at( { .x = 2, .y = 1 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::swamp };
  modern_at( { .x = 3, .y = 1 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::tundra,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 4, .y = 1 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::desert,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 0, .y = 2 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::plains,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 1, .y = 2 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::prairie,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 2, .y = 2 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::grassland,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 3, .y = 2 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::savannah,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 4, .y = 2 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::marsh,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 0, .y = 3 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::swamp,
          .overlay = rn::e_land_overlay::forest };
  modern_at( { .x = 1, .y = 3 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::arctic };
  modern_at( { .x = 2, .y = 3 } ) =
      MS{ .surface = rn::e_surface::water, .sea_lane = true };
  modern_at( { .x = 3, .y = 3 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::arctic,
          .overlay = rn::e_land_overlay::mountains };
  modern_at( { .x = 4, .y = 3 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::grassland,
          .river   = rn::e_river::major };
  modern_at( { .x = 0, .y = 4 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::grassland,
          .overlay = rn::e_land_overlay::hills,
          .river   = rn::e_river::minor };
  modern_at( { .x = 1, .y = 4 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::grassland,
          .river   = rn::e_river::minor };
  modern_at( { .x = 2, .y = 4 } ) =
      MS{ .surface = rn::e_surface::land,
          .ground  = rn::e_ground_terrain::grassland,
          .overlay = rn::e_land_overlay::hills };

  REQUIRE( convert_to_og( modern, classic ) == valid );

  auto const& tile = classic.tile;
  REQUIRE( tile.size() == ( 5 + 2 ) * ( 6 + 2 ) );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 0 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[1 + ( 0 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[2 + ( 0 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[3 + ( 0 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[4 + ( 0 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[5 + ( 0 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 0 * 7 )] == expected );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 1 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::tu };
  REQUIRE( tile[1 + ( 1 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::de };
  REQUIRE( tile[2 + ( 1 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::pl };
  REQUIRE( tile[3 + ( 1 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::pr };
  REQUIRE( tile[4 + ( 1 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::gr };
  REQUIRE( tile[5 + ( 1 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 1 * 7 )] == expected );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 2 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::sa };
  REQUIRE( tile[1 + ( 2 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::mr };
  REQUIRE( tile[2 + ( 2 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::sw };
  REQUIRE( tile[3 + ( 2 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::tuf };
  REQUIRE( tile[4 + ( 2 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::def };
  REQUIRE( tile[5 + ( 2 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 2 * 7 )] == expected );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 3 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::plf };
  REQUIRE( tile[1 + ( 3 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::prf };
  REQUIRE( tile[2 + ( 3 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::grf };
  REQUIRE( tile[3 + ( 3 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::saf };
  REQUIRE( tile[4 + ( 3 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::mrf };
  REQUIRE( tile[5 + ( 3 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 3 * 7 )] == expected );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 4 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::swf };
  REQUIRE( tile[1 + ( 4 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::arc };
  REQUIRE( tile[2 + ( 4 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::tnt };
  REQUIRE( tile[3 + ( 4 * 7 )] == expected );
  expected =
      sav::TILE{ .tile = t5t::arc, .hill_river = hr3::cc };
  REQUIRE( tile[4 + ( 4 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::gr, .hill_river = hr3::tt };
  REQUIRE( tile[5 + ( 4 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 4 * 7 )] == expected );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 5 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::gr, .hill_river = hr3::tc };
  REQUIRE( tile[1 + ( 5 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::gr, .hill_river = hr3::t };
  REQUIRE( tile[2 + ( 5 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::gr, .hill_river = hr3::c };
  REQUIRE( tile[3 + ( 5 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[4 + ( 5 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[5 + ( 5 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 5 * 7 )] == expected );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 6 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[1 + ( 6 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[2 + ( 6 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[3 + ( 6 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[4 + ( 6 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[5 + ( 6 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 6 * 7 )] == expected );

  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[0 + ( 7 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[1 + ( 7 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[2 + ( 7 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[3 + ( 7 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[4 + ( 7 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[5 + ( 7 * 7 )] == expected );
  expected = sav::TILE{ .tile = t5t::ttt };
  REQUIRE( tile[6 + ( 7 * 7 )] == expected );
}

} // namespace
} // namespace bridge
