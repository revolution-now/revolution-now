/****************************************************************
**rcl-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-07.
*
* Description: Unit tests for the sav/rcl module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/rcl.hpp"

// sav
#include "sav-struct.hpp"

// base
#include "src/base/binary-data.hpp"
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace sav {
namespace {

using namespace std;

using ::base::BinaryBuffer;

/****************************************************************
** Helpers.
*****************************************************************/
fs::path classic_sav_dir() {
  return testing::data_dir() / "saves" / "classic";
}

bool file_contents_same( fs::path const& f1,
                         fs::path const& f2 ) {
  UNWRAP_CHECK( b1, BinaryBuffer::from_file( f1 ) );
  UNWRAP_CHECK( b2, BinaryBuffer::from_file( f2 ) );
  return b1 == b2;
}

fs::path output_folder() {
  error_code ec  = {};
  fs::path   res = fs::temp_directory_path( ec );
  BASE_CHECK( ec.value() == 0,
              "failed to get temp folder path." );
  return res;
}

bool json_round_trip( fs::path const& folder,
                      fs::path const& file,
                      fs::path const& tmp ) {
  static ColonySAV sav;
  fs::path const   in  = folder / file;
  fs::path const   out = tmp / file;
  CHECK_HAS_VALUE( load_rcl_from_file( in, sav ) );
  CHECK_HAS_VALUE(
      save_rcl_to_file( out, sav, e_rcl_dialect::json ) );
  return file_contents_same( in, out );
}

template<typename T>
T const& pick_one( vector<T> const& v ) {
  BASE_CHECK( !v.empty() );
  random_device         r;
  default_random_engine e1( r() );

  uniform_int_distribution<int> uniform_dist( 0, v.size() - 1 );
  return v[uniform_dist( e1 )];
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[sav/rcl] json roundtrip" ) {
  fs::path const tmp = output_folder();

  fs::path const classic_dir =
      classic_sav_dir() / "1990s" / "json";
  fs::path const modern_dir =
      classic_sav_dir() / "dutch-viceroy-playthrough" / "json";

  // clang-format off
    static vector<pair<fs::path, fs::path>> const paths{
        { modern_dir, "dutch-viceroy-1492-options-set-cheat-enabled.SAV.json" },
        { modern_dir, "dutch-viceroy-1492-start.SAV.json" },
        { modern_dir, "dutch-viceroy-1500-first-colony.SAV.json" },
        { modern_dir, "dutch-viceroy-1516-two-colonies-two-trade-routes.SAV.json" },
        { modern_dir, "dutch-viceroy-1519-at-war-with-natives-with-treasure.SAV.json" },
        { modern_dir, "dutch-viceroy-1524-met-the-english-at-war.SAV.json" },
        { modern_dir, "dutch-viceroy-1525-just-after-declaration.SAV.json" },
        { modern_dir, "dutch-viceroy-1525-just-before-declaration.SAV.json" },
        { modern_dir, "dutch-viceroy-1526-ref-landed.SAV.json" },
        { modern_dir, "dutch-viceroy-1533-mid-war.SAV.json" },
        { modern_dir, "dutch-viceroy-1534-all-tribes-killed.SAV.json" },
        { modern_dir, "dutch-viceroy-1534-most-tribes-killed.SAV.json" },
        { modern_dir, "dutch-viceroy-1542-almost-won-independence.SAV.json" },
        { modern_dir, "dutch-viceroy-1551-ref-gone-win-next-turn.SAV.json" },
        { modern_dir, "dutch-viceroy-1552-post-independence-keep-playing.SAV.json" },
        { modern_dir, "dutch-viceroy-1560-post-independence-cleared-all-entities.SAV.json" },
        { classic_dir, "COLONY00.SAV.json" },
        { classic_dir, "COLONY01.SAV.json" },
        { classic_dir, "COLONY02.SAV.json" },
        { classic_dir, "COLONY08.SAV.json" },
        { classic_dir, "COLONY09.SAV.json" },
        { classic_dir, "COLONY10.SAV.json" },
    };
  // clang-format on

  // Can't do all of these cause it's too slow.
  auto& [dir, file] = pick_one( paths );
  INFO( fmt::format( "path: {}", dir / file ) );
  bool const good = json_round_trip( dir, file, tmp );
  REQUIRE( good );
}

} // namespace
} // namespace sav
