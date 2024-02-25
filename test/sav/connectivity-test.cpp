/****************************************************************
**connectivity-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-12-20.
*
* Description: Unit tests for the sav/connectivity module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/connectivity.hpp"

// sav
#include "src/sav/binary.hpp"
#include "src/sav/sav-struct.hpp"

// base
#include "src/base/binary-data.hpp"
#include "src/base/random.hpp"
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace sav {
namespace {

using namespace std;

using ::base::FileBinaryIO;

/****************************************************************
** Helpers.
*****************************************************************/
uint8_t from_hex_nibble( char c ) {
  c = tolower( c );
  if( c >= '0' && c <= '9' ) return c - '0';
  if( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
  return 255;
}

fs::path classic_sav_dir() {
  return testing::data_dir() / "saves" / "classic";
}

[[maybe_unused]] uint8_t bit_field_to_uint( auto o ) {
  static_assert( sizeof( uint8_t ) == sizeof( o ) );
  return bit_cast<uint8_t>( o );
}

[[maybe_unused]] void print_board( string_view name,
                                   auto const& board ) {
  fmt::println( "  {}:", name );
  for( int qy = 0; qy < 18; ++qy ) {
    fmt::print( "    " );
    for( int qx = 0; qx < 15; ++qx ) {
      int const     offset = qx * 18 + qy;
      auto const    val    = board[offset];
      uint8_t const casted = bit_field_to_uint( val );
      fmt::print( "{:02x} ", casted );
    }
    fmt::println( "" );
  }
}

[[maybe_unused]] void print_connectivity_old_new(
    auto const& old, auto const& new_ ) {
  print_board( "OLD", old );
  print_board( "NEW", new_ );
}

void de_bug_sea_lane( auto& connectivity ) {
  using Elem =
      std::remove_reference_t<decltype( connectivity[0] )>;
  static Elem const kEmpty = Elem{};
  for( int qy = 0; qy < 18 - 1; ++qy ) {
    for( int qx = 0; qx < 15 - 1; ++qx ) {
      int const offset = qx * 18 + qy;
      // The bug has only been seen to happen when this is not
      // the case.
      if( connectivity[offset] != kEmpty ) continue;
      int const offset_down            = qx * 18 + ( qy + 1 );
      int const offset_right           = ( qx + 1 ) * 18 + qy;
      connectivity[offset_down].neast  = false;
      connectivity[offset_right].swest = false;
    }
  }
}

void de_bug_land( auto& connectivity ) {
  for( int qy = 0; qy < 18 - 1; ++qy ) {
    for( int qx = 0; qx < 15 - 1; ++qx ) {
      int const offset_down            = qx * 18 + ( qy + 1 );
      int const offset_right           = ( qx + 1 ) * 18 + qy;
      connectivity[offset_down].neast  = false;
      connectivity[offset_right].swest = false;
    }
  }
}

auto reproduces_sav_connectivity_with_bug_removed(
    fs::path const& folder, fs::path const& file ) {
  static ColonySAV sav;
  fs::path const   in = folder / file;
  CHECK_HAS_VALUE( load_binary( in, sav ) );
  CONNECTIVITY new_connectivity;
  populate_connectivity( sav.tile, sav.path,
                         { .w = 58, .h = 72 },
                         new_connectivity );
  de_bug_sea_lane( new_connectivity.sea_lane_connectivity );
  de_bug_sea_lane( sav.connectivity.sea_lane_connectivity );
  de_bug_land( new_connectivity.land_connectivity );
  de_bug_land( sav.connectivity.land_connectivity );
#if 0
  print_connectivity_old_new(
      sav.connectivity.sea_lane_connectivity,
      new_connectivity.sea_lane_connectivity );
  print_connectivity_old_new(
      sav.connectivity.land_connectivity,
      new_connectivity.land_connectivity );
#endif
  bool const sea_lane_equal_without_bug =
      ( new_connectivity.sea_lane_connectivity ==
        sav.connectivity.sea_lane_connectivity );
  bool const land_equal_without_bug =
      ( new_connectivity.land_connectivity ==
        sav.connectivity.land_connectivity );
  return pair{ sea_lane_equal_without_bug,
               land_equal_without_bug };
}

void produce_connectivity_for_sav( fs::path const& folder,
                                   fs::path const& file,
                                   CONNECTIVITY&   out ) {
  static ColonySAV sav;
  fs::path const   in = folder / file;
  CHECK_HAS_VALUE( load_binary( in, sav ) );
  populate_connectivity( sav.tile, sav.path,
                         { .w = 58, .h = 72 }, out );
#if 0
  print_connectivity_old_new(
      sav.connectivity.sea_lane_connectivity,
      out.sea_lane_connectivity );
  print_connectivity_old_new(
  sav.connectivity.land_connectivity,
                              out.land_connectivity );
#endif
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[sav/connectivity] reproduction of SAV files with bug "
    "suppressed" ) {
  fs::path const classic_dir = classic_sav_dir() / "1990s";
  fs::path const modern_dir =
      classic_sav_dir() / "dutch-viceroy-playthrough";
  fs::path const rand_dir = classic_sav_dir() / "rand";

  // clang-format off
  static vector<pair<fs::path, fs::path>> const paths{
    // Only use one of these dutch playthrough files because they
    // will all have the same map-related data.
    { modern_dir, "dutch-viceroy-1492-options-set-cheat-enabled.SAV" },
    // ----------------------------------------------------------
    { rand_dir, "1492-1.SAV" },
    { rand_dir, "1492-2.SAV" },
    { rand_dir, "1492-3.SAV" },
    { rand_dir, "1492-4.SAV" },
    { rand_dir, "1492-5.SAV" },
    { rand_dir, "1492-6.SAV" },
    { rand_dir, "1492-7.SAV" },
    { rand_dir, "1492-8.SAV" },
    { rand_dir, "1492-9.SAV" },
    { rand_dir, "1492-10.SAV" },
    // ----------------------------------------------------------
    { classic_dir, "COLONY00.SAV" },
    { classic_dir, "COLONY01.SAV" },
    { classic_dir, "COLONY02.SAV" },
  };
  // clang-format on

  // Can't do all of these cause it's too slow.
  base::random rd;
  auto& [dir, file] = rd.pick_one( paths );
  INFO( fmt::format( "path: {}", dir / file ) );
  auto const [sea_lane_equal_without_bug,
              land_equal_without_bug] =
      reproduces_sav_connectivity_with_bug_removed( dir, file );
  REQUIRE( sea_lane_equal_without_bug );
  REQUIRE( land_equal_without_bug );
}

// This one is so that we have a test of the output that does not
// require supressing the neast/swest bits which is done in the
// test above to allow comparing the actual output with the con-
// tents of the SAV file that were likely generated using a buggy
// algorithm.
TEST_CASE( "[sav/connectivity] reproduction of SAV files" ) {
  static fs::path const dir =
      classic_sav_dir() / "dutch-viceroy-playthrough";
  static fs::path const file =
      "dutch-viceroy-1492-options-set-cheat-enabled.SAV";
  INFO( fmt::format( "path: {}", dir / file ) );

  // clang-format off
  static array<uint8_t, 270> const expected_sea_lane_bytes{
    0x1c,0x1f,0x17,0x13,0x19,0x1d,0x1f,0x1f,0x1f,
    0x1f,0x1f,0x1f,0x17,0x1b,0x1d,0x1f,0x1f,0x07,
    0x70,0xf1,0xe1,0x00,0x00,0xf8,0xfd,0xff,0xff,
    0xff,0xff,0xff,0xef,0x00,0xf0,0xfd,0xff,0xc7,
    0x1c,0x07,0x00,0x00,0x00,0x00,0xf4,0xf3,0xf9,
    0xfd,0xf7,0xfb,0xdd,0x87,0x00,0x78,0xfd,0xc7,
    0x74,0xc3,0x00,0x00,0x00,0x1c,0x67,0x00,0x00,
    0xfe,0xe7,0x00,0xf8,0xc5,0x00,0x00,0xfe,0xc7,
    0x64,0x00,0x0c,0x00,0x00,0x7a,0xc5,0x1c,0x37,
    0x7f,0xcf,0x00,0x00,0xc0,0x1e,0x3f,0x7f,0xc7,
    0x44,0x00,0x5e,0x97,0x23,0x00,0xc0,0x7c,0xff,
    0x7f,0xdf,0x9f,0x1f,0x3f,0x7f,0xff,0xff,0xc7,
    0x5c,0x3f,0x7f,0xef,0x00,0x00,0x00,0x7e,0xfd,
    0xff,0xf7,0xf3,0xf1,0xf1,0xff,0xff,0xff,0xc7,
    0x74,0xf3,0xf1,0xd9,0x9d,0x1f,0x37,0x43,0xf4,
    0xfb,0xe5,0x00,0x00,0x38,0x7d,0xff,0xff,0xc7,
    0x6c,0x00,0x00,0x00,0xf0,0xf1,0xe1,0x00,0x60,
    0x00,0xc0,0x00,0x00,0x1c,0xff,0xff,0xff,0xc7,
    0x5c,0x87,0x00,0x1c,0x1f,0x17,0x03,0x00,0x00,
    0x1e,0x1f,0x17,0x13,0x75,0xfb,0xfd,0xff,0xc7,
    0x7c,0xcf,0x00,0x7e,0xf7,0xe3,0x00,0x1e,0x3f,
    0x7f,0xff,0xef,0x00,0x60,0x00,0xf8,0xfd,0xc7,
    0x7c,0xdf,0xbf,0x7f,0xff,0x1f,0x3f,0x7f,0xf7,
    0xff,0xff,0xd7,0x8f,0x00,0x1e,0x1f,0xff,0xc7,
    0x7c,0xff,0xf3,0xff,0xff,0xff,0xff,0xff,0xe7,
    0x7e,0xff,0xff,0x5f,0xbf,0x7f,0xff,0xff,0xc7,
    0x70,0xf1,0xb1,0x71,0xf1,0xf1,0xf1,0xf1,0xf1,
    0x71,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xc1,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  };
  // clang-format on

  // clang-format off
  static array<uint8_t, 270> const expected_land_bytes{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x14,0x1b,0x1d,0x1f,0x0f,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x18,0x0d,0x00,0x00,0x00,
    0x60,0x00,0xfc,0xff,0xdf,0x87,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0xde,0x87,0x00,0x00,
    0x00,0x00,0x7e,0xff,0xf7,0xc3,0x1c,0x17,0x03,
    0x00,0x00,0x4c,0x00,0x34,0x73,0xc1,0x00,0x00,
    0x00,0x36,0x73,0xf1,0xe1,0x00,0x70,0xe5,0x00,
    0x00,0x00,0x50,0x91,0x61,0x00,0x00,0x00,0x00,
    0x38,0x65,0x00,0x00,0x1c,0x0f,0x00,0x42,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0xc0,0x00,0x00,0x7c,0xd7,0xa3,0x00,0x00,
    0x00,0x00,0x00,0x00,0x16,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x70,0xe1,0x00,0x1c,0x07,
    0x00,0x00,0x1c,0x37,0x63,0x00,0x00,0x00,0x04,
    0x00,0x10,0x1d,0x0f,0x00,0x1e,0x1f,0x7f,0xdf,
    0x0f,0x00,0x76,0xe3,0x00,0x00,0x00,0x00,0x40,
    0x00,0x00,0x76,0xd3,0xb1,0x71,0xfd,0xff,0xf7,
    0xd3,0xb1,0x61,0x00,0x00,0x0e,0x00,0x00,0x00,
    0x00,0x30,0x61,0x00,0x00,0x00,0x76,0xf3,0xe1,
    0x00,0x00,0x00,0x0c,0x3e,0x5f,0x87,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x30,0x61,0x00,0x00,
    0x06,0x00,0x00,0x70,0xf1,0xf1,0xc1,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,
    0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  };
  // clang-format on

  CONNECTIVITY expected_connectivity;
  static_assert(
      sizeof( expected_connectivity.sea_lane_connectivity ) ==
      sizeof( expected_sea_lane_bytes ) );
  for( int i = 0; i < int( expected_sea_lane_bytes.size() );
       ++i )
    expected_connectivity.sea_lane_connectivity[i] =
        bit_cast<SeaLaneConnectivity>(
            expected_sea_lane_bytes[i] );
  static_assert(
      sizeof( expected_connectivity.land_connectivity ) ==
      sizeof( expected_land_bytes ) );
  for( int i = 0; i < int( expected_land_bytes.size() ); ++i )
    expected_connectivity.land_connectivity[i] =
        bit_cast<LandConnectivity>( expected_land_bytes[i] );

  CONNECTIVITY connectivity;
  produce_connectivity_for_sav( dir, file, connectivity );
  REQUIRE( ( connectivity.sea_lane_connectivity ==
             expected_connectivity.sea_lane_connectivity ) );
  REQUIRE( ( connectivity.land_connectivity ==
             expected_connectivity.land_connectivity ) );
}

TEST_CASE( "[sav/connectivity] populate_region_ids" ) {
  vector<TILE> tiles( 58 * 72 );
  fill( tiles.begin(), tiles.end(), TILE{} );
  // Fill this in with some regions when ready to test.
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // ..........................................................
  // TODO: create some square regions algorithmically and draw
  // them in the map above, and populate them in `tiles`.

  vector<PATH> path;
  populate_region_ids( tiles, path );

  // clang-format off
  static string_view const expected_path =
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    "                                                          "
    ;
  // clang-format on
  for( int x = 0; x < 58; ++x ) {
    for( int y = 0; y < 72; ++y ) {
      INFO( fmt::format( "x={}, y={}\n", x, y ) );
      int const offset = y * 58 + x;
      static_assert( sizeof( PATH ) == sizeof( uint8_t ) );
      auto const path_byte = bit_cast<uint8_t>( path[offset] );
      uint8_t const expected_byte =
          ( expected_path[offset] == ' ' )
              ? 0
              : from_hex_nibble( expected_path[offset] );
      REQUIRE( path_byte == expected_byte );
    }
  }
}

} // namespace
} // namespace sav
