/****************************************************************
**fs-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: Unit tests for the base/fs module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/fs.hpp"

// base
#include "base/to-str-ext-std.hpp"

// C++ standard library.
#include <fstream>

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

using ::Catch::StartsWith;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/fs] copy_file_overwriting_destination" ) {
  fs::path const tmpdir  = fs::temp_directory_path();
  fs::path const datadir = testing::data_dir();

  fs::path src;
  fs::path dst;

  auto f = [&] {
    return copy_file_overwriting_destination( src, dst );
  };

  SECTION( "src/dst empty" ) {
    src            = "";
    dst            = "";
    auto const res = f();
    REQUIRE( !res.valid() );
    REQUIRE( res.error() ==
             "failed to copy file \"\" to \"\": code: 2, error: "
             "filesystem error: cannot copy file: No such file "
             "or directory [] []" );
  }

  SECTION( "src not exist, dst not exist" ) {
    src = tmpdir / "abcdefghi";
    dst = tmpdir / "bcdefghij.txt";
    if( fs::exists( src ) ) fs::remove( src );
    if( fs::exists( dst ) ) fs::remove( dst );
    BASE_CHECK( !fs::exists( src ) );
    BASE_CHECK( !fs::exists( dst ) );
    auto const res = f();
    REQUIRE( !res.valid() );
    REQUIRE_THAT( res.error(),
                  StartsWith( "failed to copy file" ) );
    REQUIRE( !fs::exists( src ) );
    REQUIRE( !fs::exists( dst ) );
  }

  SECTION( "src not exist, dst exist" ) {
    // In this test case it is ok to use a dst file that is ver-
    // sioned because it is not supposed to be affected, given
    // that the source file does not exist. And actually this is
    // something that we should be testing anyway.
    src = tmpdir / "abcdefghi";
    dst = datadir / "text-files" / "oneline.txt";
    if( fs::exists( src ) ) fs::remove( src );
    BASE_CHECK( !fs::exists( src ) );
    BASE_CHECK( fs::exists( dst ) );
    auto const prev_size = fs::file_size( dst );
    auto const res       = f();
    REQUIRE( !res.valid() );
    REQUIRE_THAT( res.error(),
                  StartsWith( "failed to copy file" ) );
    REQUIRE( !fs::exists( src ) );
    REQUIRE( fs::exists( dst ) );
    REQUIRE( fs::file_size( dst ) == prev_size );
  }

  SECTION( "src exist, dst not exist" ) {
    src = datadir / "text-files" / "oneline.txt";
    dst = tmpdir / "bcdefghij.txt";
    if( fs::exists( dst ) ) fs::remove( dst );
    BASE_CHECK( fs::exists( src ) );
    BASE_CHECK( !fs::exists( dst ) );
    REQUIRE( f() == valid );
    REQUIRE( fs::exists( src ) );
    REQUIRE( fs::exists( dst ) );
    REQUIRE( fs::file_size( dst ) == fs::file_size( src ) );
  }

  SECTION( "src exist, dst exist" ) {
    src = datadir / "text-files" / "oneline.txt";
    dst = tmpdir / "bcdefghij.txt";
    if( fs::exists( dst ) ) fs::remove( dst );
    { ofstream{ dst }.put( 'a' ); }
    BASE_CHECK( fs::exists( src ) );
    BASE_CHECK( fs::exists( dst ) );
    BASE_CHECK_NEQ( fs::file_size( dst ), fs::file_size( src ) );
    REQUIRE( f() == valid );
    REQUIRE( fs::exists( src ) );
    REQUIRE( fs::exists( dst ) );
    REQUIRE( fs::file_size( dst ) == fs::file_size( src ) );
  }
}

} // namespace
} // namespace base
