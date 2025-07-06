/****************************************************************
**io.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: Unit tests for the src/io.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/io.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <cstring>

namespace base {
namespace {

using namespace std;

fs::path text_path( fs::path const& p ) {
  return testing::data_dir() / "text-files" / p;
}

TEST_CASE( "[io] non-existent" ) {
  auto res = read_text_file( "/a/b/c.txt" );
  REQUIRE( res == e_error_read_text_file::file_does_not_exist );
}

TEST_CASE( "[io] empty" ) {
  fs::path p = text_path( "empty.txt" );
  REQUIRE( fs::file_size( p ) == 0 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 0;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE( s == "" );

  // Only for unix line endings.
  REQUIRE( fs::file_size( p ) == s.size() );
}

TEST_CASE( "[io] single char" ) {
  fs::path p = text_path( "single-char.txt" );
  REQUIRE( fs::file_size( p ) == 1 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 1;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE( s == "L" );

  // Only for unix line endings.
  REQUIRE( fs::file_size( p ) == s.size() );
}

TEST_CASE( "[io] one line" ) {
  fs::path p = text_path( "oneline.txt" );
  REQUIRE( fs::file_size( p ) == 26 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 26;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE( s == "Lorem ipsum dolor sit amet" );

  // Only for unix line endings.
  REQUIRE( fs::file_size( p ) == s.size() );
}

TEST_CASE( "[io] small-unix" ) {
  fs::path p = text_path( "small-unix.txt" );
  REQUIRE( fs::file_size( p ) == 445 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 445;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE(
      s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                     "adipiscing elit, sed do\n"
                     "eiusmod tempor incididunt ut labore et "
                     "dolore magna aliqua. Ut" ) );

  // Only for unix line endings.
  REQUIRE( fs::file_size( p ) == s.size() );
}

TEST_CASE( "[io] small-win" ) {
  fs::path p = text_path( "small-win.txt" );
  REQUIRE( fs::file_size( p ) == 451 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 445;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE(
      s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                     "adipiscing elit, sed do\n"
                     "eiusmod tempor incididunt ut labore et "
                     "dolore magna aliqua. Ut" ) );

  REQUIRE( fs::file_size( p ) > s.size() );
}

TEST_CASE( "[io] medium-unix" ) {
  fs::path p = text_path( "medium-unix.txt" );
  REQUIRE( fs::file_size( p ) == 6706 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 6706;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE(
      s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                     "adipiscing elit, sed do\n"
                     "eiusmod tempor incididunt ut labore et "
                     "dolore magna aliqua. Ut" ) );

  // Only for unix line endings.
  REQUIRE( fs::file_size( p ) == s.size() );
}

TEST_CASE( "[io] medium-win" ) {
  fs::path p = text_path( "medium-win.txt" );
  REQUIRE( fs::file_size( p ) == 6827 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 6706;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE(
      s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                     "adipiscing elit, sed do\n"
                     "eiusmod tempor incididunt ut labore et "
                     "dolore magna aliqua. Ut" ) );

  REQUIRE( fs::file_size( p ) > s.size() );
}

TEST_CASE( "[io] large-unix" ) {
  fs::path p = text_path( "large-unix.txt" );
  REQUIRE( fs::file_size( p ) == 28619 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 28619;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE(
      s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                     "adipiscing elit, sed do\n"
                     "eiusmod tempor incididunt ut labore et "
                     "dolore magna aliqua. Ut" ) );

  // Only for unix line endings.
  REQUIRE( fs::file_size( p ) == s.size() );
}

TEST_CASE( "[io] large-win" ) {
  fs::path p = text_path( "large-win.txt" );
  REQUIRE( fs::file_size( p ) == 29142 );
  size_t size = 1234567;
  auto res    = read_text_file( p, size );
  REQUIRE( res );
  constexpr size_t expected_size = 28619;
  REQUIRE( size == expected_size );
  REQUIRE( *res != nullptr );

  char const* cc = ( *res ).get();
  REQUIRE( strlen( cc ) == expected_size );

  string s = cc;
  REQUIRE( s.size() == expected_size );

  REQUIRE(
      s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                     "adipiscing elit, sed do\n"
                     "eiusmod tempor incididunt ut labore et "
                     "dolore magna aliqua. Ut" ) );

  REQUIRE( fs::file_size( p ) > s.size() );
}

TEST_CASE( "[io] read_text_file_as_string large" ) {
  SECTION( "unix" ) {
    fs::path p = text_path( "large-unix.txt" );
    auto res   = read_text_file_as_string( p );
    REQUIRE( res );
    string s = std::move( *res );
    REQUIRE( s.size() == 28619 );
    REQUIRE(
        s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                       "adipiscing elit, sed do\n"
                       "eiusmod tempor incididunt ut labore et "
                       "dolore magna aliqua. Ut" ) );
  }
  SECTION( "win" ) {
    fs::path p = text_path( "large-win.txt" );
    auto res   = read_text_file_as_string( p );
    REQUIRE( res );
    string s = std::move( *res );
    REQUIRE( s.size() == 28619 );
    REQUIRE(
        s.starts_with( "Lorem ipsum dolor sit amet, consectetur "
                       "adipiscing elit, sed do\n"
                       "eiusmod tempor incididunt ut labore et "
                       "dolore magna aliqua. Ut" ) );
  }
}

TEST_CASE( "[io] read_text_file_as_string oneline" ) {
  fs::path p = text_path( "oneline.txt" );
  auto res   = read_text_file_as_string( p );
  REQUIRE( res );
  string s = std::move( *res );
  REQUIRE( s.size() == 26 );
  REQUIRE( s.starts_with( "Lorem ipsum dolor sit amet" ) );
}

TEST_CASE( "[io] read_text_file_as_string one char" ) {
  fs::path p = text_path( "single-char.txt" );
  auto res   = read_text_file_as_string( p );
  REQUIRE( res );
  string s = std::move( *res );
  REQUIRE( s.size() == 1 );
  REQUIRE( s == "L" );
}

TEST_CASE( "[io] read_text_file_as_string empty" ) {
  fs::path p = text_path( "empty.txt" );
  auto res   = read_text_file_as_string( p );
  REQUIRE( res );
  string s = std::move( *res );
  REQUIRE( s.size() == 0 );
  REQUIRE( s == "" );
}

TEST_CASE( "[io] read_file_lines" ) {
  fs::path p = text_path( "large-unix.txt" );
  vector<string> res;
  REQUIRE( read_file_lines( p, res ) );
  REQUIRE( res.size() == 524 );

  REQUIRE( res[0] ==
           "Lorem ipsum dolor sit amet, consectetur adipiscing "
           "elit, sed do" );
  REQUIRE( res[1] ==
           "eiusmod tempor incididunt ut labore et dolore magna "
           "aliqua. Ut" );
  REQUIRE( res[2] ==
           "enim ad minim veniam, quis nostrud exercitation "
           "ullamco laboris" );
  REQUIRE( res[3] ==
           "nisi ut aliquip ex ea commodo consequat. Duis aute "
           "irure dolor in" );
  REQUIRE( res[4] ==
           "reprehenderit in voluptate velit esse cillum dolore "
           "eu fugiat" );
  REQUIRE( res[5] ==
           "nulla pariatur. Excepteur sint occaecat cupidatat "
           "non proident," );
  REQUIRE( res[6] ==
           "sunt in culpa qui officia deserunt mollit anim id "
           "est laborum." );
  REQUIRE( res[7] == "" );
  REQUIRE( res[8] ==
           "Lorem ipsum dolor sit amet, consectetur adipiscing "
           "elit, sed do" );
  REQUIRE( res[9] ==
           "eiusmod tempor incididunt ut labore et dolore magna "
           "aliqua. Ut" );

  REQUIRE( res[523] ==
           "sunt in culpa qui officia deserunt mollit anim id "
           "est laborum." );
}

} // namespace
} // namespace base
