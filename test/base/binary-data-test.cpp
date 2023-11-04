/****************************************************************
**binary-data-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-03.
*
* Description: Unit tests for the base/binary-data module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/binary-data.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/binary-data] BinaryData [write] [builtin]" ) {
  array<unsigned char, 16> buffer   = {};
  array<unsigned char, 16> expected = {};

  BinaryData b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  i1 = 0x05;
  uint16_t const i2 = 0x80ff;
  uint32_t const i3 = 0x10002000;
  uint64_t const i4 = 0x1234567890987654;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 16 ) );
  REQUIRE_FALSE( b.good( 17 ) );
  REQUIRE( b.pos() == 0 );
  expected = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i1 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 15 ) );
  REQUIRE_FALSE( b.good( 16 ) );
  REQUIRE( b.pos() == 1 );
  expected = { 0x05, 0, 0, 0, 0, 0, 0, 0,
               0,    0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 13 ) );
  REQUIRE_FALSE( b.good( 14 ) );
  REQUIRE( b.pos() == 3 );
  expected = { 0x05, 0xff, 0x80, 0, 0, 0, 0, 0,
               0,    0,    0,    0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i3 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 9 ) );
  REQUIRE_FALSE( b.good( 10 ) );
  REQUIRE( b.pos() == 7 );
  expected = { 0x05, 0xff, 0x80, 0, 0x20, 0, 0x10, 0,
               0,    0,    0,    0, 0,    0, 0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i1 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i1 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i4 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );
}

TEST_CASE( "[base/binary-data] BinaryData [write_bytes]" ) {
  array<unsigned char, 16> buffer   = {};
  array<unsigned char, 16> expected = {};

  BinaryData b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  i1 = 0x05;
  uint16_t const i2 = 0x80ff;
  uint32_t const i3 = 0x10002000;
  uint64_t const i4 = 0x1234567890987654;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 16 ) );
  REQUIRE_FALSE( b.good( 17 ) );
  REQUIRE( b.pos() == 0 );
  expected = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<1>( i1 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 15 ) );
  REQUIRE_FALSE( b.good( 16 ) );
  REQUIRE( b.pos() == 1 );
  expected = { 0x05, 0, 0, 0, 0, 0, 0, 0,
               0,    0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<2>( i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 13 ) );
  REQUIRE_FALSE( b.good( 14 ) );
  REQUIRE( b.pos() == 3 );
  expected = { 0x05, 0xff, 0x80, 0, 0, 0, 0, 0,
               0,    0,    0,    0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<2>( i3 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 11 ) );
  REQUIRE_FALSE( b.good( 12 ) );
  REQUIRE( b.pos() == 5 );
  expected = { 0x05, 0xff, 0x80, 0, 0x20, 0, 0, 0,
               0,    0,    0,    0, 0,    0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<2>( i3 >> 16 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 9 ) );
  REQUIRE_FALSE( b.good( 10 ) );
  REQUIRE( b.pos() == 7 );
  expected = { 0x05, 0xff, 0x80, 0, 0x20, 0, 0x10, 0,
               0,    0,    0,    0, 0,    0, 0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<7>( i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 2 ) );
  REQUIRE_FALSE( b.good( 3 ) );
  REQUIRE( b.pos() == 14 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<1>( i4 >> 56 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<8>( i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<2>( i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<1>( i1 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<1>( i1 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<0>( i1 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<8>( i4 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );
}

TEST_CASE(
    "[base/binary-data] BinaryData [write] [std::array]" ) {
  // These are not the arrays under test; we just happen to use a
  // std::array to represent the underlying binary buffer because
  // it supports operator==.
  array<unsigned char, 16 + 15> buffer   = {};
  array<unsigned char, 16 + 15> expected = {};

  BinaryData b( buffer );
  REQUIRE( b.size() == 31 );

  array<uint16_t, 8> i0 = { 1, 2, 3, 4, 5, 6, 7, 8 };
  array<uint64_t, 2> i1 = { 0x1234567801234567,
                            0xaaaaaaaaaaaaaaaa };
  array<uint32_t, 3> i2 = { 0xabcdef01, 0xbcdef987, 0xbbbbbbbb };
  array<uint8_t, 2>  i3 = { 0x34, 0x56 };

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 31 ) );
  REQUIRE_FALSE( b.good( 32 ) );
  REQUIRE( b.pos() == 0 );
  expected = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i0 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 15 ) );
  REQUIRE_FALSE( b.good( 16 ) );
  REQUIRE( b.pos() == 16 );
  expected = { 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0,
               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i1 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 7 ) );
  REQUIRE_FALSE( b.good( 8 ) );
  REQUIRE( b.pos() == 24 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0,    0,    0,    0,    0,    0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 3 ) );
  REQUIRE_FALSE( b.good( 4 ) );
  REQUIRE( b.pos() == 28 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0x01, 0xef, 0xcd, 0xab, 0,    0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i3 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 30 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0x01, 0xef, 0xcd, 0xab, 0x34, 0x56, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i3 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE( b.pos() == 31 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0x01, 0xef, 0xcd, 0xab, 0x34, 0x56, 0x34 };
  REQUIRE( buffer == expected );
}

TEST_CASE( "[base/binary-data] BinaryData [read] [builtin]" ) {
  array<unsigned char, 16> buffer = {
      0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
      0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x06 };

  BinaryData b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  expected_i1 = 0x05;
  uint16_t const expected_i2 = 0x80ff;
  uint32_t const expected_i3 = 0x10002000;
  uint64_t const expected_i4 = 0x1234567890987654;

  uint8_t  i1 = 0;
  uint16_t i2 = 0;
  uint32_t i3 = 0;
  uint64_t i4 = 0;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 16 ) );
  REQUIRE_FALSE( b.good( 17 ) );
  REQUIRE( b.pos() == 0 );

  REQUIRE( i1 == 0 );
  REQUIRE( read_binary( b, i1 ) );
  REQUIRE( i1 == expected_i1 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 15 ) );
  REQUIRE_FALSE( b.good( 16 ) );
  REQUIRE( b.pos() == 1 );

  REQUIRE( i2 == 0 );
  REQUIRE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 13 ) );
  REQUIRE_FALSE( b.good( 14 ) );
  REQUIRE( b.pos() == 3 );

  REQUIRE( i3 == 0 );
  REQUIRE( read_binary( b, i3 ) );
  REQUIRE( i3 == expected_i3 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 9 ) );
  REQUIRE_FALSE( b.good( 10 ) );
  REQUIRE( b.pos() == 7 );

  REQUIRE( i4 == 0 );
  REQUIRE( read_binary( b, i4 ) );
  REQUIRE( i4 == expected_i4 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( read_binary( b, i4 ) );
  REQUIRE( i4 == expected_i4 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );

  REQUIRE( i1 == expected_i1 );
  REQUIRE( read_binary( b, i1 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( read_binary( b, i1 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( read_binary( b, i4 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
}

TEST_CASE( "[base/binary-data] BinaryData [read_bytes]" ) {
  array<unsigned char, 16> buffer = {
      0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
      0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x06 };

  BinaryData b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  expected_i1 = 0x05;
  uint64_t const expected_i4 = 0x1234567890987654;

  uint8_t  i1 = 0;
  uint16_t i2 = 0;
  uint32_t i3 = 0;
  uint64_t i4 = 0;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 16 ) );
  REQUIRE_FALSE( b.good( 17 ) );
  REQUIRE( b.pos() == 0 );

  REQUIRE( i1 == 0 );
  REQUIRE( b.read_bytes<0>( i1 ) );
  REQUIRE( i1 == 0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 16 ) );
  REQUIRE_FALSE( b.good( 17 ) );
  REQUIRE( b.pos() == 0 );

  REQUIRE( i1 == 0 );
  REQUIRE( b.read_bytes<1>( i1 ) );
  REQUIRE( i1 == expected_i1 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 15 ) );
  REQUIRE_FALSE( b.good( 16 ) );
  REQUIRE( b.pos() == 1 );

  REQUIRE( i2 == 0 );
  REQUIRE( b.read_bytes<1>( i2 ) );
  REQUIRE( i2 == 0xff );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 14 ) );
  REQUIRE_FALSE( b.good( 15 ) );
  REQUIRE( b.pos() == 2 );

  REQUIRE( b.read_bytes<1>( i2 ) );
  REQUIRE( i2 == 0x80 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 13 ) );
  REQUIRE_FALSE( b.good( 14 ) );
  REQUIRE( b.pos() == 3 );

  REQUIRE( i3 == 0 );
  REQUIRE( b.read_bytes<3>( i3 ) );
  REQUIRE( i3 == 0x00002000 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 10 ) );
  REQUIRE_FALSE( b.good( 11 ) );
  REQUIRE( b.pos() == 6 );

  REQUIRE( b.read_bytes<1>( i3 ) );
  REQUIRE( i3 == 0x00000010 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 9 ) );
  REQUIRE_FALSE( b.good( 10 ) );
  REQUIRE( b.pos() == 7 );

  REQUIRE( i4 == 0 );
  REQUIRE( b.read_bytes<8>( i4 ) );
  REQUIRE( i4 == expected_i4 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( b.read_bytes<8>( i4 ) );
  REQUIRE( i4 == 0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( b.read_bytes<2>( i2 ) );
  REQUIRE( i2 == 0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 15 );

  REQUIRE( i1 == expected_i1 );
  REQUIRE( b.read_bytes<1>( i1 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( b.read_bytes<1>( i1 ) );
  REQUIRE( i1 == 0 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( b.read_bytes<1>( i4 ) );
  REQUIRE( i1 == 0 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE_FALSE( b.good( 2 ) );
  REQUIRE( b.pos() == 16 );
}

TEST_CASE(
    "[base/binary-data] BinaryData [read] [std::array]" ) {
  // These are not the arrays under test; we just happen to use a
  // std::array to represent the underlying binary buffer because
  // it supports operator==.
  array<unsigned char, 16> buffer = {
      0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
      0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x06 };

  BinaryData b( buffer );
  REQUIRE( b.size() == 16 );

  array<uint16_t, 3> const expected_i0 = { 0xff05, 0x0080,
                                           0x0020 };
  array<uint32_t, 2> const expected_i1 = { 0x98765410,
                                           0x34567890 };
  array<uint8_t, 2> const  expected_i2 = { 0x12, 0x06 };

  array<uint16_t, 3> i0 = { 0, 0, 0 };
  array<uint32_t, 2> i1 = { 0, 0 };
  array<uint8_t, 2>  i2 = { 0, 0 };

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 16 ) );
  REQUIRE_FALSE( b.good( 17 ) );
  REQUIRE( b.pos() == 0 );

  REQUIRE( read_binary( b, i0 ) );
  REQUIRE( i0 == expected_i0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 10 ) );
  REQUIRE_FALSE( b.good( 11 ) );
  REQUIRE( b.pos() == 6 );

  REQUIRE( read_binary( b, i1 ) );
  REQUIRE( i1 == expected_i1 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE( b.good( 2 ) );
  REQUIRE_FALSE( b.good( 3 ) );
  REQUIRE( b.pos() == 14 );

  REQUIRE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE( b.eof() );
  REQUIRE( b.good( 0 ) );
  REQUIRE_FALSE( b.good( 1 ) );
  REQUIRE( b.pos() == 16 );
}

TEST_CASE(
    "[base/binary-data] BinaryData [read/write] [signed]" ) {
  array<unsigned char, 8> buffer = { 0xff, 0xff, 0x05, 0x80,
                                     0xff, 0xff, 0xff, 0xff };
  array<unsigned char, 8> expected;

  BinaryData b( buffer );
  REQUIRE( b.size() == 8 );

  SECTION( "read" ) {
    int16_t x1 = 0;
    REQUIRE( read_binary( b, x1 ) );
    REQUIRE( x1 == -1 );

    int8_t x2 = 0;
    REQUIRE( read_binary( b, x2 ) );
    REQUIRE( x2 == 5 );

    REQUIRE( read_binary( b, x2 ) );
    REQUIRE( x2 == -128 );

    int32_t x3 = 0;
    REQUIRE( read_binary( b, x3 ) );
    REQUIRE( x3 == -1 );
  }

  SECTION( "write" ) {
    int16_t x1 = -32768;
    REQUIRE( write_binary( b, x1 ) );
    expected = { 0x00, 0x80, 0x05, 0x80,
                 0xff, 0xff, 0xff, 0xff };

    int8_t x2 = -1;
    REQUIRE( write_binary( b, x2 ) );
    expected = { 0xff, 0xff, 0xff, 0x80,
                 0xff, 0xff, 0xff, 0xff };

    x2 = 1;
    REQUIRE( write_binary( b, x2 ) );
    expected = { 0xff, 0xff, 0xff, 0x01,
                 0xff, 0xff, 0xff, 0xff };

    int32_t x3 = -234567788;
    REQUIRE( write_binary( b, x3 ) );
    expected = { 0xff, 0xff, 0xff, 0x01,
                 0x94, 0xc7, 0x04, 0xf2 };
  }
}

TEST_CASE( "[base/binary-data] BinaryBuffer" ) {
}

} // namespace
} // namespace base
