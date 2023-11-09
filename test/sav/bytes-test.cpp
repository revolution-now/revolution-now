/****************************************************************
**bytes-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-08.
*
* Description: Unit tests for the sav/bytes module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/bytes.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace sav {
namespace {

using namespace std;

using ::base::BinaryData;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[sav/bytes] construction" ) {
  [[maybe_unused]] bytes<0> const zero;

  bytes<5> const as;
  REQUIRE( as.a[0] == 0 );
  REQUIRE( as.a[1] == 0 );
  REQUIRE( as.a[2] == 0 );
  REQUIRE( as.a[3] == 0 );
  REQUIRE( as.a[4] == 0 );
}

TEST_CASE( "[sav/bytes] to_str" ) {
  SECTION( "empty" ) {
    bytes<0> const as = {};
    REQUIRE( base::to_str( as ) == "" );
  }
  SECTION( "single" ) {
    bytes<1> const as = { 255 };
    REQUIRE( base::to_str( as ) == "ff" );
  }
  SECTION( "double" ) {
    bytes<2> const as = { 255, 5 };
    REQUIRE( base::to_str( as ) == "ff 05" );
  }
  SECTION( "many" ) {
    bytes<5> const as = { 'y', 'e', 's', 1, 10 };
    REQUIRE( base::to_str( as ) == "79 65 73 01 0a" );
  }
}

TEST_CASE( "[sav/bytes] to_canonical" ) {
  cdr::converter conv;
  SECTION( "empty" ) {
    bytes<0> const as = {};
    REQUIRE( conv.to( as ) == "" );
  }
  SECTION( "single" ) {
    bytes<1> const as = { 255 };
    REQUIRE( conv.to( as ) == "ff" );
  }
  SECTION( "double" ) {
    bytes<2> const as = { 255, 5 };
    REQUIRE( conv.to( as ) == "ff 05" );
  }
  SECTION( "many" ) {
    bytes<5> const as = { 'y', 'e', 's', 1, 10 };
    REQUIRE( conv.to( as ) == "79 65 73 01 0a" );
  }
}

TEST_CASE( "[sav/bytes] from_canonical" ) {
  cdr::converter conv;
  cdr::value     v;
  SECTION( "empty" ) {
    bytes<0> const expected = {};

    v = "";
    REQUIRE( conv.from<bytes<0>>( v ) == expected );
  }
  SECTION( "single" ) {
    bytes<1> const expected = { 255 };

    v = "ff";
    REQUIRE( conv.from<bytes<1>>( v ) == expected );
  }
  SECTION( "double" ) {
    bytes<2> const expected = { 255, 5 };

    v = "ff 05";
    REQUIRE( conv.from<bytes<2>>( v ) == expected );
  }
  SECTION( "many" ) {
    bytes<5> const expected = { 'y', 'e', 's', 1, 10 };

    v = "79 65 73 01 0a";
    REQUIRE( conv.from<bytes<5>>( v ) == expected );
  }
  SECTION( "error type" ) {
    cdr::value const v = 5;
    REQUIRE(
        conv.from<bytes<5>>( v ) ==
        conv.err( "expected type string, instead found type "
                  "integer." ) );
  }
  SECTION( "error size" ) {
    cdr::value const v = "hello world";
    REQUIRE( conv.from<bytes<5>>( v ) ==
             conv.err( "expected string with length equal to "
                       "14, but instead found length 11." ) );
  }
  SECTION( "error components" ) {
    cdr::value const v = "68 65 6c 6c.66";
    REQUIRE( conv.from<bytes<5>>( v ) ==
             conv.err( "expected 5 components when splitting "
                       "string on spaces, instead found 4." ) );
  }
  SECTION( "error byte length" ) {
    cdr::value const v = "68 655 6c 6c 6";
    REQUIRE(
        conv.from<bytes<5>>( v ) ==
        conv.err( "expected a two-digit hex byte at idx 1 but "
                  "instead found a token of length 3." ) );
  }
  SECTION( "error invalid hex digit" ) {
    cdr::value const v = "68 65 6g 6c 6f";
    REQUIRE( conv.from<bytes<5>>( v ) ==
             conv.err( "failed to parse hex byte 0x6g." ) );
  }
}

TEST_CASE( "[sav/bytes] write_binary" ) {
  array<unsigned char, 16> buffer   = { 1, 1, 1, 1, 1, 1, 1, 1,
                                        1, 1, 1, 1, 1, 1, 1, 1 };
  array<unsigned char, 16> expected = {};

  BinaryData b( buffer );

  SECTION( "empty" ) {
    bytes<0> const as = {};
    write_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 16 ) );
    REQUIRE_FALSE( b.good( 17 ) );
    REQUIRE( b.pos() == 0 );
    expected = { 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1 };
    REQUIRE( buffer == expected );
  }
  SECTION( "single" ) {
    bytes<1> const as = { 0xfe };
    write_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 15 ) );
    REQUIRE_FALSE( b.good( 16 ) );
    REQUIRE( b.pos() == 1 );
    expected = { 0xfe, 1, 1, 1, 1, 1, 1, 1,
                 1,    1, 1, 1, 1, 1, 1, 1 };
    REQUIRE( buffer == expected );
  }
  SECTION( "double" ) {
    bytes<2> const as = { 0xfe, 0 };
    write_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 14 ) );
    REQUIRE_FALSE( b.good( 15 ) );
    REQUIRE( b.pos() == 2 );
    expected = { 0xfe, 0, 1, 1, 1, 1, 1, 1,
                 1,    1, 1, 1, 1, 1, 1, 1 };
    REQUIRE( buffer == expected );
  }
  SECTION( "many" ) {
    bytes<5> const as = { 0xfe, 1, 2, 10, 4 };
    write_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 11 ) );
    REQUIRE_FALSE( b.good( 12 ) );
    REQUIRE( b.pos() == 5 );
    expected = { 0xfe, 1, 2, 10, 4, 1, 1, 1,
                 1,    1, 1, 1,  1, 1, 1, 1 };
    REQUIRE( buffer == expected );
  }
}

TEST_CASE( "[sav/bytes] read_binary" ) {
  SECTION( "empty" ) {
    array<unsigned char, 16> buffer = { 1, 1, 1, 1, 1, 1, 1, 1,
                                        1, 1, 1, 1, 1, 1, 1, 1 };

    BinaryData     b( buffer );
    bytes<0> const expected = {};
    bytes<0>       as;
    read_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 16 ) );
    REQUIRE_FALSE( b.good( 17 ) );
    REQUIRE( b.pos() == 0 );
    REQUIRE( as == expected );
  }
  SECTION( "single" ) {
    array<unsigned char, 16> buffer = {
        128, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    BinaryData     b( buffer );
    bytes<1> const expected = { 128 };
    bytes<1>       as;
    read_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 15 ) );
    REQUIRE_FALSE( b.good( 16 ) );
    REQUIRE( b.pos() == 1 );
    REQUIRE( as == expected );
  }
  SECTION( "double" ) {
    array<unsigned char, 16> buffer = {
        128, 200, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    BinaryData     b( buffer );
    bytes<2> const expected = { 128, 200 };
    bytes<2>       as;
    read_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 14 ) );
    REQUIRE_FALSE( b.good( 15 ) );
    REQUIRE( b.pos() == 2 );
    REQUIRE( as == expected );
  }
  SECTION( "many" ) {
    array<unsigned char, 16> buffer = {
        128, 200, 1, 2, 3, 4, 5, 6, 1, 1, 1, 1, 1, 1, 1, 1 };
    BinaryData     b( buffer );
    bytes<5> const expected = { 128, 200, 1, 2, 3 };
    bytes<5>       as;
    read_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 11 ) );
    REQUIRE_FALSE( b.good( 12 ) );
    REQUIRE( b.pos() == 5 );
    REQUIRE( as == expected );
  }
}

} // namespace
} // namespace sav
