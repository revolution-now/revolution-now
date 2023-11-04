/****************************************************************
**binary-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-01.
*
* Description: Unit tests for the sav/binary module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/binary.hpp"

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

bool attempt_round_trip( fs::path const& folder,
                         fs::path const& file,
                         fs::path const& tmp ) {
  static ColonySAV sav;
  fs::path const   in  = folder / file;
  fs::path const   out = tmp / file;
  CHECK_HAS_VALUE( load_binary( in, sav ) );
  CHECK_HAS_VALUE( save_binary( out, sav ) );
  return file_contents_same( in, out );
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[sav/binary] round trip" ) {
  fs::path const tmp = output_folder();

  SECTION( "new" ) {
    fs::path const folder =
        classic_sav_dir() / "dutch-viceroy-playthrough";

    // clang-format off
    static fs::path const paths[] = {
        "dutch-viceroy-1492-options-set-cheat-enabled.SAV",
        "dutch-viceroy-1492-start.SAV",
        "dutch-viceroy-1500-first-colony.SAV",
        "dutch-viceroy-1516-two-colonies-two-trade-routes.SAV",
        "dutch-viceroy-1519-at-war-with-natives-with-treasure.SAV",
        "dutch-viceroy-1524-met-the-english-at-war.SAV",
        "dutch-viceroy-1525-just-after-declaration.SAV",
        "dutch-viceroy-1525-just-before-declaration.SAV",
        "dutch-viceroy-1526-ref-landed.SAV",
        "dutch-viceroy-1533-mid-war.SAV",
        "dutch-viceroy-1534-all-tribes-killed.SAV",
        "dutch-viceroy-1534-most-tribes-killed.SAV",
        "dutch-viceroy-1542-almost-won-independence.SAV",
        "dutch-viceroy-1551-ref-gone-win-next-turn.SAV",
        "dutch-viceroy-1552-post-independence-keep-playing.SAV",
        "dutch-viceroy-1560-post-independence-cleared-all-"
        "entities.SAV",
    };
    // clang-format on

    for( auto const& file : paths ) {
      INFO( fmt::format( "path: {}", file ) );
      bool const good = attempt_round_trip( folder, file, tmp );
      REQUIRE( good );
    }
  }

  SECTION( "old" ) {
    fs::path const folder = classic_sav_dir() / "1990s";

    static fs::path const paths[] = {
        "COLONY00.SAV", "COLONY01.SAV", "COLONY02.SAV",
        "COLONY08.SAV", "COLONY09.SAV", "COLONY10.SAV",
    };

    for( auto const& file : paths ) {
      INFO( fmt::format( "path: {}", file ) );
      bool const good = attempt_round_trip( folder, file, tmp );
      REQUIRE( good );
    }
  }
}

} // namespace
} // namespace sav
