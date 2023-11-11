/****************************************************************
**string-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-07.
*
* Description: Unit tests for the sav/string module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/string.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

// C++ standard library
#include <type_traits>

namespace sav {
namespace {

using namespace std;

using ::base::MemBufferBinaryIO;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[sav/string] construction" ) {
  array_string<5> const as;
  REQUIRE( as.a[0] == 0 );
  REQUIRE( as.a[1] == 0 );
  REQUIRE( as.a[2] == 0 );
  REQUIRE( as.a[3] == 0 );
  REQUIRE( as.a[4] == 0 );
}

TEST_CASE( "[sav/string] to_str" ) {
  SECTION( "null zero" ) {
    array_string<5> const as = { 'y', 'e', 's', 0, 0 };
    REQUIRE( base::to_str( as ) == "yes" );
  }
  SECTION( "no null zero" ) {
    array_string<5> const as = { 'h', 'e', 'l', 'l', 'o' };
    REQUIRE( base::to_str( as ) == "hello" );
  }
}

TEST_CASE( "[sav/string] to_canonical" ) {
  SECTION( "null zero" ) {
    array_string<5> const as = { 'y', 'e', 's', 0, 0 };
    cdr::converter        conv;
    REQUIRE( conv.to( as ) == "yes" );
  }
  SECTION( "no null zero" ) {
    array_string<5> const as = { 'h', 'e', 'l', 'l', 'o' };
    cdr::converter        conv;
    REQUIRE( conv.to( as ) == "hello" );
  }
}

TEST_CASE( "[sav/string] from_canonical" ) {
  cdr::converter conv;
  cdr::value     v;
  SECTION( "null zero" ) {
    array_string<5> const expected = { 'y', 'e', 's', 0, 0 };
    v                              = "yes";
    REQUIRE( conv.from<array_string<5>>( v ) == expected );
  }
  SECTION( "no null zero" ) {
    array_string<5> const expected = { 'h', 'e', 'l', 'l', 'o' };
    v                              = "hello";
    REQUIRE( conv.from<array_string<5>>( v ) == expected );
  }
  SECTION( "error type" ) {
    v = 5;
    REQUIRE(
        conv.from<array_string<5>>( v ) ==
        conv.err( "expected type string, instead found type "
                  "integer." ) );
  }
  SECTION( "error size" ) {
    v = "hello world";
    REQUIRE( conv.from<array_string<5>>( v ) ==
             conv.err( "expected string with length <= 5, but "
                       "instead found length 11." ) );
  }
}

TEST_CASE( "[sav/string] write_binary" ) {
  array<unsigned char, 16> buffer   = { 1, 1, 1, 1, 1, 1, 1, 1,
                                        1, 1, 1, 1, 1, 1, 1, 1 };
  array<unsigned char, 16> expected = {};
  MemBufferBinaryIO        b( buffer );

  SECTION( "null zero" ) {
    array_string<5> const as = { 'y', 'e', 's', 3, 2 };
    write_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.remaining() == 11 );
    REQUIRE( b.pos() == 5 );
    expected = { 'y', 'e', 's', 3, 2, 1, 1, 1,
                 1,   1,   1,   1, 1, 1, 1, 1 };
    REQUIRE( buffer == expected );
  }
  SECTION( "no null zero" ) {
    array_string<5> const as = { 'h', 'e', 'l', 'l', 'o' };
    write_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.remaining() == 11 );
    REQUIRE( b.pos() == 5 );
    expected = { 'h', 'e', 'l', 'l', 'o', 1, 1, 1,
                 1,   1,   1,   1,   1,   1, 1, 1 };
    REQUIRE( buffer == expected );
  }
}

TEST_CASE( "[sav/string] read_binary" ) {
  SECTION( "null zero" ) {
    array<unsigned char, 16> buffer = {
        'y', 'e', 's', 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    MemBufferBinaryIO     b( buffer );
    array_string<5> const expected = { 'y', 'e', 's', 0, 1 };
    array_string<5>       as;
    read_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.remaining() == 11 );
    REQUIRE( b.pos() == 5 );
    REQUIRE( as == expected );
  }
  SECTION( "no null zero" ) {
    array<unsigned char, 16> buffer = {
        'h', 'e', 'l', 'l', 'o', 1, 1, 1,
        1,   1,   1,   1,   1,   1, 1, 1 };
    MemBufferBinaryIO     b( buffer );
    array_string<5> const expected = { 'h', 'e', 'l', 'l', 'o' };
    array_string<5>       as;
    read_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.remaining() == 11 );
    REQUIRE( b.pos() == 5 );
    REQUIRE( as == expected );
  }
}

} // namespace
} // namespace sav
