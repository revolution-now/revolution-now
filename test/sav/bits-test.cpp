/****************************************************************
**bits-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-09.
*
* Description: Unit tests for the sav/bits module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/bits.hpp"

// cdr
#include "src/cdr/converter.hpp"

// base
#include "src/base/binary-data.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace sav {
namespace {

using namespace std;

using ::base::BinaryData;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[sav/bits] default construction" ) {
  bits<0> bs0;
  REQUIRE( bs0.n() == 0 );

  bits<1> bs1;
  REQUIRE( bs1.n() == 0 );

  bits<2> bs2;
  REQUIRE( bs2.n() == 0 );

  bits<3> bs3;
  REQUIRE( bs3.n() == 0 );

  bits<8> bs8;
  REQUIRE( bs8.n() == 0 );

  bits<10> bs10;
  REQUIRE( bs10.n() == 0 );

  bits<32> bs32( 0 );
  REQUIRE( bs32.n() == 0 );

  bits<64> bs64;
  REQUIRE( bs64.n() == 0 );
}

TEST_CASE( "[sav/bits] clamping" ) {
  bits<0> bs0( 1 );
  REQUIRE( bs0.n() == 0 );

  bits<6> bs6( 255 );
  REQUIRE( bs6.n() == 63 );

  bits<63> bs63_max( 9223372036854775807UL );
  REQUIRE( bs63_max.n() == 9223372036854775807UL );

  bits<63> bs63_over( 9223372036854775808UL );
  REQUIRE( bs63_over.n() == 0 );
}

TEST_CASE( "[sav/bits] number construction" ) {
  bits<0> bs0( 1 );
  REQUIRE( bs0.n() == 0 );

  bits<1> bs1( 1 );
  REQUIRE( bs1.n() == 1 );

  bits<2> bs2( 1 );
  REQUIRE( bs2.n() == 1 );

  bits<3> bs3( 4 );
  REQUIRE( bs3.n() == 4 );

  bits<8> bs8( 255 );
  REQUIRE( bs8.n() == 255 );

  bits<10> bs10( 123 );
  REQUIRE( bs10.n() == 123 );

  bits<32> bs32( 0 );
  REQUIRE( bs32.n() == 0 );

  bits<64> bs64( 123456789 );
  REQUIRE( bs64.n() == 123456789 );

  bits<64> bs64_max( 18446744073709551615UL );
  REQUIRE( bs64_max.n() == 18446744073709551615UL );
}

TEST_CASE( "[sav/bits] to_str" ) {
  bits<0> bs0( 1 );
  REQUIRE( base::to_str( bs0 ) == "" );

  bits<1> bs1( 1 );
  REQUIRE( base::to_str( bs1 ) == "1" );

  bits<2> bs2( 1 );
  REQUIRE( base::to_str( bs2 ) == "01" );

  bits<3> bs3( 4 );
  REQUIRE( base::to_str( bs3 ) == "100" );

  bits<8> bs8( 255 );
  REQUIRE( base::to_str( bs8 ) == "11111111" );

  bits<10> bs10( 123 );
  REQUIRE( base::to_str( bs10 ) == "0001111011" );

  bits<32> bs32( 0 );
  REQUIRE( base::to_str( bs32 ) ==
           "00000000000000000000000000000000" );

  bits<64> bs64( 123456789 );
  REQUIRE( base::to_str( bs64 ) ==
           "0000000000000000000000000000000000000111010110111100"
           "110100010101" );

  bits<64> bs64_max( 18446744073709551615UL );
  REQUIRE( base::to_str( bs64_max ) ==
           "1111111111111111111111111111111111111111111111111111"
           "111111111111" );
}

TEST_CASE( "[sav/bits] to_canonical" ) {
  cdr::converter conv;
  bits<0>        bs0( 1 );
  REQUIRE( conv.to( bs0 ) == "" );

  bits<1> bs1( 0 );
  REQUIRE( conv.to( bs1 ) == "0" );

  bits<2> bs2( 3 );
  REQUIRE( conv.to( bs2 ) == "11" );

  bits<3> bs3( 5 );
  REQUIRE( conv.to( bs3 ) == "101" );

  bits<8> bs8( 255 );
  REQUIRE( conv.to( bs8 ) == "11111111" );

  bits<10> bs10( 555 );
  REQUIRE( conv.to( bs10 ) == "1000101011" );

  bits<32> bs32( 0 );
  REQUIRE( conv.to( bs32 ) ==
           "00000000000000000000000000000000" );

  bits<64> bs64( 123456789 );
  REQUIRE( conv.to( bs64 ) ==
           "0000000000000000000000000000000000000111010110111100"
           "110100010101" );

  bits<64> bs64_max( 18446744073709551615UL );
  REQUIRE( conv.to( bs64_max ) ==
           "1111111111111111111111111111111111111111111111111111"
           "111111111111" );
}

TEST_CASE( "[sav/bits] from_canonical" ) {
  cdr::converter conv;
  cdr::value     v;

  bits<0> bs0( 1 );
  v = "";
  REQUIRE( conv.from<bits<0>>( v ) == bs0 );

  bits<1> bs1( 0 );
  v = "0";
  REQUIRE( conv.from<bits<1>>( v ) == bs1 );

  bits<2> bs2( 3 );
  v = "11";
  REQUIRE( conv.from<bits<2>>( v ) == bs2 );

  bits<3> bs3( 5 );
  v = "101";
  REQUIRE( conv.from<bits<3>>( v ) == bs3 );

  bits<8> bs8( 255 );
  v = "11111111";
  REQUIRE( conv.from<bits<8>>( v ) == bs8 );

  bits<10> bs10( 555 );
  v = "1000101011";
  REQUIRE( conv.from<bits<10>>( v ) == bs10 );

  bits<32> bs32( 0 );
  v = "00000000000000000000000000000000";
  REQUIRE( conv.from<bits<32>>( v ) == bs32 );

  bits<64> bs64( 123456789 );
  v = "000000000000000000000000000000000000011101011011110011010"
      "0010101";
  REQUIRE( conv.from<bits<64>>( v ) == bs64 );

  bits<64> bs64_max( 18446744073709551615UL );
  v = "111111111111111111111111111111111111111111111111111111111"
      "1111111";
  REQUIRE( conv.from<bits<64>>( v ) == bs64_max );

  v = "111111111";
  REQUIRE( conv.from<bits<8>>( v ) ==
           conv.err( "expected bit string of length 8 but found "
                     "length 9." ) );

  v = "1111g111";
  REQUIRE(
      conv.from<bits<8>>( v ) ==
      conv.err(
          "expected bit value '1' or '0' but found 'g'." ) );
}

TEST_CASE( "[sav/bits] read_binary" ) {
  SECTION( "empty" ) {
    array<unsigned char, 16> buffer = { 1, 1, 1, 1, 1, 1, 1, 1,
                                        1, 1, 1, 1, 1, 1, 1, 1 };

    BinaryData    b( buffer );
    bits<0> const expected = {};
    bits<0>       as;
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
    BinaryData    b( buffer );
    bits<8> const expected = bits<8>( 128 );
    bits<8>       as;
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
    bits<16> const expected =
        bits<16>( ( 128UL << 0 ) + ( 200UL << 8 ) );
    bits<16> as;
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
    bits<40> const expected = bits<40>(
        ( 128UL << 0 ) + ( 200UL << 8 ) + ( 1UL << 16 ) +
        ( 2UL << 24 ) + ( 3UL << 32 ) );
    bits<40> as;
    read_binary( b, as );
    REQUIRE_FALSE( b.eof() );
    REQUIRE( b.good( 0 ) );
    REQUIRE( b.good( 11 ) );
    REQUIRE_FALSE( b.good( 12 ) );
    REQUIRE( b.pos() == 5 );
    REQUIRE( as == expected );
  }
}

TEST_CASE( "[sav/bits] write_binary" ) {
  array<unsigned char, 16> buffer   = { 1, 1, 1, 1, 1, 1, 1, 1,
                                        1, 1, 1, 1, 1, 1, 1, 1 };
  array<unsigned char, 16> expected = {};

  BinaryData b( buffer );

  SECTION( "empty" ) {
    bits<0> const as = {};
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
    bits<8> const as( 0xfe );
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
    bits<16> const as( 0xfe );
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
    bits<40> const as( ( 0xfe << 0 ) + ( 1UL << 8 ) +
                       ( 2UL << 16 ) + ( 10UL << 24 ) +
                       ( 4UL << 32 ) );
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

} // namespace
} // namespace sav
